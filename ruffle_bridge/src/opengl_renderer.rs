use crate::gl;
use ruffle_render::backend::{
    Context3D, Context3DProfile, PixelBenderOutput, PixelBenderTarget, RenderBackend, ShapeHandle,
    ShapeHandleImpl, ViewportDimensions,
};
use ruffle_render::bitmap::{
    Bitmap, BitmapHandle, BitmapHandleImpl, BitmapSource, PixelRegion, RgbaBufRead, SyncHandle,
};
use ruffle_render::commands::{CommandHandler, CommandList, RenderBlendMode};
use ruffle_render::error::Error;
use ruffle_render::matrix::Matrix;
use ruffle_render::pixel_bender::{PixelBenderShader, PixelBenderShaderHandle};
use ruffle_render::pixel_bender_support::PixelBenderShaderArgument;
use ruffle_render::quality::StageQuality;
use ruffle_render::shape_utils::DistilledShape;
use std::borrow::Cow;
use std::num::NonZeroU32;
use std::sync::Arc;
use swf::{BlendMode, Color};

enum OpenGlDrawKind {
    Color,
    Bitmap {
        texture: gl::GLuint,
        matrix: [[f32; 3]; 3],
        smoothed: bool,
    },
}
struct OpenGlDraw {
    kind: OpenGlDrawKind,
    vertices: Vec<ruffle_render::tessellator::Vertex>,
    indices: Vec<u32>,
}
struct OpenGlShape {
    draws: Vec<OpenGlDraw>,
}
impl std::fmt::Debug for OpenGlShape {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        formatter
            .debug_struct("OpenGlShape")
            .finish_non_exhaustive()
    }
}
impl ShapeHandleImpl for OpenGlShape {}
#[derive(Debug)]
struct OpenGlBitmap {
    width: u32,
    height: u32,
    texture: gl::GLuint,
}
impl BitmapHandleImpl for OpenGlBitmap {}

/// Restores the compatibility state changed while executing a bridge frame.
struct OpenGlStateGuard {
    program: i32,
    matrix_mode: i32,
}
impl OpenGlStateGuard {
    unsafe fn new() -> Self {
        let mut program = 0;
        let mut matrix_mode = 0;
        unsafe {
            gl::glGetIntegerv(gl::GL_CURRENT_PROGRAM, &mut program);
            gl::glGetIntegerv(gl::GL_MATRIX_MODE, &mut matrix_mode);
            gl::glPushAttrib(gl::GL_ALL_ATTRIB_BITS);
            gl::glPushClientAttrib(gl::GL_CLIENT_ALL_ATTRIB_BITS);
            gl::glMatrixMode(gl::GL_PROJECTION);
            gl::glPushMatrix();
            gl::glMatrixMode(gl::GL_MODELVIEW);
            gl::glPushMatrix();
        }
        Self {
            program,
            matrix_mode,
        }
    }
}
impl Drop for OpenGlStateGuard {
    fn drop(&mut self) {
        unsafe {
            gl::glMatrixMode(gl::GL_MODELVIEW);
            gl::glPopMatrix();
            gl::glMatrixMode(gl::GL_PROJECTION);
            gl::glPopMatrix();
            gl::glPopClientAttrib();
            gl::glPopAttrib();
            gl::glUseProgram(self.program as u32);
            gl::glMatrixMode(self.matrix_mode as u32);
        }
    }
}

