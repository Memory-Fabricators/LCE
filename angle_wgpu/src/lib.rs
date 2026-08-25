//! ANGLE alternative forwarding mixed OpenGL ES 1.1 / 2.0 and EGL calls to wgpu and winit.
#![allow(non_snake_case, unused_imports, dead_code)]

pub mod display_list;
pub mod egl;
pub mod gl_context;
pub mod image;
pub mod matrix;
pub mod renderer;
pub mod shader;
pub mod texture;
pub mod types;
pub mod winit_app;

use crate::display_list::VertexData;
use crate::egl::*;
use crate::gl_context::GlContext;
use crate::matrix::MatrixMode;
use crate::types::*;
use std::ffi::{c_char, c_void};

/// Forwards `log`/wgpu-hal diagnostics to stderr. Without a registered
/// logger, wgpu silently discards `log::debug!`/`log::warn!` calls (e.g. the
/// backend-init errors behind `FailedToCreateSurfaceForAnyBackend`), which
/// otherwise looks identical to "no error, just no picture".
struct StderrLogger;
impl log::Log for StderrLogger {
    fn enabled(&self, _metadata: &log::Metadata) -> bool {
        true
    }
    fn log(&self, record: &log::Record) {
        eprintln!(
            "[{}] {}: {}",
            record.level(),
            record.target(),
            record.args()
        );
    }
    fn flush(&self) {}
}
static LOGGER_INIT: std::sync::Once = std::sync::Once::new();
pub fn init_logging() {
    LOGGER_INIT.call_once(|| {
        let _ = log::set_logger(&StderrLogger).map(|()| log::set_max_level(log::LevelFilter::Warn));
    });
}
// ============================================================================
// Internal Helper: Helper to execute with current GL context
// ============================================================================

#[inline]
fn with_context<F, R>(f: F) -> R
where
    F: FnOnce(&mut GlContext) -> R,
    R: Default,
{
    if let Some(ctx_arc) = get_current_gl_context() {
        let mut ctx = ctx_arc.lock();
        f(&mut ctx)
    } else {
        R::default()
    }
}

// ============================================================================
// OpenGL Matrix and Transform Entry Points
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn glMatrixMode(mode: GLenum) {
    with_context(|ctx| {
        ctx.matrix_mode = match mode {
            GL_MODELVIEW => MatrixMode::ModelView,
            GL_PROJECTION => MatrixMode::Projection,
            GL_TEXTURE => MatrixMode::Texture,
            _ => MatrixMode::ModelView,
        };
    });
}

#[no_mangle]
pub unsafe extern "C" fn glLoadIdentity() {
    with_context(|ctx| {
        ctx.current_matrix_stack().load_identity();
    });
}

#[no_mangle]
pub unsafe extern "C" fn glPushMatrix() {
    with_context(|ctx| {
        let mode = ctx.matrix_mode;
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::MatrixPush(mode));
        }
        let _ = ctx.current_matrix_stack().push();
    });
}

#[no_mangle]
pub unsafe extern "C" fn glPopMatrix() {
    with_context(|ctx| {
        let mode = ctx.matrix_mode;
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::MatrixPop(mode));
        }
        let _ = ctx.current_matrix_stack().pop();
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTranslatef(x: GLfloat, y: GLfloat, z: GLfloat) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::MatrixTranslate(x, y, z));
        }
        ctx.current_matrix_stack().translate(x, y, z);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTranslated(x: GLdouble, y: GLdouble, z: GLdouble) {
    glTranslatef(x as GLfloat, y as GLfloat, z as GLfloat);
}

#[no_mangle]
pub unsafe extern "C" fn glRotatef(angle: GLfloat, x: GLfloat, y: GLfloat, z: GLfloat) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::MatrixRotate(
                angle, x, y, z,
            ));
        }
        ctx.current_matrix_stack().rotate(angle, x, y, z);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glScalef(x: GLfloat, y: GLfloat, z: GLfloat) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::MatrixScale(x, y, z));
        }
        ctx.current_matrix_stack().scale(x, y, z);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glScaled(x: GLdouble, y: GLdouble, z: GLdouble) {
    glScalef(x as GLfloat, y as GLfloat, z as GLfloat);
}

#[no_mangle]
pub unsafe extern "C" fn glOrtho(
    left: GLdouble,
    right: GLdouble,
    bottom: GLdouble,
    top: GLdouble,
    near_val: GLdouble,
    far_val: GLdouble,
) {
    glOrthof(
        left as GLfloat,
        right as GLfloat,
        bottom as GLfloat,
        top as GLfloat,
        near_val as GLfloat,
        far_val as GLfloat,
    );
}

