//! PipeWire virtual camera / video sink publishing compositor frames.

use std::ffi::{c_char, c_void, CStr, CString};
use std::ptr;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

use crate::ffi::*;
use crate::shm::FrameBuffer;

#[derive(Debug, Clone)]
pub struct PipeWireConfig {
    pub node_name: String,
    pub media_name: String,
    pub width: u32,
    pub height: u32,
    pub framerate: u32,
}

impl Default for PipeWireConfig {
    fn default() -> Self {
        Self {
            node_name: "lce-wayland-camera".to_string(),
            media_name: "LCE Wayland Virtual Camera".to_string(),
            width: 1280,
            height: 720,
            framerate: 30,
        }
    }
}

pub struct PipeWireCamera {
    main_loop: *mut PwMainLoop,
    loop_: *mut PwLoop,
    context: *mut PwContext,
    core: *mut PwCore,
    stream: *mut PwStream,
    _config: PipeWireConfig,
    is_streaming: Arc<AtomicBool>,
}

// Global hook callback for stream events
static STREAM_EVENTS: PwStreamEvents = PwStreamEvents {
    version: PW_VERSION_STREAM_EVENTS,
    destroy: None,
    state_changed: Some(on_state_changed),
    control_info: None,
    io_changed: None,
    param_changed: None,
    add_buffer: None,
    remove_buffer: None,
    process: Some(on_process),
    drained: None,
    command: None,
    trigger_done: None,
};

extern "C" fn on_state_changed(
    data: *mut c_void,
    _old: PwStreamState,
    state: PwStreamState,
    error: *const c_char,
) {
    if data.is_null() {
        return;
    }
    let is_streaming = unsafe { &*(data as *const AtomicBool) };
    match state {
        PwStreamState::Streaming => {
            is_streaming.store(true, Ordering::SeqCst);
        }
        PwStreamState::Paused => {
            is_streaming.store(true, Ordering::SeqCst);
        }
        PwStreamState::Error => {
            is_streaming.store(false, Ordering::SeqCst);
            if !error.is_null() {
                let err_str = unsafe { CStr::from_ptr(error) };
                eprintln!("[PipeWire] Stream error: {:?}", err_str);
            }
        }
        _ => {
            is_streaming.store(false, Ordering::SeqCst);
        }
    }
}

extern "C" fn on_process(_data: *mut c_void) {
    // Pipewire pull-cycle callback
}

impl PipeWireCamera {
    pub fn new(config: PipeWireConfig) -> Result<Self, String> {
        let pw = pipewire_lib().map_err(|e| format!("Failed to load pipewire library: {e}"))?;

        unsafe {
            (pw.pw_init)(ptr::null_mut(), ptr::null_mut());

            let main_loop = (pw.pw_main_loop_new)(ptr::null());
            if main_loop.is_null() {
                return Err("Failed to create pw_main_loop".to_string());
            }

            let loop_ = (pw.pw_main_loop_get_loop)(main_loop);
            if loop_.is_null() {
                (pw.pw_main_loop_destroy)(main_loop);
                return Err("Failed to get pw_loop".to_string());
            }

            let context = (pw.pw_context_new)(loop_, ptr::null_mut(), 0);
            if context.is_null() {
                (pw.pw_main_loop_destroy)(main_loop);
                return Err("Failed to create pw_context".to_string());
            }

            let core = (pw.pw_context_connect)(context, ptr::null_mut(), 0);
            if core.is_null() {
                (pw.pw_context_destroy)(context);
                (pw.pw_main_loop_destroy)(main_loop);
                return Err("Failed to connect pw_core".to_string());
            }

            let is_streaming = Arc::new(AtomicBool::new(false));

            // Stream properties for a standalone Video/Source (Virtual Camera provider)
            let props_str = format!(
                "media.class = Video/Source\nmedia.type = Video\nmedia.category = Capture\nmedia.role = Camera\nmedia.name = \"{}\"\nnode.name = \"{}\"\nnode.description = \"{}\"\n",
                config.media_name, config.node_name, config.media_name
            );
            let c_props = CString::new(props_str).unwrap();
            let props = (pw.pw_properties_new_string)(c_props.as_ptr());

            let is_streaming_ptr = Arc::as_ptr(&is_streaming) as *mut c_void;
            let media_name = CString::new(config.media_name.clone()).unwrap();
            let stream = (pw.pw_stream_new)(core, media_name.as_ptr(), props);
            if stream.is_null() {
                (pw.pw_core_disconnect)(core);
                (pw.pw_context_destroy)(context);
                (pw.pw_main_loop_destroy)(main_loop);
                return Err("Failed to create pw_stream".to_string());
            }

            let hook = Box::new(SpaHook {
                link: SpaList {
                    prev: ptr::null_mut(),
                    next: ptr::null_mut(),
                },
                cb: SpaCallbacks {
                    funcs: ptr::null(),
                    data: ptr::null_mut(),
                },
                removed: None,
                priv_: ptr::null_mut(),
            });

            (pw.pw_stream_add_listener)(
                stream,
                Box::into_raw(hook),
                &STREAM_EVENTS,
                is_streaming_ptr,
            );

            // Connect as Video/Source without AUTOCONNECT flag so it acts as an available source device
            let flags = PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_ALLOC_BUFFERS;

            let res = (pw.pw_stream_connect)(
                stream,
                PwDirection::Output,
                0xffffffff, // PW_ID_ANY
                flags,
                ptr::null(),
                0,
            );

            if res != 0 {
                (pw.pw_stream_destroy)(stream);
                (pw.pw_core_disconnect)(core);
                (pw.pw_context_destroy)(context);
                (pw.pw_main_loop_destroy)(main_loop);
                return Err(format!("pw_stream_connect returned {}", res));
            }

            Ok(Self {
                main_loop,
                loop_,
                context,
                core,
                stream,
                _config: config,
                is_streaming,
            })
        }
    }

