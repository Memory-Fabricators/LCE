//! PNG image decoding using zune-png.
#![allow(unused_imports, dead_code)]

use std::ffi::{CStr, c_char, c_void};
use std::fs;
use zune_core::bytestream::ZCursor;
use zune_core::colorspace::ColorSpace;
use zune_core::options::DecoderOptions;
use zune_core::result::DecodingResult;
use zune_png::PngDecoder;

#[repr(C)]
pub struct DecodedImage {
    pub pixels: *mut u32,
    pub width: i32,
    pub height: i32,
}

#[no_mangle]
pub unsafe extern "C" fn angle_wgpu_decode_png_memory(
    data: *const u8,
    len: usize,
    out_image: *mut DecodedImage,
) -> bool {
    if data.is_null() || len == 0 || out_image.is_null() {
        return false;
    }

    let slice = std::slice::from_raw_parts(data, len);
    let mut decoder = PngDecoder::new(ZCursor::new(slice));

    if decoder.decode_headers().is_err() {
        return false;
    }

    let (width, height) = match decoder.dimensions() {
        Some(dim) => dim,
        None => return false,
    };

    let colorspace = decoder.colorspace().unwrap_or(ColorSpace::RGBA);

    let result = match decoder.decode() {
        Ok(res) => res,
        Err(e) => {
            log::error!("Failed to decode PNG: {e:?}");
            return false;
        }
    };

    let u8_pixels = match result {
        DecodingResult::U8(vec) => vec,
        _ => return false,
    };

    let pixel_count = width * height;
    let mut u32_vec: Vec<u32> = Vec::with_capacity(pixel_count);

    match colorspace {
        ColorSpace::RGBA => {
            for chunk in u8_pixels.chunks_exact(4) {
                let r = chunk[0] as u32;
                let g = chunk[1] as u32;
                let b = chunk[2] as u32;
                let a = chunk[3] as u32;
                let argb = (a << 24) | (r << 16) | (g << 8) | b;
                u32_vec.push(argb);
            }
        }
        ColorSpace::RGB => {
            for chunk in u8_pixels.chunks_exact(3) {
                let r = chunk[0] as u32;
                let g = chunk[1] as u32;
                let b = chunk[2] as u32;
                let a = 0xFF;
                let argb = (a << 24) | (r << 16) | (g << 8) | b;
                u32_vec.push(argb);
            }
        }
        ColorSpace::LumaA => {
            for chunk in u8_pixels.chunks_exact(2) {
                let l = chunk[0] as u32;
                let a = chunk[1] as u32;
                let argb = (a << 24) | (l << 16) | (l << 8) | l;
                u32_vec.push(argb);
            }
        }
        ColorSpace::Luma => {
            for &l in &u8_pixels {
                let l32 = l as u32;
                let argb = (0xFF << 24) | (l32 << 16) | (l32 << 8) | l32;
                u32_vec.push(argb);
            }
        }
        _ => {
            log::warn!("Unsupported PNG colorspace: {colorspace:?}");
            return false;
        }
    }

    let mut boxed_slice = u32_vec.into_boxed_slice();
    let ptr = boxed_slice.as_mut_ptr();
    std::mem::forget(boxed_slice);

    (*out_image).pixels = ptr;
    (*out_image).width = width as i32;
    (*out_image).height = height as i32;
    true
}

#[no_mangle]
pub unsafe extern "C" fn angle_wgpu_decode_png_file(
    path: *const c_char,
    out_image: *mut DecodedImage,
) -> bool {
    if path.is_null() || out_image.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let path_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return false,
    };

    let data = match fs::read(path_str) {
        Ok(d) => d,
        Err(e) => {
            log::debug!("Could not read PNG file '{path_str}': {e}");
            return false;
        }
    };

    angle_wgpu_decode_png_memory(data.as_ptr(), data.len(), out_image)
}

#[no_mangle]
pub unsafe extern "C" fn angle_wgpu_free_decoded_image(image: *mut DecodedImage) {
    if image.is_null() {
        return;
    }
    let img = &mut *image;
    if !img.pixels.is_null() && img.width > 0 && img.height > 0 {
        let size = (img.width * img.height) as usize;
        let _ = Box::from_raw(std::slice::from_raw_parts_mut(img.pixels, size));
        img.pixels = std::ptr::null_mut();
        img.width = 0;
        img.height = 0;
    }
}
