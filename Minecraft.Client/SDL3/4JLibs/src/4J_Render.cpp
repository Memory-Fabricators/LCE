// Deliberately avoids ../../../stdafx.h: that chain pulls in Minecraft.World
// headers with `using namespace std;` + a project-defined `byte` typedef,
// which becomes ambiguous with std::byte once this TU is compiled as C++17
// (required by Dawn's webgpu_cpp.h). This file only needs the render
// interface + Windows-compat types, not the full game header chain.
#include "../inc/4J_Render.h"
#include "../../WindowsTypes.h"
#include "4J_RenderImpl.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <expected>
#include <print>

#include <mutex>

static std::mutex g_glCompileMutex;

// The game-facing 4J_Render.h declares GL_* as small int constants (GL_TEXTURE_MIN_FILTER==1,
// GL_NEAREST==0, GL_REPEAT==1, ...). 4J_RenderImpl.h pulls in the real GL headers, whose macros
// then shadow those names with the true GL enum values (0x2801, 0x2600, ...) for the rest of
// this file. TextureSetParam(), ToGLBlendFunc(), and ToGLDepthFunc() are handed the game's values
// by the client, so use hardcoded constants here to avoid compiling against the shadowed real GL
// macros, which would cause the comparisons to never match.
namespace GameEnum
{
constexpr int TexMinFilter = 1;
constexpr int TexMagFilter = 2;
constexpr int TexWrapS = 3;
constexpr int TexWrapT = 4;
constexpr int Nearest = 0;
constexpr int Linear = 1;
constexpr int Repeat = 1;

constexpr int Greater = 1;
constexpr int Equal = 2;
constexpr int Lequal = 3;
constexpr int Gequal = 4;
constexpr int Always = 5;

constexpr int SrcAlpha = 0;
constexpr int OneMinusSrcAlpha = 1;
constexpr int One = 2;
constexpr int Zero = 3;
constexpr int DstAlpha = 4;
constexpr int SrcColor = 5;
constexpr int DstColor = 6;
constexpr int OneMinusDstColor = 7;
constexpr int OneMinusSrcColor = 8;
} // namespace GameEnum

#include "4J_RenderImpl.h"

#if defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_IOS)
// Implemented in 4J_RenderMetal.mm (Objective-C++) - see that file for why
// this is necessary.
extern "C" void ConfigureMetalLayer(void *layerPtr, int pixelWidth, int pixelHeight, float contentsScale);
#endif

C4JRender RenderManager;
C4JRenderState g_render;

// Software mirror of the modelview/projection/texture matrix stacks, kept
// alongside the real GL stack purely so MatrixGet() can hand back a matrix
// pointer without a GL round-trip. Each chunk-rebuild worker thread has its
// own real GL context (see InitialiseContext()) and therefore its own real
// GL matrix stack - this mirror must be per-thread too, or the main
// thread's camera pushes/pops and every worker thread's chunk-translate
// pushes/pops race on the same std::vector/array (data race; manifests as
// corrupted transforms - e.g. chunk geometry translated off to nowhere).
struct MatrixMirror
{
    std::vector<float> stack[3];
    int mode = 0;
    float current[16];
    MatrixMirror()
    {
        MatrixLoadIdentity(current);
    }
};
static thread_local MatrixMirror t_matrix;

// // EGL state for the native Mesa or fallback ANGLE backend
// static EGLDisplay s_eglDisplay = EGL_NO_DISPLAY;
// static EGLContext s_eglContext = EGL_NO_CONTEXT;
// static EGLSurface s_eglSurface = EGL_NO_SURFACE;
// #if defined(LCE_HAVE_WAYLAND_EGL)
// // Owns the wl_egl_window wrapper handed to EGL on Wayland; it must outlive
// // s_eglSurface and be destroyed on shutdown/resize.
// static struct wl_egl_window *s_wlEglWindow = nullptr;
// #endif

// ============================================================================
// Matrix math utilities
// ============================================================================

void MatrixLoadIdentity(float *m)
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void MatrixMultiply(float *result, const float *a, const float *b)
{
    float tmp[16];
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            tmp[j * 4 + i] = 0;
            for (int k = 0; k < 4; k++)
            {
                tmp[j * 4 + i] += a[k * 4 + i] * b[j * 4 + k];
            }
        }
    }
    memcpy(result, tmp, sizeof(tmp));
}

