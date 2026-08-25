//! OpenGL ES 1.1 / 2.0 and EGL types and constants.
#![allow(non_camel_case_types, non_upper_case_globals, dead_code)]

use std::ffi::c_void;

// Basic GL types
pub type GLenum = u32;
pub type GLboolean = u8;
pub type GLbitfield = u32;
pub type GLbyte = i8;
pub type GLshort = i16;
pub type GLint = i32;
pub type GLsizei = i32;
pub type GLubyte = u8;
pub type GLushort = u16;
pub type GLuint = u32;
pub type GLfloat = f32;
pub type GLclampf = f32;
pub type GLdouble = f64;
pub type GLclampd = f64;
pub type GLvoid = c_void;
pub type GLchar = std::ffi::c_char;
pub type GLintptr = isize;
pub type GLsizeiptr = isize;

// Basic EGL types
pub type EGLDisplay = *mut c_void;
pub type EGLConfig = *mut c_void;
pub type EGLContext = *mut c_void;
pub type EGLSurface = *mut c_void;
pub type EGLClientBuffer = *mut c_void;
pub type EGLint = i32;
pub type EGLBoolean = u32;
pub type EGLenum = u32;
pub type NativeDisplayType = *mut c_void;
pub type NativeWindowType = *mut c_void;
pub type NativePixmapType = *mut c_void;
pub type __eglMustCastToProperFunctionPointerType = Option<unsafe extern "C" fn()>;

// EGL Constants
pub const EGL_NO_DISPLAY: EGLDisplay = 0 as EGLDisplay;
pub const EGL_NO_CONTEXT: EGLContext = 0 as EGLContext;
pub const EGL_NO_SURFACE: EGLSurface = 0 as EGLSurface;
pub const EGL_DEFAULT_DISPLAY: EGLDisplay = 0 as EGLDisplay;

pub const EGL_FALSE: EGLBoolean = 0;
pub const EGL_TRUE: EGLBoolean = 1;

pub const EGL_SUCCESS: EGLint = 0x3000;
pub const EGL_NOT_INITIALIZED: EGLint = 0x3001;
pub const EGL_BAD_ACCESS: EGLint = 0x3002;
pub const EGL_BAD_ALLOC: EGLint = 0x3003;
pub const EGL_BAD_ATTRIBUTE: EGLint = 0x3004;
pub const EGL_BAD_CONFIG: EGLint = 0x3005;
pub const EGL_BAD_CONTEXT: EGLint = 0x3006;
pub const EGL_BAD_CURRENT_SURFACE: EGLint = 0x3007;
pub const EGL_BAD_DISPLAY: EGLint = 0x3008;
pub const EGL_BAD_MATCH: EGLint = 0x3009;
pub const EGL_BAD_NATIVE_PIXMAP: EGLint = 0x300A;
pub const EGL_BAD_NATIVE_WINDOW: EGLint = 0x300B;
pub const EGL_BAD_PARAMETER: EGLint = 0x300C;
pub const EGL_BAD_SURFACE: EGLint = 0x300D;
pub const EGL_CONTEXT_LOST: EGLint = 0x300E;

