use std::time::{Duration, Instant};
use wayland_compositor::{Compositor, CompositorConfig, PipeWireCamera, PipeWireConfig};

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let run_indefinitely = args.iter().any(|a| a == "--daemon" || a == "-d");

    let socket_name = std::env::var("WAYLAND_DISPLAY")
        .unwrap_or_else(|_| "wayland-lce-test".to_string());

    println!("[LCE Wayland Compositor] Starting compositor server...");

    let comp_config = CompositorConfig {
        socket_name: Some(socket_name.clone()),
        width: 1280,
        height: 720,
    };

    let mut compositor = match Compositor::new(comp_config) {
        Ok(c) => c,
        Err(e) => {
            eprintln!("[Compositor Error] Failed to initialize compositor: {}", e);
            std::process::exit(1);
        }
    };

    println!(
        "[LCE Wayland Compositor] Wayland display ready on WAYLAND_DISPLAY={}",
        compositor.socket_name()
    );

    // Initialize PipeWire virtual camera sink
    let pw_config = PipeWireConfig {
        node_name: "lce-virtual-camera".to_string(),
        media_name: "LCE Virtual Camera".to_string(),
        width: 1280,
        height: 720,
        framerate: 60,
    };

    let mut camera = match PipeWireCamera::new(pw_config) {
        Ok(cam) => {
            println!("[LCE PipeWire Camera] Virtual camera stream created as Video/Source.");
            Some(cam)
        }
        Err(e) => {
            eprintln!(
                "[LCE PipeWire Camera] Warning: Could not connect to PipeWire daemon: {}",
                e
            );
            None
        }
    };

    if run_indefinitely {
        println!("[LCE Wayland Compositor] Running in daemon mode (press Ctrl+C to stop)...");
    } else {
        println!("[LCE Wayland Compositor] Running test loop for 2 seconds (pass --daemon to keep running)...");
    }

    let start_time = Instant::now();
    let frame_interval = Duration::from_millis(16); // ~60fps
    let mut last_frame = Instant::now();
    let mut frame_count: u64 = 0;

    loop {
        if !run_indefinitely && start_time.elapsed() >= Duration::from_secs(2) {
            break;
        }

        // Dispatch Wayland client events
        compositor.dispatch(5);

        // Process PipeWire events
        if let Some(cam) = &mut camera {
            cam.iterate(1);
        }

        if last_frame.elapsed() >= frame_interval {
            let time_ms = start_time.elapsed().as_millis() as u32;
            let fb = compositor.render_frame(time_ms);

            if let Some(cam) = &mut camera {
                cam.push_frame(fb);
            }

            frame_count += 1;
            last_frame = Instant::now();
        }

        std::thread::sleep(Duration::from_millis(1));
    }

    println!(
        "[LCE Wayland Compositor] Completed: successfully composited and dispatched {} frames.",
        frame_count
    );
}