void MatrixPerspective(float *m, float fovy, float aspect, float zNear, float zFar)
{
    memset(m, 0, 16 * sizeof(float));
    float tanHalfFovy = tanf(fovy * 0.5f * 3.14159265f / 180.0f);
    m[0] = 1.0f / (aspect * tanHalfFovy);
    m[5] = 1.0f / tanHalfFovy;
    m[10] = -(zFar + zNear) / (zFar - zNear);
    m[11] = -1.0f;
    m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
}

void MatrixOrtho(float *m, float left, float right, float bottom, float top, float zNear, float zFar)
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = 2.0f / (right - left);
    m[5] = 2.0f / (top - bottom);
    m[10] = -2.0f / (zFar - zNear);
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(zFar + zNear) / (zFar - zNear);
    m[15] = 1.0f;
}

void MatrixTranslate(float *m, float x, float y, float z)
{
    float t[16];
    MatrixLoadIdentity(t);
    t[12] = x;
    t[13] = y;
    t[14] = z;
    float result[16];
    MatrixMultiply(result, m, t);
    memcpy(m, result, sizeof(result));
}

void MatrixRotate(float *m, float angle, float x, float y, float z)
{
    float rad = angle * 3.14159265f / 180.0f;
    float c = cosf(rad);
    float s = sinf(rad);
    float len = sqrtf(x * x + y * y + z * z);
    if (len > 0.0001f)
    {
        x /= len;
        y /= len;
        z /= len;
    }

    float r[16];
    MatrixLoadIdentity(r);
    r[0] = x * x * (1 - c) + c;
    r[1] = y * x * (1 - c) + z * s;
    r[2] = x * z * (1 - c) - y * s;
    r[4] = x * y * (1 - c) - z * s;
    r[5] = y * y * (1 - c) + c;
    r[6] = y * z * (1 - c) + x * s;
    r[8] = x * z * (1 - c) + y * s;
    r[9] = y * z * (1 - c) - x * s;
    r[10] = z * z * (1 - c) + c;

    float result[16];
    MatrixMultiply(result, m, r);
    memcpy(m, result, sizeof(result));
}

void MatrixScale(float *m, float x, float y, float z)
{
    float s[16];
    MatrixLoadIdentity(s);
    s[0] = x;
    s[5] = y;
    s[10] = z;
    float result[16];
    MatrixMultiply(result, m, s);
    memcpy(m, result, sizeof(result));
}

// ============================================================================
// GL state helpers
// ============================================================================

static GLenum ToGLBlendFunc(int func)
{
    if (func == GL_SRC_ALPHA || func == GL_ONE_MINUS_SRC_ALPHA ||
        func == GL_ONE || func == GL_ZERO || func == GL_DST_ALPHA ||
        func == GL_SRC_COLOR || func == GL_DST_COLOR ||
        func == GL_ONE_MINUS_DST_COLOR || func == GL_ONE_MINUS_SRC_COLOR)
    {
        return func;
    }
    switch (func)
    {
    case 0:
        return GL_SRC_ALPHA;
    case 1:
        return GL_ONE_MINUS_SRC_ALPHA;
    case 2:
        return GL_ONE;
    case 3:
        return GL_ZERO;
    case 4:
        return GL_DST_ALPHA;
    case 5:
        return GL_SRC_COLOR;
    case 6:
        return GL_DST_COLOR;
    case 7:
        return GL_ONE_MINUS_DST_COLOR;
    case 8:
        return GL_ONE_MINUS_SRC_COLOR;
    default:
        return GL_SRC_ALPHA;
    }
}

static GLenum ToGLDepthFunc(int func)
{
    if (func == GL_GREATER || func == GL_EQUAL || func == GL_LEQUAL ||
        func == GL_GEQUAL || func == GL_ALWAYS || func == GL_LESS ||
        func == GL_NOTEQUAL || func == GL_NEVER)
    {
        return func;
    }
    switch (func)
    {
    case 1:
        return GL_GREATER;
    case 2:
        return GL_EQUAL;
    case 3:
        return GL_LEQUAL;
    case 4:
        return GL_GEQUAL;
    case 5:
        return GL_ALWAYS;
    default:
        return GL_ALWAYS;
    }
}

// ============================================================================
// Initialisation
// ============================================================================

