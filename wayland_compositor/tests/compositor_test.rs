use wayland_compositor::{Compositor, CompositorConfig, FrameBuffer, PipeWireCamera, PipeWireConfig};

#[test]
fn test_framebuffer_allocation_and_clear() {
    let mut fb = FrameBuffer::new(64, 64);
    assert_eq!(fb.width, 64);
    assert_eq!(fb.height, 64);
    assert_eq!(fb.data.len(), 64 * 64 * 4);

    fb.clear(255, 128, 64, 255);
    assert_eq!(fb.data[0], 255);
    assert_eq!(fb.data[1], 128);
    assert_eq!(fb.data[2], 64);
    assert_eq!(fb.data[3], 255);
}

#[test]
fn test_compositor_lifecycle() {
    if std::env::var("XDG_RUNTIME_DIR").is_err() {
        let tmp_dir = std::env::temp_dir().join("lce-test-runtime");
        let _ = std::fs::create_dir_all(&tmp_dir);
        unsafe {
            std::env::set_var("XDG_RUNTIME_DIR", tmp_dir);
        }
    }

    let config = CompositorConfig {
        socket_name: Some("wayland-lce-unit-test".to_string()),
        width: 640,
        height: 480,
    };

    let mut compositor = match Compositor::new(config) {
        Ok(c) => c,
        Err(e) => {
            eprintln!("Skipping compositor test if display binding not possible in sandbox: {e}");
            return;
        }
    };

    assert_eq!(compositor.socket_name(), "wayland-lce-unit-test");

    // Render frame
    let fb = compositor.render_frame(0);
    assert_eq!(fb.width, 640);
    assert_eq!(fb.height, 480);

    // Dispatch loop check with timeout 0
    let res = compositor.dispatch(0);
    assert!(res >= 0);
}

#[test]
fn test_pipewire_camera_init() {
    let pw_config = PipeWireConfig {
        node_name: "lce-unit-test-camera".to_string(),
        media_name: "LCE Unit Test Camera".to_string(),
        width: 320,
        height: 240,
        framerate: 30,
    };

    if let Ok(mut cam) = PipeWireCamera::new(pw_config) {
        let fb = FrameBuffer::new(320, 240);
        cam.iterate(0);
        let _ = cam.push_frame(&fb);
    }
}