pub struct OpenGlRenderer {
    dimensions: ViewportDimensions,
    quality: StageQuality,
    shape_tessellator: ruffle_render::tessellator::ShapeTessellator,
}
impl OpenGlRenderer {
    pub fn new(width: u32, height: u32) -> Result<Self, ()> {
        if width == 0 || height == 0 {
            return Err(());
        }
        let mut stencil_bits = 0;
        unsafe {
            gl::glGetIntegerv(gl::GL_STENCIL_BITS, &mut stencil_bits);
        }
        let _guard = unsafe { OpenGlStateGuard::new() };
        if stencil_bits < 8 {
            return Err(());
        }
        let mut renderer = Self {
            dimensions: ViewportDimensions {
                width,
                height,
                scale_factor: 1.0,
            },
            quality: StageQuality::High,
            shape_tessellator: ruffle_render::tessellator::ShapeTessellator::new(),
        };
        renderer.configure_viewport();
        Ok(renderer)
    }
    fn configure_viewport(&mut self) {
        unsafe {
            gl::glViewport(
                0,
                0,
                self.dimensions.width as i32,
                self.dimensions.height as i32,
            );
            gl::glMatrixMode(gl::GL_PROJECTION);
            gl::glLoadIdentity();
            gl::glOrtho(
                0.0,
                self.dimensions.width as f64,
                self.dimensions.height as f64,
                0.0,
                -1.0,
                1.0,
            );
            gl::glMatrixMode(gl::GL_MODELVIEW);
            gl::glLoadIdentity();
        }
    }
    pub fn submit_probe(&mut self) {
        let mut commands = CommandList::new();
        commands.draw_rect(
            Color {
                r: 255,
                g: 0,
                b: 255,
                a: 255,
            },
            Matrix {
                a: 64.0,
                b: 0.0,
                c: 0.0,
                d: 64.0,
                tx: swf::Twips::from_pixels(16.0),
                ty: swf::Twips::from_pixels(16.0),
            },
        );
        self.submit_frame(Color::TRANSPARENT, commands, vec![]);
    }
    fn create_texture(
        width: u32,
        height: u32,
        pixels: Option<&[u8]>,
    ) -> Result<OpenGlBitmap, Error> {
        let mut texture = 0;
        unsafe {
            gl::glGenTextures(1, &mut texture);
            if texture == 0 {
                return Err(Error::Unimplemented("OpenGL texture allocation".into()));
            }
            gl::glBindTexture(gl::GL_TEXTURE_2D, texture);
            gl::glPixelStorei(gl::GL_UNPACK_ALIGNMENT, 1);
            gl::glTexParameteri(
                gl::GL_TEXTURE_2D,
                gl::GL_TEXTURE_MIN_FILTER,
                gl::GL_LINEAR as i32,
            );
            gl::glTexParameteri(
                gl::GL_TEXTURE_2D,
                gl::GL_TEXTURE_MAG_FILTER,
                gl::GL_LINEAR as i32,
            );
            gl::glTexParameteri(
                gl::GL_TEXTURE_2D,
                gl::GL_TEXTURE_WRAP_S,
                gl::GL_CLAMP as i32,
            );
            gl::glTexParameteri(
                gl::GL_TEXTURE_2D,
                gl::GL_TEXTURE_WRAP_T,
                gl::GL_CLAMP as i32,
            );
            gl::glTexImage2D(
                gl::GL_TEXTURE_2D,
                0,
                gl::GL_RGBA as i32,
                width as i32,
                height as i32,
                0,
                gl::GL_RGBA,
                gl::GL_UNSIGNED_BYTE,
                pixels.map_or(std::ptr::null(), |data| data.as_ptr().cast()),
            );
        }
        Ok(OpenGlBitmap {
            width,
            height,
            texture,
        })
    }
}