void C4JRender::Initialise(WinitApp *app)
{
    g_render.app = app;
    g_render.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint major = 0, minor = 0;
    eglInitialize(g_render.eglDisplay, &major, &minor);

    EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config = nullptr;
    EGLint numConfigs = 0;
    eglChooseConfig(g_render.eglDisplay, configAttribs, &config, 1, &numConfigs);

    if (app)
    {
        g_render.eglSurface = winit_app_create_egl_surface(app, g_render.eglDisplay, config);
    }
    else
    {
        EGLint pbufferAttribs[] = {
            EGL_WIDTH, 1280,
            EGL_HEIGHT, 720,
            EGL_NONE
        };
        g_render.eglSurface = eglCreatePbufferSurface(g_render.eglDisplay, config, pbufferAttribs);
    }

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    g_render.eglContext = eglCreateContext(g_render.eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    eglMakeCurrent(g_render.eglDisplay, g_render.eglSurface, g_render.eglSurface, g_render.eglContext);

    for (int i = 0; i < 8; i++)
    {
        g_render.workerContexts[i] = eglCreateContext(g_render.eglDisplay, config, g_render.eglContext, contextAttribs);
    }

    fprintf(stderr, "[angle_wgpu] Vendor: %s\n", glGetString(GL_VENDOR));
    fprintf(stderr, "[angle_wgpu] Renderer: %s\n", glGetString(GL_RENDERER));
    fprintf(stderr, "[angle_wgpu] Version: %s\n", glGetString(GL_VERSION));

    // Enable default state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Initialise matrix stacks
    for (int i = 0; i < 3; i++)
    {
        t_matrix.stack[i].reserve(C4JRenderState::MATRIX_STACK_DEPTH * 16);
    }
    MatrixLoadIdentity(t_matrix.current);
    t_matrix.mode = 0;

    // Set up projection matrix for initial viewport
    uint32_t width = 1280, height = 720;
    if (app)
    {
        winit_app_get_size(app, &width, &height);
    }
    g_render.surfaceWidth = width;
    g_render.surfaceHeight = height;

    glViewport(0, 0, width, height);
    glClearColor(g_render.clearColour[0], g_render.clearColour[1],
                 g_render.clearColour[2], g_render.clearColour[3]);
}

void C4JRender::Initialise(SDL_Window *)
{
    Initialise(static_cast<WinitApp *>(nullptr));
}

void C4JRender::InitialiseContext()
{
    static std::atomic<int> s_contextIndex(0);
    int idx = s_contextIndex.fetch_add(1);
    if (idx < 8 && g_render.workerContexts[idx])
    {
        eglMakeCurrent(g_render.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, g_render.workerContexts[idx]);
    }
    else
    {
        EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        EGLContext ctx = eglCreateContext(g_render.eglDisplay, nullptr, g_render.eglContext, contextAttribs);
        if (ctx)
        {
            eglMakeCurrent(g_render.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx);
        }
    }
}

void C4JRender::Tick()
{
}

void C4JRender::UpdateGamma(unsigned short)
{
}

void C4JRender::StartFrame()
{
    if (g_render.app)
    {
        uint32_t width = 0, height = 0;
        winit_app_get_size(g_render.app, &width, &height);
        if (width > 0 && height > 0 &&
            (width != (uint32_t)g_render.surfaceWidth || height != (uint32_t)g_render.surfaceHeight))
        {
            g_render.surfaceWidth = width;
            g_render.surfaceHeight = height;
            glViewport(0, 0, width, height);
        }
    }
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
    eglSwapBuffers(g_render.eglDisplay, g_render.eglSurface);
    g_render.debugDrawCallsThisFrame = 0;
    g_render.frameActive = false;
}
void C4JRender::Clear(int flags, void *)
{
    constexpr int kClearDepth = 1;
    constexpr int kClearColour = 2;
    GLbitfield mask = 0;
    if (flags & kClearDepth)
    {
        mask |= GL_DEPTH_BUFFER_BIT;
        glClearDepthf(1.0f);
    }
    if (flags & kClearColour)
    {
        mask |= GL_COLOR_BUFFER_BIT;
        glClearColor(g_render.clearColour[0], g_render.clearColour[1],
                     g_render.clearColour[2], 1.0f);
    }
    if (mask)
    {
        glClear(mask);
    }
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
    int width = g_render.surfaceWidth, height = g_render.surfaceHeight;
    if (height <= 0)
    {
        return true;
    }
    return ((float)width / (float)height) > 1.4f;
}

bool C4JRender::IsHiDef()
{
    int width = g_render.surfaceWidth, height = g_render.surfaceHeight;
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

// ============================================================================
// Matrix stack
// ============================================================================

static GLenum ToGLMatrixMode(int mode)
{
    switch (mode)
    {
    case 0:
        return GL_MODELVIEW;
    case 1:
        return GL_PROJECTION;
    case 2:
        return GL_TEXTURE;
    default:
        return GL_MODELVIEW;
    }
}

void C4JRender::MatrixMode(int type)
{
    t_matrix.mode = type;
    glMatrixMode(ToGLMatrixMode(type));
}

void C4JRender::MatrixSetIdentity()
{
    MatrixLoadIdentity(t_matrix.current);
    glLoadIdentity();
}

void C4JRender::MatrixTranslate(float x, float y, float z)
{
    ::MatrixTranslate(t_matrix.current, x, y, z);
    glTranslatef(x, y, z);
}

void C4JRender::MatrixRotate(float angle, float x, float y, float z)
{
    ::MatrixRotate(t_matrix.current, angle, x, y, z);
    glRotatef(angle, x, y, z);
}

void C4JRender::MatrixScale(float x, float y, float z)
{
    ::MatrixScale(t_matrix.current, x, y, z);
    glScalef(x, y, z);
}

void C4JRender::MatrixPerspective(float fovy, float aspect, float zNear, float zFar)
{
    ::MatrixPerspective(t_matrix.current, fovy, aspect, zNear, zFar);
    // GLES1 doesn't have glFrustumf-style perspective, build manually
    float f = 1.0f / tanf(fovy * 0.5f * 3.14159265f / 180.0f);
    float m[16] = {};
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    glMultMatrixf(m);
}

void C4JRender::MatrixOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar)
{
    ::MatrixOrtho(t_matrix.current, left, right, bottom, top, zNear, zFar);
#if defined(LCE_USE_MESA_GL)
    glOrtho(left, right, bottom, top, zNear, zFar);
#else
    glOrthof(left, right, bottom, top, zNear, zFar);
#endif
}

void C4JRender::MatrixPop()
{
    auto &stack = t_matrix.stack[t_matrix.mode];
    if (stack.size() >= 16)
    {
        // Read the top-of-stack entry being restored *before* shrinking:
        // this used to resize() first and then read from
        // stack.data() + stack.size() - 16 using the already-shrunk size,
        // which reads the *second-to-top* entry instead of the one just
        // popped - and, whenever the stack held exactly one frame (size
        // 16, the common case for a single MatrixPush()/MatrixPop() pair,
        // e.g. every Chunk::rebuild() call), resize(0) first made that
        // 0 - 16 underflow as size_t, handing memcpy a wild pointer and
        // segfaulting (intermittently, since it depends on what happens to
        // be mapped at that huge offset - see Chunk::rebuild() and its
        // worker-thread callers in LevelRenderer::updateDirtyChunks()).
        memcpy(t_matrix.current, stack.data() + stack.size() - 16, 16 * sizeof(float));
        stack.resize(stack.size() - 16);
    }
    glPopMatrix();
}

void C4JRender::MatrixPush()
{
    auto &stack = t_matrix.stack[t_matrix.mode];
    stack.resize(stack.size() + 16);
    memcpy(stack.data() + stack.size() - 16, t_matrix.current, 16 * sizeof(float));
    glPushMatrix();
}

void C4JRender::MatrixMult(float *mat)
{
    float result[16];
    MatrixMultiply(result, t_matrix.current, mat);
    memcpy(t_matrix.current, result, sizeof(result));
    glMultMatrixf(mat);
}

const float *C4JRender::MatrixGet(int type)
{
    if (type >= 0 && type < 3 && !t_matrix.stack[type].empty())
    {
        size_t sz = t_matrix.stack[type].size();
        return t_matrix.stack[type].data() + sz - 16;
    }
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

// ============================================================================
// Chunk vertex buffers - real OpenGL display lists
// ============================================================================
//
// Chunk terrain is tessellated on a pool of worker threads (see
// LevelRenderer::rebuildChunkThreadProc / MAX_CHUNK_REBUILD_THREADS) so it
// doesn't have to be redone every frame. Each worker thread gets its own GL
// context sharing display lists/textures with the main context (see
// InitialiseContext() above and SDL_GL_SHARE_WITH_CURRENT_CONTEXT in
// Initialise()), so it can compile a real GL_COMPILE display list directly:
// Tesselator's vertex()/end() issue plain GL calls (glVertexPointer,
// glDrawArrays, ...; see Tesselator.cpp), which the driver captures into
// whichever list is open on that thread. The main thread just calls
// glCallList() to replay already-compiled chunk geometry - list IDs come
// from real glGenLists() (see MemoryTracker::genLists, used by
// LevelRenderer to reserve one pair of IDs per chunk slot), exactly like
// every other display list in this codebase (LevelRenderer's sky/star/cloud
// lists, ModelPart's model lists). There's no separate software
// command-buffer abstraction to keep in sync with real GL state.
//
void C4JRender::CBuffLockStaticCreations()
{
}

int C4JRender::CBuffCreate(int count)
{
    return (int)glGenLists(count);
}

void C4JRender::CBuffDelete(int first, int count)
{
    glDeleteLists((GLuint)first, count);
}

void C4JRender::CBuffStart(int index, bool full)
{
    glNewList((GLuint)index, GL_COMPILE);
}

void C4JRender::CBuffClear(int index)
{
    glNewList((GLuint)index, GL_COMPILE);
    glEndList();
    glFlush();
}

int C4JRender::CBuffSize(int index)
{
    // No software tracking of display-list GPU memory - the driver manages
    // it. LevelRenderer's chunk-rebuild throttle treats 0 as "never
    // throttle", which is fine now that there's no CPU-side buffer to bound.
    return 0;
}

void C4JRender::CBuffEnd()
{
    glEndList();
    glFlush();
}

bool C4JRender::CBuffCall(int index, bool full)
{
    if (index < 0 || !g_render.frameActive)
    {
        return false;
    }
    glCallList((GLuint)index);
    return true;
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

// ============================================================================
// Texture management
// ============================================================================

int C4JRender::TextureCreate()
{
    for (int i = 0; i < 256; i++)
    {
        if (!g_render.textures[i].used)
        {
            glGenTextures(1, &g_render.textures[i].glTexture);
            g_render.textures[i].used = true;
            g_render.textures[i].width = 0;
            g_render.textures[i].height = 0;
            return i;
        }
    }
    return -1;
}

void C4JRender::TextureFree(int idx)
{
    if (idx >= 0 && idx < 256 && g_render.textures[idx].used)
    {
        glDeleteTextures(1, &g_render.textures[idx].glTexture);
        g_render.textures[idx] = TextureState();
    }
}

void C4JRender::TextureBind(int idx)
{
    if (idx >= 0 && idx < 256 && g_render.textures[idx].used)
    {
        // Binding a texture in GLES1 fixed-function does nothing on its own -
        // GL_TEXTURE_2D must be enabled for the texture to actually be sampled.
        // The game controls texturing purely through bind (glEnable(GL_TEXTURE_2D)
        // is a no-op in glWrapper.cpp), so enable it here on a valid bind.
        // Only enable if the texture actually has image data: sampling an
        // empty/incomplete texture yields black, so fall back to untextured
        // (vertex-colour) rendering rather than a black surface when a texture
        // hasn't been uploaded yet.
        glBindTexture(GL_TEXTURE_2D, g_render.textures[idx].glTexture);
        if (getenv("LCE_TEXDIAG"))
        {
            fprintf(stderr, "[TEXDIAG] TextureBind tex=%d glId=%u size=%dx%d\n", idx, g_render.textures[idx].glTexture, g_render.textures[idx].width, g_render.textures[idx].height);
        }
        if (g_render.textures[idx].width > 0 && g_render.textures[idx].height > 0)
        {
            glEnable(GL_TEXTURE_2D);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }
        g_render.boundTexture = idx;
    }
    else if (idx < 0)
    {
        glDisable(GL_TEXTURE_2D);
        g_render.boundTexture = -1;
    }
}

void C4JRender::TextureBindVertex(int idx)
{
    // OpenGL ES 2.0 doesn't have vertex texture fetch - stub
}

#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

void C4JRender::TextureSetTextureLevels(int levels)
{
    if (g_render.boundTexture >= 0 && g_render.boundTexture < 256)
    {
        g_render.textures[g_render.boundTexture].levels = levels;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levels - 1);
    }
}

int C4JRender::TextureGetTextureLevels()
{
    if (g_render.boundTexture >= 0 && g_render.boundTexture < 256)
    {
        return g_render.textures[g_render.boundTexture].levels;
    }
    return 1;
}

void C4JRender::TextureData(int width, int height, void *data, int level, eTextureFormat format)
{
    GLenum glFormat = GL_RGBA;
    GLenum glType = GL_UNSIGNED_BYTE;

    glTexImage2D(GL_TEXTURE_2D, level, glFormat, width, height, 0,
                 glFormat, glType, data);

    if (g_render.boundTexture >= 0 && g_render.boundTexture < 256)
    {
        if (level == 0)
        {
            g_render.textures[g_render.boundTexture].width = width;
            g_render.textures[g_render.boundTexture].height = height;
        }
        if (getenv("LCE_TEXDIAG"))
        {
            fprintf(stderr, "[TEXDIAG] TextureData tex=%d level=%d size=%dx%d\n", g_render.boundTexture, level, width, height);
        }
    }
}

void C4JRender::TextureDataUpdate(int xoffset, int yoffset, int width, int height, void *data, int level)
{
    glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width, height,
                    GL_RGBA, GL_UNSIGNED_BYTE, data);
    if (getenv("LCE_TEXDIAG") && g_render.boundTexture >= 0 && g_render.boundTexture < 256)
    {
        fprintf(stderr, "[TEXDIAG] TextureDataUpdate tex=%d level=%d offset=%d,%d size=%dx%d\n", g_render.boundTexture, level, xoffset, yoffset, width, height);
    }
}

void C4JRender::TextureSetParam(int param, int value)
{
    GLenum glParam;
    GLint glValue;

    if (param == GL_TEXTURE_MIN_FILTER || param == 1)
    {
        glParam = GL_TEXTURE_MIN_FILTER;
        // Avoid incomplete texture errors by mapping mipmapped filters to non-mipmapped filters
        if (value == GL_LINEAR || value == 1 || value == GL_LINEAR_MIPMAP_NEAREST || value == GL_LINEAR_MIPMAP_LINEAR)
        {
            glValue = GL_LINEAR;
        }
        else
        {
            glValue = GL_NEAREST;
        }
    }
    else if (param == GL_TEXTURE_MAG_FILTER || param == 2)
    {
        glParam = GL_TEXTURE_MAG_FILTER;
        if (value == GL_LINEAR || value == 1)
        {
            glValue = GL_LINEAR;
        }
        else
        {
            glValue = GL_NEAREST;
        }
    }
    else if (param == GL_TEXTURE_WRAP_S || param == 3)
    {
        glParam = GL_TEXTURE_WRAP_S;
        if (value == GL_REPEAT || value == 1)
        {
            glValue = GL_REPEAT;
        }
        else
        {
            glValue = GL_CLAMP_TO_EDGE;
        }
    }
    else if (param == GL_TEXTURE_WRAP_T || param == 4)
    {
        glParam = GL_TEXTURE_WRAP_T;
        if (value == GL_REPEAT || value == 1)
        {
            glValue = GL_REPEAT;
        }
        else
        {
            glValue = GL_CLAMP_TO_EDGE;
        }
    }
    else
    {
        return;
    }

    if (getenv("LCE_TEXDIAG"))
    {
        fprintf(stderr, "[TEXDIAG] tex=%d param=%d value=%d -> glParam=0x%x glValue=0x%x\n",
                g_render.boundTexture, param, value, (unsigned)glParam, (unsigned)glValue);
    }
    glTexParameteri(GL_TEXTURE_2D, glParam, glValue);
}
void C4JRender::TextureDynamicUpdateStart()
{
}

void C4JRender::TextureDynamicUpdateEnd()
{
}

void C4JRender::TextureGetStats()
{
}

void *C4JRender::TextureGetTexture(int idx)
{
    if (idx >= 0 && idx < 256 && g_render.textures[idx].used)
    {
        return (void *)(intptr_t)g_render.textures[idx].glTexture;
    }
    return nullptr;
}

// ============================================================================
// Render state
// ============================================================================

void C4JRender::StateSetColour(float r, float g, float b, float a)
{
    g_render.colour[0] = r;
    g_render.colour[1] = g;
    g_render.colour[2] = b;
    g_render.colour[3] = a;
    glColor4f(r, g, b, a);
}

void C4JRender::StateSetDepthMask(bool enable)
{
    g_render.depthMask = enable;
    glDepthMask(enable ? GL_TRUE : GL_FALSE);
}

void C4JRender::StateSetBlendEnable(bool enable)
{
    if (getenv("LCE_STATEDIAG"))
    {
        fprintf(stderr, "[STATEDIAG] BlendEnable %d\n", enable);
    }
    g_render.blendEnabled = enable;
    if (enable)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void C4JRender::StateSetBlendFunc(int src, int dst)
{
    GLenum glSrc = ToGLBlendFunc(src);
    GLenum glDst = ToGLBlendFunc(dst);
    if (getenv("LCE_STATEDIAG"))
    {
        fprintf(stderr, "[STATEDIAG] BlendFunc %d (%x), %d (%x) -> %x, %x\n", src, src, dst, dst, glSrc, glDst);
    }
    glBlendFunc(glSrc, glDst);
}

void C4JRender::StateSetBlendFactor(unsigned int colour)
{
    // glBlendColor is GLES2-only. GLES1 has no constant-colour blend factors,
    // so there is no fixed-function equivalent to apply here.
    (void)colour;
}

void C4JRender::StateSetAlphaFunc(int func, float param)
{
    GLenum glFunc = ToGLDepthFunc(func);
    if (getenv("LCE_STATEDIAG"))
    {
        fprintf(stderr, "[STATEDIAG] AlphaFunc %d (%x), %f -> %x, %f\n", func, func, param, glFunc, param);
    }
    glAlphaFunc(glFunc, param);
}

void C4JRender::StateSetDepthFunc(int func)
{
    GLenum glFunc = ToGLDepthFunc(func);
    if (getenv("LCE_STATEDIAG"))
    {
        fprintf(stderr, "[STATEDIAG] DepthFunc %d (%x) -> %x\n", func, func, glFunc);
    }
    glDepthFunc(glFunc);
}

void C4JRender::StateSetFaceCull(bool enable)
{
    g_render.cullEnabled = enable;
    if (enable)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void C4JRender::StateSetFaceCullCW(bool enable)
{
    // The game calls this with `enable` meaning "cull the face wound
    // clockwise" (glCullFace(GL_BACK) when winding is standard CCW-front).
    // glCullFace(dir) itself picks *which* face to cull, not which winding
    // counts as front - GL always treats CCW as front by default. The old
    // mapping (enable -> GL_FRONT) culled the camera-facing faces on this
    // engine's CCW-wound geometry, discarding real terrain faces while
    // leaving their (normally invisible) backfaces to poke through gaps.
    g_render.cullCW = enable;
    glCullFace(enable ? GL_BACK : GL_FRONT);
}

void C4JRender::StateSetLineWidth(float width)
{
    glLineWidth(width);
}

void C4JRender::StateSetWriteEnable(bool red, bool green, bool blue, bool alpha)
{
    glColorMask(red ? GL_TRUE : GL_FALSE, green ? GL_TRUE : GL_FALSE,
                blue ? GL_TRUE : GL_FALSE, alpha ? GL_TRUE : GL_FALSE);
}

void C4JRender::StateSetDepthTestEnable(bool enable)
{
    if (getenv("LCE_STATEDIAG"))
    {
        fprintf(stderr, "[STATEDIAG] DepthTestEnable %d\n", enable);
    }
    g_render.depthTestEnabled = enable;
    if (enable)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void C4JRender::StateSetAlphaTestEnable(bool enable)
{
    if (getenv("LCE_STATEDIAG"))
    {
        fprintf(stderr, "[STATEDIAG] AlphaTestEnable %d\n", enable);
    }
    g_render.alphaTestEnabled = enable;
    if (enable)
    {
        glEnable(GL_ALPHA_TEST);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
}

void C4JRender::StateSetDepthSlopeAndBias(float slope, float bias)
{
    // OpenGL ES 2.0 doesn't support polygon offset in the same way
    if (slope != 0.0f || bias != 0.0f)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(slope, bias);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void C4JRender::StateSetFogEnable(bool enable)
{
    g_render.fogEnabled = enable;
    if (enable)
    {
        glEnable(GL_FOG);
    }
    else
    {
        glDisable(GL_FOG);
    }
}

void C4JRender::StateSetFogMode(int mode)
{
    // `mode` arrives as the game's fog-mode constant (4J_Render.h: GL_LINEAR=1,
    // GL_EXP=2), NOT the real GL enum. The previous mapping treated 1 as EXP
    // and 2 as EXP2, so the game's *linear* fog became exponential fog with a
    // stale density that saturated the whole scene to the fog colour - i.e.
    // everything rendered as (near-black) fog.
    g_render.fogMode = mode;
    GLenum glMode = GL_LINEAR;
    if (mode == 2) // game GL_EXP
    {
        glMode = GL_EXP;
    }
    glFogf(GL_FOG_MODE, (GLfloat)glMode);
}

void C4JRender::StateSetFogNearDistance(float dist)
{
    g_render.fogNear = dist;
    glFogf(GL_FOG_START, dist);
}

void C4JRender::StateSetFogFarDistance(float dist)
{
    g_render.fogFar = dist;
    glFogf(GL_FOG_END, dist);
}

void C4JRender::StateSetFogDensity(float density)
{
    g_render.fogDensity = density;
    glFogf(GL_FOG_DENSITY, density);
}

void C4JRender::StateSetFogColour(float red, float green, float blue)
{
    g_render.fogColour[0] = red;
    g_render.fogColour[1] = green;
    g_render.fogColour[2] = blue;
    GLfloat fogColor[4] = {red, green, blue, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
}

void C4JRender::StateSetLightingEnable(bool enable)
{
    if (enable)
    {
        glEnable(GL_LIGHTING);
    }
    else
    {
        glDisable(GL_LIGHTING);
    }
}

void C4JRender::StateSetVertexTextureUV(float u, float v)
{
    g_render.vertexUV[0] = u;
    g_render.vertexUV[1] = v;
}

void C4JRender::StateSetLightColour(int light, float red, float green, float blue)
{
    if (light >= 0 && light < 2)
    {
        g_render.lightColour[light][0] = red;
        g_render.lightColour[light][1] = green;
        g_render.lightColour[light][2] = blue;
    }
}

void C4JRender::StateSetLightAmbientColour(float red, float green, float blue)
{
    g_render.lightAmbient[0] = red;
    g_render.lightAmbient[1] = green;
    g_render.lightAmbient[2] = blue;
}

void C4JRender::StateSetLightDirection(int light, float x, float y, float z)
{
    if (light >= 0 && light < 2)
    {
        g_render.lightDirection[light][0] = x;
        g_render.lightDirection[light][1] = y;
        g_render.lightDirection[light][2] = z;
    }
}

void C4JRender::StateSetLightEnable(int light, bool enable)
{
    if (light >= 0 && light < 2)
    {
        g_render.lightEnabled[light] = enable ? 1 : 0;
    }
}

void C4JRender::StateSetViewport(eViewportType viewportType)
{
    int w = g_render.surfaceWidth;
    int h = g_render.surfaceHeight;

    int x = 0, y = 0;
    int vw = w, vh = h;

    switch (viewportType)
    {
    case VIEWPORT_TYPE_FULLSCREEN:
        break;
    case VIEWPORT_TYPE_SPLIT_TOP:
        vh = h / 2;
        break;
    case VIEWPORT_TYPE_SPLIT_BOTTOM:
        y = h / 2;
        vh = h / 2;
        break;
    case VIEWPORT_TYPE_SPLIT_LEFT:
        vw = w / 2;
        break;
    case VIEWPORT_TYPE_SPLIT_RIGHT:
        x = w / 2;
        vw = w / 2;
        break;
    case VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
        vw = w / 2;
        vh = h / 2;
        break;
    case VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
        x = w / 2;
        vw = w / 2;
        vh = h / 2;
        break;
    case VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
        y = h / 2;
        vw = w / 2;
        vh = h / 2;
        break;
    case VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
        x = w / 2;
        y = h / 2;
        vw = w / 2;
        vh = h / 2;
        break;
    }

    glViewport(x, y, vw, vh);

    // Update projection matrix aspect ratio
    float aspect = (vh > 0) ? (float)vw / (float)vh : 1.0f;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // The game will set its own projection - this is just the viewport
    glMatrixMode(GL_MODELVIEW);
}

void C4JRender::StateSetEnableViewportClipPlanes(bool enable)
{
    g_render.viewportClipEnabled = enable;
}

void C4JRender::StateSetTexGenCol(int col, float x, float y, float z, float w, bool eyeSpace)
{
    // ANGLE doesn't implement glTexGen - store state for potential shader use later
}

void C4JRender::StateSetStencil(int Function, uint8_t stencil_ref, uint8_t stencil_func_mask, uint8_t stencil_write_mask)
{
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(ToGLDepthFunc(Function), stencil_ref, stencil_func_mask);
    glStencilMask(stencil_write_mask);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void C4JRender::StateSetForceLOD(int LOD)
{
    // OpenGL ES 2.0 doesn't support explicit LOD control
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
