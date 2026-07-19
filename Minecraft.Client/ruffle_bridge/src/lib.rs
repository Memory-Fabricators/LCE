//! C ABI boundary for the SDL3/OpenGL Ruffle integration.

mod embedder_ui;
mod gl;
mod menu_button;
mod navigator;
mod opengl_renderer;
mod player;

use menu_button::MenuButtonHandle;
use opengl_renderer::OpenGlRenderer;
use player::BridgePlayer;
use ruffle_render::backend::RenderBackend;
use std::panic::{AssertUnwindSafe, catch_unwind};

#[repr(C)]
pub enum RuffleBridgeStatus {
    RUFFLE_BRIDGE_OK = 0,
    RUFFLE_BRIDGE_INVALID_ARGUMENT = 1,
    RUFFLE_BRIDGE_UNSUPPORTED_CONTEXT = 2,
    RUFFLE_BRIDGE_RENDER_ERROR = 3,
    // Button resolution/AS call failed (not found on the display list, not
    // an AVM2-scriptable object, or the AS method call itself threw); see
    // stderr for the diagnostic (`Ruffle OpenGL:`-style logging convention).
    RUFFLE_BRIDGE_BUTTON_ERROR = 4,
}

#[repr(C)]
pub struct RuffleBridgeRenderer {
    renderer: *mut std::ffi::c_void,
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_renderer_create(
    width: u32,
    height: u32,
    out_renderer: *mut *mut RuffleBridgeRenderer,
) -> RuffleBridgeStatus {
    if out_renderer.is_null() || width == 0 || height == 0 {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    unsafe {
        *out_renderer = std::ptr::null_mut();
    }
    match catch_unwind(AssertUnwindSafe(|| OpenGlRenderer::new(width, height))) {
        Ok(Ok(renderer)) => {
            unsafe {
                *out_renderer = Box::into_raw(Box::new(RuffleBridgeRenderer {
                    renderer: Box::into_raw(Box::new(renderer)).cast(),
                }));
            }
            RuffleBridgeStatus::RUFFLE_BRIDGE_OK
        }
        Ok(Err(())) => RuffleBridgeStatus::RUFFLE_BRIDGE_UNSUPPORTED_CONTEXT,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_renderer_resize(
    renderer: *mut RuffleBridgeRenderer,
    width: u32,
    height: u32,
) -> RuffleBridgeStatus {
    if renderer.is_null() || width == 0 || height == 0 {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        (&mut *((*renderer).renderer.cast::<OpenGlRenderer>())).set_viewport_dimensions(
            ruffle_render::backend::ViewportDimensions {
                width,
                height,
                scale_factor: 1.0,
            },
        );
    })) {
        Ok(()) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_renderer_render_probe(
    renderer: *mut RuffleBridgeRenderer,
) -> RuffleBridgeStatus {
    if renderer.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        (&mut *((*renderer).renderer.cast::<OpenGlRenderer>())).submit_probe();
    })) {
        Ok(()) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_renderer_destroy(renderer: *mut RuffleBridgeRenderer) {
    if !renderer.is_null() {
        unsafe {
            let renderer = Box::from_raw(renderer);
            drop(Box::from_raw(renderer.renderer.cast::<OpenGlRenderer>()));
        }
    }
}

#[repr(C)]
pub struct RuffleBridgePlayer {
    player: *mut std::ffi::c_void,
}

#[repr(C)]
pub enum RuffleBridgeMouseButton {
    RUFFLE_BRIDGE_MOUSE_LEFT = 0,
    RUFFLE_BRIDGE_MOUSE_MIDDLE = 1,
    RUFFLE_BRIDGE_MOUSE_RIGHT = 2,
}

fn to_ruffle_mouse_button(button: RuffleBridgeMouseButton) -> ruffle_core::events::MouseButton {
    match button {
        RuffleBridgeMouseButton::RUFFLE_BRIDGE_MOUSE_LEFT => ruffle_core::events::MouseButton::Left,
        RuffleBridgeMouseButton::RUFFLE_BRIDGE_MOUSE_MIDDLE => {
            ruffle_core::events::MouseButton::Middle
        }
        RuffleBridgeMouseButton::RUFFLE_BRIDGE_MOUSE_RIGHT => {
            ruffle_core::events::MouseButton::Right
        }
    }
}

/// `swf_data`/`swf_len` must describe a live buffer for the duration of this call only; the
/// movie's bytes are copied into the `Player`'s owned `SwfMovie` before this function returns.
/// `url` must be a valid, NUL-terminated UTF-8 C string. `search_dirs` is an array of
/// `search_dirs_len` NUL-terminated UTF-8 C strings, checked in order, used to resolve
/// relative-URL asset fetches (e.g. ImportAssets-referenced companion SWFs); it need not
/// include the SWF's own directory unless companion assets live there too.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_player_create(
    width: u32,
    height: u32,
    swf_data: *const u8,
    swf_len: usize,
    url: *const std::ffi::c_char,
    search_dirs: *const *const std::ffi::c_char,
    search_dirs_len: usize,
    out_player: *mut *mut RuffleBridgePlayer,
) -> RuffleBridgeStatus {
    if out_player.is_null()
        || swf_data.is_null()
        || url.is_null()
        || (search_dirs.is_null() && search_dirs_len != 0)
        || width == 0
        || height == 0
    {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    unsafe {
        *out_player = std::ptr::null_mut();
    }
    let swf_bytes = unsafe { std::slice::from_raw_parts(swf_data, swf_len) };
    let Ok(url) = (unsafe { std::ffi::CStr::from_ptr(url) }).to_str() else {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    };
    let url = url.to_owned();
    let search_dir_ptrs = if search_dirs_len == 0 {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(search_dirs, search_dirs_len) }
    };
    let mut search_dirs_vec = Vec::with_capacity(search_dir_ptrs.len());
    for &dir_ptr in search_dir_ptrs {
        if dir_ptr.is_null() {
            return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
        }
        let Ok(dir) = (unsafe { std::ffi::CStr::from_ptr(dir_ptr) }).to_str() else {
            return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
        };
        search_dirs_vec.push(std::path::PathBuf::from(dir));
    }
    match catch_unwind(AssertUnwindSafe(|| {
        BridgePlayer::new(width, height, swf_bytes, url, search_dirs_vec)
    })) {
        Ok(Ok(player)) => {
            unsafe {
                *out_player = Box::into_raw(Box::new(RuffleBridgePlayer {
                    player: Box::into_raw(Box::new(player)).cast(),
                }));
            }
            RuffleBridgeStatus::RUFFLE_BRIDGE_OK
        }
        Ok(Err(())) => RuffleBridgeStatus::RUFFLE_BRIDGE_UNSUPPORTED_CONTEXT,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_player_resize(
    player: *mut RuffleBridgePlayer,
    width: u32,
    height: u32,
) -> RuffleBridgeStatus {
    if player.is_null() || width == 0 || height == 0 {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        (&*((*player).player.cast::<BridgePlayer>())).resize(width, height);
    })) {
        Ok(()) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_player_tick(
    player: *mut RuffleBridgePlayer,
    dt_seconds: f64,
) -> RuffleBridgeStatus {
    if player.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        (&*((*player).player.cast::<BridgePlayer>())).tick(dt_seconds);
    })) {
        Ok(()) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_player_render(
    player: *mut RuffleBridgePlayer,
) -> RuffleBridgeStatus {
    if player.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        (&*((*player).player.cast::<BridgePlayer>())).render();
    })) {
        Ok(()) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_player_handle_mouse_move(
    player: *mut RuffleBridgePlayer,
    x: f64,
    y: f64,
) -> RuffleBridgeStatus {
    if player.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        (&*((*player).player.cast::<BridgePlayer>())).handle_mouse_move(x, y);
    })) {
        Ok(()) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_player_handle_mouse_button(
    player: *mut RuffleBridgePlayer,
    x: f64,
    y: f64,
    button: RuffleBridgeMouseButton,
    down: bool,
) -> RuffleBridgeStatus {
    if player.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    let button = to_ruffle_mouse_button(button);
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        (&*((*player).player.cast::<BridgePlayer>())).handle_mouse_button(x, y, button, down);
    })) {
        Ok(()) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_player_handle_mouse_leave(
    player: *mut RuffleBridgePlayer,
) -> RuffleBridgeStatus {
    if player.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        (&*((*player).player.cast::<BridgePlayer>())).handle_mouse_leave();
    })) {
        Ok(()) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_player_destroy(player: *mut RuffleBridgePlayer) {
    if !player.is_null() {
        unsafe {
            let player = Box::from_raw(player);
            drop(Box::from_raw(player.player.cast::<BridgePlayer>()));
        }
    }
}

/// A name-path handle onto a menu button instance. See
/// `menu_button::MenuButtonHandle` for why this is a name, re-resolved on
/// every call, rather than a live object reference.
#[repr(C)]
pub struct RuffleBridgeButton {
    handle: *mut std::ffi::c_void,
}

fn button_error_status(err: menu_button::MenuButtonError) -> RuffleBridgeStatus {
    eprintln!("Ruffle OpenGL: menu button call failed: {err:?}");
    RuffleBridgeStatus::RUFFLE_BRIDGE_BUTTON_ERROR
}

/// `instance_name` must be a valid, NUL-terminated UTF-8 C string naming a
/// direct child of the root movie's display list; it is copied, not
/// retained by reference.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_button_create(
    instance_name: *const std::ffi::c_char,
    out_button: *mut *mut RuffleBridgeButton,
) -> RuffleBridgeStatus {
    if instance_name.is_null() || out_button.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    unsafe {
        *out_button = std::ptr::null_mut();
    }
    let Ok(name) = (unsafe { std::ffi::CStr::from_ptr(instance_name) }).to_str() else {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    };
    let handle = MenuButtonHandle::new(name);
    unsafe {
        *out_button = Box::into_raw(Box::new(RuffleBridgeButton {
            handle: Box::into_raw(Box::new(handle)).cast(),
        }));
    }
    RuffleBridgeStatus::RUFFLE_BRIDGE_OK
}

