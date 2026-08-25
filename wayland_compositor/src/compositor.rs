//! Wayland compositor implementation managing the display, client sockets, surfaces and composition loop.

use parking_lot::Mutex;
use std::collections::HashMap;
use std::ffi::{CStr, CString, c_int, c_void};
use std::ptr;
use std::sync::Arc;
use std::sync::atomic::{AtomicU32, Ordering};

use crate::ffi::*;
use crate::protocol::*;
use crate::shm::FrameBuffer;

#[derive(Debug, Clone)]
pub struct CompositorConfig {
    pub socket_name: Option<String>,
    pub width: u32,
    pub height: u32,
}

impl Default for CompositorConfig {
    fn default() -> Self {
        Self {
            socket_name: Some("wayland-lce-0".to_string()),
            width: 1280,
            height: 720,
        }
    }
}

pub struct Compositor {
    display: *mut WlDisplay,
    event_loop: *mut WlEventLoop,
    socket_name: String,
    _width: u32,
    _height: u32,
    surfaces: Arc<Mutex<HashMap<u32, SurfaceState>>>,
    framebuffer: FrameBuffer,
}

// Global server context for dispatching
struct ServerContext {
    surfaces: Arc<Mutex<HashMap<u32, SurfaceState>>>,
    next_surface_id: AtomicU32,
}

impl Compositor {
    pub fn new(config: CompositorConfig) -> Result<Self, String> {
        let wl =
            wayland_server().map_err(|e| format!("Failed to load wayland-server library: {e}"))?;

        init_xdg_interfaces();

        unsafe {
            let display = (wl.wl_display_create)();
            if display.is_null() {
                return Err("Failed to create wl_display".to_string());
            }

            let event_loop = (wl.wl_display_get_event_loop)(display);
            if event_loop.is_null() {
                (wl.wl_display_destroy)(display);
                return Err("Failed to get wl_event_loop".to_string());
            }

            // Initialize SHM formats
            if (wl.wl_display_init_shm)(display) != 0 {
                (wl.wl_display_destroy)(display);
                return Err("Failed to init SHM on wl_display".to_string());
            }

            let socket_name = if let Some(name) = config.socket_name {
                let c_name = CString::new(name.clone()).unwrap();
                if (wl.wl_display_add_socket)(display, c_name.as_ptr()) != 0 {
                    (wl.wl_display_destroy)(display);
                    return Err(format!("Failed to bind to socket {}", name));
                }
                name
            } else {
                let auto_name = (wl.wl_display_add_socket_auto)(display);
                if auto_name.is_null() {
                    (wl.wl_display_destroy)(display);
                    return Err("Failed to add auto socket".to_string());
                }
                CStr::from_ptr(auto_name).to_string_lossy().into_owned()
            };

            let surfaces = Arc::new(Mutex::new(HashMap::new()));
            let ctx = Box::new(ServerContext {
                surfaces: surfaces.clone(),
                next_surface_id: AtomicU32::new(1),
            });

            let ctx_ptr = Box::into_raw(ctx);

            // Register globals: wl_compositor, wl_seat, wl_output, xdg_wm_base
            (wl.wl_global_create)(
                display,
                wl.wl_compositor_interface,
                4,
                ctx_ptr as *mut c_void,
                compositor_bind,
            );

            (wl.wl_global_create)(
                display,
                wl.wl_seat_interface,
                5,
                ctx_ptr as *mut c_void,
                seat_bind,
            );

            (wl.wl_global_create)(
                display,
                wl.wl_output_interface,
                3,
                ctx_ptr as *mut c_void,
                output_bind,
            );

            (wl.wl_global_create)(
                display,
                &raw const XDG_WM_BASE_INTERFACE,
                4,
                ctx_ptr as *mut c_void,
                xdg_wm_base_bind,
            );

            let framebuffer = FrameBuffer::new(config.width, config.height);

            Ok(Self {
                display,
                event_loop,
                socket_name,
                _width: config.width,
                _height: config.height,
                surfaces,
                framebuffer,
            })
        }
    }

    pub fn socket_name(&self) -> &str {
        &self.socket_name
    }

