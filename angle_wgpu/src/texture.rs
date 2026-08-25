//! OpenGL texture state management and wgpu texture/sampler synchronization.

use crate::types::*;
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct TextureObject {
    pub id: GLuint,
    pub target: GLenum,
    pub width: u32,
    pub height: u32,
    pub depth: u32,
    pub levels: u32,
    pub internal_format: GLenum,
    pub format: GLenum,
    pub type_: GLenum,
    pub min_filter: GLenum,
    pub mag_filter: GLenum,
    pub wrap_s: GLenum,
    pub wrap_t: GLenum,
    pub min_lod: f32,
    pub max_lod: f32,
    pub max_level: u32,
    pub level_data: HashMap<u32, Vec<u8>>,
    pub dirty: bool,

    // GPU resources
    pub gpu_texture: Option<wgpu::Texture>,
    pub gpu_view: Option<wgpu::TextureView>,
    pub gpu_sampler: Option<wgpu::Sampler>,
    pub gpu_bind_group: Option<wgpu::BindGroup>,
}

impl TextureObject {
    pub fn new(id: GLuint, target: GLenum) -> Self {
        Self {
            id,
            target,
            width: 0,
            height: 0,
            depth: 1,
            levels: 1,
            internal_format: GL_RGBA,
            format: GL_RGBA,
            type_: GL_UNSIGNED_BYTE,
            min_filter: GL_NEAREST_MIPMAP_LINEAR,
            mag_filter: GL_LINEAR,
            wrap_s: GL_REPEAT,
            wrap_t: GL_REPEAT,
            min_lod: -1000.0,
            max_lod: 1000.0,
            max_level: 1000,
            level_data: HashMap::new(),
            dirty: true,
            gpu_texture: None,
            gpu_view: None,
            gpu_sampler: None,
            gpu_bind_group: None,
        }
    }

    pub fn set_image_data(
        &mut self,
        level: u32,
        internal_format: GLenum,
        width: u32,
        height: u32,
        format: GLenum,
        type_: GLenum,
        data: Option<&[u8]>,
    ) {
        if level == 0 {
            self.width = width;
            self.height = height;
            self.internal_format = internal_format;
            self.format = format;
            self.type_ = type_;
        }

        let rgba_data = convert_to_rgba8(width, height, format, type_, data);
        self.level_data.insert(level, rgba_data);
        self.dirty = true;
    }

    pub fn set_sub_image_data(
        &mut self,
        level: u32,
        xoffset: u32,
        yoffset: u32,
        width: u32,
        height: u32,
        format: GLenum,
        type_: GLenum,
        data: &[u8],
    ) {
        let sub_rgba = convert_to_rgba8(width, height, format, type_, Some(data));
        let base_w = if level == 0 {
            self.width
        } else {
            (self.width >> level).max(1)
        };
        let base_h = if level == 0 {
            self.height
        } else {
            (self.height >> level).max(1)
        };

        let entry = self
            .level_data
            .entry(level)
            .or_insert_with(|| vec![0u8; (base_w * base_h * 4) as usize]);
        if entry.len() < (base_w * base_h * 4) as usize {
            entry.resize((base_w * base_h * 4) as usize, 0);
        }

        // Copy scanlines
        for row in 0..height {
            let src_start = (row * width * 4) as usize;
            let src_end = src_start + (width * 4) as usize;
            let dst_y = yoffset + row;
            if dst_y >= base_h {
                break;
            }
            let dst_start = ((dst_y * base_w + xoffset) * 4) as usize;
            let copy_w = width.min(base_w.saturating_sub(xoffset));
            let dst_end = dst_start + (copy_w * 4) as usize;

            if src_end <= sub_rgba.len() && dst_end <= entry.len() {
                entry[dst_start..dst_end]
                    .copy_from_slice(&sub_rgba[src_start..src_start + (copy_w * 4) as usize]);
            }
        }

        self.dirty = true;
    }

    pub fn get_wgpu_sampler_descriptor(&self) -> wgpu::SamplerDescriptor<'static> {
        let address_mode_u = match self.wrap_s {
            GL_CLAMP_TO_EDGE => wgpu::AddressMode::ClampToEdge,
            GL_MIRRORED_REPEAT => wgpu::AddressMode::MirrorRepeat,
            _ => wgpu::AddressMode::Repeat,
        };
        let address_mode_v = match self.wrap_t {
            GL_CLAMP_TO_EDGE => wgpu::AddressMode::ClampToEdge,
            GL_MIRRORED_REPEAT => wgpu::AddressMode::MirrorRepeat,
            _ => wgpu::AddressMode::Repeat,
        };