pub const EGL_BUFFER_SIZE: EGLint = 0x3020;
pub const EGL_ALPHA_SIZE: EGLint = 0x3021;
pub const EGL_BLUE_SIZE: EGLint = 0x3022;
pub const EGL_GREEN_SIZE: EGLint = 0x3023;
pub const EGL_RED_SIZE: EGLint = 0x3024;
pub const EGL_DEPTH_SIZE: EGLint = 0x3025;
pub const EGL_STENCIL_SIZE: EGLint = 0x3026;
pub const EGL_CONFIG_CAVEAT: EGLint = 0x3027;
pub const EGL_CONFIG_ID: EGLint = 0x3028;
pub const EGL_LEVEL: EGLint = 0x3029;
pub const EGL_MAX_PBUFFER_HEIGHT: EGLint = 0x302A;
pub const EGL_MAX_PBUFFER_PIXELS: EGLint = 0x302B;
pub const EGL_MAX_PBUFFER_WIDTH: EGLint = 0x302C;
pub const EGL_NATIVE_RENDERABLE: EGLint = 0x302D;
pub const EGL_NATIVE_VISUAL_ID: EGLint = 0x302E;
pub const EGL_NATIVE_VISUAL_TYPE: EGLint = 0x302F;
pub const EGL_SAMPLES: EGLint = 0x3031;
pub const EGL_SAMPLE_BUFFERS: EGLint = 0x3032;
pub const EGL_SURFACE_TYPE: EGLint = 0x3033;
pub const EGL_TRANSPARENT_TYPE: EGLint = 0x3034;
pub const EGL_TRANSPARENT_BLUE_VALUE: EGLint = 0x3035;
pub const EGL_TRANSPARENT_GREEN_VALUE: EGLint = 0x3036;
pub const EGL_TRANSPARENT_RED_VALUE: EGLint = 0x3037;
pub const EGL_NONE: EGLint = 0x3038;
pub const EGL_BIND_TO_TEXTURE_RGB: EGLint = 0x3039;
pub const EGL_BIND_TO_TEXTURE_RGBA: EGLint = 0x303A;
pub const EGL_MIN_SWAP_INTERVAL: EGLint = 0x303B;
pub const EGL_MAX_SWAP_INTERVAL: EGLint = 0x303C;
pub const EGL_LUMINANCE_SIZE: EGLint = 0x303D;
pub const EGL_ALPHA_MASK_SIZE: EGLint = 0x303E;
pub const EGL_COLOR_BUFFER_TYPE: EGLint = 0x303F;
pub const EGL_RENDERABLE_TYPE: EGLint = 0x3040;
pub const EGL_CONFORMANT: EGLint = 0x3042;

pub const EGL_SLOW_CONFIG: EGLint = 0x3050;
pub const EGL_NON_CONFORMANT_CONFIG: EGLint = 0x3051;
pub const EGL_TRANSPARENT_RGB: EGLint = 0x3052;
pub const EGL_RGB_BUFFER: EGLint = 0x308E;
pub const EGL_LUMINANCE_BUFFER: EGLint = 0x308F;

pub const EGL_PBUFFER_BIT: EGLint = 0x0001;
pub const EGL_PIXMAP_BIT: EGLint = 0x0002;
pub const EGL_WINDOW_BIT: EGLint = 0x0004;

pub const EGL_OPENGL_ES_BIT: EGLint = 0x0001;
pub const EGL_OPENVG_BIT: EGLint = 0x0002;
pub const EGL_OPENGL_ES2_BIT: EGLint = 0x0004;
pub const EGL_OPENGL_BIT: EGLint = 0x0008;

pub const EGL_VENDOR: EGLint = 0x3053;
pub const EGL_VERSION: EGLint = 0x3054;
pub const EGL_EXTENSIONS: EGLint = 0x3055;
pub const EGL_CLIENT_APIS: EGLint = 0x308D;

pub const EGL_HEIGHT: EGLint = 0x3056;
pub const EGL_WIDTH: EGLint = 0x3057;
pub const EGL_LARGEST_PBUFFER: EGLint = 0x3058;
pub const EGL_TEXTURE_FORMAT: EGLint = 0x3080;
pub const EGL_TEXTURE_TARGET: EGLint = 0x3081;
pub const EGL_MIPMAP_TEXTURE: EGLint = 0x3082;
pub const EGL_MIPMAP_LEVEL: EGLint = 0x3083;
pub const EGL_BACK_BUFFER: EGLint = 0x3084;

pub const EGL_CONTEXT_CLIENT_VERSION: EGLint = 0x3098;

// OpenGL Constants
pub const GL_FALSE: GLboolean = 0;
pub const GL_TRUE: GLboolean = 1;

pub const GL_BYTE: GLenum = 0x1400;
pub const GL_UNSIGNED_BYTE: GLenum = 0x1401;
pub const GL_SHORT: GLenum = 0x1402;
pub const GL_UNSIGNED_SHORT: GLenum = 0x1403;
pub const GL_INT: GLenum = 0x1404;
pub const GL_UNSIGNED_INT: GLenum = 0x1405;
pub const GL_FLOAT: GLenum = 0x1406;
pub const GL_DOUBLE: GLenum = 0x140A;