/// `label` must be a valid, NUL-terminated UTF-8 C string, live for the
/// duration of this call only.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_button_init(
    player: *mut RuffleBridgePlayer,
    button: *mut RuffleBridgeButton,
    label: *const std::ffi::c_char,
    id: i32,
) -> RuffleBridgeStatus {
    if player.is_null() || button.is_null() || label.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    let Ok(label) = (unsafe { std::ffi::CStr::from_ptr(label) }).to_str() else {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    };
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        let player = &*((*player).player.cast::<BridgePlayer>());
        let handle = &*((*button).handle.cast::<MenuButtonHandle>());
        handle.init(player, label, id)
    })) {
        Ok(Ok(())) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Ok(Err(e)) => button_error_status(e),
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

/// `label` must be a valid, NUL-terminated UTF-8 C string, live for the
/// duration of this call only.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_button_set_label(
    player: *mut RuffleBridgePlayer,
    button: *mut RuffleBridgeButton,
    label: *const std::ffi::c_char,
) -> RuffleBridgeStatus {
    if player.is_null() || button.is_null() || label.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    let Ok(label) = (unsafe { std::ffi::CStr::from_ptr(label) }).to_str() else {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    };
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        let player = &*((*player).player.cast::<BridgePlayer>());
        let handle = &*((*button).handle.cast::<MenuButtonHandle>());
        handle.set_label(player, label)
    })) {
        Ok(Ok(())) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Ok(Err(e)) => button_error_status(e),
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

/// Writes the button's current label as a NUL-terminated UTF-8 string into
/// `out_buf` (capacity `out_buf_len`, including the NUL terminator);
/// truncates safely (still NUL-terminated) if the label doesn't fit.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_button_get_label(
    player: *mut RuffleBridgePlayer,
    button: *mut RuffleBridgeButton,
    out_buf: *mut std::ffi::c_char,
    out_buf_len: usize,
) -> RuffleBridgeStatus {
    if player.is_null() || button.is_null() || out_buf.is_null() || out_buf_len == 0 {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    let result = catch_unwind(AssertUnwindSafe(|| unsafe {
        let player = &*((*player).player.cast::<BridgePlayer>());
        let handle = &*((*button).handle.cast::<MenuButtonHandle>());
        handle.get_label(player)
    }));
    match result {
        Ok(Ok(label)) => {
            let bytes = label.as_bytes();
            let copy_len = bytes.len().min(out_buf_len - 1);
            unsafe {
                std::ptr::copy_nonoverlapping(bytes.as_ptr(), out_buf.cast(), copy_len);
                *out_buf.add(copy_len) = 0;
            }
            RuffleBridgeStatus::RUFFLE_BRIDGE_OK
        }
        Ok(Err(e)) => button_error_status(e),
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_button_set_enabled(
    player: *mut RuffleBridgePlayer,
    button: *mut RuffleBridgeButton,
    enabled: bool,
) -> RuffleBridgeStatus {
    if player.is_null() || button.is_null() {
        return RuffleBridgeStatus::RUFFLE_BRIDGE_INVALID_ARGUMENT;
    }
    match catch_unwind(AssertUnwindSafe(|| unsafe {
        let player = &*((*player).player.cast::<BridgePlayer>());
        let handle = &*((*button).handle.cast::<MenuButtonHandle>());
        handle.set_enabled(player, enabled)
    })) {
        Ok(Ok(())) => RuffleBridgeStatus::RUFFLE_BRIDGE_OK,
        Ok(Err(e)) => button_error_status(e),
        Err(_) => RuffleBridgeStatus::RUFFLE_BRIDGE_RENDER_ERROR,
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn ruffle_bridge_button_destroy(button: *mut RuffleBridgeButton) {
    if !button.is_null() {
        unsafe {
            let button = Box::from_raw(button);
            drop(Box::from_raw(button.handle.cast::<MenuButtonHandle>()));
        }
    }
}
