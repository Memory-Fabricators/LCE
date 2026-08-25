//! Core OpenGL Context and State Machine.
#![allow(unused_imports, dead_code, unused_mut)]
use crate::display_list::{DisplayList, DisplayListOp, DisplayListRegistry, VertexData};
use crate::matrix::{Mat4, MatrixMode, MatrixStack};
use crate::renderer::{PipelineKey, WgpuRenderer};
use crate::shader::FixedFunctionUniforms;
use crate::texture::{TextureManager, TextureObject};
use crate::types::*;
use parking_lot::Mutex;
use std::ffi::c_void;
use std::sync::Arc;

#[derive(Debug, Clone)]
pub struct LightState {
    pub position: [f32; 4],
    pub ambient: [f32; 4],
    pub diffuse: [f32; 4],
    pub specular: [f32; 4],
    pub spot_direction: [f32; 3],
    pub spot_exponent: f32,
    pub spot_cutoff: f32,
    pub constant_attenuation: f32,
    pub linear_attenuation: f32,
    pub quadratic_attenuation: f32,
}

impl Default for LightState {
    fn default() -> Self {
        Self {
            position: [0.0, 0.0, 1.0, 0.0],
            ambient: [0.0, 0.0, 0.0, 1.0],
            diffuse: [1.0, 1.0, 1.0, 1.0],
            specular: [1.0, 1.0, 1.0, 1.0],
            spot_direction: [0.0, 0.0, -1.0],
            spot_exponent: 0.0,
            spot_cutoff: 180.0,
            constant_attenuation: 1.0,
            linear_attenuation: 0.0,
            quadratic_attenuation: 0.0,
        }
    }
}

pub struct GlContext {
    pub id: u32,
    pub renderer: Option<Arc<Mutex<WgpuRenderer>>>,
    pub texture_manager: Arc<Mutex<TextureManager>>,
    pub display_lists: Arc<DisplayListRegistry>,

    // Matrix state
    pub matrix_mode: MatrixMode,
    pub modelview_stack: MatrixStack,
    pub projection_stack: MatrixStack,
    pub texture_stack: MatrixStack,

    // Current vertex attributes
    pub current_color: [f32; 4],
    pub current_normal: [f32; 3],
    pub current_texcoord: [f32; 2],

    // Client vertex arrays
    pub vertex_array_enabled: bool,
    pub vertex_pointer_size: GLint,
    pub vertex_pointer_type: GLenum,
    pub vertex_pointer_stride: GLsizei,
    pub vertex_pointer: *const c_void,

    pub texcoord_array_enabled: bool,
    pub texcoord_pointer_size: GLint,
    pub texcoord_pointer_type: GLenum,
    pub texcoord_pointer_stride: GLsizei,
    pub texcoord_pointer: *const c_void,

    pub color_array_enabled: bool,
    pub color_pointer_size: GLint,
    pub color_pointer_type: GLenum,
    pub color_pointer_stride: GLsizei,
    pub color_pointer: *const c_void,

    pub normal_array_enabled: bool,
    pub normal_pointer_type: GLenum,
    pub normal_pointer_stride: GLsizei,
    pub normal_pointer: *const c_void,

    // Server enables
    pub texture_2d_enabled: bool,
    pub blend_enabled: bool,
    pub depth_test_enabled: bool,
    pub alpha_test_enabled: bool,
    pub cull_face_enabled: bool,
    pub cull_face_mode: GLenum,
    pub front_face: GLenum,
    pub lighting_enabled: bool,
    pub lights_enabled: [bool; 8],
    pub color_material_enabled: bool,
    pub rescale_normal_enabled: bool,
    pub normalize_enabled: bool,
    pub fog_enabled: bool,
    pub scissor_test_enabled: bool,
    pub stencil_test_enabled: bool,
    pub polygon_offset_fill_enabled: bool,

    // Blend state
    pub src_factor: GLenum,
    pub dst_factor: GLenum,
    pub src_factor_alpha: GLenum,
    pub dst_factor_alpha: GLenum,
    pub blend_color: [f32; 4],

    // Depth state
    pub depth_func: GLenum,
    pub depth_mask: bool,

    // Alpha test state
    pub alpha_func: GLenum,
    pub alpha_ref: f32,

    // Fog state
    pub fog_mode: GLenum,
    pub fog_start: f32,
    pub fog_end: f32,
    pub fog_density: f32,
    pub fog_color: [f32; 4],