pub const GL_POINTS: GLenum = 0x0000;
pub const GL_LINES: GLenum = 0x0001;
pub const GL_LINE_LOOP: GLenum = 0x0002;
pub const GL_LINE_STRIP: GLenum = 0x0003;
pub const GL_TRIANGLES: GLenum = 0x0004;
pub const GL_TRIANGLE_STRIP: GLenum = 0x0005;
pub const GL_TRIANGLE_FAN: GLenum = 0x0006;
pub const GL_QUADS: GLenum = 0x0007;
pub const GL_QUAD_STRIP: GLenum = 0x0008;
pub const GL_POLYGON: GLenum = 0x0009;

pub const GL_NO_ERROR: GLenum = 0;
pub const GL_INVALID_ENUM: GLenum = 0x0500;
pub const GL_INVALID_VALUE: GLenum = 0x0501;
pub const GL_INVALID_OPERATION: GLenum = 0x0502;
pub const GL_MATRIX_MODE: GLenum = 0x0BA0;
pub const GL_STACK_OVERFLOW: GLenum = 0x0503;
pub const GL_STACK_UNDERFLOW: GLenum = 0x0504;
pub const GL_OUT_OF_MEMORY: GLenum = 0x0505;
pub const GL_INVALID_FRAMEBUFFER_OPERATION: GLenum = 0x0506;

pub const GL_NEVER: GLenum = 0x0200;
pub const GL_LESS: GLenum = 0x0201;
pub const GL_EQUAL: GLenum = 0x0202;
pub const GL_LEQUAL: GLenum = 0x0203;
pub const GL_GREATER: GLenum = 0x0204;
pub const GL_NOTEQUAL: GLenum = 0x0205;
pub const GL_GEQUAL: GLenum = 0x0206;
pub const GL_ALWAYS: GLenum = 0x0207;

pub const GL_ZERO: GLenum = 0;
pub const GL_ONE: GLenum = 1;
pub const GL_SRC_COLOR: GLenum = 0x0300;
pub const GL_ONE_MINUS_SRC_COLOR: GLenum = 0x0301;
pub const GL_SRC_ALPHA: GLenum = 0x0302;
pub const GL_ONE_MINUS_SRC_ALPHA: GLenum = 0x0303;
pub const GL_DST_ALPHA: GLenum = 0x0304;
pub const GL_ONE_MINUS_DST_ALPHA: GLenum = 0x0305;
pub const GL_DST_COLOR: GLenum = 0x0306;
pub const GL_ONE_MINUS_DST_COLOR: GLenum = 0x0307;
pub const GL_SRC_ALPHA_SATURATE: GLenum = 0x0308;
pub const GL_CONSTANT_COLOR: GLenum = 0x8001;
pub const GL_ONE_MINUS_CONSTANT_COLOR: GLenum = 0x8002;
pub const GL_CONSTANT_ALPHA: GLenum = 0x8003;
pub const GL_ONE_MINUS_CONSTANT_ALPHA: GLenum = 0x8004;

pub const GL_FRONT: GLenum = 0x0404;
pub const GL_BACK: GLenum = 0x0405;
pub const GL_FRONT_AND_BACK: GLenum = 0x0408;

pub const GL_CW: GLenum = 0x0900;
pub const GL_CCW: GLenum = 0x0901;

pub const GL_CULL_FACE: GLenum = 0x0B44;
pub const GL_LIGHTING: GLenum = 0x0B50;
pub const GL_FOG: GLenum = 0x0B60;
pub const GL_DEPTH_TEST: GLenum = 0x0B71;
pub const GL_STENCIL_TEST: GLenum = 0x0B90;
pub const GL_NORMALIZE: GLenum = 0x0BA1;
pub const GL_ALPHA_TEST: GLenum = 0x0BC0;
pub const GL_BLEND: GLenum = 0x0BE2;
pub const GL_COLOR_LOGIC_OP: GLenum = 0x0BF2;
pub const GL_SCISSOR_TEST: GLenum = 0x0C11;
pub const GL_TEXTURE_2D: GLenum = 0x0DE1;
pub const GL_POLYGON_OFFSET_FILL: GLenum = 0x8037;
pub const GL_RESCALE_NORMAL: GLenum = 0x803A;
pub const GL_COLOR_MATERIAL: GLenum = 0x0B57;