    pub fn dispatch(&mut self, timeout_ms: i32) -> i32 {
        if let Ok(wl) = wayland_server() {
            unsafe {
                (wl.wl_display_flush_clients)(self.display);
                let res = (wl.wl_event_loop_dispatch)(self.event_loop, timeout_ms);
                (wl.wl_display_flush_clients)(self.display);
                res
            }
        } else {
            -1
        }
    }

    /// Render current client surfaces to the framebuffer and trigger frame callbacks.
    pub fn render_frame(&mut self, time_ms: u32) -> &FrameBuffer {
        self.framebuffer.clear(30, 30, 40, 255); // Dark slate background

        let mut surfaces = self.surfaces.lock();
        for (_, surface) in surfaces.iter_mut() {
            if let Some(buf) = surface.current_buffer {
                self.framebuffer.blit_shm_buffer(buf, surface.x, surface.y);
            }
            surface.trigger_frame_callbacks(time_ms);
        }

        &self.framebuffer
    }

    pub fn get_framebuffer(&self) -> &FrameBuffer {
        &self.framebuffer
    }
}

impl Drop for Compositor {
    fn drop(&mut self) {
        if let Ok(wl) = wayland_server() {
            unsafe {
                if !self.display.is_null() {
                    (wl.wl_display_destroy)(self.display);
                }
            }
        }
    }
}

// Global BIND handlers

extern "C" fn compositor_bind(client: *mut WlClient, data: *mut c_void, version: u32, id: u32) {
    if let Ok(wl) = wayland_server() {
        unsafe {
            let res =
                (wl.wl_resource_create)(client, wl.wl_compositor_interface, version as c_int, id);
            (wl.wl_resource_set_dispatcher)(res, compositor_dispatch, ptr::null(), data, None);
        }
    }
}

extern "C" fn compositor_dispatch(
    user_data: *const c_void,
    target: *mut c_void,
    opcode: u32,
    _msg: *const WlMessage,
    args: *mut WlArgument,
) -> c_int {
    let ctx = unsafe { &*(user_data as *const ServerContext) };
    if let Ok(wl) = wayland_server() {
        unsafe {
            match opcode {
                0 => {
                    // create_surface(id)
                    let id = (*args.add(0)).n;
                    let client = (wl.wl_resource_get_client)(target as *mut WlResource);
                    let surface_res = (wl.wl_resource_create)(
                        client,
                        wl.wl_surface_interface,
                        (wl.wl_resource_get_version)(target as *mut WlResource),
                        id,
                    );

                    let surf_id = ctx.next_surface_id.fetch_add(1, Ordering::SeqCst);
                    let state = SurfaceState::new(surf_id, surface_res);
                    ctx.surfaces.lock().insert(surf_id, state);

                    let surf_id_ptr = Box::into_raw(Box::new(surf_id));
                    (wl.wl_resource_set_dispatcher)(
                        surface_res,
                        surface_dispatch,
                        ptr::null(),
                        surf_id_ptr as *mut c_void,
                        Some(surface_resource_destroy),
                    );
                }
                1 => {
                    // create_region(id)
                    let id = (*args.add(0)).n;
                    let client = (wl.wl_resource_get_client)(target as *mut WlResource);
                    let region_res = (wl.wl_resource_create)(
                        client,
                        wl.wl_region_interface,
                        (wl.wl_resource_get_version)(target as *mut WlResource),
                        id,
                    );
                    (wl.wl_resource_set_dispatcher)(
                        region_res,
                        region_dispatch,
                        ptr::null(),
                        ptr::null_mut(),
                        None,
                    );
                }
                _ => {}
            }
        }
    }
    0
}

extern "C" fn surface_resource_destroy(resource: *mut WlResource) {
    if let Ok(wl) = wayland_server() {
        unsafe {
            let user_data = (wl.wl_resource_get_user_data)(resource);
            if !user_data.is_null() {
                let _surf_id = Box::from_raw(user_data as *mut u32);
            }
        }
    }
}

