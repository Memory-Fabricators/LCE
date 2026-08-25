//! Wayland protocol object definitions and dispatchers for wl_compositor, wl_surface, wl_shm, xdg_wm_base, etc.

use std::ffi::c_char;
use std::ptr;

use crate::ffi::*;

/// Represents the state of a single Wayland surface.
#[derive(Debug)]
pub struct SurfaceState {
    pub id: u32,
    pub resource: *mut WlResource,
    pub current_buffer: Option<*mut WlResource>,
    pub pending_buffer: Option<*mut WlResource>,
    pub width: i32,
    pub height: i32,
    pub x: i32,
    pub y: i32,
    pub damage_x: i32,
    pub damage_y: i32,
    pub damage_w: i32,
    pub damage_h: i32,
    pub frame_callbacks: Vec<*mut WlResource>,
    pub pending_frame_callbacks: Vec<*mut WlResource>,
    pub title: Option<String>,
    pub app_id: Option<String>,
}

impl SurfaceState {
    pub fn new(id: u32, resource: *mut WlResource) -> Self {
        Self {
            id,
            resource,
            current_buffer: None,
            pending_buffer: None,
            width: 0,
            height: 0,
            x: 0,
            y: 0,
            damage_x: 0,
            damage_y: 0,
            damage_w: 0,
            damage_h: 0,
            frame_callbacks: Vec::new(),
            pending_frame_callbacks: Vec::new(),
            title: None,
            app_id: None,
        }
    }

    pub fn commit(&mut self) {
        if let Some(buf) = self.pending_buffer.take() {
            self.current_buffer = Some(buf);
            if let Ok(wl) = wayland_server() {
                unsafe {
                    let shm_buf = (wl.wl_shm_buffer_get)(buf);
                    if !shm_buf.is_null() {
                        self.width = (wl.wl_shm_buffer_get_width)(shm_buf);
                        self.height = (wl.wl_shm_buffer_get_height)(shm_buf);
                    }
                }
            }
        }
        self.frame_callbacks
            .append(&mut self.pending_frame_callbacks);
    }

    pub fn trigger_frame_callbacks(&mut self, time_ms: u32) {
        if let Ok(wl) = wayland_server() {
            for callback in self.frame_callbacks.drain(..) {
                unsafe {
                    (wl.wl_resource_post_event)(callback, 0, time_ms);
                    (wl.wl_resource_destroy)(callback);
                }
            }
        }
    }
}

// Global XDG Shell Interface definitions
pub static mut XDG_WM_BASE_INTERFACE: WlInterface = WlInterface {
    name: b"xdg_wm_base\0".as_ptr() as *const c_char,
    version: 4,
    method_count: 4,
    methods: ptr::null(),
    event_count: 1,
    events: ptr::null(),
};

pub static mut XDG_SURFACE_INTERFACE: WlInterface = WlInterface {
    name: b"xdg_surface\0".as_ptr() as *const c_char,
    version: 4,
    method_count: 5,
    methods: ptr::null(),
    event_count: 1,
    events: ptr::null(),
};

pub static mut XDG_TOPLEVEL_INTERFACE: WlInterface = WlInterface {
    name: b"xdg_toplevel\0".as_ptr() as *const c_char,
    version: 4,
    method_count: 14,
    methods: ptr::null(),
    event_count: 2,
    events: ptr::null(),
};

// Types array for xdg messages
static mut XDG_TYPES: [*const WlInterface; 16] = [ptr::null(); 16];

