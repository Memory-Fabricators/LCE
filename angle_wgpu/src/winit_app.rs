//! Winit windowing and event handling integration for EGL.
#![allow(unused_imports, dead_code)]
use crate::egl::{EglDisplayState, EglSurfaceState, block_on};
use crate::renderer::WgpuRenderer;
use crate::types::*;
use parking_lot::Mutex;
use raw_window_handle::{HasDisplayHandle, HasWindowHandle as _};
use std::collections::HashSet;
use std::ffi::{CStr, c_char, c_void};
use std::sync::Arc;
use std::sync::atomic::Ordering;
use std::time::Duration;
use winit::application::ApplicationHandler;
use winit::dpi::PhysicalSize;
use winit::event::{
    ButtonSource, DeviceEvent, DeviceId, ElementState, MouseButton, MouseScrollDelta, WindowEvent,
};
use winit::event_loop::{ActiveEventLoop, ControlFlow, EventLoop};
use winit::keyboard::{KeyCode, PhysicalKey};
use winit::window::{CursorGrabMode, Window, WindowAttributes, WindowId};

pub struct WinitAppState {
    pub window: Option<Arc<dyn Window>>,
    pub width: u32,
    pub height: u32,
    pub close_requested: bool,
    pub focused: bool,
    pub keys_down: HashSet<u32>,
    pub mouse_buttons_down: HashSet<u32>,
    pub mouse_pos: (f64, f64),
    pub accumulated_mouse_delta: (f64, f64),
    pub scroll_delta: f32,
    pub grabbed: bool,
    /// Set by an Escape keypress to force-release the cursor even while
    /// the game keeps requesting a grab (this port's pause menu is a
    /// stub, so `screen == NULL` never becomes true and the game would
    /// otherwise re-request the grab every tick with no way out).
    /// Cleared by clicking back into the window.
    pub escape_ungrab: bool,
    /// Last raw `PointerMoved` position seen while grabbed, used to
    /// compute camera-look deltas (position-vs-previous), since raw
    /// `DeviceEvent::PointerMotion` isn't available on relayed displays.
    pub last_pointer_pos: Option<(f64, f64)>,
    /// Last effective grab state actually applied to the platform, so
    /// `winit_app_set_mouse_grab` (called every game tick) doesn't redo a
    /// real `XGrabPointer` syscall each frame when nothing changed.
    pub grab_applied: Option<bool>,
    pub title: String,
    pub resizable: bool,
    pub egl_surface: Option<Arc<Mutex<EglSurfaceState>>>,
    pub initialized: bool,
    pub should_exit: bool,
}

pub struct WinitApp {
    pub state: Arc<Mutex<WinitAppState>>,
    pub thread_handle: Option<std::thread::JoinHandle<()>>,
}

struct AppHandler {
    state: Arc<Mutex<WinitAppState>>,
}
fn keycode_to_game_key(code: KeyCode) -> Option<u32> {
    match code {
        KeyCode::KeyA => Some(0),
        KeyCode::KeyB => Some(1),
        KeyCode::KeyC => Some(2),
        KeyCode::KeyD => Some(3),
        KeyCode::KeyE => Some(4),
        KeyCode::KeyF => Some(5),
        KeyCode::KeyG => Some(6),
        KeyCode::KeyH => Some(7),
        KeyCode::KeyI => Some(8),
        KeyCode::KeyJ => Some(9),
        KeyCode::KeyK => Some(10),
        KeyCode::KeyL => Some(11),
        KeyCode::KeyM => Some(12),
        KeyCode::KeyN => Some(13),
        KeyCode::KeyO => Some(14),
        KeyCode::KeyP => Some(15),
        KeyCode::KeyQ => Some(16),
        KeyCode::KeyR => Some(17),
        KeyCode::KeyS => Some(18),
        KeyCode::KeyT => Some(19),
        KeyCode::KeyU => Some(20),
        KeyCode::KeyV => Some(21),
        KeyCode::KeyW => Some(22),
        KeyCode::KeyX => Some(23),
        KeyCode::KeyY => Some(24),
        KeyCode::KeyZ => Some(25),
        KeyCode::Space => Some(26),
        KeyCode::ShiftLeft => Some(27),
        KeyCode::Escape => Some(28),
        KeyCode::Backspace => Some(29),
        KeyCode::Enter => Some(30),
        KeyCode::ShiftRight => Some(31),
        KeyCode::ArrowUp => Some(32),
        KeyCode::ArrowDown => Some(33),
        KeyCode::Tab => Some(34),
        KeyCode::ArrowLeft => Some(35),
        KeyCode::ArrowRight => Some(36),
        KeyCode::F5 => Some(37),
        _ => None,
    }
}