extern "C" fn surface_dispatch(
    user_data: *const c_void,
    target: *mut c_void,
    opcode: u32,
    _msg: *const WlMessage,
    args: *mut WlArgument,
) -> c_int {
    if user_data.is_null() {
        return 0;
    }
    let _surf_id = unsafe { *(user_data as *const u32) };

    if let Ok(wl) = wayland_server() {
        unsafe {
            match opcode {
                0 => {
                    // destroy
                    (wl.wl_resource_destroy)(target as *mut WlResource);
                }
                1 => {
                    // attach(buffer, x, y)
                    let _buffer = (*args.add(0)).o;
                }
                2 => {
                    // damage(x, y, width, height)
                }
                3 => {
                    // frame(callback_id)
                    let cb_id = (*args.add(0)).n;
                    let client = (wl.wl_resource_get_client)(target as *mut WlResource);
                    let _cb_res =
                        (wl.wl_resource_create)(client, wl.wl_callback_interface, 1, cb_id);
                }
                6 => {
                    // commit
                }
                _ => {}
            }
        }
    }
    0
}

extern "C" fn region_dispatch(
    _user_data: *const c_void,
    _target: *mut c_void,
    _opcode: u32,
    _msg: *const WlMessage,
    _args: *mut WlArgument,
) -> c_int {
    0
}

extern "C" fn seat_bind(client: *mut WlClient, data: *mut c_void, version: u32, id: u32) {
    if let Ok(wl) = wayland_server() {
        unsafe {
            let res = (wl.wl_resource_create)(client, wl.wl_seat_interface, version as c_int, id);
            // Advertise capabilities: Pointer (1) | Keyboard (2)
            (wl.wl_resource_post_event)(res, 0, 3u32); // capabilities event
            if version >= 2 {
                let seat_name = CString::new("default").unwrap();
                (wl.wl_resource_post_event)(res, 1, seat_name.as_ptr());
            }
            (wl.wl_resource_set_dispatcher)(res, seat_dispatch, ptr::null(), data, None);
        }
    }
}

extern "C" fn seat_dispatch(
    _user_data: *const c_void,
    target: *mut c_void,
    opcode: u32,
    _msg: *const WlMessage,
    args: *mut WlArgument,
) -> c_int {
    if let Ok(wl) = wayland_server() {
        unsafe {
            let client = (wl.wl_resource_get_client)(target as *mut WlResource);
            match opcode {
                0 => {
                    // get_pointer(id)
                    let id = (*args.add(0)).n;
                    let ptr_res = (wl.wl_resource_create)(
                        client,
                        wl.wl_pointer_interface,
                        (wl.wl_resource_get_version)(target as *mut WlResource),
                        id,
                    );
                    (wl.wl_resource_set_dispatcher)(
                        ptr_res,
                        pointer_dispatch,
                        ptr::null(),
                        ptr::null_mut(),
                        None,
                    );
                }
                1 => {
                    // get_keyboard(id)
                    let id = (*args.add(0)).n;
                    let kbd_res = (wl.wl_resource_create)(
                        client,
                        wl.wl_keyboard_interface,
                        (wl.wl_resource_get_version)(target as *mut WlResource),
                        id,
                    );
                    (wl.wl_resource_set_dispatcher)(
                        kbd_res,
                        keyboard_dispatch,
                        ptr::null(),
                        ptr::null_mut(),
                        None,
                    );
                }
                _ => {}
            }
        }
    }
    0
}

extern "C" fn pointer_dispatch(
    _user_data: *const c_void,
    _target: *mut c_void,
    _opcode: u32,
    _msg: *const WlMessage,
    _args: *mut WlArgument,
) -> c_int {
    0
}

extern "C" fn keyboard_dispatch(
    _user_data: *const c_void,
    _target: *mut c_void,
    _opcode: u32,
    _msg: *const WlMessage,
    _args: *mut WlArgument,
) -> c_int {
    0
}

extern "C" fn output_bind(client: *mut WlClient, _data: *mut c_void, version: u32, id: u32) {
    if let Ok(wl) = wayland_server() {
        unsafe {
            let res = (wl.wl_resource_create)(client, wl.wl_output_interface, version as c_int, id);
            let make = CString::new("LCE").unwrap();
            let model = CString::new("Virtual Display").unwrap();
            (wl.wl_resource_post_event)(
                res,
                0,
                0i32,
                0i32,
                300i32,
                200i32,
                1i32,
                make.as_ptr(),
                model.as_ptr(),
                0i32,
            );
            (wl.wl_resource_post_event)(res, 1, 3u32, 1280i32, 720i32, 60000i32);
            if version >= 2 {
                (wl.wl_resource_post_event)(res, 2, 1i32); // scale: 1
                (wl.wl_resource_post_event)(res, 3); // done event
            }
        }
    }
}

