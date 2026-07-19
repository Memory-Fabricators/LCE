//! Fixed-function WGSL shader generation and GLES2 shader/program emulation.

use std::collections::HashMap;

/// Memory layout for fixed-function uniform buffer (must match WGSL alignment rules).
#[derive(Debug, Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
#[repr(C)]
pub struct FixedFunctionUniforms {
    pub model_view: [f32; 16],
    pub projection: [f32; 16],
    pub texture_matrix: [f32; 16],
    pub normal_matrix: [f32; 16],
    pub color: [f32; 4],
    pub fog_color: [f32; 4],
    // x = start, y = end, z = density, w = mode (0=none, 1=linear, 2=exp, 3=exp2)
    pub fog_params: [f32; 4],
    pub light0_dir: [f32; 4],
    pub light0_diffuse: [f32; 4],
    pub light0_ambient: [f32; 4],
    pub light1_dir: [f32; 4],
    pub light1_diffuse: [f32; 4],
    pub light1_ambient: [f32; 4],
    pub light_model_ambient: [f32; 4],
    // x = texture_enabled, y = lighting_enabled, z = fog_enabled, w = alpha_test_func
    pub flags: [u32; 4],
    // x = alpha_ref, y = tex_gen_enabled, z = rescale_normal, w = pad
    pub alpha_ref: [f32; 4],
    pub tex_gen_s: [f32; 4],
    pub tex_gen_t: [f32; 4],
}

impl Default for FixedFunctionUniforms {
    fn default() -> Self {
        let identity = [
            1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
        ];
        Self {
            model_view: identity,
            projection: identity,
            texture_matrix: identity,
            normal_matrix: identity,
            color: [1.0, 1.0, 1.0, 1.0],
            fog_color: [0.0, 0.0, 0.0, 1.0],
            fog_params: [0.0, 1.0, 1.0, 0.0],
            light0_dir: [0.0, 0.0, -1.0, 0.0],
            light0_diffuse: [1.0, 1.0, 1.0, 1.0],
            light0_ambient: [0.0, 0.0, 0.0, 1.0],
            light1_dir: [0.0, 0.0, -1.0, 0.0],
            light1_diffuse: [1.0, 1.0, 1.0, 1.0],
            light1_ambient: [0.0, 0.0, 0.0, 1.0],
            light_model_ambient: [0.2, 0.2, 0.2, 1.0],
            flags: [0, 0, 0, 7], // alpha_test_func = GL_ALWAYS (7)
            alpha_ref: [0.0, 0.0, 0.0, 0.0],
            tex_gen_s: [0.0; 4],
            tex_gen_t: [0.0; 4],
        }
    }
}