    // Lighting state
    pub lights: [LightState; 8],
    pub light_model_ambient: [f32; 4],

    // Viewport & Scissor
    pub viewport: (i32, i32, i32, i32),
    pub scissor: (i32, i32, i32, i32),

    // Clear values
    pub clear_color: [f32; 4],
    pub clear_depth: f32,
    pub clear_stencil: i32,

    // Color Mask
    pub color_mask: (bool, bool, bool, bool),

    // Polygon Offset
    pub polygon_offset_factor: f32,
    pub polygon_offset_units: f32,

    pub line_width: f32,
    pub point_size: f32,
    pub shade_model: GLenum,

    // Stencil
    pub stencil_func: GLenum,
    pub stencil_ref: i32,
    pub stencil_value_mask: u32,
    pub stencil_fail: GLenum,
    pub stencil_zfail: GLenum,
    pub stencil_zpass: GLenum,
    pub stencil_writemask: u32,

    // Immediate Mode (glBegin/glEnd)
    pub immediate_mode: Option<GLenum>,
    pub immediate_vertices: Vec<VertexData>,

    // Display List recording
    pub active_display_list: Option<DisplayList>,
    pub display_list_mode: GLenum,

    pub error: GLenum,
}

unsafe impl Send for GlContext {}
unsafe impl Sync for GlContext {}

impl GlContext {
    pub fn new(
        id: u32,
        renderer: Option<Arc<Mutex<WgpuRenderer>>>,
        texture_manager: Arc<Mutex<TextureManager>>,
        display_lists: Arc<DisplayListRegistry>,
    ) -> Self {
        Self {
            id,
            renderer,
            texture_manager,
            display_lists,
            matrix_mode: MatrixMode::ModelView,
            modelview_stack: MatrixStack::new(32),
            projection_stack: MatrixStack::new(32),
            texture_stack: MatrixStack::new(32),
            current_color: [1.0, 1.0, 1.0, 1.0],
            current_normal: [0.0, 0.0, 1.0],
            current_texcoord: [0.0, 0.0],
            vertex_array_enabled: false,
            vertex_pointer_size: 3,
            vertex_pointer_type: GL_FLOAT,
            vertex_pointer_stride: 0,
            vertex_pointer: std::ptr::null(),
            texcoord_array_enabled: false,
            texcoord_pointer_size: 2,
            texcoord_pointer_type: GL_FLOAT,
            texcoord_pointer_stride: 0,
            texcoord_pointer: std::ptr::null(),
            color_array_enabled: false,
            color_pointer_size: 4,
            color_pointer_type: GL_FLOAT,
            color_pointer_stride: 0,
            color_pointer: std::ptr::null(),
            normal_array_enabled: false,
            normal_pointer_type: GL_FLOAT,
            normal_pointer_stride: 0,
            normal_pointer: std::ptr::null(),
            texture_2d_enabled: false,
            blend_enabled: false,
            depth_test_enabled: false,
            alpha_test_enabled: false,
            cull_face_enabled: false,
            cull_face_mode: GL_BACK,
            front_face: GL_CCW,
            lighting_enabled: false,
            lights_enabled: [false; 8],
            color_material_enabled: false,
            rescale_normal_enabled: false,
            normalize_enabled: false,
            fog_enabled: false,
            scissor_test_enabled: false,
            stencil_test_enabled: false,
            polygon_offset_fill_enabled: false,
            src_factor: GL_SRC_ALPHA,
            dst_factor: GL_ONE_MINUS_SRC_ALPHA,
            src_factor_alpha: GL_SRC_ALPHA,
            dst_factor_alpha: GL_ONE_MINUS_SRC_ALPHA,
            blend_color: [0.0, 0.0, 0.0, 0.0],
            depth_func: GL_LEQUAL,
            depth_mask: true,
            alpha_func: GL_ALWAYS,
            alpha_ref: 0.0,
            fog_mode: GL_LINEAR,
            fog_start: 0.0,
            fog_end: 1.0,
            fog_density: 1.0,
            fog_color: [0.0, 0.0, 0.0, 0.0],
            lights: [
                LightState::default(),
                LightState::default(),
                LightState::default(),
                LightState::default(),
                LightState::default(),
                LightState::default(),
                LightState::default(),
                LightState::default(),
            ],
            light_model_ambient: [0.2, 0.2, 0.2, 1.0],
            viewport: (0, 0, 1280, 720),
            scissor: (0, 0, 1280, 720),
            clear_color: [0.4, 0.6, 0.9, 1.0],
            clear_depth: 1.0,
            clear_stencil: 0,
            color_mask: (true, true, true, true),
            polygon_offset_factor: 0.0,
            polygon_offset_units: 0.0,
            line_width: 1.0,
            point_size: 1.0,
            shade_model: GL_SMOOTH,
            stencil_func: GL_ALWAYS,
            stencil_ref: 0,
            stencil_value_mask: 0xFF,
            stencil_fail: GL_KEEP,
            stencil_zfail: GL_KEEP,
            stencil_zpass: GL_KEEP,
            stencil_writemask: 0xFF,
            immediate_mode: None,
            immediate_vertices: Vec::new(),
            active_display_list: None,
            display_list_mode: GL_COMPILE,
            error: GL_NO_ERROR,
        }
    }

