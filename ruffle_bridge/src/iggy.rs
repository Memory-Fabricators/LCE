//! Iggy C ABI shim for Ruffle (OpenGL backend).
//!
//! Provides the `iggy_open_*` C-ABI functions used by the LCE UI subsystem
//! to manage SWF libraries, create/tick/render player instances against
//! an OpenGL context, dispatch ActionScript 3 callbacks, and register TrueType fonts.

use std::ffi::{c_char, c_void, CStr};
use std::sync::atomic::{AtomicPtr, AtomicU32, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{SystemTime, UNIX_EPOCH};

use crate::opengl_renderer::OpenGlRenderer;
use ruffle_core::backend::ui::FontDefinition;
use ruffle_core::context::UpdateContext;
use ruffle_core::external::{ExternalInterfaceProvider, Value as ExtValue};
use ruffle_core::font::FontFileData;
use ruffle_core::tag_utils::SwfMovie;
use ruffle_core::{Color, Player, PlayerBuilder, ViewportDimensions};

/// Global font registry. Populated by `iggy_open_install_truetype_utf8`
/// and applied to every newly-built Player via `Player::register_device_font`.
struct FontEntry {
    name: String,
    data: Arc<Vec<u8>>,
}

static FONT_REGISTRY: Mutex<Vec<FontEntry>> = Mutex::new(Vec::new());

/// Global library registry. Populated by `iggy_open_library_create_from_memory`
/// and replayed into each Player's root ApplicationDomain so secondary SWF
/// symbols and PlaceByClass references resolve properly.
static LIBRARY_REGISTRY: Mutex<Vec<Arc<SwfMovie>>> = Mutex::new(Vec::new());

/// Concurrent cap on full-inject Players (refunded on player_destroy).
static FULL_INJECT_BUDGET: AtomicU32 = AtomicU32::new(8);

fn _inject_library_abcs(player: &Arc<Mutex<Player>>) -> bool {
    let libs: Vec<Arc<SwfMovie>> = match LIBRARY_REGISTRY.lock() {
        Ok(g) => g.clone(),
        Err(_) => return false,
    };
    if libs.is_empty() {
        return false;
    }
    let use_full = FULL_INJECT_BUDGET
        .try_update(Ordering::Relaxed, Ordering::Relaxed, |b| {
            if b > 0 {
                Some(b - 1)
            } else {
                None
            }
        })
        .is_ok();

    if let Ok(mut p) = player.try_lock() {
        for lib in libs.iter() {
            if use_full {
                p.inject_secondary_swf_full(lib.clone());
            } else {
                p.inject_secondary_swf_abc(lib.clone());
            }
        }
    }
    use_full
}

fn _apply_registered_fonts(player: &Arc<Mutex<Player>>) {
    let entries: Vec<FontEntry> = match FONT_REGISTRY.lock() {
        Ok(g) => g
            .iter()
            .map(|e| FontEntry {
                name: e.name.clone(),
                data: e.data.clone(),
            })
            .collect(),
        Err(_) => return,
    };
    if entries.is_empty() {
        return;
    }
    if let Ok(mut p) = player.try_lock() {
        for e in entries {
            let arc_data: Arc<dyn AsRef<[u8]>> = e.data.clone();
            let def = FontDefinition::FontFile {
                name: e.name.clone(),
                is_bold: false,
                is_italic: false,
                data: FontFileData::new_shared(arc_data),
                index: 0,
            };
            p.register_device_font(def);
        }
    }
}

/// Install a TrueType font into the global registry for device font resolution.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_install_truetype_utf8(
    name_utf8: *const c_char,
    name_len: i32,
    ttf_data: *const u8,
    ttf_len: usize,
) {
    if name_utf8.is_null() || ttf_data.is_null() || ttf_len == 0 {
        return;
    }
    let name = if name_len > 0 {
        let slice =
            unsafe { std::slice::from_raw_parts(name_utf8 as *const u8, name_len as usize) };
        String::from_utf8_lossy(slice).into_owned()
    } else {
        unsafe { CStr::from_ptr(name_utf8) }
            .to_string_lossy()
            .into_owned()
    };
    let data: Vec<u8> = unsafe { std::slice::from_raw_parts(ttf_data, ttf_len) }.to_vec();
    if let Ok(mut g) = FONT_REGISTRY.lock() {
        g.push(FontEntry {
            name,
            data: Arc::new(data),
        });
    }
}

