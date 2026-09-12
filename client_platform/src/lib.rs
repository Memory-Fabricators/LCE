//! Application-side platform services for Minecraft.Client: winit windowing /
//! input and PNG asset decoding.
//!
//! Windowing lives here rather than in `angle_wgpu` on purpose, mirroring
//! upstream ANGLE: the application owns its window and hands ANGLE a rendering
//! surface, so `angle_wgpu` stays reusable by other applications that bring
//! their own windowing stack.
#![allow(non_snake_case, unused_imports, dead_code)]

pub mod image;
pub mod winit_app;
