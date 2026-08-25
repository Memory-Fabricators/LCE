//! Low-level FFI bindings to libwayland-server and libpipewire loaded dynamically via dlopen.

use libloading::Library;
use std::ffi::{c_char, c_int, c_void};
use std::sync::LazyLock;

#[repr(C)]
pub struct WlDisplay {
    _private: [u8; 0],
}

#[repr(C)]
pub struct WlEventLoop {
    _private: [u8; 0],
}

#[repr(C)]
pub struct WlGlobal {
    _private: [u8; 0],
}

#[repr(C)]
pub struct WlClient {
    _private: [u8; 0],
}

#[repr(C)]
pub struct WlResource {
    _private: [u8; 0],
}

#[repr(C)]
pub struct WlInterface {
    pub name: *const c_char,
    pub version: c_int,
    pub method_count: c_int,
    pub methods: *const WlMessage,
    pub event_count: c_int,
    pub events: *const WlMessage,
}

unsafe impl Sync for WlInterface {}
unsafe impl Send for WlInterface {}

#[repr(C)]
pub struct WlMessage {
    pub name: *const c_char,
    pub signature: *const c_char,
    pub types: *const *const WlInterface,
}

unsafe impl Sync for WlMessage {}
unsafe impl Send for WlMessage {}

#[repr(C)]
pub struct WlList {
    pub prev: *mut WlList,
    pub next: *mut WlList,
}

#[repr(C)]
pub struct WlArray {
    pub size: usize,
    pub alloc: usize,
    pub data: *mut c_void,
}

#[repr(C)]
pub struct WlShmBuffer {
    _private: [u8; 0],
}

pub type WlGlobalBindFuncT =
    extern "C" fn(client: *mut WlClient, data: *mut c_void, version: u32, id: u32);
pub type WlResourceDestroyFuncT = extern "C" fn(resource: *mut WlResource);
pub type WlDispatcherFuncT = extern "C" fn(
    user_data: *const c_void,
    target: *mut c_void,
    opcode: u32,
    msg: *const WlMessage,
    args: *mut WlArgument,
) -> c_int;

#[repr(C)]
pub union WlArgument {
    pub i: i32,
    pub u: u32,
    pub f: i32,
    pub s: *const c_char,
    pub o: *mut WlResource,
    pub n: u32,
    pub a: *mut WlArray,
    pub h: i32,
}

// Struct containing function pointers to libwayland-server
pub struct WaylandServerLib {
    _lib: Library,
    pub wl_compositor_interface: *const WlInterface,
    pub wl_surface_interface: *const WlInterface,
    pub wl_region_interface: *const WlInterface,
    pub wl_shm_interface: *const WlInterface,
    pub wl_shm_pool_interface: *const WlInterface,
    pub wl_buffer_interface: *const WlInterface,
    pub wl_seat_interface: *const WlInterface,
    pub wl_pointer_interface: *const WlInterface,
    pub wl_keyboard_interface: *const WlInterface,
    pub wl_output_interface: *const WlInterface,
    pub wl_callback_interface: *const WlInterface,

    pub wl_display_create: unsafe extern "C" fn() -> *mut WlDisplay,
    pub wl_display_destroy: unsafe extern "C" fn(display: *mut WlDisplay),
    pub wl_display_get_event_loop:
        unsafe extern "C" fn(display: *mut WlDisplay) -> *mut WlEventLoop,
    pub wl_display_add_socket:
        unsafe extern "C" fn(display: *mut WlDisplay, name: *const c_char) -> c_int,
    pub wl_display_add_socket_auto: unsafe extern "C" fn(display: *mut WlDisplay) -> *const c_char,
    pub wl_display_init_shm: unsafe extern "C" fn(display: *mut WlDisplay) -> c_int,
    pub wl_display_flush_clients: unsafe extern "C" fn(display: *mut WlDisplay),
    pub wl_display_run: unsafe extern "C" fn(display: *mut WlDisplay),
    pub wl_display_terminate: unsafe extern "C" fn(display: *mut WlDisplay),

    pub wl_event_loop_dispatch:
        unsafe extern "C" fn(loop_: *mut WlEventLoop, timeout: c_int) -> c_int,
    pub wl_event_loop_dispatch_idle: unsafe extern "C" fn(loop_: *mut WlEventLoop),

