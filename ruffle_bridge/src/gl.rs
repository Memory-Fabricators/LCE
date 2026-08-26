#![allow(non_upper_case_globals)]

pub(crate) type GLenum = u32;
pub(crate) type GLbitfield = u32;
pub(crate) type GLint = i32;
pub(crate) type GLsizei = i32;
pub(crate) type GLfloat = f32;
pub(crate) type GLdouble = f64;
pub(crate) type GLboolean = u8;
pub(crate) type GLuint = u32;
pub(crate) type GLvoid = std::ffi::c_void;

pub(crate) const GL_NO_ERROR: GLenum = 0;
pub(crate) const GL_STENCIL_BITS: GLenum = 0x0D57;
pub(crate) const GL_VIEWPORT: GLenum = 0x0BA2;
pub(crate) const GL_MATRIX_MODE: GLenum = 0x0BA0;
pub(crate) const GL_COLOR_BUFFER_BIT: GLbitfield = 0x0000_4000;
pub(crate) const GL_DEPTH_BUFFER_BIT: GLbitfield = 0x0000_0100;
pub(crate) const GL_STENCIL_BUFFER_BIT: GLbitfield = 0x0000_0400;
pub(crate) const GL_PROJECTION: GLenum = 0x1701;
pub(crate) const GL_MODELVIEW: GLenum = 0x1700;
pub(crate) const GL_TRIANGLE_FAN: GLenum = 0x0006;
pub(crate) const GL_TRIANGLES: GLenum = 0x0004;
pub(crate) const GL_LINES: GLenum = 0x0001;
pub(crate) const GL_BLEND: GLenum = 0x0BE2;
pub(crate) const GL_SRC_ALPHA: GLenum = 0x0302;
pub(crate) const GL_ONE_MINUS_SRC_ALPHA: GLenum = 0x0303;
pub(crate) const GL_ONE: GLenum = 1;
pub(crate) const GL_ALL_ATTRIB_BITS: GLbitfield = 0xFFFF_FFFF;
pub(crate) const GL_CLIENT_ALL_ATTRIB_BITS: GLbitfield = 0xFFFF_FFFF;
pub(crate) const GL_SCISSOR_TEST: GLenum = 0x0C11;
pub(crate) const GL_DEPTH_TEST: GLenum = 0x0B71;
pub(crate) const GL_CULL_FACE: GLenum = 0x0B44;
pub(crate) const GL_ALPHA_TEST: GLenum = 0x0BC0;
pub(crate) const GL_TEXTURE_2D: GLenum = 0x0DE1;
pub(crate) const GL_UNPACK_ALIGNMENT: GLenum = 0x0CF5;
pub(crate) const GL_RGBA: GLenum = 0x1908;
pub(crate) const GL_UNSIGNED_BYTE: GLenum = 0x1401;
pub(crate) const GL_TEXTURE_MIN_FILTER: GLenum = 0x2801;
pub(crate) const GL_TEXTURE_MAG_FILTER: GLenum = 0x2800;
pub(crate) const GL_TEXTURE_WRAP_S: GLenum = 0x2802;
pub(crate) const GL_TEXTURE_WRAP_T: GLenum = 0x2803;
pub(crate) const GL_LINEAR: GLenum = 0x2601;
pub(crate) const GL_NEAREST: GLenum = 0x2600;
pub(crate) const GL_CLAMP: GLenum = 0x2900;
pub(crate) const GL_CURRENT_PROGRAM: GLenum = 0x8B8D;
pub(crate) const GL_STENCIL_TEST: GLenum = 0x0B90;
pub(crate) const GL_ALWAYS: GLenum = 0x0207;
pub(crate) const GL_EQUAL: GLenum = 0x0202;
pub(crate) const GL_KEEP: GLenum = 0x1E00;
pub(crate) const GL_REPLACE: GLenum = 0x1E01;

unsafe extern "C" {
    pub(crate) fn glGetError() -> GLenum;
    pub(crate) fn glGetIntegerv(pname: GLenum, data: *mut GLint);
    pub(crate) fn glViewport(x: GLint, y: GLint, width: GLsizei, height: GLsizei);
    pub(crate) fn glTexCoord2f(s: GLfloat, t: GLfloat);
    pub(crate) fn glMatrixMode(mode: GLenum);
    pub(crate) fn glLoadIdentity();
    pub(crate) fn glOrtho(
        left: GLdouble,
        right: GLdouble,
        bottom: GLdouble,
        top: GLdouble,
        z_near: GLdouble,
        z_far: GLdouble,
    );
    pub(crate) fn glClearColor(red: GLfloat, green: GLfloat, blue: GLfloat, alpha: GLfloat);
    pub(crate) fn glClear(mask: GLbitfield);
    pub(crate) fn glEnable(cap: GLenum);
    pub(crate) fn glDisable(cap: GLenum);
    pub(crate) fn glBlendFunc(sfactor: GLenum, dfactor: GLenum);
    pub(crate) fn glColor4f(red: GLfloat, green: GLfloat, blue: GLfloat, alpha: GLfloat);
    pub(crate) fn glBegin(mode: GLenum);
    pub(crate) fn glVertex2f(x: GLfloat, y: GLfloat);
    pub(crate) fn glEnd();
    pub(crate) fn glPushAttrib(mask: GLbitfield);
    pub(crate) fn glPopAttrib();
    pub(crate) fn glPushClientAttrib(mask: GLbitfield);
    pub(crate) fn glPopClientAttrib();
    pub(crate) fn glPushMatrix();
    pub(crate) fn glPopMatrix();
    pub(crate) fn glUseProgram(program: u32);
    pub(crate) fn glColorMask(red: GLboolean, green: GLboolean, blue: GLboolean, alpha: GLboolean);
    pub(crate) fn glGenTextures(n: GLsizei, textures: *mut GLuint);
    pub(crate) fn glBindTexture(target: GLenum, texture: GLuint);
    pub(crate) fn glTexParameteri(target: GLenum, pname: GLenum, param: GLint);
    pub(crate) fn glTexImage2D(
        target: GLenum,
        level: GLint,
        internal_format: GLint,
        width: GLsizei,
        height: GLsizei,
        border: GLint,
        format: GLenum,
        ty: GLenum,
        pixels: *const GLvoid,
    );
    pub(crate) fn glTexSubImage2D(
        target: GLenum,
        level: GLint,
        xoffset: GLint,
        yoffset: GLint,
        width: GLsizei,
        height: GLsizei,
        format: GLenum,
        ty: GLenum,
        pixels: *const GLvoid,
    );
    pub(crate) fn glPixelStorei(pname: GLenum, param: GLint);
    pub(crate) fn glStencilFunc(func: GLenum, reference: GLint, mask: GLuint);
    pub(crate) fn glStencilOp(fail: GLenum, zfail: GLenum, zpass: GLenum);
}