extern "C" fn xdg_wm_base_bind(client: *mut WlClient, data: *mut c_void, version: u32, id: u32) {
    if let Ok(wl) = wayland_server() {
        unsafe {
            let res = (wl.wl_resource_create)(
                client,
                &raw const XDG_WM_BASE_INTERFACE,
                version as c_int,
                id,
            );
            (wl.wl_resource_set_dispatcher)(res, xdg_wm_base_dispatch, ptr::null(), data, None);
        }
    }
}

extern "C" fn xdg_wm_base_dispatch(
    _user_data: *const c_void,
    target: *mut c_void,
    opcode: u32,
    _msg: *const WlMessage,
    args: *mut WlArgument,
) -> c_int {
    if let Ok(wl) = wayland_server() {
        unsafe {
            match opcode {
                0 => {
                    // destroy
                    (wl.wl_resource_destroy)(target as *mut WlResource);
                }
                2 => {
                    // get_xdg_surface(id, surface)
                    let id = (*args.add(0)).n;
                    let surface_res = (*args.add(1)).o;
                    let client = (wl.wl_resource_get_client)(target as *mut WlResource);

                    let xdg_surface = (wl.wl_resource_create)(
                        client,
                        &raw const XDG_SURFACE_INTERFACE,
                        (wl.wl_resource_get_version)(target as *mut WlResource),
                        id,
                    );

                    (wl.wl_resource_set_dispatcher)(
                        xdg_surface,
                        xdg_surface_dispatch,
                        ptr::null(),
                        surface_res as *mut c_void,
                        None,
                    );
                }
                3 => {
                    // pong(serial)
                }
                _ => {}
            }
        }
    }
    0
}

extern "C" fn xdg_surface_dispatch(
    user_data: *const c_void,
    target: *mut c_void,
    opcode: u32,
    _msg: *const WlMessage,
    args: *mut WlArgument,
) -> c_int {
    let surface_res = user_data as *mut WlResource;
    if let Ok(wl) = wayland_server() {
        unsafe {
            match opcode {
                0 => {
                    // destroy
                    (wl.wl_resource_destroy)(target as *mut WlResource);
                }
                1 => {
                    // get_toplevel(id)
                    let id = (*args.add(0)).n;
                    let client = (wl.wl_resource_get_client)(target as *mut WlResource);
                    let toplevel = (wl.wl_resource_create)(
                        client,
                        &raw const XDG_TOPLEVEL_INTERFACE,
                        (wl.wl_resource_get_version)(target as *mut WlResource),
                        id,
                    );

                    (wl.wl_resource_set_dispatcher)(
                        toplevel,
                        xdg_toplevel_dispatch,
                        ptr::null(),
                        surface_res as *mut c_void,
                        None,
                    );

                    // Send initial configure event for toplevel and surface
                    let mut empty_array = WlArray {
                        size: 0,
                        alloc: 0,
                        data: ptr::null_mut(),
                    };
                    (wl.wl_resource_post_event)(
                        toplevel,
                        0,
                        1280i32,
                        720i32,
                        &mut empty_array as *mut WlArray,
                    );
                    (wl.wl_resource_post_event)(target as *mut WlResource, 0, 1u32); // configure(serial=1)
                }
                4 => {
                    // ack_configure(serial)
                }
                _ => {}
            }
        }
    }
    0
}

extern "C" fn xdg_toplevel_dispatch(
    _user_data: *const c_void,
    target: *mut c_void,
    opcode: u32,
    _msg: *const WlMessage,
    _args: *mut WlArgument,
) -> c_int {
    if let Ok(wl) = wayland_server() {
        unsafe {
            match opcode {
                0 => {
                    // destroy
                    (wl.wl_resource_destroy)(target as *mut WlResource);
                }
                _ => {}
            }
        }
    }
    0
}