pub const GL_MODELVIEW: GLenum = 0x1700;
pub const GL_PROJECTION: GLenum = 0x1701;
pub const GL_TEXTURE: GLenum = 0x1702;

pub const GL_COLOR_BUFFER_BIT: GLbitfield = 0x0000_4000;
pub const GL_DEPTH_BUFFER_BIT: GLbitfield = 0x0000_0100;
pub const GL_STENCIL_BUFFER_BIT: GLbitfield = 0x0000_0400;

pub const GL_CURRENT_COLOR: GLenum = 0x0B00;
pub const GL_CURRENT_NORMAL: GLenum = 0x0B02;
pub const GL_CURRENT_TEXTURE_COORDS: GLenum = 0x0B03;
pub const GL_LINE_WIDTH: GLenum = 0x0B21;
pub const GL_CULL_FACE_MODE: GLenum = 0x0B45;
pub const GL_FRONT_FACE: GLenum = 0x0B46;
pub const GL_SHADE_MODEL: GLenum = 0x0B54;
pub const GL_DEPTH_RANGE: GLenum = 0x0B70;
pub const GL_DEPTH_WRITEMASK: GLenum = 0x0B72;
pub const GL_DEPTH_CLEAR_VALUE: GLenum = 0x0B73;
pub const GL_DEPTH_FUNC: GLenum = 0x0B74;
pub const GL_STENCIL_CLEAR_VALUE: GLenum = 0x0B91;
pub const GL_STENCIL_FUNC: GLenum = 0x0B92;
pub const GL_STENCIL_VALUE_MASK: GLenum = 0x0B93;
pub const GL_STENCIL_FAIL: GLenum = 0x0B94;
pub const GL_STENCIL_PASS_DEPTH_FAIL: GLenum = 0x0B95;
pub const GL_STENCIL_PASS_DEPTH_PASS: GLenum = 0x0B96;
pub const GL_STENCIL_REF: GLenum = 0x0B97;
pub const GL_STENCIL_WRITEMASK: GLenum = 0x0B98;
pub const GL_VIEWPORT: GLenum = 0x0BA2;
pub const GL_MODELVIEW_STACK_DEPTH: GLenum = 0x0BA3;
pub const GL_PROJECTION_STACK_DEPTH: GLenum = 0x0BA4;
pub const GL_TEXTURE_STACK_DEPTH: GLenum = 0x0BA5;
pub const GL_MODELVIEW_MATRIX: GLenum = 0x0BA6;
pub const GL_PROJECTION_MATRIX: GLenum = 0x0BA7;
pub const GL_TEXTURE_MATRIX: GLenum = 0x0BA8;
pub const GL_ALPHA_TEST_FUNC: GLenum = 0x0BC1;
pub const GL_ALPHA_TEST_REF: GLenum = 0x0BC2;
pub const GL_BLEND_DST: GLenum = 0x0BE0;
pub const GL_BLEND_SRC: GLenum = 0x0BE1;
pub const GL_LOGIC_OP_MODE: GLenum = 0x0BF0;
pub const GL_SCISSOR_BOX: GLenum = 0x0C10;
pub const GL_COLOR_CLEAR_VALUE: GLenum = 0x0C22;
pub const GL_COLOR_WRITEMASK: GLenum = 0x0C23;
pub const GL_MAX_TEXTURE_SIZE: GLenum = 0x0D33;
pub const GL_MAX_VIEWPORT_DIMS: GLenum = 0x0D3A;
pub const GL_SUBPIXEL_BITS: GLenum = 0x0D50;
pub const GL_RED_BITS: GLenum = 0x0D52;
pub const GL_GREEN_BITS: GLenum = 0x0D53;
pub const GL_BLUE_BITS: GLenum = 0x0D54;
pub const GL_ALPHA_BITS: GLenum = 0x0D55;
pub const GL_DEPTH_BITS: GLenum = 0x0D56;
pub const GL_STENCIL_BITS: GLenum = 0x0D57;
pub const GL_TEXTURE_BINDING_2D: GLenum = 0x8069;
pub const GL_CURRENT_PROGRAM: GLenum = 0x8B8D;