        let mag_filter = match self.mag_filter {
            GL_NEAREST => wgpu::FilterMode::Nearest,
            _ => wgpu::FilterMode::Linear,
        };

        let min_filter = match self.min_filter {
            GL_NEAREST | GL_NEAREST_MIPMAP_NEAREST | GL_NEAREST_MIPMAP_LINEAR => {
                wgpu::FilterMode::Nearest
            }
            _ => wgpu::FilterMode::Linear,
        };
        let mipmap_filter = match self.min_filter {
            GL_NEAREST_MIPMAP_LINEAR | GL_LINEAR_MIPMAP_LINEAR => wgpu::MipmapFilterMode::Linear,
            _ => wgpu::MipmapFilterMode::Nearest,
        };

        wgpu::SamplerDescriptor {
            label: Some("GL Texture Sampler"),
            address_mode_u,
            address_mode_v,
            address_mode_w: wgpu::AddressMode::Repeat,
            mag_filter,
            min_filter,
            mipmap_filter,
            lod_min_clamp: self.min_lod.max(0.0),
            lod_max_clamp: self.max_lod.max(0.0),
            compare: None,
            anisotropy_clamp: 1,
            border_color: None,
        }
    }

    pub fn sync_gpu(
        &mut self,
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        bind_group_layout: &wgpu::BindGroupLayout,
    ) {
        if !self.dirty && self.gpu_bind_group.is_some() {
            return;
        }

        let width = self.width.max(1);
        let height = self.height.max(1);
        let mip_count = (self.level_data.keys().max().copied().unwrap_or(0) + 1).max(1);

        let size = wgpu::Extent3d {
            width,
            height,
            depth_or_array_layers: 1,
        };

        let texture = device.create_texture(&wgpu::TextureDescriptor {
            label: Some("GL Texture2D"),
            size,
            mip_level_count: mip_count,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        });

        // Upload level data
        for (&level, data) in &self.level_data {
            let level_w = (width >> level).max(1);
            let level_h = (height >> level).max(1);
            let expected_bytes = (level_w * level_h * 4) as usize;

            if data.len() >= expected_bytes {
                queue.write_texture(
                    wgpu::TexelCopyTextureInfo {
                        texture: &texture,
                        mip_level: level,
                        origin: wgpu::Origin3d::ZERO,
                        aspect: wgpu::TextureAspect::All,
                    },
                    &data[0..expected_bytes],
                    wgpu::TexelCopyBufferLayout {
                        offset: 0,
                        bytes_per_row: Some(level_w * 4),
                        rows_per_image: Some(level_h),
                    },
                    wgpu::Extent3d {
                        width: level_w,
                        height: level_h,
                        depth_or_array_layers: 1,
                    },
                );
            }
        }

        // If level 0 was not uploaded, fill with 1x1 white
        if !self.level_data.contains_key(&0) {
            queue.write_texture(
                wgpu::TexelCopyTextureInfo {
                    texture: &texture,
                    mip_level: 0,
                    origin: wgpu::Origin3d::ZERO,
                    aspect: wgpu::TextureAspect::All,
                },
                &[255, 255, 255, 255],
                wgpu::TexelCopyBufferLayout {
                    offset: 0,
                    bytes_per_row: Some(4),
                    rows_per_image: Some(1),
                },
                wgpu::Extent3d {
                    width: 1,
                    height: 1,
                    depth_or_array_layers: 1,
                },
            );
        }

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = device.create_sampler(&self.get_wgpu_sampler_descriptor());

        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("GL Texture BindGroup"),
            layout: bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: wgpu::BindingResource::TextureView(&view),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&sampler),
                },
            ],
        });

        self.gpu_texture = Some(texture);
        self.gpu_view = Some(view);
        self.gpu_sampler = Some(sampler);
        self.gpu_bind_group = Some(bind_group);
        self.dirty = false;
    }
}