    #[inline]
    pub fn current_matrix_stack(&mut self) -> &mut MatrixStack {
        match self.matrix_mode {
            MatrixMode::ModelView => &mut self.modelview_stack,
            MatrixMode::Projection => &mut self.projection_stack,
            MatrixMode::Texture => &mut self.texture_stack,
        }
    }

    #[inline]
    pub fn current_matrix_stack_ref(&self) -> &MatrixStack {
        match self.matrix_mode {
            MatrixMode::ModelView => &self.modelview_stack,
            MatrixMode::Projection => &self.projection_stack,
            MatrixMode::Texture => &self.texture_stack,
        }
    }

    pub fn build_uniforms(&self) -> FixedFunctionUniforms {
        let mv = self.modelview_stack.current;
        let proj = self.projection_stack.current;
        let tex_m = self.texture_stack.current;

        let norm_m = mv.normal_matrix_3x3();
        let normal_mat4 = [
            norm_m[0], norm_m[1], norm_m[2], 0.0, norm_m[3], norm_m[4], norm_m[5], 0.0, norm_m[6],
            norm_m[7], norm_m[8], 0.0, 0.0, 0.0, 0.0, 1.0,
        ];

        let alpha_test_code = if self.alpha_test_enabled {
            match self.alpha_func {
                GL_NEVER => 0,
                GL_LESS => 1,
                GL_EQUAL => 2,
                GL_LEQUAL => 3,
                GL_GREATER => 4,
                GL_NOTEQUAL => 5,
                GL_GEQUAL => 6,
                GL_ALWAYS => 7,
                _ => 7,
            }
        } else {
            7 // GL_ALWAYS
        };

        let fog_mode_code = if self.fog_enabled {
            match self.fog_mode {
                GL_LINEAR => 1.0,
                GL_EXP => 2.0,
                GL_EXP2 => 3.0,
                _ => 1.0,
            }
        } else {
            0.0
        };

        FixedFunctionUniforms {
            model_view: mv.to_array(),
            projection: proj.to_array(),
            texture_matrix: tex_m.to_array(),
            normal_matrix: normal_mat4,
            color: self.current_color,
            fog_color: self.fog_color,
            fog_params: [
                self.fog_start,
                self.fog_end,
                self.fog_density,
                fog_mode_code,
            ],
            light0_dir: self.lights[0].position,
            light0_diffuse: self.lights[0].diffuse,
            light0_ambient: self.lights[0].ambient,
            light1_dir: self.lights[1].position,
            light1_diffuse: self.lights[1].diffuse,
            light1_ambient: self.lights[1].ambient,
            light_model_ambient: self.light_model_ambient,
            flags: [
                if self.texture_2d_enabled { 1 } else { 0 },
                if self.lighting_enabled { 1 } else { 0 },
                if self.fog_enabled { 1 } else { 0 },
                alpha_test_code,
            ],
            alpha_ref: [
                self.alpha_ref,
                0.0, // tex_gen
                if self.rescale_normal_enabled {
                    1.0
                } else {
                    0.0
                },
                0.0,
            ],
            tex_gen_s: [0.0; 4],
            tex_gen_t: [0.0; 4],
        }
    }