#[cfg(all(unix, not(target_os = "macos"), not(target_os = "android")))]
fn pick_opaque_x11_visual(event_loop: &dyn ActiveEventLoop) -> Option<u32> {
    use raw_window_handle::RawDisplayHandle;

    let handle = event_loop.display_handle().ok()?;
    let RawDisplayHandle::Xlib(xlib) = handle.as_raw() else {
        return None;
    };
    let dpy = xlib.display?.as_ptr().cast();

    // Loads libX11 via dlopen (same mechanism winit-x11 itself uses via the
    // x11-dl crate), instead of linking against it directly.
    let xlib_fns = x11_dl::xlib::Xlib::open().ok()?;
    const TRUE_COLOR: i32 = 4;
    let mut info: x11_dl::xlib::XVisualInfo = unsafe { std::mem::zeroed() };
    let ok = unsafe { (xlib_fns.XMatchVisualInfo)(dpy, xlib.screen, 24, TRUE_COLOR, &mut info) };
    (ok != 0).then_some(info.visualid as u32)
}

impl ApplicationHandler for AppHandler {
    fn can_create_surfaces(&mut self, event_loop: &dyn ActiveEventLoop) {
        self.resumed(event_loop);
    }

    fn resumed(&mut self, event_loop: &dyn ActiveEventLoop) {
        let mut state = self.state.lock();
        if state.window.is_none() {
            let mut attrs = WindowAttributes::default()
                .with_title(&state.title)
                .with_surface_size(PhysicalSize::new(state.width, state.height))
                .with_resizable(state.resizable)
                .with_transparent(false);

            #[cfg(all(unix, not(target_os = "macos"), not(target_os = "android")))]
            {
                use winit::platform::x11::{ActiveEventLoopExtX11, WindowAttributesX11};
                if event_loop.is_x11() {
                    if let Some(visual) = pick_opaque_x11_visual(event_loop) {
                        eprintln!("[angle_wgpu] X11 24-bit TrueColor visual {visual:#x}");
                        attrs = attrs.with_platform_attributes(Box::new(
                            WindowAttributesX11::default().with_x11_visual(visual),
                        ));
                    } else {
                        eprintln!("[angle_wgpu] no 24-bit X11 visual; window may stay ARGB");
                    }
                }
            }

            match event_loop.create_window(attrs) {
                Ok(win) => {
                    eprintln!(
                        "[angle_wgpu] created window handle={:?}",
                        win.window_handle().map(|h| h.as_raw())
                    );
                    state.window = Some(Arc::from(win));
                    state.initialized = true;
                }
                Err(e) => eprintln!("[angle_wgpu] create_window failed: {e}"),
            }
        }
    }

