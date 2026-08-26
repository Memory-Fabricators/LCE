//! EGL 1.4 API implementation and context/surface management.
#![allow(unused_imports, dead_code)]
use crate::display_list::DisplayListRegistry;
use crate::gl_context::GlContext;
use crate::renderer::WgpuRenderer;
use crate::texture::TextureManager;
use crate::types::*;
use parking_lot::Mutex;
use std::cell::RefCell;
use std::collections::HashMap;
use std::ffi::{CStr, c_void};
use std::future::Future;
use std::sync::Arc;
use std::sync::atomic::{AtomicU32, Ordering};

pub fn block_on<F: Future>(mut future: F) -> F::Output {
    use std::pin::Pin;
    use std::task::{Context, Poll, RawWaker, RawWakerVTable, Waker};

    fn noop_clone(_: *const ()) -> RawWaker {
        noop_raw_waker()
    }
    fn noop(_: *const ()) {}
    fn noop_raw_waker() -> RawWaker {
        static VTABLE: RawWakerVTable = RawWakerVTable::new(noop_clone, noop, noop, noop);
        RawWaker::new(std::ptr::null(), &VTABLE)
    }

    let waker = unsafe { Waker::from_raw(noop_raw_waker()) };
    let mut cx = Context::from_waker(&waker);
    let mut future = unsafe { Pin::new_unchecked(&mut future) };

    loop {
        match future.as_mut().poll(&mut cx) {
            Poll::Ready(result) => return result,
            Poll::Pending => std::thread::yield_now(),
        }
    }
}
pub struct EglSurfaceState {
    pub id: u32,
    pub width: u32,
    pub height: u32,
    pub native_window: NativeWindowType,
    pub renderer: Option<Arc<Mutex<WgpuRenderer>>>,
}

pub struct EglContextState {
    pub id: u32,
    pub gl_context: Arc<Mutex<GlContext>>,
}

pub struct EglDisplayState {
    pub initialized: bool,
    pub surfaces: HashMap<u32, Arc<Mutex<EglSurfaceState>>>,
    pub contexts: HashMap<u32, Arc<Mutex<EglContextState>>>,
    pub shared_textures: Arc<Mutex<TextureManager>>,
    pub shared_display_lists: Arc<DisplayListRegistry>,
    next_surface_id: AtomicU32,
    next_context_id: AtomicU32,
}

impl Default for EglDisplayState {
    fn default() -> Self {
        Self {
            initialized: false,
            surfaces: HashMap::new(),
            contexts: HashMap::new(),
            shared_textures: Arc::new(Mutex::new(TextureManager::new())),
            shared_display_lists: Arc::new(DisplayListRegistry::new()),
            next_surface_id: AtomicU32::new(1),
            next_context_id: AtomicU32::new(1),
        }
    }
}
impl EglDisplayState {
    pub fn allocate_surface_id(&self) -> u32 {
        self.next_surface_id.fetch_add(1, Ordering::SeqCst)
    }

    pub fn allocate_context_id(&self) -> u32 {
        self.next_context_id.fetch_add(1, Ordering::SeqCst)
    }
}

unsafe impl Send for EglSurfaceState {}
unsafe impl Sync for EglSurfaceState {}
unsafe impl Send for EglContextState {}
unsafe impl Sync for EglContextState {}
unsafe impl Send for EglDisplayState {}
unsafe impl Sync for EglDisplayState {}

// Global display singleton
static GLOBAL_DISPLAY: Mutex<Option<Arc<Mutex<EglDisplayState>>>> = Mutex::new(None);
static LAST_EGL_ERROR: Mutex<EGLint> = Mutex::new(EGL_SUCCESS);

// The game spawns chunk-rebuild worker threads that issue GL calls (via
// display-list compilation) without ever calling `eglMakeCurrent` on that
// thread themselves - real consoles this was ported from share one implicit
// GL context across threads. Desktop EGL is strictly per-thread-current, so
// without this fallback every GL call from those threads silently no-ops
// (see `with_context`), and displays lists built there are never actually
// created: `glCallList` on the render thread then calls empty/nonexistent
// lists, so terrain compiles but never draws. This app only ever creates a
// single GL context, so falling back to "whichever context was last made
// current on any thread" is safe and matches the game's actual usage.
static LAST_ACTIVE_CONTEXT: Mutex<Option<Arc<Mutex<GlContext>>>> = Mutex::new(None);