// ======================================================================
// ExternalInterface bridge & AS3 dispatch
// ======================================================================

#[repr(C)]
#[derive(Clone, Copy)]
pub struct IggyDataValueRaw {
    pub typ: i32,
    pub _pad: i32,
    pub temp_ref: usize,
    pub union_data: [u8; 16],
}

impl IggyDataValueRaw {
    pub fn undefined() -> Self {
        Self {
            typ: 1,
            _pad: 0,
            temp_ref: 0,
            union_data: [0; 16],
        }
    }
    pub fn null() -> Self {
        Self {
            typ: 2,
            _pad: 0,
            temp_ref: 0,
            union_data: [0; 16],
        }
    }
    pub fn boolean(b: bool) -> Self {
        let mut d = [0u8; 16];
        d[0..4].copy_from_slice(&(if b { 1i32 } else { 0i32 }).to_le_bytes());
        Self {
            typ: 3,
            _pad: 0,
            temp_ref: 0,
            union_data: d,
        }
    }
    pub fn number(n: f64) -> Self {
        let mut d = [0u8; 16];
        d[0..8].copy_from_slice(&n.to_le_bytes());
        Self {
            typ: 4,
            _pad: 0,
            temp_ref: 0,
            union_data: d,
        }
    }
    pub fn string_utf16(ptr: *const u16, len: i32) -> Self {
        let mut d = [0u8; 16];
        d[0..8].copy_from_slice(&(ptr as usize).to_le_bytes());
        d[8..12].copy_from_slice(&len.to_le_bytes());
        Self {
            typ: 6,
            _pad: 0,
            temp_ref: 0,
            union_data: d,
        }
    }
}

pub type As3DispatchFn = extern "C" fn(
    player: *mut OpaquePlayer,
    func_name_utf16: *const u16,
    func_name_len: i32,
    args: *const IggyDataValueRaw,
    num_args: i32,
) -> i32;

static AS3_DISPATCH: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_set_as3_dispatch(fn_ptr: Option<As3DispatchFn>) {
    let p = match fn_ptr {
        Some(f) => f as *mut (),
        None => std::ptr::null_mut(),
    };
    AS3_DISPATCH.store(p, Ordering::SeqCst);
}