    fn window_event(
        &mut self,
        _event_loop: &dyn ActiveEventLoop,
        _window_id: WindowId,
        event: WindowEvent,
    ) {
        let mut state = self.state.lock();
        match event {
            WindowEvent::CloseRequested => {
                state.close_requested = true;
            }
            WindowEvent::SurfaceResized(size) => {
                state.width = size.width.max(1);
                state.height = size.height.max(1);
                if let Some(surf) = &state.egl_surface {
                    let mut s = surf.lock();
                    s.width = state.width;
                    s.height = state.height;
                    if let Some(r) = &s.renderer {
                        let mut rend = r.lock();
                        rend.resize(state.width, state.height);
                    }
                }
            }
            WindowEvent::Focused(f) => {
                state.focused = f;
            }
            WindowEvent::PointerMoved { position, .. } => {
                // `Confined` (the X11 fallback for `Locked`, which X11
                // doesn't support) clamps the OS cursor at the window
                // edge: without recentering, a real physical mouse
                // sweep can only cover one window-width before further
                // motion produces zero delta (cursor stuck at the
                // border). Recenter once the cursor has drifted
                // meaningfully from center so range stays unbounded.
                // Recentering on *every* event instead of only when
                // needed made per-tick deltas jerky (extra warp
                // syscalls interleaving with real motion events in the
                // relay's event stream).
                //
                // No echo-swallowing flag needed: since deltas are
                // computed as position-vs-last-known-position, and we
                // set `last_pointer_pos` to the recenter target
                // immediately (not waiting for the warp's own event to
                // arrive), any echo event reporting position == center
                // naturally yields delta == 0 on its own.
                if state.grabbed && !state.escape_ungrab {
                    if let Some((lx, ly)) = state.last_pointer_pos {
                        let (dx, dy) = (position.x - lx, position.y - ly);
                        state.accumulated_mouse_delta.0 += dx;
                        state.accumulated_mouse_delta.1 += dy;
                    }
                    state.mouse_pos = (position.x, position.y);
                    let (cx, cy) = ((state.width / 2) as f64, (state.height / 2) as f64);
                    let drift = (position.x - cx).abs().max((position.y - cy).abs());
                    let threshold = (state.width.min(state.height) as f64) * 0.25;
                    if drift > threshold {
                        if let Some(win) = state.window.clone() {
                            let _ = win.set_cursor_position(
                                winit::dpi::PhysicalPosition::new(cx, cy).into(),
                            );
                            state.last_pointer_pos = Some((cx, cy));
                        } else {
                            state.last_pointer_pos = Some((position.x, position.y));
                        }
                    } else {
                        state.last_pointer_pos = Some((position.x, position.y));
                    }
                } else {
                    state.mouse_pos = (position.x, position.y);
                    state.last_pointer_pos = Some((position.x, position.y));
                }
            }
            WindowEvent::PointerButton {
                state: btn_state,
                button,
                ..
            } => {
                let btn_id = match button {
                    ButtonSource::Mouse(MouseButton::Left) => 1,
                    ButtonSource::Mouse(MouseButton::Right) => 2,
                    ButtonSource::Mouse(MouseButton::Middle) => 3,
                    ButtonSource::Mouse(MouseButton::Back) => 4,
                    ButtonSource::Mouse(MouseButton::Forward) => 5,
                    _ => 1,
                };
                if btn_state == ElementState::Pressed {
                    state.mouse_buttons_down.insert(btn_id);
                    // Clicking back into the window re-arms grabbing immediately
                    // if the application is currently in grabbed state.
                    if state.escape_ungrab {
                        state.escape_ungrab = false;
                        if state.grabbed {
                            if let Some(win) = state.window.clone() {
                                let _ = win.set_cursor_grab(CursorGrabMode::Locked);
                                let _ = win.set_cursor_grab(CursorGrabMode::Confined);
                                win.set_cursor_visible(false);
                                let (cx, cy) = ((state.width / 2) as f64, (state.height / 2) as f64);
                                let _ = win.set_cursor_position(winit::dpi::PhysicalPosition::new(cx, cy).into());
                                state.last_pointer_pos = Some((cx, cy));
                            }
                            state.grab_applied = Some(true);
                        } else {
                            state.grab_applied = None;
                        }
                    }
                } else {
                    state.mouse_buttons_down.remove(&btn_id);
                }
            }
            WindowEvent::MouseWheel { delta, .. } => match delta {
                MouseScrollDelta::LineDelta(_x, y) => {
                    state.scroll_delta += y;
                }
                MouseScrollDelta::PixelDelta(pos) => {
                    state.scroll_delta += (pos.y / 20.0) as f32;
                }
            },
            WindowEvent::KeyboardInput { event, .. } => {
                if let PhysicalKey::Code(code) = event.physical_key {
                    // Only the translated game key code goes into
                    // `keys_down`: the C++ side queries exclusively by
                    // `Keyboard::KEY_*` values (0-37). Winit's own
                    // `KeyCode` discriminant used to also go in here, but
                    // it shares that same small numeric range (e.g.
                    // `KeyCode::KeyS as u32 == 37`, colliding with this
                    // game's `KEY_F5`), so holding S spuriously toggled
                    // third-person view.
                    let game_code = keycode_to_game_key(code);
                    if event.state == ElementState::Pressed {
                        if let Some(gc) = game_code {
                            state.keys_down.insert(gc);
                        }
                        if code == KeyCode::Escape && state.grabbed {
                            state.escape_ungrab = true;
                            state.grab_applied = None;
                            if let Some(win) = &state.window {
                                win.set_cursor_visible(true);
                            }
                        }
                    } else if let Some(gc) = game_code {
                        state.keys_down.remove(&gc);
                    }
                }
            }
            _ => {}
        }
    }