thread_local! {
    pub static CURRENT_CONTEXT: RefCell<Option<Arc<Mutex<GlContext>>>> = const { RefCell::new(None) };
    pub static CURRENT_SURFACE: RefCell<Option<Arc<Mutex<EglSurfaceState>>>> = const { RefCell::new(None) };
}

pub fn get_current_gl_context() -> Option<Arc<Mutex<GlContext>>> {
    if let Some(ctx) = CURRENT_CONTEXT.with(|c| c.borrow().clone()) {
        return Some(ctx);
    }
    LAST_ACTIVE_CONTEXT.lock().clone()
}

pub fn set_egl_error(err: EGLint) {
    *LAST_EGL_ERROR.lock() = err;
}

pub fn get_or_create_display() -> Arc<Mutex<EglDisplayState>> {
    let mut gd = GLOBAL_DISPLAY.lock();
    if gd.is_none() {
        *gd = Some(Arc::new(Mutex::new(EglDisplayState::default())));
    }
    gd.as_ref().unwrap().clone()
}

// ============================================================================
// EGL C API
// ============================================================================

pub unsafe fn egl_get_display(_display_id: NativeDisplayType) -> EGLDisplay {
    crate::init_logging();
    let dpy = get_or_create_display();
    Arc::into_raw(dpy) as EGLDisplay
}