unsafe fn _decode_iggy_args(args: *const IggyDataValueRaw, num_args: i32) -> Vec<ExtValue> {
    let mut out: Vec<ExtValue> = Vec::with_capacity(num_args.max(0) as usize);
    if num_args <= 0 || args.is_null() {
        return out;
    }
    for i in 0..num_args as isize {
        let raw = unsafe { &*args.offset(i) };
        out.push(match raw.typ {
            1 => ExtValue::Undefined,
            2 => ExtValue::Null,
            3 => {
                let b = i32::from_le_bytes([
                    raw.union_data[0],
                    raw.union_data[1],
                    raw.union_data[2],
                    raw.union_data[3],
                ]);
                ExtValue::Bool(b != 0)
            }
            4 => {
                let n = f64::from_le_bytes([
                    raw.union_data[0],
                    raw.union_data[1],
                    raw.union_data[2],
                    raw.union_data[3],
                    raw.union_data[4],
                    raw.union_data[5],
                    raw.union_data[6],
                    raw.union_data[7],
                ]);
                ExtValue::Number(n)
            }
            6 => {
                let ptr_bytes = &raw.union_data[0..8];
                let len_bytes = &raw.union_data[8..12];
                let ptr_v =
                    usize::from_le_bytes(ptr_bytes.try_into().unwrap_or([0; 8])) as *const u16;
                let len = i32::from_le_bytes(len_bytes.try_into().unwrap_or([0; 4]));
                if ptr_v.is_null() || len <= 0 {
                    ExtValue::String(String::new())
                } else {
                    let slice = unsafe { std::slice::from_raw_parts(ptr_v, len as usize) };
                    ExtValue::String(String::from_utf16_lossy(slice))
                }
            }
            _ => ExtValue::Undefined,
        });
    }
    out
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_call_as3(
    p: *mut OpaquePlayer,
    name_utf8: *const c_char,
    args: *const IggyDataValueRaw,
    num_args: i32,
) -> i32 {
    if p.is_null() || name_utf8.is_null() {
        return 0;
    }
    let opaque = unsafe { &mut *p };
    let name = unsafe { CStr::from_ptr(name_utf8) }
        .to_string_lossy()
        .into_owned();
    let ext_args = unsafe { _decode_iggy_args(args, num_args) };
    if let Ok(mut player) = opaque.player.try_lock() {
        let _ = player.call_root_method_avm2(&name, ext_args);
        1
    } else {
        0
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_call_as3_path(
    p: *mut OpaquePlayer,
    path_utf8: *const c_char,
    method_utf8: *const c_char,
    args: *const IggyDataValueRaw,
    num_args: i32,
) -> i32 {
    if p.is_null() || method_utf8.is_null() {
        return 0;
    }
    let opaque = unsafe { &mut *p };
    let path = if path_utf8.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(path_utf8) }
            .to_string_lossy()
            .into_owned()
    };
    let method = unsafe { CStr::from_ptr(method_utf8) }
        .to_string_lossy()
        .into_owned();
    let ext_args = unsafe { _decode_iggy_args(args, num_args) };

    if let Ok(mut player) = opaque.player.try_lock() {
        let _ = player.call_method_at_path_avm2(&path, &method, ext_args);
        1
    } else {
        0
    }
}

fn _invoke_as3_dispatch(
    player: *mut OpaquePlayer,
    name: *const u16,
    name_len: i32,
    args: *const IggyDataValueRaw,
    num_args: i32,
) -> i32 {
    let p = AS3_DISPATCH.load(Ordering::SeqCst);
    if p.is_null() {
        return 0;
    }
    let f: As3DispatchFn = unsafe { std::mem::transmute(p) };
    f(player, name, name_len, args, num_args)
}

struct IggyExternalBridge {
    player_ptr: *mut OpaquePlayer,
}

unsafe impl Send for IggyExternalBridge {}
unsafe impl Sync for IggyExternalBridge {}

impl ExternalInterfaceProvider for IggyExternalBridge {
    fn call_method(
        &self,
        _ctx: &mut UpdateContext<'_>,
        name: &str,
        args: &[ExtValue],
    ) -> ExtValue {
        let name_utf16: Vec<u16> = name.encode_utf16().collect();
        let mut string_holders: Vec<Vec<u16>> = Vec::with_capacity(args.len());
        let mut iggy_args: Vec<IggyDataValueRaw> = Vec::with_capacity(args.len());
        for arg in args {
            let raw = match arg {
                ExtValue::Undefined => IggyDataValueRaw::undefined(),
                ExtValue::Null => IggyDataValueRaw::null(),
                ExtValue::Bool(b) => IggyDataValueRaw::boolean(*b),
                ExtValue::Number(n) => IggyDataValueRaw::number(*n),
                ExtValue::String(s) => {
                    let utf16: Vec<u16> =
                        s.encode_utf16().chain(std::iter::once(0u16)).collect();
                    let ptr = utf16.as_ptr();
                    let len = (utf16.len() - 1) as i32;
                    string_holders.push(utf16);
                    IggyDataValueRaw::string_utf16(ptr, len)
                }
                ExtValue::Object(_) | ExtValue::List(_) => IggyDataValueRaw::undefined(),
            };
            iggy_args.push(raw);
        }
        _invoke_as3_dispatch(
            self.player_ptr,
            name_utf16.as_ptr(),
            name_utf16.len() as i32,
            iggy_args.as_ptr(),
            iggy_args.len() as i32,
        );
        drop(string_holders);
        ExtValue::Undefined
    }
    fn on_callback_available(&self, _name: &str) {}
    fn get_id(&self) -> Option<String> {
        None
    }
}

// ======================================================================
// Opaque types & Player lifecycle
// ======================================================================

pub struct OpaqueLibrary {
    pub movie: Arc<SwfMovie>,
}

pub struct OpaquePlayer {
    pub player: Arc<Mutex<Player>>,
    pub viewport: (u32, u32),
    pub next_frame_due_us: u64,
    pub used_full_inject: bool,
}

fn _now_us() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_micros() as u64)
        .unwrap_or(0)
}