    fn device_event(
        &mut self,
        _event_loop: &dyn ActiveEventLoop,
        _device_id: Option<DeviceId>,
        _event: DeviceEvent,
    ) {
        // Raw XInput2 motion isn't available on this relayed display (real
        // hardware motion never arrives here - only synthetic/local XTest
        // events do); `WindowEvent::PointerMoved` is the reliable source,
        // handled in `window_event` instead.
    }

    fn about_to_wait(&mut self, event_loop: &dyn ActiveEventLoop) {
        let state = self.state.lock();
        if state.should_exit {
            event_loop.exit();
        }
    }
}

// ============================================================================
// C API for Winit Windowing
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn winit_app_create(
    title: *const c_char,
    width: u32,
    height: u32,
    resizable: bool,
) -> *mut WinitApp {
    crate::init_logging();
    let title_str = if !title.is_null() {
        CStr::from_ptr(title).to_string_lossy().into_owned()
    } else {
        "Minecraft".to_string()
    };

    let state = Arc::new(Mutex::new(WinitAppState {
        window: None,
        width: width.max(1),
        height: height.max(1),
        close_requested: false,
        focused: true,
        keys_down: HashSet::new(),
        mouse_buttons_down: HashSet::new(),
        mouse_pos: (0.0, 0.0),
        accumulated_mouse_delta: (0.0, 0.0),
        scroll_delta: 0.0,
        grabbed: false,
        escape_ungrab: false,
        last_pointer_pos: None,
        grab_applied: None,
        title: title_str,
        resizable,
        egl_surface: None,
        initialized: false,
        should_exit: false,
    }));

    let state_clone = state.clone();
    let thread_handle = std::thread::spawn(move || {
        let mut builder = EventLoop::builder();
        #[cfg(all(unix, not(target_os = "macos"), not(target_os = "android")))]
        {
            use winit::platform::wayland::EventLoopBuilderExtWayland;
            use winit::platform::x11::EventLoopBuilderExtX11;
            EventLoopBuilderExtX11::with_x11(&mut builder);
            EventLoopBuilderExtX11::with_any_thread(&mut builder, true);
            EventLoopBuilderExtWayland::with_any_thread(&mut builder, true);
        }
        let Ok(event_loop) = builder.build() else {
            return;
        };
        let handler = AppHandler { state: state_clone };
        event_loop.set_control_flow(ControlFlow::Poll);
        let _ = event_loop.run_app(handler);
    });

    // Wait briefly for window initialization (up to 500ms)
    for _ in 0..50 {
        if state.lock().initialized {
            break;
        }
        std::thread::sleep(Duration::from_millis(10));
    }

    let app = Box::new(WinitApp {
        state,
        thread_handle: Some(thread_handle),
    });

    Box::into_raw(app)
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_destroy(app: *mut WinitApp) {
    if !app.is_null() {
        let mut app_box = Box::from_raw(app);
        {
            let mut s = app_box.state.lock();
            s.should_exit = true;
        }
        if let Some(h) = app_box.thread_handle.take() {
            let _ = h.join();
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_pump_events(app: *mut WinitApp) -> bool {
    if app.is_null() {
        return false;
    }
    let app_ref = &*app;
    let s = app_ref.state.lock();
    !s.close_requested
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_poll_events(app: *mut WinitApp) {
    let _ = winit_app_pump_events(app);
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_should_close(app: *mut WinitApp) -> bool {
    if app.is_null() {
        return true;
    }
    (*app).state.lock().close_requested
}
#[no_mangle]
pub unsafe extern "C" fn winit_app_get_size(app: *mut WinitApp, width: *mut u32, height: *mut u32) {
    if app.is_null() {
        return;
    }
    let s = (*app).state.lock();
    if !width.is_null() {
        *width = s.width;
    }
    if !height.is_null() {
        *height = s.height;
    }
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_set_mouse_grab(app: *mut WinitApp, grab: bool) {
    if app.is_null() {
        return;
    }
    let mut s = (*app).state.lock();
    s.grabbed = grab;
    // Escape forced a release; keep the cursor free until the user
    // clicks back into the window, regardless of what the game requests.
    let effective_grab = grab && !s.escape_ungrab;
    if s.grab_applied == Some(effective_grab) {
        // Game calls this every tick; nothing changed, skip the syscalls.
        return;
    }
    if let Some(win) = s.window.clone() {
        // `Locked` always errors on X11; `Confined` still keeps the
        // (hidden) cursor from wandering onto other windows, but camera
        // look itself comes from raw `PointerMoved` position deltas in
        // `window_event`, which recenters every event (see there).
        let mode = if effective_grab {
            CursorGrabMode::Locked
        } else {
            CursorGrabMode::None
        };
        if win.set_cursor_grab(mode).is_err() && effective_grab {
            let _ = win.set_cursor_grab(CursorGrabMode::Confined);
        }
        win.set_cursor_visible(!effective_grab);
        if effective_grab {
            let (cx, cy) = ((s.width / 2) as f64, (s.height / 2) as f64);
            let _ = win.set_cursor_position(winit::dpi::PhysicalPosition::new(cx, cy).into());
            s.last_pointer_pos = Some((cx, cy));
        }
    }
    s.grab_applied = Some(effective_grab);
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_set_cursor_visible(app: *mut WinitApp, visible: bool) {
    if app.is_null() {
        return;
    }
    let s = (*app).state.lock();
    if let Some(win) = &s.window {
        win.set_cursor_visible(visible);
    }
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_is_key_down(app: *mut WinitApp, keycode: u32) -> bool {
    if app.is_null() {
        return false;
    }
    let s = (*app).state.lock();
    s.keys_down.contains(&keycode)
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_is_button_down(app: *mut WinitApp, button: u32) -> bool {
    if app.is_null() {
        return false;
    }
    let s = (*app).state.lock();
    s.mouse_buttons_down.contains(&button)
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_get_mouse_pos(app: *mut WinitApp, x: *mut f64, y: *mut f64) {
    if app.is_null() {
        return;
    }
    let s = (*app).state.lock();
    if !x.is_null() {
        *x = s.mouse_pos.0;
    }
    if !y.is_null() {
        *y = s.mouse_pos.1;
    }
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_get_mouse_delta(app: *mut WinitApp, dx: *mut f64, dy: *mut f64) {
    if app.is_null() {
        return;
    }
    let mut s = (*app).state.lock();
    if !dx.is_null() {
        *dx = s.accumulated_mouse_delta.0;
    }
    if !dy.is_null() {
        *dy = s.accumulated_mouse_delta.1;
    }
    s.accumulated_mouse_delta = (0.0, 0.0);
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_consume_wheel_delta(app: *mut WinitApp) -> f32 {
    if app.is_null() {
        return 0.0;
    }
    let mut s = (*app).state.lock();
    let d = s.scroll_delta;
    s.scroll_delta = 0.0;
    d
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_create_egl_surface(
    app: *mut WinitApp,
    dpy: EGLDisplay,
    _config: EGLConfig,
) -> EGLSurface {
    if app.is_null() {
        return EGL_NO_SURFACE;
    }
    let app_ref = &*app;
    let (window_arc, width, height) = {
        let s = app_ref.state.lock();
        let Some(win) = s.window.clone() else {
            return EGL_NO_SURFACE;
        };
        (win, s.width, s.height)
    };

    // `Arc<dyn Window>` doesn't itself impl `HasDisplayHandle` (no blanket
    // impl through `Arc` in raw-window-handle); this thin wrapper forwards to
    // the window's own `dyn Window: HasDisplayHandle` impl so wgpu can use
    // the real platform display instead of probing blind.
    #[derive(Debug)]
    struct WindowDisplayHandle(Arc<dyn Window>);
    impl raw_window_handle::HasDisplayHandle for WindowDisplayHandle {
        fn display_handle(
            &self,
        ) -> Result<raw_window_handle::DisplayHandle<'_>, raw_window_handle::HandleError> {
            self.0.display_handle()
        }
    }

    let instance = wgpu::Instance::new(wgpu::InstanceDescriptor::new_with_display_handle(
        Box::new(WindowDisplayHandle(window_arc.clone())),
    ));

    let surface = match instance.create_surface(window_arc.clone()) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("[angle_wgpu] create_surface failed: {e:#?}");
            return EGL_NO_SURFACE;
        }
    };

    let adapter = match block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::HighPerformance,
        compatible_surface: Some(&surface),
        force_fallback_adapter: false,
        apply_limit_buckets: false,
    })) {
        Ok(a) => a,
        Err(e) => {
            eprintln!("[angle_wgpu] request_adapter failed: {e:?}");
            return EGL_NO_SURFACE;
        }
    };

    let (device, queue) = match block_on(adapter.request_device(&wgpu::DeviceDescriptor {
        label: Some("angle_wgpu Winit Surface Device"),
        ..Default::default()
    })) {
        Ok(dq) => dq,
        Err(e) => {
            eprintln!("[angle_wgpu] request_device failed: {e:?}");
            return EGL_NO_SURFACE;
        }
    };

    let renderer = match WgpuRenderer::new_with_surface(
        instance,
        adapter,
        device,
        queue,
        surface,
        window_arc.clone(),
        width,
        height,
    ) {
        Ok(r) => Arc::new(Mutex::new(r)),
        Err(e) => {
            eprintln!("[angle_wgpu] new_with_surface failed: {e}");
            return EGL_NO_SURFACE;
        }
    };

    let dpy_arc = if !dpy.is_null() {
        Arc::from_raw(dpy as *const Mutex<EglDisplayState>)
    } else {
        crate::egl::get_or_create_display()
    };

    let id = {
        let d = dpy_arc.lock();
        d.allocate_surface_id()
    };

    let surface_state = Arc::new(Mutex::new(EglSurfaceState {
        id,
        width,
        height,
        native_window: Arc::as_ptr(&window_arc) as *mut c_void,
        renderer: Some(renderer),
    }));

    {
        let mut d = dpy_arc.lock();
        d.surfaces.insert(id, surface_state.clone());
    }

    if !dpy.is_null() {
        std::mem::forget(dpy_arc);
    }

    {
        let mut s = app_ref.state.lock();
        s.egl_surface = Some(surface_state.clone());
    }

    Arc::into_raw(surface_state) as EGLSurface
}