pub const GL_VENDOR: GLenum = 0x1F00;
pub const GL_RENDERER: GLenum = 0x1F01;
pub const GL_VERSION: GLenum = 0x1F02;
pub const GL_EXTENSIONS: GLenum = 0x1F03;

pub const GL_NEAREST: GLenum = 0x2600;
pub const GL_LINEAR: GLenum = 0x2601;
pub const GL_NEAREST_MIPMAP_NEAREST: GLenum = 0x2700;
pub const GL_LINEAR_MIPMAP_NEAREST: GLenum = 0x2701;
pub const GL_NEAREST_MIPMAP_LINEAR: GLenum = 0x2702;
pub const GL_LINEAR_MIPMAP_LINEAR: GLenum = 0x2703;

pub const GL_TEXTURE_MAG_FILTER: GLenum = 0x2800;
pub const GL_TEXTURE_MIN_FILTER: GLenum = 0x2801;
pub const GL_TEXTURE_WRAP_S: GLenum = 0x2802;
pub const GL_TEXTURE_WRAP_T: GLenum = 0x2803;
pub const GL_TEXTURE_WIDTH: GLenum = 0x1000;
pub const GL_TEXTURE_HEIGHT: GLenum = 0x1001;
pub const GL_TEXTURE_INTERNAL_FORMAT: GLenum = 0x1003;

pub const GL_CLAMP: GLenum = 0x2900;
pub const GL_REPEAT: GLenum = 0x2901;
pub const GL_CLAMP_TO_EDGE: GLenum = 0x812F;
pub const GL_MIRRORED_REPEAT: GLenum = 0x8370;
pub const GL_S: GLenum = 0x2000;
pub const GL_T: GLenum = 0x2001;
pub const GL_R: GLenum = 0x2002;
pub const GL_Q: GLenum = 0x2003;
pub const GL_TEXTURE_GEN_MODE: GLenum = 0x2500;
pub const GL_OBJECT_PLANAR: GLenum = 0x2501;
pub const GL_EYE_PLANAR: GLenum = 0x2502;
pub const GL_EYE_LINEAR: GLenum = 0x2400;
pub const GL_OBJECT_LINEAR: GLenum = 0x2401;
pub const GL_SPHERE_MAP: GLenum = 0x2402;
pub const GL_TEXTURE_GEN_S: GLenum = 0x0C60;
pub const GL_TEXTURE_GEN_T: GLenum = 0x0C61;
pub const GL_TEXTURE_GEN_R: GLenum = 0x0C62;
pub const GL_TEXTURE_GEN_Q: GLenum = 0x0C63;

pub const GL_ALPHA: GLenum = 0x1906;
pub const GL_RGB: GLenum = 0x1907;
pub const GL_RGBA: GLenum = 0x1908;
pub const GL_LUMINANCE: GLenum = 0x1909;
pub const GL_LUMINANCE_ALPHA: GLenum = 0x190A;
pub const GL_BGRA_EXT: GLenum = 0x80E1;

pub const GL_TEXTURE0: GLenum = 0x84C0;
pub const GL_TEXTURE1: GLenum = 0x84C1;
pub const GL_TEXTURE2: GLenum = 0x84C2;
pub const GL_TEXTURE3: GLenum = 0x84C3;
pub const GL_TEXTURE4: GLenum = 0x84C4;
pub const GL_TEXTURE5: GLenum = 0x84C5;
pub const GL_TEXTURE6: GLenum = 0x84C6;
pub const GL_TEXTURE7: GLenum = 0x84C7;
pub const GL_ACTIVE_TEXTURE: GLenum = 0x84E0;
pub const GL_CLIENT_ACTIVE_TEXTURE: GLenum = 0x84E1;
pub const GL_MAX_TEXTURE_UNITS: GLenum = 0x84E2;

