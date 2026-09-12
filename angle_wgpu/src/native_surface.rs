//! Application-supplied native-window EGL surface creation.
//!
//! The host owns its native window and uses the normal EGL calls
//! `eglGetDisplay` and `eglCreateWindowSurface`.  This module translates the
//! target platform's EGLNativeWindowType into raw-window-handle for wgpu.

use crate::egl::{block_on, get_or_create_display, EglDisplayState, EglSurfaceState};
use crate::renderer::{PresentWindow, WgpuRenderer};
use crate::types::*;
use parking_lot::Mutex;
use raw_window_handle::{DisplayHandle, HandleError, HasDisplayHandle, HasWindowHandle, WindowHandle};
use std::ffi::c_void;
use std::sync::Arc;

#[derive(Debug)]
struct NativePresenter;
impl PresentWindow for NativePresenter {
    fn pre_present_notify(&self) {}
    fn request_redraw(&self) {}
}

fn register_surface<T>(dpy: EGLDisplay, native_window: NativeWindowType, handles: T, width: u32, height: u32) -> EGLSurface
where
    T: HasDisplayHandle + HasWindowHandle + std::fmt::Debug + Send + Sync + Clone + 'static,
{
    crate::init_logging();
    let instance = wgpu::Instance::new(wgpu::InstanceDescriptor::new_with_display_handle(Box::new(handles.clone())));
    let surface = match instance.create_surface(Arc::new(handles)) {
        Ok(surface) => surface,
        Err(error) => {
            eprintln!("[angle_wgpu] create_surface failed: {error:#?}");
            return EGL_NO_SURFACE;
        }
    };
    let adapter = match block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::HighPerformance,
        compatible_surface: Some(&surface),
        force_fallback_adapter: false,
        apply_limit_buckets: false,
    })) {
        Ok(adapter) => adapter,
        Err(error) => {
            eprintln!("[angle_wgpu] request_adapter failed: {error:?}");
            return EGL_NO_SURFACE;
        }
    };
    let (device, queue) = match block_on(adapter.request_device(&wgpu::DeviceDescriptor {
        label: Some("angle_wgpu Native Surface Device"),
        ..Default::default()
    })) {
        Ok(result) => result,
        Err(error) => {
            eprintln!("[angle_wgpu] request_device failed: {error:?}");
            return EGL_NO_SURFACE;
        }
    };
    let renderer = match WgpuRenderer::new_with_surface(instance, adapter, device, queue, surface,
        Arc::new(NativePresenter), width.max(1), height.max(1)) {
        Ok(renderer) => Arc::new(Mutex::new(renderer)),
        Err(error) => {
            eprintln!("[angle_wgpu] new_with_surface failed: {error}");
            return EGL_NO_SURFACE;
        }
    };
    let dpy_arc = if dpy.is_null() { get_or_create_display() } else { unsafe { Arc::from_raw(dpy as *const Mutex<EglDisplayState>) } };
    let id = dpy_arc.lock().allocate_surface_id();
    let surface_state = Arc::new(Mutex::new(EglSurfaceState { id, width: width.max(1), height: height.max(1), native_window, renderer: Some(renderer) }));
    dpy_arc.lock().surfaces.insert(id, surface_state.clone());
    if !dpy.is_null() { std::mem::forget(dpy_arc); }
    Arc::into_raw(surface_state) as EGLSurface
}

