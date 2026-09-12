//! Winit windowing and event handling integration for EGL.
#![allow(unused_imports, dead_code)]
use parking_lot::Mutex;
use raw_window_handle::{HasDisplayHandle, HasWindowHandle as _, RawDisplayHandle, RawWindowHandle};
use std::collections::HashSet;
use std::ffi::{c_char, c_void, CStr};
use std::sync::atomic::Ordering;
use std::sync::Arc;
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
    pub initialized: bool,
    pub should_exit: bool,
}

pub struct WinitApp {
    pub state: Arc<Mutex<WinitAppState>>,
    pub thread_handle: Option<std::thread::JoinHandle<()>>,
    /// macOS requires the event loop and its AppKit window to stay on the process main thread.
    #[cfg(target_os = "macos")]
    mac_event_loop: Option<EventLoop<()>>,
    #[cfg(target_os = "macos")]
    mac_handler: Option<AppHandler>,
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
                                let (cx, cy) =
                                    ((state.width / 2) as f64, (state.height / 2) as f64);
                                let _ = win.set_cursor_position(
                                    winit::dpi::PhysicalPosition::new(cx, cy).into(),
                                );
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
                _ => {}
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
        initialized: false,
        should_exit: false,
    }));

    #[cfg(target_os = "macos")]
    let (thread_handle, mac_event_loop, mac_handler) = {
        use winit::platform::pump_events::EventLoopExtPumpEvents;
        let mut event_loop = EventLoop::builder().build().expect("create macOS event loop");
        event_loop.set_control_flow(ControlFlow::Poll);
        let mut handler = AppHandler { state: state.clone() };
        event_loop.pump_app_events(Some(Duration::ZERO), &mut handler);
        (None, Some(event_loop), Some(handler))
    };
    #[cfg(not(target_os = "macos"))]
    let thread_handle = {
        let state_clone = state.clone();
        Some(std::thread::spawn(move || {
            let mut builder = EventLoop::builder();
            #[cfg(all(unix, not(target_os = "android")))]
            {
                use winit::platform::x11::EventLoopBuilderExtX11;
                EventLoopBuilderExtX11::with_x11(&mut builder);
                EventLoopBuilderExtX11::with_any_thread(&mut builder, true);
            }
            let Ok(event_loop) = builder.build() else { return; };
            let handler = AppHandler { state: state_clone };
            event_loop.set_control_flow(ControlFlow::Poll);
            let _ = event_loop.run_app(handler);
        }))
    };

    // Wait briefly for window initialization (up to 500ms)
    for _ in 0..50 {
        if state.lock().initialized {
            break;
        }
        std::thread::sleep(Duration::from_millis(10));
    }

    let app = Box::new(WinitApp {
        state,
        #[cfg(target_os = "macos")]
        thread_handle,
        #[cfg(not(target_os = "macos"))]
        thread_handle,
        #[cfg(target_os = "macos")]
        mac_event_loop,
        #[cfg(target_os = "macos")]
        mac_handler,
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
    #[cfg(target_os = "macos")]
    {
        use winit::platform::pump_events::EventLoopExtPumpEvents;
        let app_mut = &mut *app;
        if let (Some(event_loop), Some(handler)) = (&mut app_mut.mac_event_loop, &mut app_mut.mac_handler) {
            event_loop.pump_app_events(Some(Duration::ZERO), handler);
        }
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
pub unsafe extern "C" fn winit_app_get_native_display(app: *mut WinitApp) -> *mut c_void {
    if app.is_null() { return std::ptr::null_mut(); }
    let window = match (*app).state.lock().window.clone() { Some(window) => window, None => return std::ptr::null_mut() };
    let Ok(display) = window.display_handle() else { return std::ptr::null_mut(); };
    match display.as_raw() {
        #[cfg(all(unix, not(target_os = "macos")))]
        RawDisplayHandle::Xlib(handle) => handle.display.map_or(std::ptr::null_mut(), |display| display.as_ptr()),
        // EGL_DEFAULT_DISPLAY is the correct display argument on Win32 and AppKit.
        _ => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub unsafe extern "C" fn winit_app_get_native_window(app: *mut WinitApp) -> *mut c_void {
    if app.is_null() { return std::ptr::null_mut(); }
    let window = match (*app).state.lock().window.clone() { Some(window) => window, None => return std::ptr::null_mut() };
    let Ok(handle) = window.window_handle() else { return std::ptr::null_mut(); };
    match handle.as_raw() {
        #[cfg(all(unix, not(target_os = "macos")))]
        RawWindowHandle::Xlib(handle) => handle.window as usize as *mut c_void,
        #[cfg(target_os = "windows")]
        RawWindowHandle::Win32(handle) => handle.hwnd.get() as *mut c_void,
        #[cfg(target_os = "macos")]
        RawWindowHandle::AppKit(handle) => handle.ns_view.as_ptr(),
        _ => std::ptr::null_mut(),
    }
}