pub const GL_VERTEX_ARRAY: GLenum = 0x8074;
pub const GL_NORMAL_ARRAY: GLenum = 0x8075;
pub const GL_COLOR_ARRAY: GLenum = 0x8076;
pub const GL_TEXTURE_COORD_ARRAY: GLenum = 0x8078;

pub const GL_COMPILE: GLenum = 0x1300;
pub const GL_COMPILE_AND_EXECUTE: GLenum = 0x1301;

pub const GL_FLAT: GLenum = 0x1D00;
pub const GL_SMOOTH: GLenum = 0x1D01;

pub const GL_KEEP: GLenum = 0x1E00;
pub const GL_REPLACE: GLenum = 0x1E01;
pub const GL_INCR: GLenum = 0x1E02;
pub const GL_DECR: GLenum = 0x1E03;
pub const GL_INVERT: GLenum = 0x150A;
pub const GL_INCR_WRAP: GLenum = 0x8507;
pub const GL_DECR_WRAP: GLenum = 0x8508;

pub const GL_FOG_MODE: GLenum = 0x0B65;
pub const GL_FOG_DENSITY: GLenum = 0x0B62;
pub const GL_FOG_START: GLenum = 0x0B63;
pub const GL_FOG_END: GLenum = 0x0B64;
pub const GL_FOG_COLOR: GLenum = 0x0B66;
pub const GL_EXP: GLenum = 0x0800;
pub const GL_EXP2: GLenum = 0x0801;

pub const GL_LIGHT0: GLenum = 0x4000;
pub const GL_LIGHT1: GLenum = 0x4001;
pub const GL_LIGHT2: GLenum = 0x4002;
pub const GL_LIGHT3: GLenum = 0x4003;
pub const GL_LIGHT4: GLenum = 0x4004;
pub const GL_LIGHT5: GLenum = 0x4005;
pub const GL_LIGHT6: GLenum = 0x4006;
pub const GL_LIGHT7: GLenum = 0x4007;

pub const GL_AMBIENT: GLenum = 0x1200;
pub const GL_DIFFUSE: GLenum = 0x1201;
pub const GL_SPECULAR: GLenum = 0x1202;
pub const GL_POSITION: GLenum = 0x1203;
pub const GL_SPOT_DIRECTION: GLenum = 0x1204;
pub const GL_SPOT_EXPONENT: GLenum = 0x1205;
pub const GL_SPOT_CUTOFF: GLenum = 0x1206;
pub const GL_CONSTANT_ATTENUATION: GLenum = 0x1207;
pub const GL_LINEAR_ATTENUATION: GLenum = 0x1208;
pub const GL_QUADRATIC_ATTENUATION: GLenum = 0x1209;
pub const GL_LIGHT_MODEL_AMBIENT: GLenum = 0x0B53;
pub const GL_LIGHT_MODEL_TWO_SIDE: GLenum = 0x0B52;

pub const GL_EMISSION: GLenum = 0x1600;
pub const GL_SHININESS: GLenum = 0x1601;
pub const GL_AMBIENT_AND_DIFFUSE: GLenum = 0x1602;

pub const GL_UNPACK_ALIGNMENT: GLenum = 0x0CF5;
pub const GL_PACK_ALIGNMENT: GLenum = 0x0D05;

pub const GL_ARRAY_BUFFER: GLenum = 0x8892;
pub const GL_ELEMENT_ARRAY_BUFFER: GLenum = 0x8893;
pub const GL_ARRAY_BUFFER_BINDING: GLenum = 0x8894;
pub const GL_ELEMENT_ARRAY_BUFFER_BINDING: GLenum = 0x8895;
pub const GL_STATIC_DRAW: GLenum = 0x88E4;
pub const GL_DYNAMIC_DRAW: GLenum = 0x88E8;
pub const GL_STREAM_DRAW: GLenum = 0x88E0;