struct Handler {
    mask_depth: i32,
}
impl Handler {
    fn vertex(matrix: Matrix, x: f32, y: f32) {
        unsafe {
            gl::glVertex2f(
                matrix.a * x + matrix.c * y + matrix.tx.to_pixels() as f32,
                matrix.b * x + matrix.d * y + matrix.ty.to_pixels() as f32,
            );
        }
    }
}
impl CommandHandler for Handler {
    fn render_bitmap(
        &mut self,
        bitmap: BitmapHandle,
        transform: ruffle_render::transform::Transform,
        smoothing: bool,
        pixel_snapping: ruffle_render::bitmap::PixelSnapping,
        _region: ruffle_render::bitmap::PixelRegion,
    ) {
        let Some(texture) =
            (bitmap.0.as_ref() as &dyn std::any::Any).downcast_ref::<OpenGlBitmap>()
        else {
            eprintln!("Ruffle OpenGL: foreign bitmap handle");
            return;
        };
        let mut matrix = transform.matrix;
        pixel_snapping.apply(&mut matrix);
        matrix *= Matrix::scale(texture.width as f32, texture.height as f32);

        let mult = transform.color_transform.mult_rgba_normalized();
        let filter = if smoothing {
            gl::GL_LINEAR
        } else {
            gl::GL_NEAREST
        } as i32;
        unsafe {
            gl::glEnable(gl::GL_TEXTURE_2D);
            gl::glBindTexture(gl::GL_TEXTURE_2D, texture.texture);
            gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, filter);
            gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, filter);
            gl::glColor4f(mult[0], mult[1], mult[2], mult[3]);
            gl::glBegin(gl::GL_TRIANGLE_FAN);
            gl::glTexCoord2f(0.0, 0.0);
            Handler::vertex(matrix, 0.0, 0.0);
            gl::glTexCoord2f(1.0, 0.0);
            Handler::vertex(matrix, 1.0, 0.0);
            gl::glTexCoord2f(1.0, 1.0);
            Handler::vertex(matrix, 1.0, 1.0);
            gl::glTexCoord2f(0.0, 1.0);
            Handler::vertex(matrix, 0.0, 1.0);
            gl::glEnd();
            gl::glDisable(gl::GL_TEXTURE_2D);
        }
    }
    fn render_stage3d(&mut self, _: BitmapHandle, _: ruffle_render::transform::Transform) {
        eprintln!("Ruffle OpenGL: Context3D is unsupported");
    }
    fn render_shape(&mut self, shape: ShapeHandle, transform: ruffle_render::transform::Transform) {
        let Some(shape) = (shape.0.as_ref() as &dyn std::any::Any).downcast_ref::<OpenGlShape>()
        else {
            eprintln!("Ruffle OpenGL: foreign shape handle");
            return;
        };
        let mult = transform.color_transform.mult_rgba_normalized();
        for draw in &shape.draws {
            match &draw.kind {
                OpenGlDrawKind::Color => unsafe {
                    gl::glBegin(gl::GL_TRIANGLES);
                    for index in &draw.indices {
                        let vertex = &draw.vertices[*index as usize];
                        gl::glColor4f(
                            vertex.color.r as f32 / 255.0 * mult[0],
                            vertex.color.g as f32 / 255.0 * mult[1],
                            vertex.color.b as f32 / 255.0 * mult[2],
                            vertex.color.a as f32 / 255.0 * mult[3],
                        );
                        Handler::vertex(transform.matrix, vertex.x, vertex.y);
                    }
                    gl::glEnd();
                },
                OpenGlDrawKind::Bitmap {
                    texture,
                    matrix,
                    smoothed,
                } => unsafe {
                    let filter = if *smoothed {
                        gl::GL_LINEAR
                    } else {
                        gl::GL_NEAREST
                    } as i32;
                    gl::glEnable(gl::GL_TEXTURE_2D);
                    gl::glBindTexture(gl::GL_TEXTURE_2D, *texture);
                    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, filter);
                    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, filter);
                    gl::glBegin(gl::GL_TRIANGLES);
                    for index in &draw.indices {
                        let vertex = &draw.vertices[*index as usize];
                        let u = matrix[0][0] * vertex.x + matrix[1][0] * vertex.y + matrix[2][0];
                        let v = matrix[0][1] * vertex.x + matrix[1][1] * vertex.y + matrix[2][1];
                        gl::glColor4f(
                            vertex.color.r as f32 / 255.0 * mult[0],
                            vertex.color.g as f32 / 255.0 * mult[1],
                            vertex.color.b as f32 / 255.0 * mult[2],
                            vertex.color.a as f32 / 255.0 * mult[3],
                        );
                        gl::glTexCoord2f(u, v);
                        Handler::vertex(transform.matrix, vertex.x, vertex.y);
                    }
                    gl::glEnd();
                    gl::glDisable(gl::GL_TEXTURE_2D);
                },
            }
        }
    }
    fn render_alpha_mask(&mut self, _: CommandList, _: CommandList) {
        eprintln!("Ruffle OpenGL: alpha masks are unsupported");
    }
    fn draw_rect(&mut self, color: Color, matrix: Matrix) {
        unsafe {
            gl::glColor4f(
                color.r as f32 / 255.0,
                color.g as f32 / 255.0,
                color.b as f32 / 255.0,
                color.a as f32 / 255.0,
            );
            gl::glBegin(gl::GL_TRIANGLE_FAN);
            Self::vertex(matrix, 0.0, 0.0);
            Self::vertex(matrix, 1.0, 0.0);
            Self::vertex(matrix, 1.0, 1.0);
            Self::vertex(matrix, 0.0, 1.0);
            gl::glEnd();
        }
    }
    fn draw_line(&mut self, color: Color, matrix: Matrix) {
        unsafe {
            gl::glColor4f(
                color.r as f32 / 255.0,
                color.g as f32 / 255.0,
                color.b as f32 / 255.0,
                color.a as f32 / 255.0,
            );
            gl::glBegin(gl::GL_LINES);
            Self::vertex(matrix, 0.0, 0.0);
            Self::vertex(matrix, 1.0, 1.0);
            gl::glEnd();
        }
    }
    fn draw_line_rect(&mut self, color: Color, matrix: Matrix) {
        self.draw_rect(color, matrix);
    }
    fn push_mask(&mut self) {
        unsafe {
            gl::glEnable(gl::GL_STENCIL_TEST);
            gl::glColorMask(0, 0, 0, 0);
            gl::glStencilFunc(gl::GL_ALWAYS, self.mask_depth + 1, 0xFF);
            gl::glStencilOp(gl::GL_KEEP, gl::GL_KEEP, gl::GL_REPLACE);
        }
    }
    fn activate_mask(&mut self) {
        self.mask_depth += 1;
        unsafe {
            gl::glColorMask(1, 1, 1, 1);
            gl::glStencilFunc(gl::GL_EQUAL, self.mask_depth, 0xFF);
            gl::glStencilOp(gl::GL_KEEP, gl::GL_KEEP, gl::GL_KEEP);
        }
    }
    fn deactivate_mask(&mut self) {}
    fn pop_mask(&mut self) {
        self.mask_depth -= 1;
        unsafe {
            if self.mask_depth <= 0 {
                self.mask_depth = 0;
                gl::glDisable(gl::GL_STENCIL_TEST);
            } else {
                gl::glStencilFunc(gl::GL_EQUAL, self.mask_depth, 0xFF);
                gl::glStencilOp(gl::GL_KEEP, gl::GL_KEEP, gl::GL_KEEP);
            }
        }
    }
    fn blend(&mut self, commands: CommandList, blend: RenderBlendMode) {
        match blend {
            RenderBlendMode::Builtin(BlendMode::Normal | BlendMode::Layer) => {
                commands.execute(self)
            }
            RenderBlendMode::Builtin(BlendMode::Add) => unsafe {
                gl::glBlendFunc(gl::GL_ONE, gl::GL_ONE);
                commands.execute(self);
                gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);
            },
            _ => eprintln!("Ruffle OpenGL: unsupported blend mode"),
        }
    }
}