pub fn convert_to_rgba8(
    width: u32,
    height: u32,
    format: GLenum,
    type_: GLenum,
    data: Option<&[u8]>,
) -> Vec<u8> {
    let num_pixels = (width * height) as usize;
    let mut out = vec![255u8; num_pixels * 4];

    let Some(data) = data else {
        return out;
    };

    match (format, type_) {
        (GL_RGBA, GL_UNSIGNED_BYTE) => {
            let copy_len = (num_pixels * 4).min(data.len());
            out[0..copy_len].copy_from_slice(&data[0..copy_len]);
        }
        (GL_BGRA_EXT, GL_UNSIGNED_BYTE) => {
            for i in 0..num_pixels {
                let src_idx = i * 4;
                let dst_idx = i * 4;
                if src_idx + 3 < data.len() {
                    out[dst_idx + 0] = data[src_idx + 2]; // R
                    out[dst_idx + 1] = data[src_idx + 1]; // G
                    out[dst_idx + 2] = data[src_idx + 0]; // B
                    out[dst_idx + 3] = data[src_idx + 3]; // A
                }
            }
        }
        (GL_RGB, GL_UNSIGNED_BYTE) => {
            for i in 0..num_pixels {
                let src_idx = i * 3;
                let dst_idx = i * 4;
                if src_idx + 2 < data.len() {
                    out[dst_idx + 0] = data[src_idx + 0];
                    out[dst_idx + 1] = data[src_idx + 1];
                    out[dst_idx + 2] = data[src_idx + 2];
                    out[dst_idx + 3] = 255;
                }
            }
        }
        (GL_ALPHA, GL_UNSIGNED_BYTE) => {
            for i in 0..num_pixels {
                let dst_idx = i * 4;
                if i < data.len() {
                    out[dst_idx + 0] = 255;
                    out[dst_idx + 1] = 255;
                    out[dst_idx + 2] = 255;
                    out[dst_idx + 3] = data[i];
                }
            }
        }
        (GL_LUMINANCE, GL_UNSIGNED_BYTE) => {
            for i in 0..num_pixels {
                let dst_idx = i * 4;
                if i < data.len() {
                    let l = data[i];
                    out[dst_idx + 0] = l;
                    out[dst_idx + 1] = l;
                    out[dst_idx + 2] = l;
                    out[dst_idx + 3] = 255;
                }
            }
        }
        (GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE) => {
            for i in 0..num_pixels {
                let src_idx = i * 2;
                let dst_idx = i * 4;
                if src_idx + 1 < data.len() {
                    let l = data[src_idx + 0];
                    let a = data[src_idx + 1];
                    out[dst_idx + 0] = l;
                    out[dst_idx + 1] = l;
                    out[dst_idx + 2] = l;
                    out[dst_idx + 3] = a;
                }
            }
        }
        _ => {
            // Default copy if matching length
            let copy_len = (num_pixels * 4).min(data.len());
            out[0..copy_len].copy_from_slice(&data[0..copy_len]);
        }
    }

    out
}

#[derive(Debug)]
pub struct TextureManager {
    pub textures: HashMap<GLuint, TextureObject>,
    pub bound_textures: [GLuint; 8],
    pub active_unit: usize,
    pub client_active_unit: usize,
    next_id: GLuint,
    pub default_white: Option<TextureObject>,
}

impl Default for TextureManager {
    fn default() -> Self {
        Self::new()
    }
}

impl TextureManager {
    pub fn new() -> Self {
        let mut default_white = TextureObject::new(0, GL_TEXTURE_2D);
        default_white.width = 1;
        default_white.height = 1;
        default_white.level_data.insert(0, vec![255, 255, 255, 255]);

        Self {
            textures: HashMap::new(),
            bound_textures: [0; 8],
            active_unit: 0,
            client_active_unit: 0,
            next_id: 1,
            default_white: Some(default_white),
        }
    }

    pub fn gen_textures(&mut self, count: usize) -> Vec<GLuint> {
        let mut ids = Vec::with_capacity(count);
        for _ in 0..count {
            let id = self.next_id;
            self.next_id += 1;
            self.textures
                .insert(id, TextureObject::new(id, GL_TEXTURE_2D));
            ids.push(id);
        }
        ids
    }

    pub fn delete_textures(&mut self, ids: &[GLuint]) {
        for &id in ids {
            self.textures.remove(&id);
            for b in &mut self.bound_textures {
                if *b == id {
                    *b = 0;
                }
            }
        }
    }

    pub fn bind_texture(&mut self, target: GLenum, id: GLuint) {
        if id != 0 && !self.textures.contains_key(&id) {
            self.textures.insert(id, TextureObject::new(id, target));
        }
        if self.active_unit < 8 {
            self.bound_textures[self.active_unit] = id;
        }
    }

    pub fn get_current_texture(&self) -> Option<&TextureObject> {
        if self.active_unit < 8 {
            let id = self.bound_textures[self.active_unit];
            if id != 0 {
                return self.textures.get(&id);
            }
        }
        self.default_white.as_ref()
    }

    pub fn get_current_texture_mut(&mut self) -> Option<&mut TextureObject> {
        if self.active_unit < 8 {
            let id = self.bound_textures[self.active_unit];
            if id != 0 {
                return self.textures.get_mut(&id);
            }
        }
        self.default_white.as_mut()
    }
}