pub unsafe fn egl_initialize(
    dpy: EGLDisplay,
    major: *mut EGLint,
    minor: *mut EGLint,
) -> EGLBoolean {
    if dpy.is_null() {
        set_egl_error(EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    let dpy_arc = Arc::from_raw(dpy as *const Mutex<EglDisplayState>);
    let res = {
        let mut d = dpy_arc.lock();
        d.initialized = true;
        if !major.is_null() {
            *major = 1;
        }
        if !minor.is_null() {
            *minor = 4;
        }
        EGL_TRUE
    };
    std::mem::forget(dpy_arc);
    res
}

pub unsafe fn egl_terminate(dpy: EGLDisplay) -> EGLBoolean {
    if dpy.is_null() {
        set_egl_error(EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    let dpy_arc = Arc::from_raw(dpy as *const Mutex<EglDisplayState>);
    {
        let mut d = dpy_arc.lock();
        d.initialized = false;
        d.contexts.clear();
        d.surfaces.clear();
    }
    std::mem::forget(dpy_arc);
    EGL_TRUE
}

pub unsafe fn egl_get_configs(
    _dpy: EGLDisplay,
    configs: *mut EGLConfig,
    config_size: EGLint,
    num_config: *mut EGLint,
) -> EGLBoolean {
    if !num_config.is_null() {
        *num_config = 1;
    }
    if !configs.is_null() && config_size > 0 {
        *configs = 1 as EGLConfig;
    }
    EGL_TRUE
}

pub unsafe fn egl_choose_config(
    _dpy: EGLDisplay,
    _attrib_list: *const EGLint,
    configs: *mut EGLConfig,
    config_size: EGLint,
    num_config: *mut EGLint,
) -> EGLBoolean {
    if !num_config.is_null() {
        *num_config = 1;
    }
    if !configs.is_null() && config_size > 0 {
        *configs = 1 as EGLConfig;
    }
    EGL_TRUE
}

pub unsafe fn egl_get_config_attrib(
    _dpy: EGLDisplay,
    _config: EGLConfig,
    attribute: EGLint,
    value: *mut EGLint,
) -> EGLBoolean {
    if value.is_null() {
        return EGL_FALSE;
    }

    match attribute {
        EGL_BUFFER_SIZE => *value = 32,
        EGL_RED_SIZE => *value = 8,
        EGL_GREEN_SIZE => *value = 8,
        EGL_BLUE_SIZE => *value = 8,
        EGL_ALPHA_SIZE => *value = 8,
        EGL_DEPTH_SIZE => *value = 24,
        EGL_STENCIL_SIZE => *value = 8,
        EGL_SURFACE_TYPE => *value = EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE => *value = EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT,
        _ => *value = 0,
    }
    EGL_TRUE
}

pub unsafe fn egl_create_window_surface(
    dpy: EGLDisplay,
    _config: EGLConfig,
    win: NativeWindowType,
    attrib_list: *const EGLint,
) -> EGLSurface {
    let mut width = 1280;
    let mut height = 720;

    if !attrib_list.is_null() {
        let mut ptr = attrib_list;
        while *ptr != EGL_NONE {
            let attr = *ptr;
            let val = *ptr.add(1);
            if attr == EGL_WIDTH {
                width = val.max(1) as u32;
            } else if attr == EGL_HEIGHT {
                height = val.max(1) as u32;
            }
            ptr = ptr.add(2);
        }
    }

    let dpy_arc = if !dpy.is_null() {
        Arc::from_raw(dpy as *const Mutex<EglDisplayState>)
    } else {
        get_or_create_display()
    };

    let id = {
        let d = dpy_arc.lock();
        d.next_surface_id.fetch_add(1, Ordering::SeqCst)
    };

    let renderer = match block_on(WgpuRenderer::new_headless(width, height)) {
        Ok(r) => Some(Arc::new(Mutex::new(r))),
        Err(e) => {
            log::warn!("WgpuRenderer headless fallback: {e}");
            None
        }
    };

    let surface_state = Arc::new(Mutex::new(EglSurfaceState {
        id,
        width,
        height,
        native_window: win,
        renderer,
    }));

    {
        let mut d = dpy_arc.lock();
        d.surfaces.insert(id, surface_state.clone());
    }

    if !dpy.is_null() {
        std::mem::forget(dpy_arc);
    }

    Arc::into_raw(surface_state) as EGLSurface
}

pub unsafe fn egl_create_pbuffer_surface(
    dpy: EGLDisplay,
    _config: EGLConfig,
    attrib_list: *const EGLint,
) -> EGLSurface {
    egl_create_window_surface(dpy, _config, std::ptr::null_mut(), attrib_list)
}

pub unsafe fn egl_destroy_surface(dpy: EGLDisplay, surface: EGLSurface) -> EGLBoolean {
    if surface.is_null() {
        return EGL_TRUE;
    }
    let surf_arc = Arc::from_raw(surface as *const Mutex<EglSurfaceState>);
    let id = surf_arc.lock().id;

    if !dpy.is_null() {
        let dpy_arc = Arc::from_raw(dpy as *const Mutex<EglDisplayState>);
        dpy_arc.lock().surfaces.remove(&id);
        std::mem::forget(dpy_arc);
    }

    EGL_TRUE
}

pub unsafe fn egl_create_context(
    dpy: EGLDisplay,
    _config: EGLConfig,
    _share_context: EGLContext,
    _attrib_list: *const EGLint,
) -> EGLContext {
    let dpy_arc = if !dpy.is_null() {
        Arc::from_raw(dpy as *const Mutex<EglDisplayState>)
    } else {
        get_or_create_display()
    };

    let (id, shared_textures, shared_lists) = {
        let d = dpy_arc.lock();
        (
            d.next_context_id.fetch_add(1, Ordering::SeqCst),
            d.shared_textures.clone(),
            d.shared_display_lists.clone(),
        )
    };

    let gl_ctx = Arc::new(Mutex::new(GlContext::new(
        id,
        None,
        shared_textures,
        shared_lists,
    )));
    let ctx_state = Arc::new(Mutex::new(EglContextState {
        id,
        gl_context: gl_ctx,
    }));

    {
        let mut d = dpy_arc.lock();
        d.contexts.insert(id, ctx_state.clone());
    }

    if !dpy.is_null() {
        std::mem::forget(dpy_arc);
    }

    Arc::into_raw(ctx_state) as EGLContext
}

pub unsafe fn egl_destroy_context(dpy: EGLDisplay, ctx: EGLContext) -> EGLBoolean {
    if ctx.is_null() {
        return EGL_TRUE;
    }
    let ctx_arc = Arc::from_raw(ctx as *const Mutex<EglContextState>);
    let id = ctx_arc.lock().id;

    if !dpy.is_null() {
        let dpy_arc = Arc::from_raw(dpy as *const Mutex<EglDisplayState>);
        dpy_arc.lock().contexts.remove(&id);
        std::mem::forget(dpy_arc);
    }

    EGL_TRUE
}

pub unsafe fn egl_make_current(
    _dpy: EGLDisplay,
    draw: EGLSurface,
    _read: EGLSurface,
    ctx: EGLContext,
) -> EGLBoolean {
    if ctx.is_null() {
        CURRENT_CONTEXT.with(|c| *c.borrow_mut() = None);
        CURRENT_SURFACE.with(|s| *s.borrow_mut() = None);
        return EGL_TRUE;
    }

    let ctx_arc = Arc::from_raw(ctx as *const Mutex<EglContextState>);
    let gl_ctx = ctx_arc.lock().gl_context.clone();
    std::mem::forget(ctx_arc);

    if !draw.is_null() {
        let surf_arc = Arc::from_raw(draw as *const Mutex<EglSurfaceState>);
        let (w, h, renderer) = {
            let s = surf_arc.lock();
            (s.width, s.height, s.renderer.clone())
        };
        std::mem::forget(surf_arc.clone());

        {
            let mut gl = gl_ctx.lock();
            gl.renderer = renderer;
            gl.viewport = (0, 0, w as i32, h as i32);
            gl.scissor = (0, 0, w as i32, h as i32);
        }

        CURRENT_SURFACE.with(|s| *s.borrow_mut() = Some(surf_arc));
    }

    *LAST_ACTIVE_CONTEXT.lock() = Some(gl_ctx.clone());
    CURRENT_CONTEXT.with(|c| *c.borrow_mut() = Some(gl_ctx));
    EGL_TRUE
}

pub unsafe fn egl_get_current_context() -> EGLContext {
    CURRENT_CONTEXT.with(|c| {
        if let Some(ctx) = c.borrow().as_ref() {
            Arc::as_ptr(ctx) as EGLContext
        } else {
            EGL_NO_CONTEXT
        }
    })
}

pub unsafe fn egl_get_current_surface(_readdraw: EGLint) -> EGLSurface {
    CURRENT_SURFACE.with(|s| {
        if let Some(surf) = s.borrow().as_ref() {
            Arc::as_ptr(surf) as EGLSurface
        } else {
            EGL_NO_SURFACE
        }
    })
}

pub unsafe fn egl_get_current_display() -> EGLDisplay {
    let dpy = get_or_create_display();
    Arc::as_ptr(&dpy) as EGLDisplay
}

pub unsafe fn egl_query_surface(
    _dpy: EGLDisplay,
    surface: EGLSurface,
    attribute: EGLint,
    value: *mut EGLint,
) -> EGLBoolean {
    if surface.is_null() || value.is_null() {
        return EGL_FALSE;
    }

    let surf_arc = Arc::from_raw(surface as *const Mutex<EglSurfaceState>);
    {
        let s = surf_arc.lock();
        match attribute {
            EGL_WIDTH => *value = s.width as EGLint,
            EGL_HEIGHT => *value = s.height as EGLint,
            _ => *value = 0,
        }
    }
    std::mem::forget(surf_arc);
    EGL_TRUE
}

pub unsafe fn egl_swap_buffers(_dpy: EGLDisplay, surface: EGLSurface) -> EGLBoolean {
    if surface.is_null() {
        return EGL_FALSE;
    }

    let surf_arc = Arc::from_raw(surface as *const Mutex<EglSurfaceState>);
    let renderer = surf_arc.lock().renderer.clone();
    std::mem::forget(surf_arc);

    // Flush any pending batched draw before presenting so it isn't lost /
    // reordered relative to the swap.
    if let Some(ctx_arc) = get_current_gl_context() {
        ctx_arc.lock().flush_pending_batch();
    }

    if let Some(r) = renderer {
        let mut rend = r.lock();
        if let Err(e) = rend.swap_buffers() {
            log::error!("eglSwapBuffers error: {e}");
            return EGL_FALSE;
        }
    }
    EGL_TRUE
}

pub unsafe fn egl_swap_interval(_dpy: EGLDisplay, _interval: EGLint) -> EGLBoolean {
    EGL_TRUE
}

pub unsafe fn egl_get_error() -> EGLint {
    let mut err = LAST_EGL_ERROR.lock();
    let res = *err;
    *err = EGL_SUCCESS;
    res
}

pub unsafe fn egl_get_proc_address(
    procname: *const std::ffi::c_char,
) -> __eglMustCastToProperFunctionPointerType {
    if procname.is_null() {
        return None;
    }
    let Ok(name) = CStr::from_ptr(procname).to_str() else {
        return None;
    };

    crate::get_gl_proc_address(name)
}