#[unsafe(no_mangle)]
pub extern "C" fn iggy_open_version() -> *const c_char {
    static VERSION: &[u8] = b"ruffle_iggy_shim 0.3.0 (OpenGL)\0";
    VERSION.as_ptr() as *const c_char
}

#[unsafe(no_mangle)]
pub extern "C" fn iggy_open_magic() -> u32 {
    0x4F50454E // "OPEN"
}

#[unsafe(no_mangle)]
pub extern "C" fn iggy_open_init(_allocator: *const c_void) {}

#[unsafe(no_mangle)]
pub extern "C" fn iggy_open_shutdown() {}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_library_create_from_memory(
    data: *const u8,
    len: usize,
    name_utf8: *const c_char,
) -> *mut OpaqueLibrary {
    if data.is_null() || len == 0 {
        return std::ptr::null_mut();
    }
    let bytes: Vec<u8> = unsafe { std::slice::from_raw_parts(data, len) }.to_vec();
    let name: String = if name_utf8.is_null() {
        String::from("unnamed.swf")
    } else {
        unsafe { CStr::from_ptr(name_utf8) }
            .to_string_lossy()
            .into_owned()
    };
    let url = format!("file:///lce/{}", name);
    match SwfMovie::from_data(&bytes, url, None, None) {
        Ok(movie) => {
            let movie_arc = Arc::new(movie);
            if let Ok(mut g) = LIBRARY_REGISTRY.lock() {
                g.push(movie_arc.clone());
            }
            let lib = OpaqueLibrary { movie: movie_arc };
            Box::into_raw(Box::new(lib))
        }
        Err(_) => std::ptr::null_mut(),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_library_destroy(lib: *mut OpaqueLibrary) {
    if lib.is_null() {
        return;
    }
    {
        let opaque = unsafe { &*lib };
        let target_ptr = Arc::as_ptr(&opaque.movie);
        if let Ok(mut g) = LIBRARY_REGISTRY.lock() {
            g.retain(|m| !std::ptr::eq(Arc::as_ptr(m), target_ptr));
        }
    }
    drop(unsafe { Box::from_raw(lib) });
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_player_create_from_memory(
    data: *const u8,
    len: usize,
) -> *mut OpaquePlayer {
    if data.is_null() || len == 0 {
        return std::ptr::null_mut();
    }
    let bytes: Vec<u8> = unsafe { std::slice::from_raw_parts(data, len) }.to_vec();
    let movie = match SwfMovie::from_data(&bytes, String::from("file:///lce/inline.swf"), None, None)
    {
        Ok(m) => m,
        Err(_) => return std::ptr::null_mut(),
    };

    let init_w = movie.width().to_pixels().max(1.0).round() as u32;
    let init_h = movie.height().to_pixels().max(1.0).round() as u32;

    let renderer = match OpenGlRenderer::new(init_w, init_h) {
        Ok(r) => r,
        Err(_) => return std::ptr::null_mut(),
    };

    use std::mem::MaybeUninit;
    let opaque_storage: Box<MaybeUninit<OpaquePlayer>> = Box::new(MaybeUninit::uninit());
    let opaque_ptr: *mut OpaquePlayer = Box::into_raw(opaque_storage) as *mut OpaquePlayer;
    let bridge = IggyExternalBridge {
        player_ptr: opaque_ptr,
    };

    let player = PlayerBuilder::new()
        .with_boxed_renderer(Box::new(renderer))
        .with_movie(movie)
        .with_autoplay(true)
        .with_external_interface(Box::new(bridge))
        .build();

    if let Ok(mut p) = player.try_lock() {
        p.set_background_color(Some(Color {
            r: 0,
            g: 0,
            b: 0,
            a: 0,
        }));
    }

    _apply_registered_fonts(&player);
    let used_full_inject = _inject_library_abcs(&player);

    unsafe {
        std::ptr::write(
            opaque_ptr,
            OpaquePlayer {
                player,
                viewport: (init_w, init_h),
                next_frame_due_us: _now_us(),
                used_full_inject,
            },
        );
    }
    opaque_ptr
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_player_destroy(p: *mut OpaquePlayer) {
    if p.is_null() {
        return;
    }
    let used_full = unsafe { (*p).used_full_inject };
    drop(unsafe { Box::from_raw(p) });
    if used_full {
        FULL_INJECT_BUDGET.fetch_add(1, Ordering::Relaxed);
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_player_tick(p: *mut OpaquePlayer) {
    if p.is_null() {
        return;
    }
    let opaque = unsafe { &mut *p };
    let fps = {
        if let Ok(player) = opaque.player.try_lock() {
            let f = player.frame_rate();
            if f.is_finite() && f > 1.0 && f < 240.0 {
                f
            } else {
                30.0
            }
        } else {
            30.0
        }
    };
    let interval_us = (1_000_000.0 / fps) as u64;
    let now = _now_us();
    opaque.next_frame_due_us = if now > opaque.next_frame_due_us + 4 * interval_us {
        now + interval_us
    } else {
        opaque.next_frame_due_us + interval_us
    };
    if let Ok(mut player) = opaque.player.try_lock() {
        player.run_frame();
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_player_ready_to_tick(p: *mut OpaquePlayer) -> i32 {
    if p.is_null() {
        return 0;
    }
    let opaque = unsafe { &*p };
    if _now_us() >= opaque.next_frame_due_us {
        1
    } else {
        0
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_player_set_viewport(
    p: *mut OpaquePlayer,
    w: u32,
    h: u32,
) {
    if p.is_null() || w == 0 || h == 0 {
        return;
    }
    let opaque = unsafe { &mut *p };
    if opaque.viewport == (w, h) {
        return;
    }
    if let Ok(mut player) = opaque.player.try_lock() {
        player.set_viewport_dimensions(ViewportDimensions {
            width: w,
            height: h,
            scale_factor: 1.0,
        });
        opaque.viewport = (w, h);
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn iggy_open_player_render(
    p: *mut OpaquePlayer,
    out_w: *mut u32,
    out_h: *mut u32,
) -> *const u8 {
    if p.is_null() {
        return std::ptr::null();
    }
    let opaque = unsafe { &mut *p };
    if let Ok(mut player) = opaque.player.try_lock() {
        player.render();
    }
    if !out_w.is_null() {
        unsafe {
            *out_w = opaque.viewport.0;
        }
    }
    if !out_h.is_null() {
        unsafe {
            *out_h = opaque.viewport.1;
        }
    }
    // Direct OpenGL rendering renders into the active OpenGL framebuffer.
    std::ptr::null()
}
