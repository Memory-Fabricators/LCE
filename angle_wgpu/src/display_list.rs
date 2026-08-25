//! OpenGL Display List compilation and playback engine.

use crate::matrix::{Mat4, MatrixMode};
use crate::types::*;
use parking_lot::RwLock;
use std::collections::HashMap;
use std::sync::atomic::{AtomicU32, Ordering};

#[derive(Debug, Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
#[repr(C)]
pub struct VertexData {
    pub position: [f32; 3],
    pub tex_coord: [f32; 2],
    pub color: [f32; 4],
    pub normal: [f32; 3],
}

impl Default for VertexData {
    fn default() -> Self {
        Self {
            position: [0.0, 0.0, 0.0],
            tex_coord: [0.0, 0.0],
            color: [1.0, 1.0, 1.0, 1.0],
            normal: [0.0, 0.0, 1.0],
        }
    }
}

#[derive(Debug, Clone)]
pub enum DisplayListOp {
    Draw {
        mode: GLenum,
        vertices: Vec<VertexData>,
        indices: Vec<u32>,
    },
    MatrixPush(MatrixMode),
    MatrixPop(MatrixMode),
    MatrixLoad(MatrixMode, Mat4),
    MatrixMult(MatrixMode, Mat4),
    MatrixTranslate(f32, f32, f32),
    MatrixRotate(f32, f32, f32, f32),
    MatrixScale(f32, f32, f32),
    BindTexture(GLuint),
    Enable(GLenum),
    Disable(GLenum),
    Color4f(f32, f32, f32, f32),
    Normal3f(f32, f32, f32),
    TexCoord2f(f32, f32),
    BlendFunc(GLenum, GLenum),
    DepthFunc(GLenum),
    DepthMask(bool),
    AlphaFunc(GLenum, f32),
    CullFace(GLenum),
    PolygonOffset(f32, f32),
    ShadeModel(GLenum),
    CallList(GLuint),
}

#[derive(Debug, Clone, Default)]
pub struct DisplayList {
    pub id: GLuint,
    pub ops: Vec<DisplayListOp>,
}

impl DisplayList {
    pub fn new(id: GLuint) -> Self {
        Self {
            id,
            ops: Vec::new(),
        }
    }

    #[inline]
    pub fn push_op(&mut self, op: DisplayListOp) {
        self.ops.push(op);
    }
}

/// Global/Context-shared display list registry (thread-safe for chunk worker threads).
#[derive(Debug)]
pub struct DisplayListRegistry {
    lists: RwLock<HashMap<GLuint, DisplayList>>,
    next_id: AtomicU32,
}

impl Default for DisplayListRegistry {
    fn default() -> Self {
        Self::new()
    }
}

impl DisplayListRegistry {
    pub fn new() -> Self {
        Self {
            lists: RwLock::new(HashMap::new()),
            next_id: AtomicU32::new(1),
        }
    }

    pub fn gen_lists(&self, range: usize) -> GLuint {
        let start = self.next_id.fetch_add(range as u32, Ordering::SeqCst);
        let mut lists = self.lists.write();
        for i in 0..range {
            let id = start + i as u32;
            lists.insert(id, DisplayList::new(id));
        }
        start
    }

    pub fn delete_lists(&self, list: GLuint, range: usize) {
        let mut lists = self.lists.write();
        for i in 0..range {
            let id = list + i as u32;
            lists.remove(&id);
        }
    }

    pub fn is_list(&self, list: GLuint) -> bool {
        let lists = self.lists.read();
        lists.contains_key(&list)
    }

    pub fn store_list(&self, list: DisplayList) {
        let mut lists = self.lists.write();
        lists.insert(list.id, list);
    }

    pub fn get_list(&self, list: GLuint) -> Option<DisplayList> {
        let lists = self.lists.read();
        lists.get(&list).cloned()
    }
}