    pub wl_global_create: unsafe extern "C" fn(
        display: *mut WlDisplay,
        interface: *const WlInterface,
        version: c_int,
        data: *mut c_void,
        bind: WlGlobalBindFuncT,
    ) -> *mut WlGlobal,
    pub wl_global_destroy: unsafe extern "C" fn(global: *mut WlGlobal),

    pub wl_resource_create: unsafe extern "C" fn(
        client: *mut WlClient,
        interface: *const WlInterface,
        version: c_int,
        id: u32,
    ) -> *mut WlResource,
    pub wl_resource_destroy: unsafe extern "C" fn(resource: *mut WlResource),
    pub wl_resource_set_implementation: unsafe extern "C" fn(
        resource: *mut WlResource,
        implementation: *const c_void,
        data: *mut c_void,
        destroy: Option<WlResourceDestroyFuncT>,
    ),
    pub wl_resource_set_dispatcher: unsafe extern "C" fn(
        resource: *mut WlResource,
        dispatcher: WlDispatcherFuncT,
        implementation: *const c_void,
        data: *mut c_void,
        destroy: Option<WlResourceDestroyFuncT>,
    ),
    pub wl_resource_get_user_data: unsafe extern "C" fn(resource: *mut WlResource) -> *mut c_void,
    pub wl_resource_get_id: unsafe extern "C" fn(resource: *mut WlResource) -> u32,
    pub wl_resource_get_version: unsafe extern "C" fn(resource: *mut WlResource) -> c_int,
    pub wl_resource_get_client: unsafe extern "C" fn(resource: *mut WlResource) -> *mut WlClient,
    pub wl_resource_post_event: unsafe extern "C" fn(resource: *mut WlResource, opcode: u32, ...),
    pub wl_resource_post_error:
        unsafe extern "C" fn(resource: *mut WlResource, code: u32, msg: *const c_char, ...),

    pub wl_shm_buffer_get: unsafe extern "C" fn(resource: *mut WlResource) -> *mut WlShmBuffer,
    pub wl_shm_buffer_begin_access: unsafe extern "C" fn(buffer: *mut WlShmBuffer),
    pub wl_shm_buffer_end_access: unsafe extern "C" fn(buffer: *mut WlShmBuffer),
    pub wl_shm_buffer_get_data: unsafe extern "C" fn(buffer: *mut WlShmBuffer) -> *mut c_void,
    pub wl_shm_buffer_get_stride: unsafe extern "C" fn(buffer: *mut WlShmBuffer) -> i32,
    pub wl_shm_buffer_get_format: unsafe extern "C" fn(buffer: *mut WlShmBuffer) -> u32,
    pub wl_shm_buffer_get_width: unsafe extern "C" fn(buffer: *mut WlShmBuffer) -> i32,
    pub wl_shm_buffer_get_height: unsafe extern "C" fn(buffer: *mut WlShmBuffer) -> i32,
}

unsafe impl Send for WaylandServerLib {}
unsafe impl Sync for WaylandServerLib {}

