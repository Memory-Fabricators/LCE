// Deliberately avoids ../../../stdafx.h: that chain pulls in Minecraft.World
// headers with `using namespace std;` + a project-defined `byte` typedef,
// which becomes ambiguous with std::byte once this TU is compiled as C++17
// (required by Dawn's webgpu_cpp.h). This file only needs the render
// interface + Windows-compat types, not the full game header chain.
#include "../inc/4J_Render.h"
#include "../../WindowsTypes.h"
#include "4J_RenderImpl.h"

#include <SDL3/SDL_metal.h>

#include <cstdio>

C4JRender RenderManager;
C4JRenderState g_render;

namespace
{
void PrintUncapturedError(const wgpu::Device &, wgpu::ErrorType type, wgpu::StringView message)
{
    fprintf(stderr, "[Dawn] uncaptured error (%d): %.*s\n", (int)type, (int)message.length, message.data);
}

void PrintDeviceLost(const wgpu::Device &, wgpu::DeviceLostReason reason, wgpu::StringView message)
{
    fprintf(stderr, "[Dawn] device lost (%d): %.*s\n", (int)reason, (int)message.length, message.data);
}

void PrintLog(wgpu::LoggingType type, wgpu::StringView message)
{
    fprintf(stderr, "[Dawn] log (%d): %.*s\n", (int)type, (int)message.length, message.data);
}

void CreateDepthTarget(int width, int height)
{
    wgpu::TextureDescriptor depthDesc{};
    depthDesc.dimension = wgpu::TextureDimension::e2D;
    depthDesc.size = {(uint32_t)width, (uint32_t)height, 1};
    depthDesc.format = wgpu::TextureFormat::Depth24Plus;
    depthDesc.usage = wgpu::TextureUsage::RenderAttachment;
    depthDesc.mipLevelCount = 1;
    depthDesc.sampleCount = 1;

    g_render.depthTexture = g_render.device.CreateTexture(&depthDesc);
    g_render.depthTextureView = g_render.depthTexture.CreateView();
}

void ConfigureSurface(int width, int height)
{
    wgpu::SurfaceConfiguration config{};
    config.device = g_render.device;
    config.format = g_render.surfaceFormat;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.width = (uint32_t)width;
    config.height = (uint32_t)height;
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Auto;
    g_render.surface.Configure(&config);

    g_render.surfaceWidth = width;
    g_render.surfaceHeight = height;

    CreateDepthTarget(width, height);
}
} // namespace

void C4JRender::Initialise(void *window)
{
    g_render.window = (SDL_Window *)window;
    g_render.metalView = SDL_Metal_CreateView(g_render.window);
    void *metalLayer = SDL_Metal_GetLayer(g_render.metalView);

    wgpu::InstanceFeatureName instanceFeatures[] = {wgpu::InstanceFeatureName::TimedWaitAny};
    wgpu::InstanceDescriptor instanceDesc{};
    instanceDesc.requiredFeatureCount = 1;
    instanceDesc.requiredFeatures = instanceFeatures;
    g_render.instance = wgpu::CreateInstance(&instanceDesc);

    wgpu::SurfaceSourceMetalLayer metalSource{};
    metalSource.layer = metalLayer;

    wgpu::SurfaceDescriptor surfaceDesc{};
    surfaceDesc.nextInChain = &metalSource;
    g_render.surface = g_render.instance.CreateSurface(&surfaceDesc);

    wgpu::RequestAdapterOptions adapterOpts{};
    adapterOpts.powerPreference = wgpu::PowerPreference::HighPerformance;
    adapterOpts.compatibleSurface = g_render.surface;

    wgpu::Adapter adapter;
    auto adapterFuture = g_render.instance.RequestAdapter(
        &adapterOpts,
        wgpu::CallbackMode::WaitAnyOnly,
        [&adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter result, wgpu::StringView message) {
            if (status == wgpu::RequestAdapterStatus::Success)
            {
                adapter = result;
            }
            else
            {
                fprintf(stderr, "[Dawn] RequestAdapter failed: %.*s\n", (int)message.length, message.data);
            }
        });
    g_render.instance.WaitAny(adapterFuture, UINT64_MAX);
    g_render.adapter = adapter;

    wgpu::DeviceDescriptor deviceDesc{};
    deviceDesc.SetUncapturedErrorCallback(PrintUncapturedError);
    deviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, PrintDeviceLost);

    wgpu::Device device;
    auto deviceFuture = g_render.adapter.RequestDevice(
        &deviceDesc,
        wgpu::CallbackMode::WaitAnyOnly,
        [&device](wgpu::RequestDeviceStatus status, wgpu::Device result, wgpu::StringView message) {
            if (status == wgpu::RequestDeviceStatus::Success)
            {
                device = result;
            }
            else
            {
                fprintf(stderr, "[Dawn] RequestDevice failed: %.*s\n", (int)message.length, message.data);
            }
        });
    g_render.instance.WaitAny(deviceFuture, UINT64_MAX);
    g_render.device = device;
    g_render.device.SetLoggingCallback(PrintLog);

    g_render.queue = g_render.device.GetQueue();

    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(g_render.window, &width, &height);
    if (width <= 0)
    {
        width = 1280;
    }
    if (height <= 0)
    {
        height = 720;
    }

    ConfigureSurface(width, height);
}

