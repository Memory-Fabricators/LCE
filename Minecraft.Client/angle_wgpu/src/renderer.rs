//! wgpu device, queue, surface, pipeline cache, and rendering backend.
#![allow(unused_imports, dead_code)]
use crate::display_list::VertexData;
use crate::shader::{FIXED_FUNCTION_WGSL, FixedFunctionUniforms};
use crate::texture::TextureObject;
use crate::types::*;
use std::collections::HashMap;
use std::sync::Arc;
use winit::window::Window;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct PipelineKey {
    pub topology: u32, // 0=triangles, 1=triangle_strip, 2=triangle_fan, 3=lines, 4=line_strip, 5=points
    pub cull_mode: u32, // 0=none, 1=front, 2=back
    pub front_face_ccw: bool,
    pub blend_enabled: bool,
    pub src_factor: u32,
    pub dst_factor: u32,
    pub depth_test: bool,
    pub depth_write: bool,
    pub depth_func: u32,
    pub color_mask: u8,
}

/// A single deferred draw call: uploaded data plus the state needed to
/// record it, resolved eagerly at `draw_mesh` time so `flush` only has to
/// walk a flat list and issue GPU commands (no per-draw CPU work there).
struct PendingDraw {
    key: PipelineKey,
    uniform_offset: u64,
    tex_bind_group: wgpu::BindGroup,
    vertex_offset: u64,
    vertex_bytes: u64,
    index_range: Option<(u64, u64, u32)>,
    viewport: (f32, f32, f32, f32),
    scissor: Option<(u32, u32, u32, u32)>,
}

/// Frame-ordered operation: draws between two `Clear`s (or between the
/// start of the frame and the first `Clear`) are batched into a single
/// `wgpu::RenderPass` at flush time instead of one pass per draw call.
enum FrameOp {
    Clear {
        color: Option<[f64; 4]>,
        depth: Option<f32>,
    },
    Draw(PendingDraw),
}

pub struct WgpuRenderer {
    pub instance: wgpu::Instance,
    pub adapter: wgpu::Adapter,
    pub device: wgpu::Device,
    pub queue: wgpu::Queue,
    pub surface: Option<wgpu::Surface<'static>>,
    pub surface_config: Option<wgpu::SurfaceConfiguration>,
    pub surface_format: wgpu::TextureFormat,
    pub alpha_mode: wgpu::CompositeAlphaMode,
    pub window: Option<Arc<dyn Window>>,
    pub width: u32,
    pub height: u32,

    // Offscreen / Render Target fallback
    pub offscreen_color: Option<wgpu::Texture>,
    pub offscreen_color_view: Option<wgpu::TextureView>,
    pub depth_texture: Option<wgpu::Texture>,
    pub depth_view: Option<wgpu::TextureView>,

    // Pipelines and Shaders
    pub shader_module: wgpu::ShaderModule,
    pub uniform_bind_group_layout: wgpu::BindGroupLayout,
    pub texture_bind_group_layout: wgpu::BindGroupLayout,
    pub pipeline_layout: wgpu::PipelineLayout,
    pub pipelines: HashMap<PipelineKey, wgpu::RenderPipeline>,

    pub uniform_buffer: wgpu::Buffer,
    pub uniform_bind_group: wgpu::BindGroup,
    /// Aligned per-draw byte stride into `uniform_buffer` (dynamic offset unit).
    pub uniform_stride: u64,
    /// Number of draw-sized slots `uniform_buffer` currently holds.
    pub uniform_capacity: u64,
    /// Next free slot index within the current (unsubmitted) frame.
    pub uniform_used: u64,

    /// Reused scratch vertex buffer: each `draw_mesh` call appends at
    /// `vertex_scratch_offset` instead of allocating a fresh GPU buffer.
    pub vertex_scratch: wgpu::Buffer,
    pub vertex_scratch_capacity: u64,
    pub vertex_scratch_offset: u64,
    pub index_scratch: wgpu::Buffer,
    pub index_scratch_capacity: u64,
    pub index_scratch_offset: u64,