static mut XDG_WM_BASE_REQUESTS: [WlMessage; 4] = [
    WlMessage {
        name: b"destroy\0".as_ptr() as *const c_char,
        signature: b"\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"create_positioner\0".as_ptr() as *const c_char,
        signature: b"n\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"get_xdg_surface\0".as_ptr() as *const c_char,
        signature: b"no\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"pong\0".as_ptr() as *const c_char,
        signature: b"u\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
];

static mut XDG_WM_BASE_EVENTS: [WlMessage; 1] = [WlMessage {
    name: b"ping\0".as_ptr() as *const c_char,
    signature: b"u\0".as_ptr() as *const c_char,
    types: ptr::null(),
}];

static mut XDG_SURFACE_REQUESTS: [WlMessage; 5] = [
    WlMessage {
        name: b"destroy\0".as_ptr() as *const c_char,
        signature: b"\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"get_toplevel\0".as_ptr() as *const c_char,
        signature: b"n\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"get_popup\0".as_ptr() as *const c_char,
        signature: b"n?oo\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_window_geometry\0".as_ptr() as *const c_char,
        signature: b"iiii\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"ack_configure\0".as_ptr() as *const c_char,
        signature: b"u\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
];

static mut XDG_SURFACE_EVENTS: [WlMessage; 1] = [WlMessage {
    name: b"configure\0".as_ptr() as *const c_char,
    signature: b"u\0".as_ptr() as *const c_char,
    types: ptr::null(),
}];

static mut XDG_TOPLEVEL_REQUESTS: [WlMessage; 14] = [
    WlMessage {
        name: b"destroy\0".as_ptr() as *const c_char,
        signature: b"\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_parent\0".as_ptr() as *const c_char,
        signature: b"?o\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_title\0".as_ptr() as *const c_char,
        signature: b"s\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_app_id\0".as_ptr() as *const c_char,
        signature: b"s\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"show_window_menu\0".as_ptr() as *const c_char,
        signature: b"ouii\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"move\0".as_ptr() as *const c_char,
        signature: b"ou\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"resize\0".as_ptr() as *const c_char,
        signature: b"ouu\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_max_size\0".as_ptr() as *const c_char,
        signature: b"ii\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_min_size\0".as_ptr() as *const c_char,
        signature: b"ii\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_maximized\0".as_ptr() as *const c_char,
        signature: b"\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"unset_maximized\0".as_ptr() as *const c_char,
        signature: b"\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_fullscreen\0".as_ptr() as *const c_char,
        signature: b"?o\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"unset_fullscreen\0".as_ptr() as *const c_char,
        signature: b"\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"set_minimized\0".as_ptr() as *const c_char,
        signature: b"\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
];

static mut XDG_TOPLEVEL_EVENTS: [WlMessage; 2] = [
    WlMessage {
        name: b"configure\0".as_ptr() as *const c_char,
        signature: b"iia\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
    WlMessage {
        name: b"close\0".as_ptr() as *const c_char,
        signature: b"\0".as_ptr() as *const c_char,
        types: ptr::null(),
    },
];

pub fn init_xdg_interfaces() {
    let wl = match wayland_server() {
        Ok(w) => w,
        Err(_) => return,
    };

    unsafe {
        let xdg_types_ptr = &raw mut XDG_TYPES as *mut *const WlInterface;
        let xdg_wm_req_ptr = &raw mut XDG_WM_BASE_REQUESTS as *mut WlMessage;
        let xdg_wm_evt_ptr = &raw mut XDG_WM_BASE_EVENTS as *mut WlMessage;
        let xdg_surf_req_ptr = &raw mut XDG_SURFACE_REQUESTS as *mut WlMessage;
        let xdg_surf_evt_ptr = &raw mut XDG_SURFACE_EVENTS as *mut WlMessage;
        let xdg_top_req_ptr = &raw mut XDG_TOPLEVEL_REQUESTS as *mut WlMessage;
        let xdg_top_evt_ptr = &raw mut XDG_TOPLEVEL_EVENTS as *mut WlMessage;

        *xdg_types_ptr.add(0) = &raw const XDG_SURFACE_INTERFACE;
        *xdg_types_ptr.add(1) = wl.wl_surface_interface;
        *xdg_types_ptr.add(2) = &raw const XDG_TOPLEVEL_INTERFACE;

        (*xdg_wm_req_ptr.add(2)).types = xdg_types_ptr;
        (*xdg_surf_req_ptr.add(1)).types = xdg_types_ptr.add(2);

        XDG_WM_BASE_INTERFACE.methods = xdg_wm_req_ptr;
        XDG_WM_BASE_INTERFACE.events = xdg_wm_evt_ptr;

        XDG_SURFACE_INTERFACE.methods = xdg_surf_req_ptr;
        XDG_SURFACE_INTERFACE.events = xdg_surf_evt_ptr;

        XDG_TOPLEVEL_INTERFACE.methods = xdg_top_req_ptr;
        XDG_TOPLEVEL_INTERFACE.events = xdg_top_evt_ptr;
    }
}