    pub fn is_streaming(&self) -> bool {
        self.is_streaming.load(Ordering::SeqCst)
    }

    /// Iterate the pipewire loop with a timeout.
    pub fn iterate(&mut self, timeout_ms: i32) {
        if let Ok(pw) = pipewire_lib() {
            unsafe {
                if !self.loop_.is_null() {
                    (pw.pw_loop_iterate)(self.loop_, timeout_ms);
                }
            }
        }
    }

    /// Push a frame to PipeWire subscribers.
    pub fn push_frame(&mut self, fb: &FrameBuffer) -> bool {
        let pw = match pipewire_lib() {
            Ok(p) => p,
            Err(_) => return false,
        };

        unsafe {
            if self.stream.is_null() {
                return false;
            }

            let pw_buf = (pw.pw_stream_dequeue_buffer)(self.stream);
            if pw_buf.is_null() {
                return false;
            }

            let spa_buf = (*pw_buf).buffer;
            if spa_buf.is_null() || (*spa_buf).n_datas == 0 {
                (pw.pw_stream_queue_buffer)(self.stream, pw_buf);
                return false;
            }

            let data = (*spa_buf).datas;
            if data.is_null() || (*data).data.is_null() {
                (pw.pw_stream_queue_buffer)(self.stream, pw_buf);
                return false;
            }

            let dest_ptr = (*data).data as *mut u8;
            let dest_max = (*data).maxsize as usize;
            let copy_len = fb.data.len().min(dest_max);

            ptr::copy_nonoverlapping(fb.data.as_ptr(), dest_ptr, copy_len);

            let chunk = (*data).chunk;
            if !chunk.is_null() {
                (*chunk).offset = 0;
                (*chunk).size = copy_len as u32;
                (*chunk).stride = (fb.width * 4) as i32;
            }

            (pw.pw_stream_queue_buffer)(self.stream, pw_buf);
            (pw.pw_stream_trigger_process)(self.stream);
            true
        }
    }
}

impl Drop for PipeWireCamera {
    fn drop(&mut self) {
        if let Ok(pw) = pipewire_lib() {
            unsafe {
                if !self.stream.is_null() {
                    (pw.pw_stream_disconnect)(self.stream);
                    (pw.pw_stream_destroy)(self.stream);
                }
                if !self.core.is_null() {
                    (pw.pw_core_disconnect)(self.core);
                }
                if !self.context.is_null() {
                    (pw.pw_context_destroy)(self.context);
                }
                if !self.main_loop.is_null() {
                    (pw.pw_main_loop_destroy)(self.main_loop);
                }
            }
        }
    }
}
