//! 4x4 Matrix mathematics and OpenGL matrix stack emulation.
//!
//! Backed by `glam` (SIMD-accelerated) instead of a hand-rolled
//! implementation. `Mat4` keeps the OpenGL column-major `[f32; 16]` layout
//! `glam::Mat4` already uses, so this is a thin API-compatible wrapper for
//! the rest of the fixed-function emulation.

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum MatrixMode {
    ModelView = 0,
    Projection = 1,
    Texture = 2,
}

impl Default for MatrixMode {
    fn default() -> Self {
        Self::ModelView
    }
}

/// 4x4 float matrix stored in OpenGL column-major order.
///
/// Array indexing:
///   [ 0,  4,  8, 12 ]
///   [ 1,  5,  9, 13 ]
///   [ 2,  6, 10, 14 ]
///   [ 3,  7, 11, 15 ]
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct Mat4(pub glam::Mat4);

impl Default for Mat4 {
    fn default() -> Self {
        Self::IDENTITY
    }
}

impl Mat4 {
    pub const IDENTITY: Self = Self(glam::Mat4::IDENTITY);
    pub const ZERO: Self = Self(glam::Mat4::ZERO);

    #[inline]
    pub fn from_array(data: [f32; 16]) -> Self {
        Self(glam::Mat4::from_cols_array(&data))
    }

    #[inline]
    pub fn to_array(&self) -> [f32; 16] {
        self.0.to_cols_array()
    }

    /// Matrix product `self * rhs`, i.e. `rhs` is applied first (standard GL
    /// column-vector convention: `glTranslatef`/`glRotatef`/etc. all
    /// post-multiply the current matrix this way).
    #[inline]
    pub fn multiply(&self, rhs: &Self) -> Self {
        Self(self.0 * rhs.0)
    }

    #[inline]
    pub fn translate(&mut self, x: f32, y: f32, z: f32) {
        self.0 *= glam::Mat4::from_translation(glam::Vec3::new(x, y, z));
    }

    #[inline]
    pub fn rotate(&mut self, angle_deg: f32, mut x: f32, mut y: f32, mut z: f32) {
        let len = (x * x + y * y + z * z).sqrt();
        if len > 1e-6 {
            x /= len;
            y /= len;
            z /= len;
        }
        self.0 *= glam::Mat4::from_axis_angle(glam::Vec3::new(x, y, z), angle_deg.to_radians());
    }

    #[inline]
    pub fn scale(&mut self, x: f32, y: f32, z: f32) {
        self.0 *= glam::Mat4::from_scale(glam::Vec3::new(x, y, z));
    }

    /// Standard OpenGL glOrtho
    pub fn ortho(left: f32, right: f32, bottom: f32, top: f32, near: f32, far: f32) -> Self {
        if right - left == 0.0 || top - bottom == 0.0 || far - near == 0.0 {
            return Self::IDENTITY;
        }
        Self(glam::camera::rh::proj::opengl::orthographic(
            left, right, bottom, top, near, far,
        ))
    }

    /// Standard OpenGL glFrustum
    pub fn frustum(left: f32, right: f32, bottom: f32, top: f32, near: f32, far: f32) -> Self {
        let rl = right - left;
        let tb = top - bottom;
        let fn_diff = far - near;

        if rl == 0.0 || tb == 0.0 || fn_diff == 0.0 || near <= 0.0 || far <= 0.0 {
            return Self::IDENTITY;
        }

        // glam has no asymmetric-frustum constructor; this is the standard
        // OpenGL formula (matches `orthographic_rh_gl`'s clip-space convention).
        let mut m = [0.0f32; 16];
        m[0] = (2.0 * near) / rl;
        m[5] = (2.0 * near) / tb;
        m[8] = (right + left) / rl;
        m[9] = (top + bottom) / tb;
        m[10] = -(far + near) / fn_diff;
        m[11] = -1.0;
        m[14] = -(2.0 * far * near) / fn_diff;
        Self::from_array(m)
    }

    /// Standard gluPerspective
    pub fn perspective(fovy_deg: f32, aspect: f32, near: f32, far: f32) -> Self {
        let fovy_rad = fovy_deg.to_radians();
        if fovy_rad == 0.0 || aspect == 0.0 || far == near {
            return Self::IDENTITY;
        }
        Self(glam::camera::rh::proj::opengl::perspective(
            fovy_rad, aspect, near, far,
        ))
    }

    #[inline]
    pub fn transpose(&self) -> Self {
        Self(self.0.transpose())
    }

    pub fn inverse(&self) -> Option<Self> {
        if self.0.determinant().abs() < 1e-8 {
            return None;
        }
        Some(Self(self.0.inverse()))
    }

    /// Normal matrix (transpose of inverse of upper 3x3)
    pub fn normal_matrix_3x3(&self) -> [f32; 9] {
        if let Some(inv) = self.inverse() {
            let m = inv.to_array();
            [m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]]
        } else {
            [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
        }
    }
}

/// Stack of 4x4 matrices with a current top matrix.
#[derive(Debug, Clone)]
pub struct MatrixStack {
    stack: Vec<Mat4>,
    pub current: Mat4,
    max_depth: usize,
}

impl Default for MatrixStack {
    fn default() -> Self {
        Self::new(32)
    }
}

impl MatrixStack {
    pub fn new(max_depth: usize) -> Self {
        let mut stack = Vec::with_capacity(max_depth);
        stack.push(Mat4::IDENTITY);
        Self {
            stack,
            current: Mat4::IDENTITY,
            max_depth,
        }
    }

    #[inline]
    pub fn depth(&self) -> usize {
        self.stack.len()
    }

    #[inline]
    pub fn load_identity(&mut self) {
        self.current = Mat4::IDENTITY;
    }

    #[inline]
    pub fn load_matrix(&mut self, m: &Mat4) {
        self.current = *m;
    }

    #[inline]
    pub fn mult_matrix(&mut self, m: &Mat4) {
        self.current = self.current.multiply(m);
    }

    #[inline]
    pub fn translate(&mut self, x: f32, y: f32, z: f32) {
        self.current.translate(x, y, z);
    }

    #[inline]
    pub fn rotate(&mut self, angle: f32, x: f32, y: f32, z: f32) {
        self.current.rotate(angle, x, y, z);
    }

    #[inline]
    pub fn scale(&mut self, x: f32, y: f32, z: f32) {
        self.current.scale(x, y, z);
    }

    pub fn push(&mut self) -> Result<(), ()> {
        if self.stack.len() >= self.max_depth {
            return Err(());
        }
        self.stack.push(self.current);
        Ok(())
    }

    pub fn pop(&mut self) -> Result<(), ()> {
        if self.stack.len() <= 1 {
            return Err(());
        }
        self.current = self.stack.pop().unwrap();
        Ok(())
    }
}