pub const GL_FRAGMENT_SHADER: GLenum = 0x8B30;
pub const GL_VERTEX_SHADER: GLenum = 0x8B31;
pub const GL_COMPILE_STATUS: GLenum = 0x8B81;
pub const GL_LINK_STATUS: GLenum = 0x8B82;
pub const GL_VALIDATE_STATUS: GLenum = 0x8B83;
pub const GL_INFO_LOG_LENGTH: GLenum = 0x8B84;
pub const GL_ATTACHED_SHADERS: GLenum = 0x8B85;
pub const GL_ACTIVE_UNIFORMS: GLenum = 0x8B86;
pub const GL_ACTIVE_ATTRIBUTES: GLenum = 0x8B89;

pub const GL_FRAMEBUFFER: GLenum = 0x8D40;
pub const GL_RENDERBUFFER: GLenum = 0x8D41;
pub const GL_COLOR_ATTACHMENT0: GLenum = 0x8CE0;
pub const GL_DEPTH_ATTACHMENT: GLenum = 0x8D00;
pub const GL_STENCIL_ATTACHMENT: GLenum = 0x8D20;
pub const GL_FRAMEBUFFER_COMPLETE: GLenum = 0x8CD5;

pub const GL_ALL_ATTRIB_BITS: GLbitfield = 0xFFFF_FFFF;
pub const GL_CLIENT_ALL_ATTRIB_BITS: GLbitfield = 0xFFFF_FFFF;
pub const GL_CURRENT_BIT: GLbitfield = 0x0000_0001;
pub const GL_POINT_BIT: GLbitfield = 0x0000_0002;
pub const GL_LINE_BIT: GLbitfield = 0x0000_0004;
pub const GL_POLYGON_BIT: GLbitfield = 0x0000_0008;
pub const GL_POLYGON_STIPPLE_BIT: GLbitfield = 0x0000_0010;
pub const GL_PIXEL_MODE_BIT: GLbitfield = 0x0000_0020;
pub const GL_LIGHTING_BIT: GLbitfield = 0x0000_0040;
pub const GL_FOG_BIT: GLbitfield = 0x0000_0080;
pub const GL_DEPTH_BUFFER_BIT_ATTRIB: GLbitfield = 0x0000_0100;
pub const GL_ACCUM_BUFFER_BIT: GLbitfield = 0x0000_0200;
pub const GL_STENCIL_BUFFER_BIT_ATTRIB: GLbitfield = 0x0000_0400;
pub const GL_VIEWPORT_BIT: GLbitfield = 0x0000_0800;
pub const GL_TRANSFORM_BIT: GLbitfield = 0x0000_1000;
pub const GL_ENABLE_BIT: GLbitfield = 0x0000_2000;
pub const GL_COLOR_BUFFER_BIT_ATTRIB: GLbitfield = 0x0000_4000;
pub const GL_HINT_BIT: GLbitfield = 0x0000_8000;
pub const GL_EVAL_BIT: GLbitfield = 0x0001_0000;
pub const GL_LIST_BIT: GLbitfield = 0x0002_0000;
pub const GL_TEXTURE_BIT: GLbitfield = 0x0004_0000;
pub const GL_SCISSOR_BIT: GLbitfield = 0x0008_0000;
pub const GL_CLIENT_PIXEL_STORE_BIT: GLbitfield = 0x0000_0001;
pub const GL_CLIENT_VERTEX_ARRAY_BIT: GLbitfield = 0x0000_0002;

pub const GL_MODULATE: GLenum = 0x2100;
pub const GL_DECAL: GLenum = 0x2101;
pub const GL_ADD: GLenum = 0x0104;
pub const GL_TEXTURE_ENV: GLenum = 0x2300;
pub const GL_TEXTURE_ENV_MODE: GLenum = 0x2200;
pub const GL_TEXTURE_ENV_COLOR: GLenum = 0x2201;
pub const GL_COMBINE: GLenum = 0x8570;
pub const GL_SRC0_RGB: GLenum = 0x8580;
pub const GL_SRC1_RGB: GLenum = 0x8581;
pub const GL_SRC2_RGB: GLenum = 0x8582;
pub const GL_OPERAND0_RGB: GLenum = 0x8590;
pub const GL_OPERAND1_RGB: GLenum = 0x8591;
pub const GL_OPERAND2_RGB: GLenum = 0x8592;
pub const GL_RGB_SCALE: GLenum = 0x8573;
