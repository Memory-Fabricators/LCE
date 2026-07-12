#pragma once

// Private implementation state for C4JRender's OpenGL ES / ANGLE backend.
// Only 4J_Render.cpp includes this - GL types must never leak into
// the public 4J_Render.h, which is included transitively by a lot of
// gameplay code that doesn't need to know about the render backend.

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <SDL3/SDL.h>

#include <unordered_map>
#include <vector>

// EGL for context management
#include <EGL/egl.h>
#include <EGL/eglext.h>

// Maximum number of texture units we track
static const int MAX_TEXTURE_UNITS = 8;

// Vertex format sizes (in floats)
static const int VERTEX_SIZE_PF3_TF2_CB4_NB4_XW1 = 3 + 2 + 4 + 4 + 1; // 14 floats
static const int VERTEX_SIZE_COMPRESSED = 1;                          // Packed 32-bit per component

struct TextureState
{
    GLuint glTexture = 0;
    int width = 0;
    int height = 0;
    int levels = 1;
    bool used = false;
};

struct C4JRenderState
{
    SDL_Window *window = nullptr;
    SDL_GLContext glContext = nullptr;
    SDL_GLContext workerContexts[8] = {nullptr};

    int surfaceWidth = 0;
    int surfaceHeight = 0;

    float clearColour[4] = {0.4f, 0.6f, 0.9f, 1.0f};

    // Matrix stacks (modelview=0, projection=1, texture=2)
    static const int MATRIX_STACK_DEPTH = 32;
    std::vector<float> matrixStack[3];
    int matrixMode = 0;
    float currentMatrix[16];

    // Texture state
    TextureState textures[256];
    int boundTexture = 0;
    int boundVertexTexture = -1;
    int activeTextureUnit = 0;

    // Render state
    bool blendEnabled = false;
    bool depthTestEnabled = false;
    bool depthMask = true;
    bool alphaTestEnabled = false;
    bool cullEnabled = false;
    bool cullCW = false;
    bool fogEnabled = false;
    float fogColour[3] = {0.0f, 0.0f, 0.0f};
    float fogNear = 0.0f;
    float fogFar = 1.0f;
    float fogDensity = 1.0f;
    int fogMode = 0; // 0=linear, 1=exp, 2=exp2
    float colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float vertexUV[2] = {0.0f, 0.0f};
    int lightEnabled[2] = {0, 0};
    float lightColour[2][3] = {{1, 1, 1}, {1, 1, 1}};
    float lightAmbient[3] = {0.2f, 0.2f, 0.2f};
    float lightDirection[2][3] = {{0, 0, -1}, {0, 0, -1}};

    // Viewport clipping
    bool viewportClipEnabled = false;

    // Stencil
    int stencilFunc = GL_ALWAYS;
    uint8_t stencilRef = 0;
    uint8_t stencilFuncMask = 0xFF;
    uint8_t stencilWriteMask = 0xFF;

    bool suspended = false;
    bool frameActive = false;

    // Debug instrumentation only - reset each Present()
    int debugDrawCallsThisFrame = 0;

    // Current pass state
    int currentPrimitiveType = 0;
};

extern C4JRenderState g_render;

// Helper to load identity matrix
void MatrixLoadIdentity(float *m);
void MatrixMultiply(float *result, const float *a, const float *b);
void MatrixPerspective(float *m, float fovy, float aspect, float zNear, float zFar);
void MatrixOrtho(float *m, float left, float right, float bottom, float top, float zNear, float zFar);
void MatrixTranslate(float *m, float x, float y, float z);
void MatrixRotate(float *m, float angle, float x, float y, float z);
void MatrixScale(float *m, float x, float y, float z);