void C4JRender::InitialiseContext()
{
    // Device/surface setup already happened in Initialise(); nothing further needed.
}

void C4JRender::Tick()
{
}

void C4JRender::UpdateGamma(unsigned short)
{
}

void C4JRender::StartFrame()
{
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(g_render.window, &width, &height);
    if (width > 0 && height > 0 &&
        (width != g_render.surfaceWidth || height != g_render.surfaceHeight))
    {
        ConfigureSurface(width, height);
    }

    wgpu::SurfaceTexture surfaceTexture{};
    g_render.surface.GetCurrentTexture(&surfaceTexture);
    if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
        surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal)
    {
        g_render.frameActive = false;
        return;
    }

    g_render.currentSurfaceView = surfaceTexture.texture.CreateView();
    g_render.currentEncoder = g_render.device.CreateCommandEncoder();
    g_render.frameActive = true;
}

void C4JRender::DoScreenGrabOnNextPresent()
{
}

void C4JRender::Present()
{
    if (!g_render.frameActive)
    {
        return;
    }

    wgpu::CommandBuffer commands = g_render.currentEncoder.Finish();
    g_render.queue.Submit(1, &commands);

    g_render.surface.Present();
    g_render.instance.ProcessEvents();

    g_render.currentEncoder = nullptr;
    g_render.currentSurfaceView = nullptr;
    g_render.frameActive = false;
}

void C4JRender::Clear(int flags, void *)
{
    if (!g_render.frameActive)
    {
        return;
    }

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = g_render.currentSurfaceView;
    colorAttachment.loadOp = (flags & CLEAR_COLOUR_FLAG) ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {g_render.clearColour[0], g_render.clearColour[1], g_render.clearColour[2], g_render.clearColour[3]};

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = g_render.depthTextureView;
    depthAttachment.depthLoadOp = (flags & CLEAR_DEPTH_FLAG) ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
    depthAttachment.depthClearValue = 1.0f;

    wgpu::RenderPassDescriptor passDesc{};
    passDesc.colorAttachmentCount = 1;
    passDesc.colorAttachments = &colorAttachment;
    passDesc.depthStencilAttachment = &depthAttachment;

    // A pass that only clears; drawing passes are opened lazily by DrawVertices/CBuffCall (added in M2+).
    wgpu::RenderPassEncoder pass = g_render.currentEncoder.BeginRenderPass(&passDesc);
    pass.End();
}

void C4JRender::SetClearColour(const float colourRGBA[4])
{
    for (int i = 0; i < 4; i++)
    {
        g_render.clearColour[i] = colourRGBA[i];
    }
}

bool C4JRender::IsWidescreen()
{
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(g_render.window, &width, &height);
    if (height <= 0)
    {
        return true;
    }
    return ((float)width / (float)height) > 1.4f;
}

bool C4JRender::IsHiDef()
{
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(g_render.window, &width, &height);
    return width >= 1280 && height >= 720;
}

void C4JRender::CaptureThumbnail(ImageFileBuffer *)
{
}

void C4JRender::CaptureScreen(ImageFileBuffer *, XSOCIAL_PREVIEWIMAGE *)
{
}

void C4JRender::BeginConditionalSurvey(int)
{
}

void C4JRender::EndConditionalSurvey()
{
}

void C4JRender::BeginConditionalRendering(int)
{
}

void C4JRender::EndConditionalRendering()
{
}

// ---------------------------------------------------------------------------
// Matrix stack, DrawVertices, CBuff*, textures, and StateSet* pipeline plumbing
// are implemented in later milestones (M2-M5). Stub bodies for now so the
// whole client links.
// ---------------------------------------------------------------------------

void C4JRender::MatrixMode(int)
{
}

void C4JRender::MatrixSetIdentity()
{
}

void C4JRender::MatrixTranslate(float, float, float)
{
}

void C4JRender::MatrixRotate(float, float, float, float)
{
}

void C4JRender::MatrixScale(float, float, float)
{
}

void C4JRender::MatrixPerspective(float, float, float, float)
{
}

void C4JRender::MatrixOrthogonal(float, float, float, float, float, float)
{
}

void C4JRender::MatrixPop()
{
}

void C4JRender::MatrixPush()
{
}

void C4JRender::MatrixMult(float *)
{
}

