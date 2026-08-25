pub mod compositor;
pub mod ffi;
pub mod pipewire;
pub mod protocol;
pub mod shm;

pub use compositor::{Compositor, CompositorConfig};
pub use pipewire::{PipeWireCamera, PipeWireConfig};
pub use shm::FrameBuffer;