pub const FIXED_FUNCTION_WGSL: &str = r#"
struct Uniforms {
    model_view: mat4x4<f32>,
    projection: mat4x4<f32>,
    texture_matrix: mat4x4<f32>,
    normal_matrix: mat4x4<f32>,
    color: vec4<f32>,
    fog_color: vec4<f32>,
    fog_params: vec4<f32>,
    light0_dir: vec4<f32>,
    light0_diffuse: vec4<f32>,
    light0_ambient: vec4<f32>,
    light1_dir: vec4<f32>,
    light1_diffuse: vec4<f32>,
    light1_ambient: vec4<f32>,
    light_model_ambient: vec4<f32>,
    flags: vec4<u32>,
    alpha_ref: vec4<f32>,
    tex_gen_s: vec4<f32>,
    tex_gen_t: vec4<f32>,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

@group(1) @binding(0)
var t_diffuse: texture_2d<f32>;
@group(1) @binding(1)
var s_diffuse: sampler;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) tex_coord: vec2<f32>,
    @location(2) color: vec4<f32>,
    @location(3) normal: vec3<f32>,
};

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) tex_coord: vec2<f32>,
    @location(1) color: vec4<f32>,
    @location(2) eye_dist: f32,
};

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    let eye_pos = uniforms.model_view * vec4<f32>(in.position, 1.0);
    out.clip_position = uniforms.projection * eye_pos;

    // TexCoord or TexGen
    if (uniforms.alpha_ref.y != 0.0) {
        let s = dot(vec4<f32>(in.position, 1.0), uniforms.tex_gen_s);
        let t = dot(vec4<f32>(in.position, 1.0), uniforms.tex_gen_t);
        out.tex_coord = vec2<f32>(s, t);
    } else {
        let tc = uniforms.texture_matrix * vec4<f32>(in.tex_coord, 0.0, 1.0);
        out.tex_coord = tc.xy;
    }

    // Normal and lighting
    var vcolor = in.color;
    if (uniforms.flags.y != 0u) {
        var norm = (uniforms.normal_matrix * vec4<f32>(in.normal, 0.0)).xyz;
        let nlen = length(norm);
        if (nlen > 1e-6) {
            norm = norm / nlen;
        }

        var light_acc = uniforms.light_model_ambient.rgb;

        // Light0
        let l0_dot = max(0.0, dot(norm, -normalize(uniforms.light0_dir.xyz)));
        light_acc += uniforms.light0_ambient.rgb + uniforms.light0_diffuse.rgb * l0_dot;

        // Light1
        let l1_dot = max(0.0, dot(norm, -normalize(uniforms.light1_dir.xyz)));
        light_acc += uniforms.light1_ambient.rgb + uniforms.light1_diffuse.rgb * l1_dot;

        vcolor = vec4<f32>(vcolor.rgb * light_acc, vcolor.a);
    }

    out.color = vcolor;
    out.eye_dist = length(eye_pos.xyz);

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var final_color = in.color * uniforms.color;

    // Texture sampling
    if (uniforms.flags.x != 0u) {
        let tex_sample = textureSample(t_diffuse, s_diffuse, in.tex_coord);
        final_color = final_color * tex_sample;
    }

    // Alpha test
    // 0: NEVER, 1: LESS, 2: EQUAL, 3: LEQUAL, 4: GREATER, 5: NOTEQUAL, 6: GEQUAL, 7: ALWAYS
    let a_func = uniforms.flags.w;
    let a_ref = uniforms.alpha_ref.x;
    let alpha = final_color.a;

    var pass_alpha = true;
    if (a_func == 0u) {
        pass_alpha = false;
    } else if (a_func == 1u) {
        pass_alpha = alpha < a_ref;
    } else if (a_func == 2u) {
        pass_alpha = abs(alpha - a_ref) < 1e-5;
    } else if (a_func == 3u) {
        pass_alpha = alpha <= a_ref;
    } else if (a_func == 4u) {
        pass_alpha = alpha > a_ref;
    } else if (a_func == 5u) {
        pass_alpha = abs(alpha - a_ref) >= 1e-5;
    } else if (a_func == 6u) {
        pass_alpha = alpha >= a_ref;
    }

    if (!pass_alpha) {
        discard;
    }

    // Fog blending
    // mode: 0=none, 1=linear, 2=exp, 3=exp2
    let fog_mode = u32(uniforms.fog_params.w);
    if (uniforms.flags.z != 0u && fog_mode != 0u) {
        var fog_factor = 1.0;
        let dist = in.eye_dist;
        let fog_start = uniforms.fog_params.x;
        let fog_end = uniforms.fog_params.y;
        let fog_density = uniforms.fog_params.z;

        if (fog_mode == 1u) {
            let range = fog_end - fog_start;
            if (range > 1e-5) {
                fog_factor = clamp((fog_end - dist) / range, 0.0, 1.0);
            }
        } else if (fog_mode == 2u) {
            fog_factor = clamp(exp(-fog_density * dist), 0.0, 1.0);
        } else if (fog_mode == 3u) {
            let f = fog_density * dist;
            fog_factor = clamp(exp(-f * f), 0.0, 1.0);
        }

        final_color = vec4<f32>(mix(uniforms.fog_color.rgb, final_color.rgb, fog_factor), final_color.a);
    }

    // X11 compositors treat 32-bit ARGB windows as see-through when alpha is 0.
    return vec4<f32>(final_color.rgb, 1.0);
}
"#;

// ============================================================================
// GLES2 Shader and Program emulation
// ============================================================================

#[derive(Debug, Clone)]
pub struct ShaderObject {
    pub id: u32,
    pub shader_type: u32, // GL_VERTEX_SHADER, GL_FRAGMENT_SHADER
    pub source: String,
    pub compiled: bool,
    pub info_log: String,
}

#[derive(Debug, Clone)]
pub struct ProgramObject {
    pub id: u32,
    pub attached_shaders: Vec<u32>,
    pub linked: bool,
    pub info_log: String,
    pub uniforms_f32: HashMap<i32, Vec<f32>>,
    pub uniforms_i32: HashMap<i32, Vec<i32>>,
    pub uniform_names: HashMap<String, i32>,
    pub attrib_names: HashMap<String, i32>,
}

impl ProgramObject {
    pub fn new(id: u32) -> Self {
        Self {
            id,
            attached_shaders: Vec::new(),
            linked: false,
            info_log: String::new(),
            uniforms_f32: HashMap::new(),
            uniforms_i32: HashMap::new(),
            uniform_names: HashMap::new(),
            attrib_names: HashMap::new(),
        }
    }
}