static WAYLAND_LIB: LazyLock<Result<WaylandServerLib, String>> = LazyLock::new(|| unsafe {
    let lib = Library::new("libwayland-server.so.0")
        .or_else(|_| Library::new("libwayland-server.so"))
        .map_err(|e| format!("Failed to dlopen libwayland-server: {e}"))?;

    let wl_compositor_interface = *lib
        .get::<*const WlInterface>(b"wl_compositor_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_surface_interface = *lib
        .get::<*const WlInterface>(b"wl_surface_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_region_interface = *lib
        .get::<*const WlInterface>(b"wl_region_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_shm_interface = *lib
        .get::<*const WlInterface>(b"wl_shm_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_shm_pool_interface = *lib
        .get::<*const WlInterface>(b"wl_shm_pool_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_buffer_interface = *lib
        .get::<*const WlInterface>(b"wl_buffer_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_seat_interface = *lib
        .get::<*const WlInterface>(b"wl_seat_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_pointer_interface = *lib
        .get::<*const WlInterface>(b"wl_pointer_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_keyboard_interface = *lib
        .get::<*const WlInterface>(b"wl_keyboard_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_output_interface = *lib
        .get::<*const WlInterface>(b"wl_output_interface\0")
        .map_err(|e| e.to_string())?;
    let wl_callback_interface = *lib
        .get::<*const WlInterface>(b"wl_callback_interface\0")
        .map_err(|e| e.to_string())?;

    let wl_display_create = *lib.get(b"wl_display_create\0").map_err(|e| e.to_string())?;
    let wl_display_destroy = *lib
        .get(b"wl_display_destroy\0")
        .map_err(|e| e.to_string())?;
    let wl_display_get_event_loop = *lib
        .get(b"wl_display_get_event_loop\0")
        .map_err(|e| e.to_string())?;
    let wl_display_add_socket = *lib
        .get(b"wl_display_add_socket\0")
        .map_err(|e| e.to_string())?;
    let wl_display_add_socket_auto = *lib
        .get(b"wl_display_add_socket_auto\0")
        .map_err(|e| e.to_string())?;
    let wl_display_init_shm = *lib
        .get(b"wl_display_init_shm\0")
        .map_err(|e| e.to_string())?;
    let wl_display_flush_clients = *lib
        .get(b"wl_display_flush_clients\0")
        .map_err(|e| e.to_string())?;
    let wl_display_run = *lib.get(b"wl_display_run\0").map_err(|e| e.to_string())?;
    let wl_display_terminate = *lib
        .get(b"wl_display_terminate\0")
        .map_err(|e| e.to_string())?;

    let wl_event_loop_dispatch = *lib
        .get(b"wl_event_loop_dispatch\0")
        .map_err(|e| e.to_string())?;
    let wl_event_loop_dispatch_idle = *lib
        .get(b"wl_event_loop_dispatch_idle\0")
        .map_err(|e| e.to_string())?;

    let wl_global_create = *lib.get(b"wl_global_create\0").map_err(|e| e.to_string())?;
    let wl_global_destroy = *lib.get(b"wl_global_destroy\0").map_err(|e| e.to_string())?;

    let wl_resource_create = *lib
        .get(b"wl_resource_create\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_destroy = *lib
        .get(b"wl_resource_destroy\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_set_implementation = *lib
        .get(b"wl_resource_set_implementation\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_set_dispatcher = *lib
        .get(b"wl_resource_set_dispatcher\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_get_user_data = *lib
        .get(b"wl_resource_get_user_data\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_get_id = *lib
        .get(b"wl_resource_get_id\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_get_version = *lib
        .get(b"wl_resource_get_version\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_get_client = *lib
        .get(b"wl_resource_get_client\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_post_event = *lib
        .get(b"wl_resource_post_event\0")
        .map_err(|e| e.to_string())?;
    let wl_resource_post_error = *lib
        .get(b"wl_resource_post_error\0")
        .map_err(|e| e.to_string())?;

    let wl_shm_buffer_get = *lib.get(b"wl_shm_buffer_get\0").map_err(|e| e.to_string())?;
    let wl_shm_buffer_begin_access = *lib
        .get(b"wl_shm_buffer_begin_access\0")
        .map_err(|e| e.to_string())?;
    let wl_shm_buffer_end_access = *lib
        .get(b"wl_shm_buffer_end_access\0")
        .map_err(|e| e.to_string())?;
    let wl_shm_buffer_get_data = *lib
        .get(b"wl_shm_buffer_get_data\0")
        .map_err(|e| e.to_string())?;
    let wl_shm_buffer_get_stride = *lib
        .get(b"wl_shm_buffer_get_stride\0")
        .map_err(|e| e.to_string())?;
    let wl_shm_buffer_get_format = *lib
        .get(b"wl_shm_buffer_get_format\0")
        .map_err(|e| e.to_string())?;
    let wl_shm_buffer_get_width = *lib
        .get(b"wl_shm_buffer_get_width\0")
        .map_err(|e| e.to_string())?;
    let wl_shm_buffer_get_height = *lib
        .get(b"wl_shm_buffer_get_height\0")
        .map_err(|e| e.to_string())?;

    Ok(WaylandServerLib {
        _lib: lib,
        wl_compositor_interface,
        wl_surface_interface,
        wl_region_interface,
        wl_shm_interface,
        wl_shm_pool_interface,
        wl_buffer_interface,
        wl_seat_interface,
        wl_pointer_interface,
        wl_keyboard_interface,
        wl_output_interface,
        wl_callback_interface,
        wl_display_create,
        wl_display_destroy,
        wl_display_get_event_loop,
        wl_display_add_socket,
        wl_display_add_socket_auto,
        wl_display_init_shm,
        wl_display_flush_clients,
        wl_display_run,
        wl_display_terminate,
        wl_event_loop_dispatch,
        wl_event_loop_dispatch_idle,
        wl_global_create,
        wl_global_destroy,
        wl_resource_create,
        wl_resource_destroy,
        wl_resource_set_implementation,
        wl_resource_set_dispatcher,
        wl_resource_get_user_data,
        wl_resource_get_id,
        wl_resource_get_version,
        wl_resource_get_client,
        wl_resource_post_event,
        wl_resource_post_error,
        wl_shm_buffer_get,
        wl_shm_buffer_begin_access,
        wl_shm_buffer_end_access,
        wl_shm_buffer_get_data,
        wl_shm_buffer_get_stride,
        wl_shm_buffer_get_format,
        wl_shm_buffer_get_width,
        wl_shm_buffer_get_height,
    })
});

pub fn wayland_server() -> Result<&'static WaylandServerLib, &'static str> {
    WAYLAND_LIB.as_ref().map_err(|s| s.as_str())
}

// PipeWire C structures and functions
#[repr(C)]
pub struct PwMainLoop {
    _private: [u8; 0],
}

#[repr(C)]
pub struct PwLoop {
    _private: [u8; 0],
}

#[repr(C)]
pub struct PwContext {
    _private: [u8; 0],
}

#[repr(C)]
pub struct PwCore {
    _private: [u8; 0],
}

#[repr(C)]
pub struct PwStream {
    _private: [u8; 0],
}

#[repr(C)]
pub struct PwProperties {
    _private: [u8; 0],
}

#[repr(C)]
pub struct SpaList {
    pub prev: *mut SpaList,
    pub next: *mut SpaList,
}

#[repr(C)]
pub struct SpaCallbacks {
    pub funcs: *const c_void,
    pub data: *mut c_void,
}

#[repr(C)]
pub struct SpaHook {
    pub link: SpaList,
    pub cb: SpaCallbacks,
    pub removed: Option<unsafe extern "C" fn(hook: *mut SpaHook)>,
    pub priv_: *mut c_void,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum PwStreamState {
    Error = -1,
    Unconnected = 0,
    Connecting = 1,
    Paused = 2,
    Streaming = 3,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum PwDirection {
    Input = 0,
    Output = 1,
}

#[repr(C)]
pub struct SpaBuffer {
    pub n_metas: u32,
    pub n_datas: u32,
    pub metas: *mut c_void,
    pub datas: *mut SpaData,
}

#[repr(C)]
pub struct SpaData {
    pub type_: u32,
    pub flags: u32,
    pub fd: i64,
    pub mapoffset: u32,
    pub maxsize: u32,
    pub data: *mut c_void,
    pub chunk: *mut SpaChunk,
}

#[repr(C)]
pub struct SpaChunk {
    pub offset: u32,
    pub size: u32,
    pub stride: i32,
    pub flags: i32,
}

#[repr(C)]
pub struct PwBuffer {
    pub buffer: *mut SpaBuffer,
    pub user_data: *mut c_void,
    pub size: u64,
    pub requested: u64,
    pub time: u64,
}

#[repr(C)]
pub struct SpaPod {
    pub size: u32,
    pub type_: u32,
}

#[repr(C)]
pub struct PwStreamEvents {
    pub version: u32,
    pub destroy: Option<extern "C" fn(data: *mut c_void)>,
    pub state_changed: Option<
        extern "C" fn(
            data: *mut c_void,
            old: PwStreamState,
            state: PwStreamState,
            error: *const c_char,
        ),
    >,
    pub control_info: Option<extern "C" fn(data: *mut c_void, id: u32, control: *const c_void)>,
    pub io_changed: Option<extern "C" fn(data: *mut c_void, id: u32, area: *mut c_void, size: u32)>,
    pub param_changed: Option<extern "C" fn(data: *mut c_void, id: u32, param: *const SpaPod)>,
    pub add_buffer: Option<extern "C" fn(data: *mut c_void, buffer: *mut PwBuffer)>,
    pub remove_buffer: Option<extern "C" fn(data: *mut c_void, buffer: *mut PwBuffer)>,
    pub process: Option<extern "C" fn(data: *mut c_void)>,
    pub drained: Option<extern "C" fn(data: *mut c_void)>,
    pub command: Option<extern "C" fn(data: *mut c_void, command: *const SpaPod)>,
    pub trigger_done: Option<extern "C" fn(data: *mut c_void)>,
}

pub const PW_VERSION_STREAM_EVENTS: u32 = 2;

pub const PW_STREAM_FLAG_AUTOCONNECT: u32 = 1 << 0;
pub const PW_STREAM_FLAG_MAP_BUFFERS: u32 = 1 << 2;
pub const PW_STREAM_FLAG_ALLOC_BUFFERS: u32 = 1 << 4;

pub struct PipeWireLib {
    _lib: Library,
    pub pw_init: unsafe extern "C" fn(argc: *mut c_int, argv: *mut *mut *mut c_char),
    pub pw_deinit: unsafe extern "C" fn(),
    pub pw_main_loop_new: unsafe extern "C" fn(props: *const PwProperties) -> *mut PwMainLoop,
    pub pw_main_loop_destroy: unsafe extern "C" fn(loop_: *mut PwMainLoop),
    pub pw_main_loop_get_loop: unsafe extern "C" fn(loop_: *mut PwMainLoop) -> *mut PwLoop,
    pub pw_main_loop_run: unsafe extern "C" fn(loop_: *mut PwMainLoop) -> c_int,
    pub pw_main_loop_quit: unsafe extern "C" fn(loop_: *mut PwMainLoop) -> c_int,

    pub pw_loop_iterate: unsafe extern "C" fn(loop_: *mut PwLoop, timeout: c_int) -> c_int,

    pub pw_context_new: unsafe extern "C" fn(
        loop_: *mut PwLoop,
        props: *mut PwProperties,
        user_data_size: usize,
    ) -> *mut PwContext,
    pub pw_context_destroy: unsafe extern "C" fn(context: *mut PwContext),

    pub pw_context_connect: unsafe extern "C" fn(
        context: *mut PwContext,
        props: *mut PwProperties,
        user_data_size: usize,
    ) -> *mut PwCore,
    pub pw_core_disconnect: unsafe extern "C" fn(core: *mut PwCore) -> c_int,

    pub pw_properties_new_string: unsafe extern "C" fn(args: *const c_char) -> *mut PwProperties,
    pub pw_properties_set: unsafe extern "C" fn(
        props: *mut PwProperties,
        key: *const c_char,
        value: *const c_char,
    ) -> c_int,

    pub pw_stream_new: unsafe extern "C" fn(
        core: *mut PwCore,
        name: *const c_char,
        props: *mut PwProperties,
    ) -> *mut PwStream,
    pub pw_stream_new_simple: unsafe extern "C" fn(
        loop_: *mut PwLoop,
        name: *const c_char,
        props: *mut PwProperties,
        events: *const PwStreamEvents,
        data: *mut c_void,
    ) -> *mut PwStream,
    pub pw_stream_destroy: unsafe extern "C" fn(stream: *mut PwStream),
    pub pw_stream_add_listener: unsafe extern "C" fn(
        stream: *mut PwStream,
        listener: *mut SpaHook,
        events: *const PwStreamEvents,
        data: *mut c_void,
    ),
    pub pw_stream_connect: unsafe extern "C" fn(
        stream: *mut PwStream,
        direction: PwDirection,
        target_id: u32,
        flags: u32,
        params: *const *const SpaPod,
        n_params: u32,
    ) -> c_int,
    pub pw_stream_disconnect: unsafe extern "C" fn(stream: *mut PwStream) -> c_int,
    pub pw_stream_get_state:
        unsafe extern "C" fn(stream: *mut PwStream, error: *mut *const c_char) -> PwStreamState,
    pub pw_stream_dequeue_buffer: unsafe extern "C" fn(stream: *mut PwStream) -> *mut PwBuffer,
    pub pw_stream_queue_buffer:
        unsafe extern "C" fn(stream: *mut PwStream, buffer: *mut PwBuffer) -> c_int,
    pub pw_stream_trigger_process: unsafe extern "C" fn(stream: *mut PwStream) -> c_int,
}

unsafe impl Send for PipeWireLib {}
unsafe impl Sync for PipeWireLib {}

static PIPEWIRE_LIB: LazyLock<Result<PipeWireLib, String>> = LazyLock::new(|| unsafe {
    let lib = Library::new("libpipewire-0.3.so.0")
        .or_else(|_| Library::new("libpipewire-0.3.so"))
        .map_err(|e| format!("Failed to dlopen libpipewire-0.3: {e}"))?;

    let pw_init = *lib.get(b"pw_init\0").map_err(|e| e.to_string())?;
    let pw_deinit = *lib.get(b"pw_deinit\0").map_err(|e| e.to_string())?;
    let pw_main_loop_new = *lib.get(b"pw_main_loop_new\0").map_err(|e| e.to_string())?;
    let pw_main_loop_destroy = *lib
        .get(b"pw_main_loop_destroy\0")
        .map_err(|e| e.to_string())?;
    let pw_main_loop_get_loop = *lib
        .get(b"pw_main_loop_get_loop\0")
        .map_err(|e| e.to_string())?;
    let pw_main_loop_run = *lib.get(b"pw_main_loop_run\0").map_err(|e| e.to_string())?;
    let pw_main_loop_quit = *lib.get(b"pw_main_loop_quit\0").map_err(|e| e.to_string())?;

    let pw_loop_iterate = *lib.get(b"pw_loop_iterate\0").map_err(|e| e.to_string())?;

    let pw_context_new = *lib.get(b"pw_context_new\0").map_err(|e| e.to_string())?;
    let pw_context_destroy = *lib
        .get(b"pw_context_destroy\0")
        .map_err(|e| e.to_string())?;

    let pw_context_connect = *lib
        .get(b"pw_context_connect\0")
        .map_err(|e| e.to_string())?;
    let pw_core_disconnect = *lib
        .get(b"pw_core_disconnect\0")
        .map_err(|e| e.to_string())?;

    let pw_properties_new_string = *lib
        .get(b"pw_properties_new_string\0")
        .map_err(|e| e.to_string())?;
    let pw_properties_set = *lib.get(b"pw_properties_set\0").map_err(|e| e.to_string())?;

    let pw_stream_new = *lib.get(b"pw_stream_new\0").map_err(|e| e.to_string())?;
    let pw_stream_new_simple = *lib
        .get(b"pw_stream_new_simple\0")
        .map_err(|e| e.to_string())?;
    let pw_stream_destroy = *lib.get(b"pw_stream_destroy\0").map_err(|e| e.to_string())?;
    let pw_stream_add_listener = *lib
        .get(b"pw_stream_add_listener\0")
        .map_err(|e| e.to_string())?;
    let pw_stream_connect = *lib.get(b"pw_stream_connect\0").map_err(|e| e.to_string())?;
    let pw_stream_disconnect = *lib
        .get(b"pw_stream_disconnect\0")
        .map_err(|e| e.to_string())?;
    let pw_stream_get_state = *lib
        .get(b"pw_stream_get_state\0")
        .map_err(|e| e.to_string())?;
    let pw_stream_dequeue_buffer = *lib
        .get(b"pw_stream_dequeue_buffer\0")
        .map_err(|e| e.to_string())?;
    let pw_stream_queue_buffer = *lib
        .get(b"pw_stream_queue_buffer\0")
        .map_err(|e| e.to_string())?;
    let pw_stream_trigger_process = *lib
        .get(b"pw_stream_trigger_process\0")
        .map_err(|e| e.to_string())?;

    Ok(PipeWireLib {
        _lib: lib,
        pw_init,
        pw_deinit,
        pw_main_loop_new,
        pw_main_loop_destroy,
        pw_main_loop_get_loop,
        pw_main_loop_run,
        pw_main_loop_quit,
        pw_loop_iterate,
        pw_context_new,
        pw_context_destroy,
        pw_context_connect,
        pw_core_disconnect,
        pw_properties_new_string,
        pw_properties_set,
        pw_stream_new,
        pw_stream_new_simple,
        pw_stream_destroy,
        pw_stream_add_listener,
        pw_stream_connect,
        pw_stream_disconnect,
        pw_stream_get_state,
        pw_stream_dequeue_buffer,
        pw_stream_queue_buffer,
        pw_stream_trigger_process,
    })
});

pub fn pipewire_lib() -> Result<&'static PipeWireLib, &'static str> {
    PIPEWIRE_LIB.as_ref().map_err(|s| s.as_str())
}