#[no_mangle]
pub unsafe extern "C" fn glOrthof(
    left: GLfloat,
    right: GLfloat,
    bottom: GLfloat,
    top: GLfloat,
    near_val: GLfloat,
    far_val: GLfloat,
) {
    with_context(|ctx| {
        let ortho_m = crate::matrix::Mat4::ortho(left, right, bottom, top, near_val, far_val);
        ctx.current_matrix_stack().mult_matrix(&ortho_m);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glFrustum(
    left: GLdouble,
    right: GLdouble,
    bottom: GLdouble,
    top: GLdouble,
    near_val: GLdouble,
    far_val: GLdouble,
) {
    glFrustumf(
        left as GLfloat,
        right as GLfloat,
        bottom as GLfloat,
        top as GLfloat,
        near_val as GLfloat,
        far_val as GLfloat,
    );
}

#[no_mangle]
pub unsafe extern "C" fn glFrustumf(
    left: GLfloat,
    right: GLfloat,
    bottom: GLfloat,
    top: GLfloat,
    near_val: GLfloat,
    far_val: GLfloat,
) {
    with_context(|ctx| {
        let frustum_m = crate::matrix::Mat4::frustum(left, right, bottom, top, near_val, far_val);
        ctx.current_matrix_stack().mult_matrix(&frustum_m);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glMultMatrixf(m: *const GLfloat) {
    if m.is_null() {
        return;
    }
    with_context(|ctx| {
        let mut data = [0.0f32; 16];
        data.copy_from_slice(std::slice::from_raw_parts(m, 16));
        let mat = crate::matrix::Mat4::from_array(data);
        ctx.current_matrix_stack().mult_matrix(&mat);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glLoadMatrixf(m: *const GLfloat) {
    if m.is_null() {
        return;
    }
    with_context(|ctx| {
        let mut data = [0.0f32; 16];
        data.copy_from_slice(std::slice::from_raw_parts(m, 16));
        let mat = crate::matrix::Mat4::from_array(data);
        ctx.current_matrix_stack().load_matrix(&mat);
    });
}

// ============================================================================
// Client State and Vertex Arrays
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn glEnableClientState(array: GLenum) {
    with_context(|ctx| match array {
        GL_VERTEX_ARRAY => ctx.vertex_array_enabled = true,
        GL_TEXTURE_COORD_ARRAY => ctx.texcoord_array_enabled = true,
        GL_COLOR_ARRAY => ctx.color_array_enabled = true,
        GL_NORMAL_ARRAY => ctx.normal_array_enabled = true,
        _ => {}
    });
}

#[no_mangle]
pub unsafe extern "C" fn glDisableClientState(array: GLenum) {
    with_context(|ctx| match array {
        GL_VERTEX_ARRAY => ctx.vertex_array_enabled = false,
        GL_TEXTURE_COORD_ARRAY => ctx.texcoord_array_enabled = false,
        GL_COLOR_ARRAY => ctx.color_array_enabled = false,
        GL_NORMAL_ARRAY => ctx.normal_array_enabled = false,
        _ => {}
    });
}

#[no_mangle]
pub unsafe extern "C" fn glVertexPointer(
    size: GLint,
    type_: GLenum,
    stride: GLsizei,
    pointer: *const c_void,
) {
    with_context(|ctx| {
        ctx.vertex_pointer_size = size;
        ctx.vertex_pointer_type = type_;
        ctx.vertex_pointer_stride = stride;
        ctx.vertex_pointer = pointer;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTexCoordPointer(
    size: GLint,
    type_: GLenum,
    stride: GLsizei,
    pointer: *const c_void,
) {
    with_context(|ctx| {
        ctx.texcoord_pointer_size = size;
        ctx.texcoord_pointer_type = type_;
        ctx.texcoord_pointer_stride = stride;
        ctx.texcoord_pointer = pointer;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glColorPointer(
    size: GLint,
    type_: GLenum,
    stride: GLsizei,
    pointer: *const c_void,
) {
    with_context(|ctx| {
        ctx.color_pointer_size = size;
        ctx.color_pointer_type = type_;
        ctx.color_pointer_stride = stride;
        ctx.color_pointer = pointer;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glNormalPointer(type_: GLenum, stride: GLsizei, pointer: *const c_void) {
    with_context(|ctx| {
        ctx.normal_pointer_type = type_;
        ctx.normal_pointer_stride = stride;
        ctx.normal_pointer = pointer;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glClientActiveTexture(texture: GLenum) {
    with_context(|ctx| {
        let idx = (texture.saturating_sub(GL_TEXTURE0)) as usize;
        if idx < 8 {
            ctx.texture_manager.lock().client_active_unit = idx;
        }
    });
}

// ============================================================================
// Draw Calls
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn glDrawArrays(mode: GLenum, first: GLint, count: GLsizei) {
    if count <= 0 {
        return;
    }

    with_context(|ctx| {
        if !ctx.vertex_array_enabled || ctx.vertex_pointer.is_null() {
            return;
        }

        let total = count as usize;
        let mut vertices = Vec::with_capacity(total);

        let v_stride = if ctx.vertex_pointer_stride > 0 {
            ctx.vertex_pointer_stride as usize
        } else {
            (ctx.vertex_pointer_size * 4) as usize
        };
        let t_stride = if ctx.texcoord_pointer_stride > 0 {
            ctx.texcoord_pointer_stride as usize
        } else {
            (ctx.texcoord_pointer_size * 4) as usize
        };
        let c_stride = if ctx.color_pointer_stride > 0 {
            ctx.color_pointer_stride as usize
        } else {
            (ctx.color_pointer_size * 4) as usize
        };
        let n_stride = if ctx.normal_pointer_stride > 0 {
            ctx.normal_pointer_stride as usize
        } else {
            12
        };

        for i in 0..total {
            let idx = (first as usize) + i;

            // Position
            let mut pos = [0.0f32; 3];
            if !ctx.vertex_pointer.is_null() {
                let p = (ctx.vertex_pointer as *const u8).add(idx * v_stride) as *const f32;
                if ctx.vertex_pointer_size >= 2 {
                    pos[0] = *p.add(0);
                    pos[1] = *p.add(1);
                }
                if ctx.vertex_pointer_size >= 3 {
                    pos[2] = *p.add(2);
                }
            }

            // TexCoord
            let mut tex = ctx.current_texcoord;
            if ctx.texcoord_array_enabled && !ctx.texcoord_pointer.is_null() {
                let p = (ctx.texcoord_pointer as *const u8).add(idx * t_stride) as *const f32;
                if ctx.texcoord_pointer_size >= 2 {
                    tex[0] = *p.add(0);
                    tex[1] = *p.add(1);
                }
            }

            // Color
            let mut col = ctx.current_color;
            if ctx.color_array_enabled && !ctx.color_pointer.is_null() {
                let p = (ctx.color_pointer as *const u8).add(idx * c_stride);
                if ctx.color_pointer_type == GL_UNSIGNED_BYTE {
                    let pb = p as *const u8;
                    col[0] = (*pb.add(0) as f32) / 255.0;
                    col[1] = (*pb.add(1) as f32) / 255.0;
                    col[2] = (*pb.add(2) as f32) / 255.0;
                    if ctx.color_pointer_size >= 4 {
                        col[3] = (*pb.add(3) as f32) / 255.0;
                    } else {
                        col[3] = 1.0;
                    }
                } else {
                    let pf = p as *const f32;
                    col[0] = *pf.add(0);
                    col[1] = *pf.add(1);
                    col[2] = *pf.add(2);
                    if ctx.color_pointer_size >= 4 {
                        col[3] = *pf.add(3);
                    } else {
                        col[3] = 1.0;
                    }
                }
            }

            // Normal
            let mut norm = ctx.current_normal;
            if ctx.normal_array_enabled && !ctx.normal_pointer.is_null() {
                let p = (ctx.normal_pointer as *const u8).add(idx * n_stride) as *const f32;
                norm[0] = *p.add(0);
                norm[1] = *p.add(1);
                norm[2] = *p.add(2);
            }

            vertices.push(VertexData {
                position: pos,
                tex_coord: tex,
                color: col,
                normal: norm,
            });
        }

        ctx.draw_vertex_data(mode, &vertices, None);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glDrawElements(
    mode: GLenum,
    count: GLsizei,
    type_: GLenum,
    indices: *const c_void,
) {
    if count <= 0 || indices.is_null() {
        return;
    }

    with_context(|ctx| {
        if !ctx.vertex_array_enabled || ctx.vertex_pointer.is_null() {
            return;
        }

        let num_indices = count as usize;
        let mut idx_vec = Vec::with_capacity(num_indices);

        let mut max_index = 0usize;
        for i in 0..num_indices {
            let idx = match type_ {
                GL_UNSIGNED_BYTE => *(indices as *const u8).add(i) as u32,
                GL_UNSIGNED_SHORT => *(indices as *const u16).add(i) as u32,
                GL_UNSIGNED_INT => *(indices as *const u32).add(i),
                _ => *(indices as *const u16).add(i) as u32,
            };
            max_index = max_index.max(idx as usize);
            idx_vec.push(idx);
        }

        let vertex_count = max_index + 1;
        let mut vertices = Vec::with_capacity(vertex_count);

        let v_stride = if ctx.vertex_pointer_stride > 0 {
            ctx.vertex_pointer_stride as usize
        } else {
            (ctx.vertex_pointer_size * 4) as usize
        };
        let t_stride = if ctx.texcoord_pointer_stride > 0 {
            ctx.texcoord_pointer_stride as usize
        } else {
            (ctx.texcoord_pointer_size * 4) as usize
        };
        let c_stride = if ctx.color_pointer_stride > 0 {
            ctx.color_pointer_stride as usize
        } else {
            (ctx.color_pointer_size * 4) as usize
        };

        for i in 0..vertex_count {
            let mut pos = [0.0f32; 3];
            let p = (ctx.vertex_pointer as *const u8).add(i * v_stride) as *const f32;
            pos[0] = *p.add(0);
            pos[1] = *p.add(1);
            if ctx.vertex_pointer_size >= 3 {
                pos[2] = *p.add(2);
            }

            let mut tex = ctx.current_texcoord;
            if ctx.texcoord_array_enabled && !ctx.texcoord_pointer.is_null() {
                let tp = (ctx.texcoord_pointer as *const u8).add(i * t_stride) as *const f32;
                tex[0] = *tp.add(0);
                tex[1] = *tp.add(1);
            }

            let mut col = ctx.current_color;
            if ctx.color_array_enabled && !ctx.color_pointer.is_null() {
                let cp = (ctx.color_pointer as *const u8).add(i * c_stride) as *const f32;
                col[0] = *cp.add(0);
                col[1] = *cp.add(1);
                col[2] = *cp.add(2);
                col[3] = *cp.add(3);
            }

            vertices.push(VertexData {
                position: pos,
                tex_coord: tex,
                color: col,
                normal: ctx.current_normal,
            });
        }

        ctx.draw_vertex_data(mode, &vertices, Some(&idx_vec));
    });
}

// ============================================================================
// Immediate Mode (glBegin/glEnd)
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn glBegin(mode: GLenum) {
    with_context(|ctx| {
        ctx.immediate_mode = Some(mode);
        ctx.immediate_vertices.clear();
    });
}

#[no_mangle]
pub unsafe extern "C" fn glEnd() {
    with_context(|ctx| {
        if let Some(mode) = ctx.immediate_mode.take() {
            let verts = std::mem::take(&mut ctx.immediate_vertices);
            ctx.draw_vertex_data(mode, &verts, None);
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glVertex3f(x: GLfloat, y: GLfloat, z: GLfloat) {
    with_context(|ctx| {
        if ctx.immediate_mode.is_some() {
            ctx.immediate_vertices.push(VertexData {
                position: [x, y, z],
                tex_coord: ctx.current_texcoord,
                color: ctx.current_color,
                normal: ctx.current_normal,
            });
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glVertex2f(x: GLfloat, y: GLfloat) {
    glVertex3f(x, y, 0.0);
}

#[no_mangle]
pub unsafe extern "C" fn glTexCoord2f(u: GLfloat, v: GLfloat) {
    with_context(|ctx| {
        ctx.current_texcoord = [u, v];
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::TexCoord2f(u, v));
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glMultiTexCoord2f(_target: GLenum, s: GLfloat, t: GLfloat) {
    glTexCoord2f(s, t);
}

#[no_mangle]
pub unsafe extern "C" fn glMultiTexCoord4f(
    _target: GLenum,
    s: GLfloat,
    t: GLfloat,
    _r: GLfloat,
    _q: GLfloat,
) {
    glTexCoord2f(s, t);
}

#[no_mangle]
pub unsafe extern "C" fn glColor4f(r: GLfloat, g: GLfloat, b: GLfloat, a: GLfloat) {
    with_context(|ctx| {
        ctx.current_color = [r, g, b, a];
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::Color4f(r, g, b, a));
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glColor3f(r: GLfloat, g: GLfloat, b: GLfloat) {
    glColor4f(r, g, b, 1.0);
}

#[no_mangle]
pub unsafe extern "C" fn glColor4ub(r: GLubyte, g: GLubyte, b: GLubyte, a: GLubyte) {
    glColor4f(
        r as f32 / 255.0,
        g as f32 / 255.0,
        b as f32 / 255.0,
        a as f32 / 255.0,
    );
}

#[no_mangle]
pub unsafe extern "C" fn glColor3ub(r: GLubyte, g: GLubyte, b: GLubyte) {
    glColor4ub(r, g, b, 255);
}

#[no_mangle]
pub unsafe extern "C" fn glColor4fv(v: *const GLfloat) {
    if !v.is_null() {
        glColor4f(*v.add(0), *v.add(1), *v.add(2), *v.add(3));
    }
}

#[no_mangle]
pub unsafe extern "C" fn glNormal3f(x: GLfloat, y: GLfloat, z: GLfloat) {
    with_context(|ctx| {
        ctx.current_normal = [x, y, z];
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::Normal3f(x, y, z));
        }
    });
}

// ============================================================================
// Display Lists
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn glGenLists(range: GLsizei) -> GLuint {
    if range <= 0 {
        return 0;
    }
    with_context(|ctx| ctx.display_lists.gen_lists(range as usize))
}

#[no_mangle]
pub unsafe extern "C" fn glDeleteLists(list: GLuint, range: GLsizei) {
    if range <= 0 {
        return;
    }
    with_context(|ctx| {
        ctx.display_lists.delete_lists(list, range as usize);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glNewList(list: GLuint, mode: GLenum) {
    with_context(|ctx| {
        ctx.active_display_list = Some(crate::display_list::DisplayList::new(list));
        ctx.display_list_mode = mode;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glEndList() {
    with_context(|ctx| {
        if let Some(list) = ctx.active_display_list.take() {
            ctx.display_lists.store_list(list);
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glCallList(list: GLuint) {
    with_context(|ctx| {
        if let Some(active) = &mut ctx.active_display_list {
            active.push_op(crate::display_list::DisplayListOp::CallList(list));
            if ctx.display_list_mode == GL_COMPILE {
                return;
            }
        }
        ctx.call_display_list(list);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glCallLists(n: GLsizei, type_: GLenum, lists: *const c_void) {
    if n <= 0 || lists.is_null() {
        return;
    }
    for i in 0..n as usize {
        let list_id = match type_ {
            GL_UNSIGNED_BYTE => *(lists as *const u8).add(i) as u32,
            GL_UNSIGNED_SHORT => *(lists as *const u16).add(i) as u32,
            GL_UNSIGNED_INT => *(lists as *const u32).add(i),
            GL_INT => *(lists as *const i32).add(i) as u32,
            _ => *(lists as *const u32).add(i),
        };
        glCallList(list_id);
    }
}

#[no_mangle]
pub unsafe extern "C" fn glIsList(list: GLuint) -> GLboolean {
    with_context(|ctx| {
        if ctx.display_lists.is_list(list) {
            GL_TRUE
        } else {
            GL_FALSE
        }
    })
}

// ============================================================================
// Textures
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn glGenTextures(n: GLsizei, textures: *mut GLuint) {
    if n <= 0 || textures.is_null() {
        return;
    }
    with_context(|ctx| {
        let mut tm = ctx.texture_manager.lock();
        let ids = tm.gen_textures(n as usize);
        for (i, id) in ids.iter().enumerate() {
            *textures.add(i) = *id;
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glDeleteTextures(n: GLsizei, textures: *const GLuint) {
    if n <= 0 || textures.is_null() {
        return;
    }
    with_context(|ctx| {
        let slice = std::slice::from_raw_parts(textures, n as usize);
        ctx.texture_manager.lock().delete_textures(slice);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glBindTexture(target: GLenum, texture: GLuint) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::BindTexture(texture));
        }
        ctx.texture_manager.lock().bind_texture(target, texture);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTexImage2D(
    _target: GLenum,
    level: GLint,
    internalformat: GLint,
    width: GLsizei,
    height: GLsizei,
    _border: GLint,
    format: GLenum,
    type_: GLenum,
    pixels: *const c_void,
) {
    if width <= 0 || height <= 0 {
        return;
    }
    with_context(|ctx| {
        let mut tm = ctx.texture_manager.lock();
        if let Some(tex) = tm.get_current_texture_mut() {
            let data_slice = if !pixels.is_null() {
                let num_pixels = (width * height) as usize;
                let bpp = match format {
                    GL_RGBA | GL_BGRA_EXT => 4,
                    GL_RGB => 3,
                    GL_LUMINANCE_ALPHA => 2,
                    _ => 4,
                };
                Some(std::slice::from_raw_parts(
                    pixels as *const u8,
                    num_pixels * bpp,
                ))
            } else {
                None
            };

            tex.set_image_data(
                level as u32,
                internalformat as GLenum,
                width as u32,
                height as u32,
                format,
                type_,
                data_slice,
            );
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTexSubImage2D(
    _target: GLenum,
    level: GLint,
    xoffset: GLint,
    yoffset: GLint,
    width: GLsizei,
    height: GLsizei,
    format: GLenum,
    type_: GLenum,
    pixels: *const c_void,
) {
    if width <= 0 || height <= 0 || pixels.is_null() {
        return;
    }
    with_context(|ctx| {
        let mut tm = ctx.texture_manager.lock();
        if let Some(tex) = tm.get_current_texture_mut() {
            let num_pixels = (width * height) as usize;
            let bpp = match format {
                GL_RGBA | GL_BGRA_EXT => 4,
                GL_RGB => 3,
                GL_LUMINANCE_ALPHA => 2,
                _ => 4,
            };
            let data_slice = std::slice::from_raw_parts(pixels as *const u8, num_pixels * bpp);
            tex.set_sub_image_data(
                level as u32,
                xoffset as u32,
                yoffset as u32,
                width as u32,
                height as u32,
                format,
                type_,
                data_slice,
            );
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTexParameteri(_target: GLenum, pname: GLenum, param: GLint) {
    with_context(|ctx| {
        let mut tm = ctx.texture_manager.lock();
        if let Some(tex) = tm.get_current_texture_mut() {
            match pname {
                GL_TEXTURE_MIN_FILTER => tex.min_filter = param as GLenum,
                GL_TEXTURE_MAG_FILTER => tex.mag_filter = param as GLenum,
                GL_TEXTURE_WRAP_S => tex.wrap_s = param as GLenum,
                GL_TEXTURE_WRAP_T => tex.wrap_t = param as GLenum,
                _ => {}
            }
            tex.dirty = true;
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTexParameterf(target: GLenum, pname: GLenum, param: GLfloat) {
    glTexParameteri(target, pname, param as GLint);
}

#[no_mangle]
pub unsafe extern "C" fn glTexParameteriv(target: GLenum, pname: GLenum, params: *const GLint) {
    if !params.is_null() {
        glTexParameteri(target, pname, *params);
    }
}

#[no_mangle]
pub unsafe extern "C" fn glTexParameterfv(target: GLenum, pname: GLenum, params: *const GLfloat) {
    if !params.is_null() {
        glTexParameterf(target, pname, *params);
    }
}

#[no_mangle]
pub unsafe extern "C" fn glActiveTexture(texture: GLenum) {
    with_context(|ctx| {
        let idx = (texture.saturating_sub(GL_TEXTURE0)) as usize;
        if idx < 8 {
            ctx.texture_manager.lock().active_unit = idx;
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTexImage1D(
    target: GLenum,
    level: GLint,
    internalformat: GLint,
    width: GLsizei,
    border: GLint,
    format: GLenum,
    type_: GLenum,
    pixels: *const c_void,
) {
    glTexImage2D(
        target,
        level,
        internalformat,
        width,
        1,
        border,
        format,
        type_,
        pixels,
    );
}

#[no_mangle]
pub unsafe extern "C" fn glTexImage3D(
    _target: GLenum,
    _level: GLint,
    _internalformat: GLint,
    _width: GLsizei,
    _height: GLsizei,
    _depth: GLsizei,
    _border: GLint,
    _format: GLenum,
    _type_: GLenum,
    _pixels: *const c_void,
) {
}

#[no_mangle]
pub unsafe extern "C" fn glGetTexLevelParameteri(
    _target: GLenum,
    _level: GLint,
    pname: GLenum,
    params: *mut GLint,
) {
    if params.is_null() {
        return;
    }
    with_context(|ctx| {
        let tm = ctx.texture_manager.lock();
        if let Some(tex) = tm.get_current_texture() {
            match pname {
                GL_TEXTURE_WIDTH => *params = tex.width as GLint,
                GL_TEXTURE_HEIGHT => *params = tex.height as GLint,
                GL_TEXTURE_INTERNAL_FORMAT => *params = tex.internal_format as GLint,
                _ => *params = 0,
            }
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glTexGen(_coord: GLenum, _pname: GLenum, _param: GLfloat) {}

#[no_mangle]
pub unsafe extern "C" fn glTexGeni(_coord: GLenum, _pname: GLenum, _param: GLint) {}

#[no_mangle]
pub unsafe extern "C" fn glTexEnvf(_target: GLenum, _pname: GLenum, _param: GLfloat) {}

#[no_mangle]
pub unsafe extern "C" fn glTexEnvi(_target: GLenum, _pname: GLenum, _param: GLint) {}

#[no_mangle]
pub unsafe extern "C" fn glTexEnvfv(_target: GLenum, _pname: GLenum, _params: *const GLfloat) {}

// ============================================================================
// States and Enables
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn glEnable(cap: GLenum) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::Enable(cap));
        }
        ctx.set_enable(cap, true);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glDisable(cap: GLenum) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::Disable(cap));
        }
        ctx.set_enable(cap, false);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glIsEnabled(cap: GLenum) -> GLboolean {
    with_context(|ctx| {
        let is_on = match cap {
            GL_TEXTURE_2D => ctx.texture_2d_enabled,
            GL_BLEND => ctx.blend_enabled,
            GL_DEPTH_TEST => ctx.depth_test_enabled,
            GL_ALPHA_TEST => ctx.alpha_test_enabled,
            GL_CULL_FACE => ctx.cull_face_enabled,
            GL_LIGHTING => ctx.lighting_enabled,
            GL_FOG => ctx.fog_enabled,
            GL_SCISSOR_TEST => ctx.scissor_test_enabled,
            GL_STENCIL_TEST => ctx.stencil_test_enabled,
            GL_COLOR_MATERIAL => ctx.color_material_enabled,
            GL_RESCALE_NORMAL => ctx.rescale_normal_enabled,
            GL_NORMALIZE => ctx.normalize_enabled,
            GL_POLYGON_OFFSET_FILL => ctx.polygon_offset_fill_enabled,
            _ => false,
        };
        if is_on { GL_TRUE } else { GL_FALSE }
    })
}

#[no_mangle]
pub unsafe extern "C" fn glAlphaFunc(func: GLenum, ref_val: GLclampf) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::AlphaFunc(func, ref_val));
        }
        ctx.alpha_func = func;
        ctx.alpha_ref = ref_val;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glBlendFunc(sfactor: GLenum, dfactor: GLenum) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::BlendFunc(
                sfactor, dfactor,
            ));
        }
        ctx.src_factor = sfactor;
        ctx.dst_factor = dfactor;
        ctx.src_factor_alpha = sfactor;
        ctx.dst_factor_alpha = dfactor;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glBlendColor(
    red: GLclampf,
    green: GLclampf,
    blue: GLclampf,
    alpha: GLclampf,
) {
    with_context(|ctx| {
        ctx.blend_color = [red, green, blue, alpha];
    });
}

#[no_mangle]
pub unsafe extern "C" fn glBlendFuncSeparate(
    srcRGB: GLenum,
    dstRGB: GLenum,
    srcAlpha: GLenum,
    dstAlpha: GLenum,
) {
    with_context(|ctx| {
        ctx.src_factor = srcRGB;
        ctx.dst_factor = dstRGB;
        ctx.src_factor_alpha = srcAlpha;
        ctx.dst_factor_alpha = dstAlpha;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glDepthFunc(func: GLenum) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::DepthFunc(func));
        }
        ctx.depth_func = func;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glDepthMask(flag: GLboolean) {
    with_context(|ctx| {
        let enable = flag != GL_FALSE;
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::DepthMask(enable));
        }
        ctx.depth_mask = enable;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glColorMask(
    red: GLboolean,
    green: GLboolean,
    blue: GLboolean,
    alpha: GLboolean,
) {
    with_context(|ctx| {
        ctx.color_mask = (
            red != GL_FALSE,
            green != GL_FALSE,
            blue != GL_FALSE,
            alpha != GL_FALSE,
        );
    });
}

#[no_mangle]
pub unsafe extern "C" fn glCullFace(mode: GLenum) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::CullFace(mode));
        }
        ctx.cull_face_mode = mode;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glFrontFace(mode: GLenum) {
    with_context(|ctx| {
        ctx.front_face = mode;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glPolygonOffset(factor: GLfloat, units: GLfloat) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::PolygonOffset(
                factor, units,
            ));
        }
        ctx.polygon_offset_factor = factor;
        ctx.polygon_offset_units = units;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glLineWidth(width: GLfloat) {
    with_context(|ctx| {
        ctx.line_width = width;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glPointSize(size: GLfloat) {
    with_context(|ctx| {
        ctx.point_size = size;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glShadeModel(mode: GLenum) {
    with_context(|ctx| {
        if let Some(list) = &mut ctx.active_display_list {
            list.push_op(crate::display_list::DisplayListOp::ShadeModel(mode));
        }
        ctx.shade_model = mode;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glColorMaterial(_face: GLenum, _mode: GLenum) {
    with_context(|ctx| {
        ctx.color_material_enabled = true;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glFogf(pname: GLenum, param: GLfloat) {
    with_context(|ctx| match pname {
        GL_FOG_START => ctx.fog_start = param,
        GL_FOG_END => ctx.fog_end = param,
        GL_FOG_DENSITY => ctx.fog_density = param,
        GL_FOG_MODE => ctx.fog_mode = param as GLenum,
        _ => {}
    });
}

#[no_mangle]
pub unsafe extern "C" fn glFogfv(pname: GLenum, params: *const GLfloat) {
    if params.is_null() {
        return;
    }
    with_context(|ctx| {
        if pname == GL_FOG_COLOR {
            ctx.fog_color = [
                *params.add(0),
                *params.add(1),
                *params.add(2),
                *params.add(3),
            ];
        } else {
            glFogf(pname, *params);
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glFogi(pname: GLenum, param: GLint) {
    glFogf(pname, param as GLfloat);
}

#[no_mangle]
pub unsafe extern "C" fn glLightf(_light: GLenum, _pname: GLenum, _param: GLfloat) {}

#[no_mangle]
pub unsafe extern "C" fn glLightfv(light: GLenum, pname: GLenum, params: *const GLfloat) {
    if params.is_null() {
        return;
    }
    with_context(|ctx| {
        let idx = (light.saturating_sub(GL_LIGHT0)) as usize;
        if idx < 8 {
            match pname {
                GL_POSITION => {
                    ctx.lights[idx].position = [
                        *params.add(0),
                        *params.add(1),
                        *params.add(2),
                        *params.add(3),
                    ];
                }
                GL_DIFFUSE => {
                    ctx.lights[idx].diffuse = [
                        *params.add(0),
                        *params.add(1),
                        *params.add(2),
                        *params.add(3),
                    ];
                }
                GL_AMBIENT => {
                    ctx.lights[idx].ambient = [
                        *params.add(0),
                        *params.add(1),
                        *params.add(2),
                        *params.add(3),
                    ];
                }
                GL_SPECULAR => {
                    ctx.lights[idx].specular = [
                        *params.add(0),
                        *params.add(1),
                        *params.add(2),
                        *params.add(3),
                    ];
                }
                _ => {}
            }
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glLightModelf(_pname: GLenum, _param: GLfloat) {}

#[no_mangle]
pub unsafe extern "C" fn glLightModelfv(pname: GLenum, params: *const GLfloat) {
    if params.is_null() {
        return;
    }
    with_context(|ctx| {
        if pname == GL_LIGHT_MODEL_AMBIENT {
            ctx.light_model_ambient = [
                *params.add(0),
                *params.add(1),
                *params.add(2),
                *params.add(3),
            ];
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glMaterialf(_face: GLenum, _pname: GLenum, _param: GLfloat) {}

#[no_mangle]
pub unsafe extern "C" fn glMaterialfv(_face: GLenum, _pname: GLenum, _params: *const GLfloat) {}

#[no_mangle]
pub unsafe extern "C" fn glStencilFunc(func: GLenum, ref_val: GLint, mask: GLuint) {
    with_context(|ctx| {
        ctx.stencil_func = func;
        ctx.stencil_ref = ref_val;
        ctx.stencil_value_mask = mask;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glStencilMask(mask: GLuint) {
    with_context(|ctx| {
        ctx.stencil_writemask = mask;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glStencilOp(fail: GLenum, zfail: GLenum, zpass: GLenum) {
    with_context(|ctx| {
        ctx.stencil_fail = fail;
        ctx.stencil_zfail = zfail;
        ctx.stencil_zpass = zpass;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glViewport(x: GLint, y: GLint, width: GLsizei, height: GLsizei) {
    with_context(|ctx| {
        ctx.viewport = (x, y, width, height);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glScissor(x: GLint, y: GLint, width: GLsizei, height: GLsizei) {
    with_context(|ctx| {
        ctx.scissor = (x, y, width, height);
    });
}

#[no_mangle]
pub unsafe extern "C" fn glClearColor(
    red: GLclampf,
    green: GLclampf,
    blue: GLclampf,
    alpha: GLclampf,
) {
    with_context(|ctx| {
        ctx.clear_color = [red, green, blue, alpha];
        if let Some(r) = &ctx.renderer {
            r.lock().clear_color = [red as f64, green as f64, blue as f64, 1.0];
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glClearDepthf(depth: GLclampf) {
    with_context(|ctx| {
        ctx.clear_depth = depth;
        if let Some(r) = &ctx.renderer {
            r.lock().clear_depth = depth;
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glClearDepth(depth: GLclampd) {
    glClearDepthf(depth as GLclampf);
}

#[no_mangle]
pub unsafe extern "C" fn glClearStencil(s: GLint) {
    with_context(|ctx| {
        ctx.clear_stencil = s;
    });
}

#[no_mangle]
pub unsafe extern "C" fn glClear(mask: GLbitfield) {
    with_context(|ctx| {
        let clear_color = (mask & GL_COLOR_BUFFER_BIT) != 0;
        let clear_depth = (mask & GL_DEPTH_BUFFER_BIT) != 0;
        let clear_stencil = (mask & GL_STENCIL_BUFFER_BIT) != 0;

        if let Some(r) = &ctx.renderer {
            r.lock().clear(clear_color, clear_depth, clear_stencil);
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glPixelStorei(_pname: GLenum, _param: GLint) {}

#[no_mangle]
pub unsafe extern "C" fn glReadPixels(
    _x: GLint,
    _y: GLint,
    width: GLsizei,
    height: GLsizei,
    _format: GLenum,
    _type_: GLenum,
    pixels: *mut c_void,
) {
    if pixels.is_null() || width <= 0 || height <= 0 {
        return;
    }
    // Fill with opaque black by default if not read back
    let size = (width * height * 4) as usize;
    std::ptr::write_bytes(pixels as *mut u8, 0, size);
}

#[no_mangle]
pub unsafe extern "C" fn glFlush() {
    with_context(|ctx| {
        if let Some(r) = &ctx.renderer {
            r.lock().flush();
        }
    });
}

#[no_mangle]
pub unsafe extern "C" fn glFinish() {
    glFlush();
}

#[no_mangle]
pub unsafe extern "C" fn glGetIntegerv(pname: GLenum, params: *mut GLint) {
    if params.is_null() {
        return;
    }
    with_context(|ctx| match pname {
        GL_VIEWPORT => {
            *params.add(0) = ctx.viewport.0;
            *params.add(1) = ctx.viewport.1;
            *params.add(2) = ctx.viewport.2;
            *params.add(3) = ctx.viewport.3;
        }
        GL_SCISSOR_BOX => {
            *params.add(0) = ctx.scissor.0;
            *params.add(1) = ctx.scissor.1;
            *params.add(2) = ctx.scissor.2;
            *params.add(3) = ctx.scissor.3;
        }
        GL_MATRIX_MODE => {
            *params = match ctx.matrix_mode {
                MatrixMode::ModelView => GL_MODELVIEW as GLint,
                MatrixMode::Projection => GL_PROJECTION as GLint,
                MatrixMode::Texture => GL_TEXTURE as GLint,
            };
        }
        GL_MAX_TEXTURE_SIZE => *params = 8192,
        GL_MAX_VIEWPORT_DIMS => {
            *params.add(0) = 8192;
            *params.add(1) = 8192;
        }
        GL_STENCIL_BITS => *params = 8,
        GL_DEPTH_BITS => *params = 24,
        GL_RED_BITS => *params = 8,
        GL_GREEN_BITS => *params = 8,
        GL_BLUE_BITS => *params = 8,
        GL_ALPHA_BITS => *params = 8,
        _ => *params = 0,
    });
}

#[no_mangle]
pub unsafe extern "C" fn glGetFloatv(pname: GLenum, params: *mut GLfloat) {
    if params.is_null() {
        return;
    }
    with_context(|ctx| match pname {
        GL_MODELVIEW_MATRIX => {
            let data = ctx.modelview_stack.current.to_array();
            for i in 0..16 {
                *params.add(i) = data[i];
            }
        }
        GL_PROJECTION_MATRIX => {
            let data = ctx.projection_stack.current.to_array();
            for i in 0..16 {
                *params.add(i) = data[i];
            }
        }
        GL_TEXTURE_MATRIX => {
            let data = ctx.texture_stack.current.to_array();
            for i in 0..16 {
                *params.add(i) = data[i];
            }
        }
        _ => {}
    });
}

#[no_mangle]
pub unsafe extern "C" fn glGetBooleanv(pname: GLenum, params: *mut GLboolean) {
    if params.is_null() {
        return;
    }
    *params = glIsEnabled(pname);
}

#[no_mangle]
pub unsafe extern "C" fn glGetString(name: GLenum) -> *const GLubyte {
    match name {
        GL_VENDOR => b"angle_wgpu\0".as_ptr(),
        GL_RENDERER => b"wgpu FixedFunction OpenGL ES 1.1/2.0 Emulation\0".as_ptr(),
        GL_VERSION => b"OpenGL ES 2.0 (angle_wgpu)\0".as_ptr(),
        GL_EXTENSIONS => {
            b"GL_OES_texture_npot GL_OES_packed_depth_stencil GL_EXT_texture_format_BGRA8888\0"
                .as_ptr()
        }
        _ => b"\0".as_ptr(),
    }
}

#[no_mangle]
pub unsafe extern "C" fn glGetError() -> GLenum {
    with_context(|ctx| {
        let err = ctx.error;
        ctx.error = GL_NO_ERROR;
        err
    })
}

#[no_mangle]
pub unsafe extern "C" fn glPushAttrib(_mask: GLbitfield) {}

#[no_mangle]
pub unsafe extern "C" fn glPopAttrib() {}

#[no_mangle]
pub unsafe extern "C" fn glPushClientAttrib(_mask: GLbitfield) {}

#[no_mangle]
pub unsafe extern "C" fn glPopClientAttrib() {}

// ============================================================================
// GLES2 Shaders & Buffers & Programs
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn glCreateShader(shader_type: GLenum) -> GLuint {
    shader_type
}

#[no_mangle]
pub unsafe extern "C" fn glShaderSource(
    _shader: GLuint,
    _count: GLsizei,
    _string: *const *const GLchar,
    _length: *const GLint,
) {
}

#[no_mangle]
pub unsafe extern "C" fn glCompileShader(_shader: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glGetShaderiv(_shader: GLuint, pname: GLenum, params: *mut GLint) {
    if !params.is_null() {
        if pname == GL_COMPILE_STATUS {
            *params = GL_TRUE as GLint;
        } else {
            *params = 0;
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn glGetShaderInfoLog(
    _shader: GLuint,
    _buf_size: GLsizei,
    length: *mut GLsizei,
    info_log: *mut GLchar,
) {
    if !length.is_null() {
        *length = 0;
    }
    if !info_log.is_null() {
        *info_log = 0;
    }
}

#[no_mangle]
pub unsafe extern "C" fn glDeleteShader(_shader: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glCreateProgram() -> GLuint {
    1
}

#[no_mangle]
pub unsafe extern "C" fn glAttachShader(_program: GLuint, _shader: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glDetachShader(_program: GLuint, _shader: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glLinkProgram(_program: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glGetProgramiv(_program: GLuint, pname: GLenum, params: *mut GLint) {
    if !params.is_null() {
        if pname == GL_LINK_STATUS {
            *params = GL_TRUE as GLint;
        } else {
            *params = 0;
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn glGetProgramInfoLog(
    _program: GLuint,
    _buf_size: GLsizei,
    length: *mut GLsizei,
    info_log: *mut GLchar,
) {
    if !length.is_null() {
        *length = 0;
    }
    if !info_log.is_null() {
        *info_log = 0;
    }
}

#[no_mangle]
pub unsafe extern "C" fn glUseProgram(_program: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glDeleteProgram(_program: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glGetUniformLocation(_program: GLuint, _name: *const GLchar) -> GLint {
    -1
}

#[no_mangle]
pub unsafe extern "C" fn glGetAttribLocation(_program: GLuint, _name: *const GLchar) -> GLint {
    -1
}

#[no_mangle]
pub unsafe extern "C" fn glUniform1f(_location: GLint, _v0: GLfloat) {}
#[no_mangle]
pub unsafe extern "C" fn glUniform2f(_location: GLint, _v0: GLfloat, _v1: GLfloat) {}
#[no_mangle]
pub unsafe extern "C" fn glUniform3f(_location: GLint, _v0: GLfloat, _v1: GLfloat, _v2: GLfloat) {}
#[no_mangle]
pub unsafe extern "C" fn glUniform4f(
    _location: GLint,
    _v0: GLfloat,
    _v1: GLfloat,
    _v2: GLfloat,
    _v3: GLfloat,
) {
}
#[no_mangle]
pub unsafe extern "C" fn glUniform1i(_location: GLint, _v0: GLint) {}
#[no_mangle]
pub unsafe extern "C" fn glUniform2i(_location: GLint, _v0: GLint, _v1: GLint) {}
#[no_mangle]
pub unsafe extern "C" fn glUniform3i(_location: GLint, _v0: GLint, _v1: GLint, _v2: GLint) {}
#[no_mangle]
pub unsafe extern "C" fn glUniform4i(
    _location: GLint,
    _v0: GLint,
    _v1: GLint,
    _v2: GLint,
    _v3: GLint,
) {
}
#[no_mangle]
pub unsafe extern "C" fn glUniformMatrix4fv(
    _location: GLint,
    _count: GLsizei,
    _transpose: GLboolean,
    _value: *const GLfloat,
) {
}

#[no_mangle]
pub unsafe extern "C" fn glGenBuffers(n: GLsizei, buffers: *mut GLuint) {
    if n <= 0 || buffers.is_null() {
        return;
    }
    for i in 0..n as usize {
        *buffers.add(i) = (i + 1) as GLuint;
    }
}

#[no_mangle]
pub unsafe extern "C" fn glGenBuffersARB(n: GLsizei, buffers: *mut GLuint) {
    glGenBuffers(n, buffers);
}

#[no_mangle]
pub unsafe extern "C" fn glBindBuffer(_target: GLenum, _buffer: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glBindBufferARB(target: GLenum, buffer: GLuint) {
    glBindBuffer(target, buffer);
}

#[no_mangle]
pub unsafe extern "C" fn glBufferData(
    _target: GLenum,
    _size: GLsizeiptr,
    _data: *const c_void,
    _usage: GLenum,
) {
}

#[no_mangle]
pub unsafe extern "C" fn glBufferDataARB(
    target: GLenum,
    size: GLsizeiptr,
    data: *const c_void,
    usage: GLenum,
) {
    glBufferData(target, size, data, usage);
}

#[no_mangle]
pub unsafe extern "C" fn glBufferSubData(
    _target: GLenum,
    _offset: GLintptr,
    _size: GLsizeiptr,
    _data: *const c_void,
) {
}

#[no_mangle]
pub unsafe extern "C" fn glDeleteBuffers(_n: GLsizei, _buffers: *const GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glDeleteBuffersARB(n: GLsizei, buffers: *const GLuint) {
    glDeleteBuffers(n, buffers);
}

#[no_mangle]
pub unsafe extern "C" fn glVertexAttribPointer(
    _index: GLuint,
    _size: GLint,
    _type_: GLenum,
    _normalized: GLboolean,
    _stride: GLsizei,
    _pointer: *const c_void,
) {
}

#[no_mangle]
pub unsafe extern "C" fn glEnableVertexAttribArray(_index: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glDisableVertexAttribArray(_index: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glGenFramebuffers(n: GLsizei, framebuffers: *mut GLuint) {
    if n <= 0 || framebuffers.is_null() {
        return;
    }
    for i in 0..n as usize {
        *framebuffers.add(i) = (i + 1) as GLuint;
    }
}

#[no_mangle]
pub unsafe extern "C" fn glBindFramebuffer(_target: GLenum, _framebuffer: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glFramebufferTexture2D(
    _target: GLenum,
    _attachment: GLenum,
    _textarget: GLenum,
    _texture: GLuint,
    _level: GLint,
) {
}

#[no_mangle]
pub unsafe extern "C" fn glDeleteFramebuffers(_n: GLsizei, _framebuffers: *const GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glCheckFramebufferStatus(_target: GLenum) -> GLenum {
    GL_FRAMEBUFFER_COMPLETE
}

#[no_mangle]
pub unsafe extern "C" fn glGenRenderbuffers(n: GLsizei, renderbuffers: *mut GLuint) {
    if n <= 0 || renderbuffers.is_null() {
        return;
    }
    for i in 0..n as usize {
        *renderbuffers.add(i) = (i + 1) as GLuint;
    }
}

#[no_mangle]
pub unsafe extern "C" fn glBindRenderbuffer(_target: GLenum, _renderbuffer: GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glRenderbufferStorage(
    _target: GLenum,
    _internalformat: GLenum,
    _width: GLsizei,
    _height: GLsizei,
) {
}

#[no_mangle]
pub unsafe extern "C" fn glDeleteRenderbuffers(_n: GLsizei, _renderbuffers: *const GLuint) {}

#[no_mangle]
pub unsafe extern "C" fn glGenerateMipmap(_target: GLenum) {}

#[no_mangle]
pub unsafe extern "C" fn glGenQueries(n: GLsizei, ids: *mut GLuint) {
    if n <= 0 || ids.is_null() {
        return;
    }
    for i in 0..n as usize {
        *ids.add(i) = (i + 1) as GLuint;
    }
}

#[no_mangle]
pub unsafe extern "C" fn glGenQueriesARB(n: GLsizei, ids: *mut GLuint) {
    glGenQueries(n, ids);
}

#[no_mangle]
pub unsafe extern "C" fn glBeginQuery(_target: GLenum, _id: GLuint) {}
#[no_mangle]
pub unsafe extern "C" fn glBeginQueryARB(target: GLenum, id: GLuint) {
    glBeginQuery(target, id);
}

#[no_mangle]
pub unsafe extern "C" fn glEndQuery(_target: GLenum) {}
#[no_mangle]
pub unsafe extern "C" fn glEndQueryARB(target: GLenum) {
    glEndQuery(target);
}

#[no_mangle]
pub unsafe extern "C" fn glGetQueryObjectuiv(_id: GLuint, _pname: GLenum, params: *mut GLuint) {
    if !params.is_null() {
        *params = 1;
    }
}

#[no_mangle]
pub unsafe extern "C" fn glGetQueryObjectuivARB(id: GLuint, pname: GLenum, params: *mut GLuint) {
    glGetQueryObjectuiv(id, pname, params);
}

#[no_mangle]
pub unsafe extern "C" fn glDeleteQueries(_n: GLsizei, _ids: *const GLuint) {}
#[no_mangle]
pub unsafe extern "C" fn glDeleteQueriesARB(n: GLsizei, ids: *const GLuint) {
    glDeleteQueries(n, ids);
}

// ============================================================================
// EGL Exports
// ============================================================================

#[no_mangle]
pub unsafe extern "C" fn eglGetDisplay(display_id: NativeDisplayType) -> EGLDisplay {
    egl_get_display(display_id)
}

#[no_mangle]
pub unsafe extern "C" fn eglInitialize(
    dpy: EGLDisplay,
    major: *mut EGLint,
    minor: *mut EGLint,
) -> EGLBoolean {
    egl_initialize(dpy, major, minor)
}

#[no_mangle]
pub unsafe extern "C" fn eglTerminate(dpy: EGLDisplay) -> EGLBoolean {
    egl_terminate(dpy)
}

#[no_mangle]
pub unsafe extern "C" fn eglGetConfigs(
    dpy: EGLDisplay,
    configs: *mut EGLConfig,
    config_size: EGLint,
    num_config: *mut EGLint,
) -> EGLBoolean {
    egl_get_configs(dpy, configs, config_size, num_config)
}

#[no_mangle]
pub unsafe extern "C" fn eglChooseConfig(
    dpy: EGLDisplay,
    attrib_list: *const EGLint,
    configs: *mut EGLConfig,
    config_size: EGLint,
    num_config: *mut EGLint,
) -> EGLBoolean {
    egl_choose_config(dpy, attrib_list, configs, config_size, num_config)
}

#[no_mangle]
pub unsafe extern "C" fn eglGetConfigAttrib(
    dpy: EGLDisplay,
    config: EGLConfig,
    attribute: EGLint,
    value: *mut EGLint,
) -> EGLBoolean {
    egl_get_config_attrib(dpy, config, attribute, value)
}

#[no_mangle]
pub unsafe extern "C" fn eglCreateWindowSurface(
    dpy: EGLDisplay,
    config: EGLConfig,
    win: NativeWindowType,
    attrib_list: *const EGLint,
) -> EGLSurface {
    egl_create_window_surface(dpy, config, win, attrib_list)
}

#[no_mangle]
pub unsafe extern "C" fn eglCreatePbufferSurface(
    dpy: EGLDisplay,
    config: EGLConfig,
    attrib_list: *const EGLint,
) -> EGLSurface {
    egl_create_pbuffer_surface(dpy, config, attrib_list)
}

#[no_mangle]
pub unsafe extern "C" fn eglDestroySurface(dpy: EGLDisplay, surface: EGLSurface) -> EGLBoolean {
    egl_destroy_surface(dpy, surface)
}

#[no_mangle]
pub unsafe extern "C" fn eglCreateContext(
    dpy: EGLDisplay,
    config: EGLConfig,
    share_context: EGLContext,
    attrib_list: *const EGLint,
) -> EGLContext {
    egl_create_context(dpy, config, share_context, attrib_list)
}

#[no_mangle]
pub unsafe extern "C" fn eglDestroyContext(dpy: EGLDisplay, ctx: EGLContext) -> EGLBoolean {
    egl_destroy_context(dpy, ctx)
}

#[no_mangle]
pub unsafe extern "C" fn eglMakeCurrent(
    dpy: EGLDisplay,
    draw: EGLSurface,
    read: EGLSurface,
    ctx: EGLContext,
) -> EGLBoolean {
    egl_make_current(dpy, draw, read, ctx)
}

#[no_mangle]
pub unsafe extern "C" fn eglGetCurrentContext() -> EGLContext {
    egl_get_current_context()
}

#[no_mangle]
pub unsafe extern "C" fn eglGetCurrentSurface(readdraw: EGLint) -> EGLSurface {
    egl_get_current_surface(readdraw)
}

#[no_mangle]
pub unsafe extern "C" fn eglGetCurrentDisplay() -> EGLDisplay {
    egl_get_current_display()
}

#[no_mangle]
pub unsafe extern "C" fn eglQuerySurface(
    dpy: EGLDisplay,
    surface: EGLSurface,
    attribute: EGLint,
    value: *mut EGLint,
) -> EGLBoolean {
    egl_query_surface(dpy, surface, attribute, value)
}

#[no_mangle]
pub unsafe extern "C" fn eglSwapBuffers(dpy: EGLDisplay, surface: EGLSurface) -> EGLBoolean {
    egl_swap_buffers(dpy, surface)
}

#[no_mangle]
pub unsafe extern "C" fn eglSwapInterval(dpy: EGLDisplay, interval: EGLint) -> EGLBoolean {
    egl_swap_interval(dpy, interval)
}

#[no_mangle]
pub unsafe extern "C" fn eglGetError() -> EGLint {
    egl_get_error()
}

#[no_mangle]
pub unsafe extern "C" fn eglGetProcAddress(
    procname: *const c_char,
) -> __eglMustCastToProperFunctionPointerType {
    egl_get_proc_address(procname)
}

pub fn get_gl_proc_address(name: &str) -> __eglMustCastToProperFunctionPointerType {
    let ptr = match name {
        "glMatrixMode" => glMatrixMode as *const (),
        "glLoadIdentity" => glLoadIdentity as *const (),
        "glPushMatrix" => glPushMatrix as *const (),
        "glPopMatrix" => glPopMatrix as *const (),
        "glTranslatef" => glTranslatef as *const (),
        "glRotatef" => glRotatef as *const (),
        "glScalef" => glScalef as *const (),
        "glOrtho" => glOrtho as *const (),
        "glOrthof" => glOrthof as *const (),
        "glFrustum" => glFrustum as *const (),
        "glFrustumf" => glFrustumf as *const (),
        "glMultMatrixf" => glMultMatrixf as *const (),
        "glLoadMatrixf" => glLoadMatrixf as *const (),
        "glEnableClientState" => glEnableClientState as *const (),
        "glDisableClientState" => glDisableClientState as *const (),
        "glVertexPointer" => glVertexPointer as *const (),
        "glTexCoordPointer" => glTexCoordPointer as *const (),
        "glColorPointer" => glColorPointer as *const (),
        "glNormalPointer" => glNormalPointer as *const (),
        "glClientActiveTexture" => glClientActiveTexture as *const (),
        "glDrawArrays" => glDrawArrays as *const (),
        "glDrawElements" => glDrawElements as *const (),
        "glBegin" => glBegin as *const (),
        "glEnd" => glEnd as *const (),
        "glVertex3f" => glVertex3f as *const (),
        "glVertex2f" => glVertex2f as *const (),
        "glTexCoord2f" => glTexCoord2f as *const (),
        "glColor4f" => glColor4f as *const (),
        "glColor3f" => glColor3f as *const (),
        "glColor4ub" => glColor4ub as *const (),
        "glNormal3f" => glNormal3f as *const (),
        "glGenLists" => glGenLists as *const (),
        "glDeleteLists" => glDeleteLists as *const (),
        "glNewList" => glNewList as *const (),
        "glEndList" => glEndList as *const (),
        "glCallList" => glCallList as *const (),
        "glCallLists" => glCallLists as *const (),
        "glGenTextures" => glGenTextures as *const (),
        "glDeleteTextures" => glDeleteTextures as *const (),
        "glBindTexture" => glBindTexture as *const (),
        "glTexImage2D" => glTexImage2D as *const (),
        "glTexSubImage2D" => glTexSubImage2D as *const (),
        "glTexParameteri" => glTexParameteri as *const (),
        "glTexParameterf" => glTexParameterf as *const (),
        "glActiveTexture" => glActiveTexture as *const (),
        "glEnable" => glEnable as *const (),
        "glDisable" => glDisable as *const (),
        "glIsEnabled" => glIsEnabled as *const (),
        "glAlphaFunc" => glAlphaFunc as *const (),
        "glBlendFunc" => glBlendFunc as *const (),
        "glBlendColor" => glBlendColor as *const (),
        "glDepthFunc" => glDepthFunc as *const (),
        "glDepthMask" => glDepthMask as *const (),
        "glColorMask" => glColorMask as *const (),
        "glCullFace" => glCullFace as *const (),
        "glFrontFace" => glFrontFace as *const (),
        "glPolygonOffset" => glPolygonOffset as *const (),
        "glLineWidth" => glLineWidth as *const (),
        "glPointSize" => glPointSize as *const (),
        "glShadeModel" => glShadeModel as *const (),
        "glFogf" => glFogf as *const (),
        "glFogfv" => glFogfv as *const (),
        "glFogi" => glFogi as *const (),
        "glLightf" => glLightf as *const (),
        "glLightfv" => glLightfv as *const (),
        "glLightModelfv" => glLightModelfv as *const (),
        "glStencilFunc" => glStencilFunc as *const (),
        "glStencilMask" => glStencilMask as *const (),
        "glStencilOp" => glStencilOp as *const (),
        "glViewport" => glViewport as *const (),
        "glScissor" => glScissor as *const (),
        "glClearColor" => glClearColor as *const (),
        "glClearDepthf" => glClearDepthf as *const (),
        "glClear" => glClear as *const (),
        "glReadPixels" => glReadPixels as *const (),
        "glFlush" => glFlush as *const (),
        "glFinish" => glFinish as *const (),
        "glGetIntegerv" => glGetIntegerv as *const (),
        "glGetFloatv" => glGetFloatv as *const (),
        "glGetBooleanv" => glGetBooleanv as *const (),
        "glGetString" => glGetString as *const (),
        "glGetError" => glGetError as *const (),
        "glCreateShader" => glCreateShader as *const (),
        "glShaderSource" => glShaderSource as *const (),
        "glCompileShader" => glCompileShader as *const (),
        "glGetShaderiv" => glGetShaderiv as *const (),
        "glDeleteShader" => glDeleteShader as *const (),
        "glCreateProgram" => glCreateProgram as *const (),
        "glAttachShader" => glAttachShader as *const (),
        "glLinkProgram" => glLinkProgram as *const (),
        "glGetProgramiv" => glGetProgramiv as *const (),
        "glUseProgram" => glUseProgram as *const (),
        "glDeleteProgram" => glDeleteProgram as *const (),
        "glGenBuffers" => glGenBuffers as *const (),
        "glBindBuffer" => glBindBuffer as *const (),
        "glBufferData" => glBufferData as *const (),
        "glDeleteBuffers" => glDeleteBuffers as *const (),
        "glGenFramebuffers" => glGenFramebuffers as *const (),
        "glBindFramebuffer" => glBindFramebuffer as *const (),
        "glFramebufferTexture2D" => glFramebufferTexture2D as *const (),
        "glDeleteFramebuffers" => glDeleteFramebuffers as *const (),
        "eglGetDisplay" => eglGetDisplay as *const (),
        "eglInitialize" => eglInitialize as *const (),
        "eglTerminate" => eglTerminate as *const (),
        "eglChooseConfig" => eglChooseConfig as *const (),
        "eglCreateWindowSurface" => eglCreateWindowSurface as *const (),
        "eglCreateContext" => eglCreateContext as *const (),
        "eglMakeCurrent" => eglMakeCurrent as *const (),
        "eglSwapBuffers" => eglSwapBuffers as *const (),
        "eglGetProcAddress" => eglGetProcAddress as *const (),
        _ => std::ptr::null(),
    };

    if ptr.is_null() {
        None
    } else {
        Some(unsafe { std::mem::transmute(ptr) })
    }
}
