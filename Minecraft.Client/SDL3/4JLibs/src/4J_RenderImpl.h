#pragma once

// Private implementation state for C4JRender's Dawn/WebGPU backend.
// Only 4J_Render.cpp includes this - Dawn/WebGPU types must never leak into
// the public 4J_Render.h, which is included transitively by a lot of
// gameplay code that doesn't need to know about the render backend.

#include <SDL3/SDL.h>
#include <webgpu/webgpu_cpp.h>

#include <unordered_map>
#include <vector>

struct C4JRenderState
{
    SDL_Window *window = nullptr;
    SDL_MetalView metalView = nullptr;

    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::Surface surface;
    wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::BGRA8Unorm;

    wgpu::Texture depthTexture;
    wgpu::TextureView depthTextureView;

    int surfaceWidth = 0;
    int surfaceHeight = 0;

    float clearColour[4] = {0.4f, 0.6f, 0.9f, 1.0f};

    // Set once per frame between StartFrame()/Present().
    wgpu::TextureView currentSurfaceView;
    wgpu::CommandEncoder currentEncoder;
    wgpu::RenderPassEncoder currentPass;
    bool frameActive = false;

    bool suspended = false;
};

extern C4JRenderState g_render;
