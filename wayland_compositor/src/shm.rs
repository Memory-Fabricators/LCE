//! SHM buffer reading and rasterization utilities.

use crate::ffi::*;

/// A simple RGBA buffer for composition.
#[derive(Debug, Clone)]
pub struct FrameBuffer {
    pub width: u32,
    pub height: u32,
    pub data: Vec<u8>,
}

impl FrameBuffer {
    pub fn new(width: u32, height: u32) -> Self {
        Self {
            width,
            height,
            data: vec![0; (width * height * 4) as usize],
        }
    }

    pub fn clear(&mut self, r: u8, g: u8, b: u8, a: u8) {
        for chunk in self.data.chunks_exact_mut(4) {
            chunk[0] = r;
            chunk[1] = g;
            chunk[2] = b;
            chunk[3] = a;
        }
    }

    pub fn blit_shm_buffer(
        &mut self,
        buffer_resource: *mut WlResource,
        dest_x: i32,
        dest_y: i32,
    ) -> bool {
        if buffer_resource.is_null() {
            return false;
        }

        let wl = match wayland_server() {
            Ok(w) => w,
            Err(_) => return false,
        };

        unsafe {
            let shm_buf = (wl.wl_shm_buffer_get)(buffer_resource);
            if shm_buf.is_null() {
                return false;
            }

            (wl.wl_shm_buffer_begin_access)(shm_buf);
            let src_data = (wl.wl_shm_buffer_get_data)(shm_buf) as *const u8;
            let src_w = (wl.wl_shm_buffer_get_width)(shm_buf);
            let src_h = (wl.wl_shm_buffer_get_height)(shm_buf);
            let src_stride = (wl.wl_shm_buffer_get_stride)(shm_buf) as usize;
            let src_format = (wl.wl_shm_buffer_get_format)(shm_buf);

            if src_data.is_null() || src_w <= 0 || src_h <= 0 {
                (wl.wl_shm_buffer_end_access)(shm_buf);
                return false;
            }

            let dst_w = self.width as i32;
            let dst_h = self.height as i32;

            for y in 0..src_h {
                let target_y = dest_y + y;
                if target_y < 0 || target_y >= dst_h {
                    continue;
                }

                let src_row = src_data.add(y as usize * src_stride);

                for x in 0..src_w {
                    let target_x = dest_x + x;
                    if target_x < 0 || target_x >= dst_w {
                        continue;
                    }

                    let dst_idx = ((target_y * dst_w + target_x) * 4) as usize;
                    let src_px = src_row.add((x * 4) as usize);

                    // Wayland SHM formats:
                    // WL_SHM_FORMAT_ARGB8888 = 0
                    // WL_SHM_FORMAT_XRGB8888 = 1
                    // In little-endian memory, ARGB8888 is BGRA byte order [B, G, R, A]
                    let (b, g, r, a) = (*src_px, *src_px.add(1), *src_px.add(2), *src_px.add(3));
                    let alpha = if src_format == 1 { 255 } else { a };

                    // Simple alpha blend into RGBA target
                    if alpha == 255 {
                        self.data[dst_idx] = r;
                        self.data[dst_idx + 1] = g;
                        self.data[dst_idx + 2] = b;
                        self.data[dst_idx + 3] = 255;
                    } else if alpha > 0 {
                        let inv_a = 255 - alpha as u32;
                        let dst_r = self.data[dst_idx] as u32;
                        let dst_g = self.data[dst_idx + 1] as u32;
                        let dst_b = self.data[dst_idx + 2] as u32;

                        self.data[dst_idx] =
                            ((r as u32 * alpha as u32 + dst_r * inv_a) / 255) as u8;
                        self.data[dst_idx + 1] =
                            ((g as u32 * alpha as u32 + dst_g * inv_a) / 255) as u8;
                        self.data[dst_idx + 2] =
                            ((b as u32 * alpha as u32 + dst_b * inv_a) / 255) as u8;
                        self.data[dst_idx + 3] = 255;
                    }
                }
            }

            (wl.wl_shm_buffer_end_access)(shm_buf);
        }

        true
    }
}