    pub fn build_pipeline_key(&self, mode: GLenum) -> PipelineKey {
        let topology = match mode {
            GL_TRIANGLES | GL_QUADS | GL_POLYGON => 0,
            GL_TRIANGLE_STRIP => 1,
            GL_TRIANGLE_FAN => 2,
            GL_LINES => 3,
            GL_LINE_STRIP | GL_LINE_LOOP => 4,
            GL_POINTS => 5,
            _ => 0,
        };

        let cull_mode = if self.cull_face_enabled {
            match self.cull_face_mode {
                GL_FRONT => 1,
                GL_BACK => 2,
                _ => 0,
            }
        } else {
            0
        };

        let mut color_mask_byte = 0u8;
        if self.color_mask.0 {
            color_mask_byte |= 1;
        }
        if self.color_mask.1 {
            color_mask_byte |= 2;
        }
        if self.color_mask.2 {
            color_mask_byte |= 4;
        }
        if self.color_mask.3 {
            color_mask_byte |= 8;
        }

        PipelineKey {
            topology,
            cull_mode,
            front_face_ccw: self.front_face == GL_CCW,
            blend_enabled: self.blend_enabled,
            src_factor: self.src_factor,
            dst_factor: self.dst_factor,
            depth_test: self.depth_test_enabled,
            depth_write: self.depth_mask,
            depth_func: self.depth_func,
            color_mask: color_mask_byte,
        }
    }

    pub fn draw_vertex_data(
        &mut self,
        mode: GLenum,
        vertices: &[VertexData],
        indices: Option<&[u32]>,
    ) {
        if let Some(list) = &mut self.active_display_list {
            list.push_op(DisplayListOp::Draw {
                mode,
                vertices: vertices.to_vec(),
                indices: indices.map(|i| i.to_vec()).unwrap_or_default(),
            });
            if self.display_list_mode == GL_COMPILE {
                return;
            }
        }

        let Some(renderer) = &self.renderer else {
            return;
        };

        let (mut final_vertices, final_indices) = if mode == GL_QUADS {
            // Expand quads to triangle indexed list
            let quad_count = vertices.len() / 4;
            let mut inds = Vec::with_capacity(quad_count * 6);
            for q in 0..quad_count as u32 {
                let base = q * 4;
                inds.push(base + 0);
                inds.push(base + 1);
                inds.push(base + 2);
                inds.push(base + 0);
                inds.push(base + 2);
                inds.push(base + 3);
            }
            (vertices.to_vec(), Some(inds))
        } else if let Some(inds) = indices {
            (vertices.to_vec(), Some(inds.to_vec()))
        } else {
            (vertices.to_vec(), None)
        };

        let key = self.build_pipeline_key(mode);
        let uniforms = self.build_uniforms();

        let mut tex_mgr = self.texture_manager.lock();
        let mut default_white = TextureObject::new(0, GL_TEXTURE_2D);
        default_white.width = 1;
        default_white.height = 1;
        default_white.level_data.insert(0, vec![255, 255, 255, 255]);

        let tex = tex_mgr
            .get_current_texture_mut()
            .unwrap_or(&mut default_white);

        let vp = (
            self.viewport.0.max(0) as u32,
            self.viewport.1.max(0) as u32,
            self.viewport.2.max(1) as u32,
            self.viewport.3.max(1) as u32,
        );

        let scissor = if self.scissor_test_enabled {
            Some((
                self.scissor.0.max(0) as u32,
                self.scissor.1.max(0) as u32,
                self.scissor.2.max(1) as u32,
                self.scissor.3.max(1) as u32,
            ))
        } else {
            None
        };

        let mut rend = renderer.lock();
        rend.draw_mesh(
            &key,
            &uniforms,
            tex,
            &final_vertices,
            final_indices.as_deref(),
            vp,
            scissor,
        );
    }