    // Current frame state
    pub current_surface_texture: Option<wgpu::SurfaceTexture>,
    /// Ops (clears + draws) accumulated since the last `flush`, recorded
    /// into GPU commands in one pass over this list.
    frame_ops: Vec<FrameOp>,
    pub clear_color: [f64; 4],
    pub clear_depth: f32,
}

impl WgpuRenderer {
    pub async fn new_headless(width: u32, height: u32) -> Result<Self, String> {
        let instance = wgpu::Instance::new(wgpu::InstanceDescriptor::new_without_display_handle());

        let adapter = match instance
            .request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::None,
                compatible_surface: None,
                force_fallback_adapter: false,
                apply_limit_buckets: false,
            })
            .await
        {
            Ok(a) => a,
            Err(_) => instance
                .request_adapter(&wgpu::RequestAdapterOptions {
                    power_preference: wgpu::PowerPreference::LowPower,
                    compatible_surface: None,
                    force_fallback_adapter: true,
                    apply_limit_buckets: false,
                })
                .await
                .map_err(|e| format!("Failed to find suitable wgpu adapter: {e:?}"))?,
        };

        let (device, queue) = adapter
            .request_device(&wgpu::DeviceDescriptor {
                label: Some("angle_wgpu Device"),
                required_features: wgpu::Features::empty(),
                required_limits: wgpu::Limits::default(),
                memory_hints: wgpu::MemoryHints::default(),
                trace: wgpu::Trace::Off,
                experimental_features: wgpu::ExperimentalFeatures::default(),
            })
            .await
            .map_err(|e| format!("Failed to create wgpu device: {e}"))?;

        let surface_format = wgpu::TextureFormat::Bgra8Unorm;
        Self::init_with_device(
            instance,
            adapter,
            device,
            queue,
            None,
            None,
            surface_format,
            wgpu::CompositeAlphaMode::Opaque,
            width,
            height,
        )
    }

    pub fn new_with_surface(
        instance: wgpu::Instance,
        adapter: wgpu::Adapter,
        device: wgpu::Device,
        queue: wgpu::Queue,
        surface: wgpu::Surface<'static>,
        window: Arc<dyn Window>,
        width: u32,
        height: u32,
    ) -> Result<Self, String> {
        let caps = surface.get_capabilities(&adapter);
        let surface_format = caps
            .formats
            .iter()
            .copied()
            .find(|f| {
                matches!(
                    f,
                    wgpu::TextureFormat::Bgra8Unorm
                        | wgpu::TextureFormat::Rgba8Unorm
                        | wgpu::TextureFormat::Bgra8UnormSrgb
                        | wgpu::TextureFormat::Rgba8UnormSrgb
                )
            })
            .unwrap_or(caps.formats[0]);
        let alpha_mode = if caps.alpha_modes.contains(&wgpu::CompositeAlphaMode::Opaque) {
            wgpu::CompositeAlphaMode::Opaque
        } else {
            caps.alpha_modes
                .first()
                .copied()
                .unwrap_or(wgpu::CompositeAlphaMode::Opaque)
        };
        eprintln!(
            "[angle_wgpu] adapter={:?} format={surface_format:?} alpha={alpha_mode:?} modes={:?}",
            adapter.get_info(),
            caps.alpha_modes
        );

        Self::init_with_device(
            instance,
            adapter,
            device,
            queue,
            Some(surface),
            Some(window),
            surface_format,
            alpha_mode,
            width,
            height,
        )
    }

    fn init_with_device(
        instance: wgpu::Instance,
        adapter: wgpu::Adapter,
        device: wgpu::Device,
        queue: wgpu::Queue,
        surface: Option<wgpu::Surface<'static>>,
        window: Option<Arc<dyn Window>>,
        surface_format: wgpu::TextureFormat,
        alpha_mode: wgpu::CompositeAlphaMode,
        width: u32,
        height: u32,
    ) -> Result<Self, String> {
        let w = width.max(1);
        let h = height.max(1);

        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("angle_wgpu FixedFunction WGSL"),
            source: wgpu::ShaderSource::Wgsl(FIXED_FUNCTION_WGSL.into()),
        });

        let uniform_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: Some("angle_wgpu Uniform Layout"),
                entries: &[wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::VERTEX_FRAGMENT,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: true,
                        min_binding_size: std::num::NonZeroU64::new(std::mem::size_of::<
                            FixedFunctionUniforms,
                        >()
                            as u64),
                    },
                    count: None,
                }],
            });

        let texture_bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: Some("angle_wgpu Texture Layout"),
                entries: &[
                    wgpu::BindGroupLayoutEntry {
                        binding: 0,
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Texture {
                            sample_type: wgpu::TextureSampleType::Float { filterable: true },
                            view_dimension: wgpu::TextureViewDimension::D2,
                            multisampled: false,
                        },
                        count: None,
                    },
                    wgpu::BindGroupLayoutEntry {
                        binding: 1,
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                        count: None,
                    },
                ],
            });

        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("angle_wgpu Pipeline Layout"),
            bind_group_layouts: &[
                Some(&uniform_bind_group_layout),
                Some(&texture_bind_group_layout),
            ],
            immediate_size: 0,
        });

        // Dynamic-offset uniform pool: one big buffer sliced per draw call
        // instead of a single slot rewritten (and mis-synchronized, since
        // draws within a frame share one un-submitted command encoder)
        // every draw.
        let uniform_align = device.limits().min_uniform_buffer_offset_alignment as u64;
        let uniform_struct_size = std::mem::size_of::<FixedFunctionUniforms>() as u64;
        let uniform_stride = uniform_struct_size.div_ceil(uniform_align) * uniform_align;
        let uniform_capacity: u64 = 4096;
        let uniform_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("angle_wgpu Uniform Buffer Pool"),
            size: uniform_stride * uniform_capacity,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let uniform_bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("angle_wgpu Uniform BindGroup"),
            layout: &uniform_bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: wgpu::BindingResource::Buffer(wgpu::BufferBinding {
                    buffer: &uniform_buffer,
                    offset: 0,
                    size: std::num::NonZeroU64::new(uniform_struct_size),
                }),
            }],
        });

        // Vertex/index scratch pools: reused across draw calls within a
        // frame instead of allocating a fresh GPU buffer per draw (was the
        // dominant per-frame cost - every chunk face layer allocated two
        // buffers). Grown (doubled) on demand; reset once submitted.
        let vertex_scratch_capacity: u64 = 4 * 1024 * 1024;
        let vertex_scratch = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("angle_wgpu Vertex Scratch Pool"),
            size: vertex_scratch_capacity,
            usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        let index_scratch_capacity: u64 = 1024 * 1024;
        let index_scratch = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("angle_wgpu Index Scratch Pool"),
            size: index_scratch_capacity,
            usage: wgpu::BufferUsages::INDEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let mut renderer = Self {
            instance,
            adapter,
            device,
            queue,
            surface,
            surface_config: None,
            surface_format,
            alpha_mode,
            window,
            width: w,
            height: h,
            offscreen_color: None,
            offscreen_color_view: None,
            depth_texture: None,
            depth_view: None,
            shader_module,
            uniform_bind_group_layout,
            texture_bind_group_layout,
            pipeline_layout,
            pipelines: HashMap::new(),
            uniform_buffer,
            uniform_bind_group,
            uniform_stride,
            uniform_capacity,
            uniform_used: 0,
            vertex_scratch,
            vertex_scratch_capacity,
            vertex_scratch_offset: 0,
            index_scratch,
            index_scratch_capacity,
            index_scratch_offset: 0,
            current_surface_texture: None,
            frame_ops: Vec::new(),
            clear_color: [0.4, 0.6, 0.9, 1.0],
            clear_depth: 1.0,
        };

        renderer.resize(w, h);
        Ok(renderer)
    }

    pub fn resize(&mut self, width: u32, height: u32) {
        let w = width.max(1);
        let h = height.max(1);
        self.width = w;
        self.height = h;

        // The surface's current texture (if any) must be dropped before
        // reconfiguring, or wgpu will panic in `Surface::configure`.
        self.current_surface_texture = None;

        if let Some(surface) = &self.surface {
            let config = wgpu::SurfaceConfiguration {
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
                format: self.surface_format,
                width: w,
                height: h,
                present_mode: wgpu::PresentMode::Fifo,
                desired_maximum_frame_latency: 2,
                alpha_mode: self.alpha_mode,
                view_formats: vec![],
                color_space: wgpu::SurfaceColorSpace::Auto,
            };
            surface.configure(&self.device, &config);
            self.surface_config = Some(config);
        } else {
            // Offscreen render target
            let color_tex = self.device.create_texture(&wgpu::TextureDescriptor {
                label: Some("angle_wgpu Offscreen Color Target"),
                size: wgpu::Extent3d {
                    width: w,
                    height: h,
                    depth_or_array_layers: 1,
                },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: self.surface_format,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT
                    | wgpu::TextureUsages::COPY_SRC
                    | wgpu::TextureUsages::TEXTURE_BINDING,
                view_formats: &[],
            });
            self.offscreen_color_view =
                Some(color_tex.create_view(&wgpu::TextureViewDescriptor::default()));
            self.offscreen_color = Some(color_tex);
        }

        // Depth buffer
        let depth_tex = self.device.create_texture(&wgpu::TextureDescriptor {
            label: Some("angle_wgpu Depth Buffer"),
            size: wgpu::Extent3d {
                width: w,
                height: h,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Depth24PlusStencil8,
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            view_formats: &[],
        });
        self.depth_view = Some(depth_tex.create_view(&wgpu::TextureViewDescriptor::default()));
        self.depth_texture = Some(depth_tex);
    }

    pub fn get_or_create_pipeline(&mut self, key: &PipelineKey) -> &wgpu::RenderPipeline {
        if !self.pipelines.contains_key(key) {
            let topology = match key.topology {
                1 => wgpu::PrimitiveTopology::TriangleStrip,
                3 => wgpu::PrimitiveTopology::LineList,
                4 => wgpu::PrimitiveTopology::LineStrip,
                5 => wgpu::PrimitiveTopology::PointList,
                _ => wgpu::PrimitiveTopology::TriangleList,
            };

            let cull_mode = match key.cull_mode {
                1 => Some(wgpu::Face::Front),
                2 => Some(wgpu::Face::Back),
                _ => None,
            };

            let front_face = if key.front_face_ccw {
                wgpu::FrontFace::Ccw
            } else {
                wgpu::FrontFace::Cw
            };

            let blend = if key.blend_enabled {
                let to_wgpu_factor = |f: u32| match f {
                    GL_ZERO => wgpu::BlendFactor::Zero,
                    GL_ONE => wgpu::BlendFactor::One,
                    GL_SRC_COLOR => wgpu::BlendFactor::Src,
                    GL_ONE_MINUS_SRC_COLOR => wgpu::BlendFactor::OneMinusSrc,
                    GL_SRC_ALPHA => wgpu::BlendFactor::SrcAlpha,
                    GL_ONE_MINUS_SRC_ALPHA => wgpu::BlendFactor::OneMinusSrcAlpha,
                    GL_DST_ALPHA => wgpu::BlendFactor::DstAlpha,
                    GL_ONE_MINUS_DST_ALPHA => wgpu::BlendFactor::OneMinusDstAlpha,
                    GL_DST_COLOR => wgpu::BlendFactor::Dst,
                    GL_ONE_MINUS_DST_COLOR => wgpu::BlendFactor::OneMinusDst,
                    GL_SRC_ALPHA_SATURATE => wgpu::BlendFactor::SrcAlphaSaturated,
                    _ => wgpu::BlendFactor::One,
                };

                let src = to_wgpu_factor(key.src_factor);
                let dst = to_wgpu_factor(key.dst_factor);

                Some(wgpu::BlendState {
                    color: wgpu::BlendComponent {
                        src_factor: src,
                        dst_factor: dst,
                        operation: wgpu::BlendOperation::Add,
                    },
                    alpha: wgpu::BlendComponent {
                        src_factor: src,
                        dst_factor: dst,
                        operation: wgpu::BlendOperation::Add,
                    },
                })
            } else {
                Some(wgpu::BlendState::REPLACE)
            };

            let to_wgpu_compare = |func: u32| match func {
                GL_NEVER => wgpu::CompareFunction::Never,
                GL_LESS => wgpu::CompareFunction::Less,
                GL_EQUAL => wgpu::CompareFunction::Equal,
                GL_LEQUAL => wgpu::CompareFunction::LessEqual,
                GL_GREATER => wgpu::CompareFunction::Greater,
                GL_NOTEQUAL => wgpu::CompareFunction::NotEqual,
                GL_GEQUAL => wgpu::CompareFunction::GreaterEqual,
                GL_ALWAYS => wgpu::CompareFunction::Always,
                _ => wgpu::CompareFunction::LessEqual,
            };

            let depth_stencil = Some(wgpu::DepthStencilState {
                format: wgpu::TextureFormat::Depth24PlusStencil8,
                depth_write_enabled: Some(key.depth_write),
                depth_compare: if key.depth_test {
                    Some(to_wgpu_compare(key.depth_func))
                } else {
                    Some(wgpu::CompareFunction::Always)
                },
                stencil: wgpu::StencilState::default(),
                bias: wgpu::DepthBiasState::default(),
            });

            let mut write_mask = wgpu::ColorWrites::empty();
            if key.color_mask & 1 != 0 {
                write_mask |= wgpu::ColorWrites::RED;
            }
            if key.color_mask & 2 != 0 {
                write_mask |= wgpu::ColorWrites::GREEN;
            }
            if key.color_mask & 4 != 0 {
                write_mask |= wgpu::ColorWrites::BLUE;
            }
            write_mask |= wgpu::ColorWrites::ALPHA;

            let pipeline = self
                .device
                .create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                    label: Some("angle_wgpu RenderPipeline"),
                    layout: Some(&self.pipeline_layout),
                    vertex: wgpu::VertexState {
                        module: &self.shader_module,
                        entry_point: Some("vs_main"),
                        compilation_options: Default::default(),
                        buffers: &[Some(wgpu::VertexBufferLayout {
                            array_stride: std::mem::size_of::<VertexData>() as u64,
                            step_mode: wgpu::VertexStepMode::Vertex,
                            attributes: &[
                                wgpu::VertexAttribute {
                                    offset: 0,
                                    shader_location: 0,
                                    format: wgpu::VertexFormat::Float32x3,
                                },
                                wgpu::VertexAttribute {
                                    offset: 12,
                                    shader_location: 1,
                                    format: wgpu::VertexFormat::Float32x2,
                                },
                                wgpu::VertexAttribute {
                                    offset: 20,
                                    shader_location: 2,
                                    format: wgpu::VertexFormat::Float32x4,
                                },
                                wgpu::VertexAttribute {
                                    offset: 36,
                                    shader_location: 3,
                                    format: wgpu::VertexFormat::Float32x3,
                                },
                            ],
                        })],
                    },
                    fragment: Some(wgpu::FragmentState {
                        module: &self.shader_module,
                        entry_point: Some("fs_main"),
                        compilation_options: Default::default(),
                        targets: &[Some(wgpu::ColorTargetState {
                            format: self.surface_format,
                            blend,
                            write_mask,
                        })],
                    }),
                    primitive: wgpu::PrimitiveState {
                        topology,
                        strip_index_format: None,
                        front_face,
                        cull_mode,
                        unclipped_depth: false,
                        polygon_mode: wgpu::PolygonMode::Fill,
                        conservative: false,
                    },
                    depth_stencil,
                    multisample: wgpu::MultisampleState::default(),
                    multiview_mask: None,
                    cache: None,
                });

            self.pipelines.insert(*key, pipeline);
        }

        self.pipelines.get(key).unwrap()
    }

    pub fn ensure_frame_target(&mut self) -> Result<wgpu::TextureView, String> {
        if self.surface.is_some() {
            if self.current_surface_texture.is_none() {
                let res = self.surface.as_ref().unwrap().get_current_texture();
                match res {
                    wgpu::CurrentSurfaceTexture::Success(tex)
                    | wgpu::CurrentSurfaceTexture::Suboptimal(tex) => {
                        self.current_surface_texture = Some(tex);
                    }
                    wgpu::CurrentSurfaceTexture::Lost | wgpu::CurrentSurfaceTexture::Outdated => {
                        self.resize(self.width, self.height);
                        let res2 = self.surface.as_ref().unwrap().get_current_texture();
                        if let wgpu::CurrentSurfaceTexture::Success(tex)
                        | wgpu::CurrentSurfaceTexture::Suboptimal(tex) = res2
                        {
                            self.current_surface_texture = Some(tex);
                        } else {
                            return Err(
                                "Failed to acquire surface texture after resize".to_string()
                            );
                        }
                    }
                    _ => return Err("Failed to acquire surface texture".to_string()),
                }
            }
            let tex = self.current_surface_texture.as_ref().unwrap();
            Ok(tex
                .texture
                .create_view(&wgpu::TextureViewDescriptor::default()))
        } else if let Some(view) = &self.offscreen_color_view {
            Ok(view.clone())
        } else {
            Err("No render target available".to_string())
        }
    }

    pub fn clear(&mut self, clear_color: bool, clear_depth: bool, _clear_stencil: bool) {
        if !clear_color && !clear_depth {
            return;
        }
        if self.ensure_frame_target().is_err() {
            return;
        }
        self.frame_ops.push(FrameOp::Clear {
            color: clear_color.then_some(self.clear_color),
            depth: clear_depth.then_some(self.clear_depth),
        });
    }

    fn grow_uniform_pool(&mut self, needed_slots: u64) {
        let mut new_cap = self.uniform_capacity.max(1);
        while new_cap < needed_slots {
            new_cap *= 2;
        }
        self.uniform_buffer = self.device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("angle_wgpu Uniform Buffer Pool"),
            size: self.uniform_stride * new_cap,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        self.uniform_bind_group = self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("angle_wgpu Uniform BindGroup"),
            layout: &self.uniform_bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: wgpu::BindingResource::Buffer(wgpu::BufferBinding {
                    buffer: &self.uniform_buffer,
                    offset: 0,
                    size: std::num::NonZeroU64::new(
                        std::mem::size_of::<FixedFunctionUniforms>() as u64
                    ),
                }),
            }],
        });
        self.uniform_capacity = new_cap;
        self.uniform_used = 0;
    }

    fn grow_vertex_scratch(&mut self, needed_bytes: u64) {
        let mut new_cap = self.vertex_scratch_capacity.max(1);
        while new_cap < needed_bytes {
            new_cap *= 2;
        }
        self.vertex_scratch = self.device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("angle_wgpu Vertex Scratch Pool"),
            size: new_cap,
            usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        self.vertex_scratch_capacity = new_cap;
        self.vertex_scratch_offset = 0;
    }

    fn grow_index_scratch(&mut self, needed_bytes: u64) {
        let mut new_cap = self.index_scratch_capacity.max(1);
        while new_cap < needed_bytes {
            new_cap *= 2;
        }
        self.index_scratch = self.device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("angle_wgpu Index Scratch Pool"),
            size: new_cap,
            usage: wgpu::BufferUsages::INDEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        self.index_scratch_capacity = new_cap;
        self.index_scratch_offset = 0;
    }

    pub fn draw_mesh(
        &mut self,
        key: &PipelineKey,
        uniforms: &FixedFunctionUniforms,
        texture: &mut TextureObject,
        vertices: &[VertexData],
        indices: Option<&[u32]>,
        viewport: (u32, u32, u32, u32),
        scissor: Option<(u32, u32, u32, u32)>,
    ) {
        if vertices.is_empty() {
            return;
        }

        // Reserve a uniform slot for this draw (dynamic offset into a
        // shared pool) instead of a single buffer every draw call in the
        // frame overwrites before any of them are actually submitted.
        if self.uniform_used >= self.uniform_capacity {
            self.grow_uniform_pool(self.uniform_used + 1);
        }
        let uniform_offset = self.uniform_used * self.uniform_stride;
        self.queue.write_buffer(
            &self.uniform_buffer,
            uniform_offset,
            bytemuck::bytes_of(uniforms),
        );
        self.uniform_used += 1;

        // Sync texture bind group
        texture.sync_gpu(&self.device, &self.queue, &self.texture_bind_group_layout);
        let Some(tex_bind_group) = &texture.gpu_bind_group else {
            return;
        };

        // Append into the reused vertex/index scratch pools instead of
        // allocating a fresh GPU buffer per draw call (was the dominant
        // per-frame cost: every chunk face layer allocated two buffers).
        let vertex_bytes = (vertices.len() * std::mem::size_of::<VertexData>()) as u64;
        if self.vertex_scratch_offset + vertex_bytes > self.vertex_scratch_capacity {
            self.grow_vertex_scratch(self.vertex_scratch_offset + vertex_bytes);
        }
        let vertex_offset = self.vertex_scratch_offset;
        self.queue.write_buffer(
            &self.vertex_scratch,
            vertex_offset,
            bytemuck::cast_slice(vertices),
        );
        self.vertex_scratch_offset += vertex_bytes;

        let index_range = indices.map(|idx| {
            let index_bytes = (idx.len() * std::mem::size_of::<u32>()) as u64;
            if self.index_scratch_offset + index_bytes > self.index_scratch_capacity {
                self.grow_index_scratch(self.index_scratch_offset + index_bytes);
            }
            let offset = self.index_scratch_offset;
            self.queue
                .write_buffer(&self.index_scratch, offset, bytemuck::cast_slice(idx));
            self.index_scratch_offset += index_bytes;
            (offset, offset + index_bytes, idx.len() as u32)
        });

        if self.ensure_frame_target().is_err() {
            return;
        }
        self.get_or_create_pipeline(key);

        let (vx, vy, vw, vh) = viewport;
        let (target_w, target_h) = (self.width.max(1), self.height.max(1));
        let safe_vw = vw.min(target_w.saturating_sub(vx)).max(1);
        let safe_vh = vh.min(target_h.saturating_sub(vy)).max(1);
        let resolved_scissor = scissor.map(|(sx, sy, sw, sh)| {
            let safe_sw = sw.min(target_w.saturating_sub(sx)).max(1);
            let safe_sh = sh.min(target_h.saturating_sub(sy)).max(1);
            (sx, sy, safe_sw, safe_sh)
        });

        self.frame_ops.push(FrameOp::Draw(PendingDraw {
            key: *key,
            uniform_offset,
            tex_bind_group: tex_bind_group.clone(),
            vertex_offset,
            vertex_bytes,
            index_range,
            viewport: (vx as f32, vy as f32, safe_vw as f32, safe_vh as f32),
            scissor: resolved_scissor,
        }));
    }

    /// Record every accumulated `FrameOp` into GPU commands and submit.
    /// Consecutive draws between `Clear`s share one `wgpu::RenderPass`
    /// instead of paying a begin/end pass per draw call, which is the
    /// dominant per-frame cost once chunk counts get into the thousands.
    pub fn flush(&mut self) {
        if self.frame_ops.is_empty() {
            return;
        }
        let Ok(color_view) = self.ensure_frame_target() else {
            self.frame_ops.clear();
            return;
        };
        let Some(depth_view) = self.depth_view.clone() else {
            self.frame_ops.clear();
            return;
        };

        let mut encoder = self
            .device
            .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Frame Encoder"),
            });

        let ops = std::mem::take(&mut self.frame_ops);
        let mut iter = ops.into_iter().peekable();
        while iter.peek().is_some() {
            let (clear_color, clear_depth) = match iter.peek() {
                Some(FrameOp::Clear { .. }) => match iter.next() {
                    Some(FrameOp::Clear { color, depth }) => (color, depth),
                    _ => unreachable!(),
                },
                _ => (None, None),
            };

            let mut draws: Vec<PendingDraw> = Vec::new();
            while let Some(FrameOp::Draw(_)) = iter.peek() {
                if let Some(FrameOp::Draw(d)) = iter.next() {
                    draws.push(d);
                }
            }

            let color_ops = match clear_color {
                Some(c) => wgpu::Operations {
                    load: wgpu::LoadOp::Clear(wgpu::Color {
                        r: c[0],
                        g: c[1],
                        b: c[2],
                        a: 1.0,
                    }),
                    store: wgpu::StoreOp::Store,
                },
                None => wgpu::Operations {
                    load: wgpu::LoadOp::Load,
                    store: wgpu::StoreOp::Store,
                },
            };
            let depth_ops = match clear_depth {
                Some(d) => wgpu::Operations {
                    load: wgpu::LoadOp::Clear(d),
                    store: wgpu::StoreOp::Store,
                },
                None => wgpu::Operations {
                    load: wgpu::LoadOp::Load,
                    store: wgpu::StoreOp::Store,
                },
            };

            let mut pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Frame Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &color_view,
                    resolve_target: None,
                    depth_slice: None,
                    ops: color_ops,
                })],
                depth_stencil_attachment: Some(wgpu::RenderPassDepthStencilAttachment {
                    view: &depth_view,
                    depth_ops: Some(depth_ops),
                    stencil_ops: None,
                }),
                timestamp_writes: None,
                occlusion_query_set: None,
                multiview_mask: None,
            });

            for d in &draws {
                let Some(pipeline) = self.pipelines.get(&d.key) else {
                    continue;
                };
                pass.set_pipeline(pipeline);
                pass.set_bind_group(0, &self.uniform_bind_group, &[d.uniform_offset as u32]);
                pass.set_bind_group(1, &d.tex_bind_group, &[]);

                let (vx, vy, vw, vh) = d.viewport;
                pass.set_viewport(vx, vy, vw, vh, 0.0, 1.0);
                if let Some((sx, sy, sw, sh)) = d.scissor {
                    pass.set_scissor_rect(sx, sy, sw, sh);
                }

                pass.set_vertex_buffer(
                    0,
                    self.vertex_scratch
                        .slice(d.vertex_offset..d.vertex_offset + d.vertex_bytes),
                );

                if let Some((start, end, count)) = d.index_range {
                    pass.set_index_buffer(
                        self.index_scratch.slice(start..end),
                        wgpu::IndexFormat::Uint32,
                    );
                    pass.draw_indexed(0..count, 0, 0..1);
                } else {
                    let vertex_count =
                        (d.vertex_bytes / std::mem::size_of::<VertexData>() as u64) as u32;
                    pass.draw(0..vertex_count, 0..1);
                }
            }
        }

        self.queue.submit(std::iter::once(encoder.finish()));
        // Safe to reuse immediately: wgpu queue operations (writes and
        // submits) execute in issue order on the same queue, so the next
        // frame's `write_buffer` calls at these same offsets are
        // guaranteed to happen only after this submission's GPU reads of
        // them have been scheduled/completed in order.
        self.vertex_scratch_offset = 0;
        self.index_scratch_offset = 0;
        self.uniform_used = 0;
    }

    pub fn swap_buffers(&mut self) -> Result<(), String> {
        if self.current_surface_texture.is_none() && self.surface.is_some() {
            self.clear(true, true, false);
        }
        self.flush();
        if let Some(window) = &self.window {
            window.pre_present_notify();
        }
        if let Some(frame) = self.current_surface_texture.take() {
            self.queue.present(frame);
        }
        {
            static FRAME_COUNT: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);
            static LAST_LOG: std::sync::Mutex<Option<std::time::Instant>> =
                std::sync::Mutex::new(None);
            let n = FRAME_COUNT.fetch_add(1, std::sync::atomic::Ordering::Relaxed) + 1;
            if n % 60 == 0 {
                let mut last = LAST_LOG.lock().unwrap();
                let now = std::time::Instant::now();
                if let Some(prev) = *last {
                    let dt = now.duration_since(prev).as_secs_f64();
                    let fps = 60.0 / dt;
                    eprintln!("[angle_wgpu] FPS over last 60 frames: {fps:.2}");
                }
                *last = Some(now);
            }
        }
        if let Some(window) = &self.window {
            window.request_redraw();
        }
        Ok(())
    }
}