#[cfg(all(unix, not(target_os = "macos")))]
#[derive(Debug, Clone)]
struct XlibNativeWindow { display: raw_window_handle::XlibDisplayHandle, window: raw_window_handle::XlibWindowHandle }
#[cfg(all(unix, not(target_os = "macos")))]
unsafe impl Send for XlibNativeWindow {}
#[cfg(all(unix, not(target_os = "macos")))]
unsafe impl Sync for XlibNativeWindow {}
#[cfg(all(unix, not(target_os = "macos")))]
impl HasDisplayHandle for XlibNativeWindow { fn display_handle(&self) -> Result<DisplayHandle<'_>, HandleError> { Ok(unsafe { DisplayHandle::borrow_raw(raw_window_handle::RawDisplayHandle::Xlib(self.display)) }) } }
#[cfg(all(unix, not(target_os = "macos")))]
impl HasWindowHandle for XlibNativeWindow { fn window_handle(&self) -> Result<WindowHandle<'_>, HandleError> { Ok(unsafe { WindowHandle::borrow_raw(raw_window_handle::RawWindowHandle::Xlib(self.window)) }) } }

#[cfg(target_os = "windows")]
#[derive(Debug, Clone)]
struct Win32NativeWindow { window: raw_window_handle::Win32WindowHandle }
#[cfg(target_os = "windows")]
unsafe impl Send for Win32NativeWindow {}
#[cfg(target_os = "windows")]
unsafe impl Sync for Win32NativeWindow {}
#[cfg(target_os = "windows")]
impl HasDisplayHandle for Win32NativeWindow { fn display_handle(&self) -> Result<DisplayHandle<'_>, HandleError> { Ok(unsafe { DisplayHandle::borrow_raw(raw_window_handle::RawDisplayHandle::Windows(raw_window_handle::WindowsDisplayHandle::new())) }) } }
#[cfg(target_os = "windows")]
impl HasWindowHandle for Win32NativeWindow { fn window_handle(&self) -> Result<WindowHandle<'_>, HandleError> { Ok(unsafe { WindowHandle::borrow_raw(raw_window_handle::RawWindowHandle::Win32(self.window)) }) } }

#[cfg(target_os = "macos")]
#[derive(Debug, Clone)]
struct AppKitNativeWindow { window: raw_window_handle::AppKitWindowHandle }
#[cfg(target_os = "macos")]
unsafe impl Send for AppKitNativeWindow {}
#[cfg(target_os = "macos")]
unsafe impl Sync for AppKitNativeWindow {}
#[cfg(target_os = "macos")]
impl HasDisplayHandle for AppKitNativeWindow { fn display_handle(&self) -> Result<DisplayHandle<'_>, HandleError> { Ok(unsafe { DisplayHandle::borrow_raw(raw_window_handle::RawDisplayHandle::AppKit(raw_window_handle::AppKitDisplayHandle::new())) }) } }
#[cfg(target_os = "macos")]
impl HasWindowHandle for AppKitNativeWindow { fn window_handle(&self) -> Result<WindowHandle<'_>, HandleError> { Ok(unsafe { WindowHandle::borrow_raw(raw_window_handle::RawWindowHandle::AppKit(self.window)) }) } }

/// Creates a wgpu-backed EGL surface for the platform-native window passed to EGL.
pub unsafe fn create_native_egl_surface(dpy: EGLDisplay, native_display: *mut c_void, native_window: NativeWindowType, width: u32, height: u32) -> EGLSurface {
    if native_window.is_null() { return EGL_NO_SURFACE; }
    #[cfg(all(unix, not(target_os = "macos")))] {
        use std::ptr::NonNull;
        let window = raw_window_handle::XlibWindowHandle::new(native_window as usize as _);
        let handles = XlibNativeWindow { display: raw_window_handle::XlibDisplayHandle::new(NonNull::new(native_display), 0), window };
        return register_surface(dpy, native_window, handles, width, height);
    }
    #[cfg(target_os = "windows")] {
        use std::num::NonZeroIsize;
        let Some(hwnd) = NonZeroIsize::new(native_window as isize) else { return EGL_NO_SURFACE; };
        return register_surface(dpy, native_window, Win32NativeWindow { window: raw_window_handle::Win32WindowHandle::new(hwnd) }, width, height);
    }
    #[cfg(target_os = "macos")] {
        let Some(view) = std::ptr::NonNull::new(native_window) else { return EGL_NO_SURFACE; };
        return register_surface(dpy, native_window, AppKitNativeWindow { window: raw_window_handle::AppKitWindowHandle::new(view) }, width, height);
    }
    #[allow(unreachable_code)]
    EGL_NO_SURFACE
}