impl RenderBackend for OpenGlRenderer {
    fn viewport_dimensions(&self) -> ViewportDimensions {
        self.dimensions
    }
    fn set_viewport_dimensions(&mut self, dimensions: ViewportDimensions) {
        if dimensions.width != 0 && dimensions.height != 0 {
            self.dimensions = dimensions;
            self.configure_viewport();
        }
    }
    fn register_shape(
        &mut self,
        shape: DistilledShape,
        bitmap_source: &dyn BitmapSource,
    ) -> ShapeHandle {
        let mesh = self
            .shape_tessellator
            .tessellate_shape(shape, bitmap_source);
        let mut draws = Vec::with_capacity(mesh.draws.len());
        for draw in mesh.draws {
            let kind = match &draw.draw_type {
                ruffle_render::tessellator::DrawType::Color => Some(OpenGlDrawKind::Color),
                ruffle_render::tessellator::DrawType::Bitmap(bitmap) => bitmap_source
                    .bitmap_handle(bitmap.bitmap_id, self)
                    .and_then(|handle| {
                        (handle.0.as_ref() as &dyn std::any::Any)
                            .downcast_ref::<OpenGlBitmap>()
                            .map(|texture| OpenGlDrawKind::Bitmap {
                                texture: texture.texture,
                                matrix: bitmap.matrix,
                                smoothed: bitmap.is_smoothed,
                            })
                    })
                    .or_else(|| {
                        eprintln!(
                            "Ruffle OpenGL: unresolved bitmap fill (id {})",
                            bitmap.bitmap_id
                        );
                        None
                    }),
                ruffle_render::tessellator::DrawType::Gradient { .. } => {
                    eprintln!("Ruffle OpenGL: gradient-filled shapes are unsupported");
                    None
                }
            };
            if let Some(kind) = kind {
                draws.push(OpenGlDraw {
                    kind,
                    vertices: draw.vertices,
                    indices: draw.indices,
                });
            }
        }
        ShapeHandle(Arc::new(OpenGlShape { draws }))
    }
    fn render_offscreen(
        &mut self,
        _: BitmapHandle,
        _: CommandList,
        _: StageQuality,
        _: PixelRegion,
    ) -> Option<Box<dyn SyncHandle>> {
        None
    }
    fn submit_frame(
        &mut self,
        clear: Color,
        commands: CommandList,
        cache_entries: Vec<ruffle_render::backend::BitmapCacheEntry>,
    ) {
        let _guard = unsafe { OpenGlStateGuard::new() };
        if !cache_entries.is_empty() {
            eprintln!("Ruffle OpenGL: bitmap cache entries are unsupported");
        }
        self.configure_viewport();
        unsafe {
            gl::glUseProgram(0);
            gl::glDisable(gl::GL_SCISSOR_TEST);
            gl::glDisable(gl::GL_DEPTH_TEST);
            gl::glDisable(gl::GL_CULL_FACE);
            gl::glDisable(gl::GL_ALPHA_TEST);
            gl::glDisable(gl::GL_TEXTURE_2D);
            gl::glDisable(gl::GL_STENCIL_TEST);
            gl::glColorMask(1, 1, 1, 1);
            if clear.a > 0 {
                gl::glClearColor(
                    clear.r as f32 / 255.0,
                    clear.g as f32 / 255.0,
                    clear.b as f32 / 255.0,
                    clear.a as f32 / 255.0,
                );
                gl::glClear(
                    gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT | gl::GL_STENCIL_BUFFER_BIT,
                );
            } else {
                gl::glClear(gl::GL_DEPTH_BUFFER_BIT | gl::GL_STENCIL_BUFFER_BIT);
            }
            gl::glEnable(gl::GL_BLEND);
            gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);
        }
        let mut handler = Handler { mask_depth: 0 };
        commands.execute(&mut handler);
    }
    fn create_empty_texture(
        &mut self,
        width: NonZeroU32,
        height: NonZeroU32,
    ) -> Result<BitmapHandle, Error> {
        Ok(BitmapHandle(Arc::new(Self::create_texture(
            width.get(),
            height.get(),
            None,
        )?)))
    }
    fn register_bitmap(&mut self, bitmap: Bitmap<'_>) -> Result<BitmapHandle, Error> {
        let bitmap = bitmap.to_rgba();
        Ok(BitmapHandle(Arc::new(Self::create_texture(
            bitmap.width(),
            bitmap.height(),
            Some(bitmap.data()),
        )?)))
    }
    fn update_texture(
        &mut self,
        handle: &BitmapHandle,
        bitmap: Bitmap<'_>,
        region: PixelRegion,
    ) -> Result<(), Error> {
        let Some(texture) =
            (handle.0.as_ref() as &dyn std::any::Any).downcast_ref::<OpenGlBitmap>()
        else {
            return Err(Error::UnknownHandle(handle.clone()));
        };
        let bitmap = bitmap.to_rgba();
        if region.x_max > texture.width
            || region.y_max > texture.height
            || bitmap.width() != region.x_max - region.x_min
            || bitmap.height() != region.y_max - region.y_min
        {
            return Err(Error::Unimplemented("invalid texture update region".into()));
        }
        unsafe {
            gl::glBindTexture(gl::GL_TEXTURE_2D, texture.texture);
            gl::glPixelStorei(gl::GL_UNPACK_ALIGNMENT, 1);
            gl::glTexSubImage2D(
                gl::GL_TEXTURE_2D,
                0,
                region.x_min as i32,
                region.y_min as i32,
                bitmap.width() as i32,
                bitmap.height() as i32,
                gl::GL_RGBA,
                gl::GL_UNSIGNED_BYTE,
                bitmap.data().as_ptr().cast(),
            );
        }
        Ok(())
    }
    fn create_context3d(&mut self, _: Context3DProfile) -> Result<Box<dyn Context3D>, Error> {
        Err(Error::Unimplemented("Context3D".into()))
    }
    fn debug_info(&self) -> Cow<'static, str> {
        "Renderer: SDL3 OpenGL 1.1 compatibility".into()
    }
    fn name(&self) -> &'static str {
        "opengl"
    }
    fn set_quality(&mut self, quality: StageQuality) {
        self.quality = quality;
    }
    fn compile_pixelbender_shader(
        &mut self,
        _: PixelBenderShader,
    ) -> Result<PixelBenderShaderHandle, Error> {
        Err(Error::Unimplemented("Pixel Bender".into()))
    }
    fn run_pixelbender_shader(
        &mut self,
        _: PixelBenderShaderHandle,
        _: &[PixelBenderShaderArgument],
        _: &PixelBenderTarget,
    ) -> Result<PixelBenderOutput, Error> {
        Err(Error::Unimplemented("Pixel Bender".into()))
    }
    fn resolve_sync_handle(&mut self, _: Box<dyn SyncHandle>, _: RgbaBufRead) -> Result<(), Error> {
        Err(Error::Unimplemented("sync handles".into()))
    }
}