    pub fn call_display_list(&mut self, list_id: GLuint) {
        let Some(list) = self.display_lists.get_list(list_id) else {
            return;
        };
        for op in list.ops {
            match op {
                DisplayListOp::Draw {
                    mode,
                    vertices,
                    indices,
                } => {
                    let idx_slice = if indices.is_empty() {
                        None
                    } else {
                        Some(indices.as_slice())
                    };
                    self.draw_vertex_data(mode, &vertices, idx_slice);
                }
                DisplayListOp::MatrixPush(mode) => {
                    let prev = self.matrix_mode;
                    self.matrix_mode = mode;
                    let _ = self.current_matrix_stack().push();
                    self.matrix_mode = prev;
                }
                DisplayListOp::MatrixPop(mode) => {
                    let prev = self.matrix_mode;
                    self.matrix_mode = mode;
                    let _ = self.current_matrix_stack().pop();
                    self.matrix_mode = prev;
                }
                DisplayListOp::MatrixLoad(mode, m) => {
                    let prev = self.matrix_mode;
                    self.matrix_mode = mode;
                    self.current_matrix_stack().load_matrix(&m);
                    self.matrix_mode = prev;
                }
                DisplayListOp::MatrixMult(mode, m) => {
                    let prev = self.matrix_mode;
                    self.matrix_mode = mode;
                    self.current_matrix_stack().mult_matrix(&m);
                    self.matrix_mode = prev;
                }
                DisplayListOp::MatrixTranslate(x, y, z) => {
                    self.current_matrix_stack().translate(x, y, z);
                }
                DisplayListOp::MatrixRotate(a, x, y, z) => {
                    self.current_matrix_stack().rotate(a, x, y, z);
                }
                DisplayListOp::MatrixScale(x, y, z) => {
                    self.current_matrix_stack().scale(x, y, z);
                }
                DisplayListOp::BindTexture(id) => {
                    let mut tm = self.texture_manager.lock();
                    tm.bind_texture(GL_TEXTURE_2D, id);
                }
                DisplayListOp::Enable(cap) => {
                    self.set_enable(cap, true);
                }
                DisplayListOp::Disable(cap) => {
                    self.set_enable(cap, false);
                }
                DisplayListOp::Color4f(r, g, b, a) => {
                    self.current_color = [r, g, b, a];
                }
                DisplayListOp::Normal3f(x, y, z) => {
                    self.current_normal = [x, y, z];
                }
                DisplayListOp::TexCoord2f(u, v) => {
                    self.current_texcoord = [u, v];
                }
                DisplayListOp::BlendFunc(src, dst) => {
                    self.src_factor = src;
                    self.dst_factor = dst;
                }
                DisplayListOp::DepthFunc(func) => {
                    self.depth_func = func;
                }
                DisplayListOp::DepthMask(mask) => {
                    self.depth_mask = mask;
                }
                DisplayListOp::AlphaFunc(func, r) => {
                    self.alpha_func = func;
                    self.alpha_ref = r;
                }
                DisplayListOp::CullFace(mode) => {
                    self.cull_face_mode = mode;
                }
                DisplayListOp::PolygonOffset(factor, units) => {
                    self.polygon_offset_factor = factor;
                    self.polygon_offset_units = units;
                }
                DisplayListOp::ShadeModel(model) => {
                    self.shade_model = model;
                }
                DisplayListOp::CallList(child_id) => {
                    self.call_display_list(child_id);
                }
            }
        }
    }

    pub fn set_enable(&mut self, cap: GLenum, enable: bool) {
        match cap {
            GL_TEXTURE_2D => self.texture_2d_enabled = enable,
            GL_BLEND => self.blend_enabled = enable,
            GL_DEPTH_TEST => self.depth_test_enabled = enable,
            GL_ALPHA_TEST => self.alpha_test_enabled = enable,
            GL_CULL_FACE => self.cull_face_enabled = enable,
            GL_LIGHTING => self.lighting_enabled = enable,
            GL_LIGHT0 => self.lights_enabled[0] = enable,
            GL_LIGHT1 => self.lights_enabled[1] = enable,
            GL_LIGHT2 => self.lights_enabled[2] = enable,
            GL_LIGHT3 => self.lights_enabled[3] = enable,
            GL_LIGHT4 => self.lights_enabled[4] = enable,
            GL_LIGHT5 => self.lights_enabled[5] = enable,
            GL_LIGHT6 => self.lights_enabled[6] = enable,
            GL_LIGHT7 => self.lights_enabled[7] = enable,
            GL_COLOR_MATERIAL => self.color_material_enabled = enable,
            GL_RESCALE_NORMAL => self.rescale_normal_enabled = enable,
            GL_NORMALIZE => self.normalize_enabled = enable,
            GL_FOG => self.fog_enabled = enable,
            GL_SCISSOR_TEST => self.scissor_test_enabled = enable,
            GL_STENCIL_TEST => self.stencil_test_enabled = enable,
            GL_POLYGON_OFFSET_FILL => self.polygon_offset_fill_enabled = enable,
            _ => {}
        }
    }
}