const float *C4JRender::MatrixGet(int)
{
    static const float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
    return identity;
}

void C4JRender::Set_matrixDirty()
{
}

void C4JRender::DrawVertices(ePrimitiveType, int, void *, eVertexType, ePixelShaderType)
{
}

void C4JRender::CBuffLockStaticCreations()
{
}

int C4JRender::CBuffCreate(int)
{
    return 0;
}

void C4JRender::CBuffDelete(int, int)
{
}

void C4JRender::CBuffStart(int, bool)
{
}

void C4JRender::CBuffClear(int)
{
}

int C4JRender::CBuffSize(int)
{
    return 0;
}

void C4JRender::CBuffEnd()
{
}

bool C4JRender::CBuffCall(int, bool)
{
    return false;
}

void C4JRender::CBuffTick()
{
}

void C4JRender::CBuffDeferredModeStart()
{
}

void C4JRender::CBuffDeferredModeEnd()
{
}

int C4JRender::TextureCreate()
{
    return 0;
}

void C4JRender::TextureFree(int)
{
}

void C4JRender::TextureBind(int)
{
}

void C4JRender::TextureBindVertex(int)
{
}

void C4JRender::TextureSetTextureLevels(int)
{
}

int C4JRender::TextureGetTextureLevels()
{
    return 1;
}

void C4JRender::TextureData(int, int, void *, int, eTextureFormat)
{
}

void C4JRender::TextureDataUpdate(int, int, int, int, void *, int)
{
}

void C4JRender::TextureSetParam(int, int)
{
}

void C4JRender::TextureDynamicUpdateStart()
{
}

void C4JRender::TextureDynamicUpdateEnd()
{
}

HRESULT C4JRender::LoadTextureData(const char *, D3DXIMAGE_INFO *, int **)
{
    return E_NOTIMPL;
}

HRESULT C4JRender::LoadTextureData(BYTE *, DWORD, D3DXIMAGE_INFO *, int **)
{
    return E_NOTIMPL;
}

HRESULT C4JRender::SaveTextureData(const char *, D3DXIMAGE_INFO *, int *)
{
    return E_NOTIMPL;
}

void C4JRender::TextureGetStats()
{
}

void *C4JRender::TextureGetTexture(int)
{
    return nullptr;
}

void C4JRender::StateSetColour(float, float, float, float)
{
}

void C4JRender::StateSetDepthMask(bool)
{
}

void C4JRender::StateSetBlendEnable(bool)
{
}

void C4JRender::StateSetBlendFunc(int, int)
{
}

void C4JRender::StateSetBlendFactor(unsigned int)
{
}

void C4JRender::StateSetAlphaFunc(int, float)
{
}

void C4JRender::StateSetDepthFunc(int)
{
}

void C4JRender::StateSetFaceCull(bool)
{
}

void C4JRender::StateSetFaceCullCW(bool)
{
}

void C4JRender::StateSetLineWidth(float)
{
}

void C4JRender::StateSetWriteEnable(bool, bool, bool, bool)
{
}

void C4JRender::StateSetDepthTestEnable(bool)
{
}

void C4JRender::StateSetAlphaTestEnable(bool)
{
}

void C4JRender::StateSetDepthSlopeAndBias(float, float)
{
}

void C4JRender::StateSetFogEnable(bool)
{
}

void C4JRender::StateSetFogMode(int)
{
}

void C4JRender::StateSetFogNearDistance(float)
{
}

void C4JRender::StateSetFogFarDistance(float)
{
}

void C4JRender::StateSetFogDensity(float)
{
}

void C4JRender::StateSetFogColour(float, float, float)
{
}

void C4JRender::StateSetLightingEnable(bool)
{
}

void C4JRender::StateSetVertexTextureUV(float, float)
{
}

void C4JRender::StateSetLightColour(int, float, float, float)
{
}

void C4JRender::StateSetLightAmbientColour(float, float, float)
{
}

void C4JRender::StateSetLightDirection(int, float, float, float)
{
}

void C4JRender::StateSetLightEnable(int, bool)
{
}

void C4JRender::StateSetViewport(eViewportType)
{
}

void C4JRender::StateSetEnableViewportClipPlanes(bool)
{
}

void C4JRender::StateSetTexGenCol(int, float, float, float, float, bool)
{
}

void C4JRender::StateSetStencil(int, uint8_t, uint8_t, uint8_t)
{
}

void C4JRender::StateSetForceLOD(int)
{
}

void C4JRender::BeginEvent(const char *)
{
}

void C4JRender::EndEvent()
{
}

void C4JRender::Suspend()
{
    g_render.suspended = true;
}

bool C4JRender::Suspended()
{
    return g_render.suspended;
}

void C4JRender::Resume()
{
    g_render.suspended = false;
}
