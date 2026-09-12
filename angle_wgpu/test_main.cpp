#include "angle_wgpu.h"
#include <cassert>
#include <cstdio>
#include <cmath>

int main()
{
    printf("[Test] Initializing EGL with angle_wgpu...\n");
    EGLDisplay dpy = eglGetDisplay(nullptr);
    assert(dpy != nullptr);

    EGLint major = 0, minor = 0;
    EGLBoolean init_res = eglInitialize(dpy, &major, &minor);
    assert(init_res == EGL_TRUE);
    printf("[Test] EGL initialized version %d.%d\n", major, minor);

    EGLConfig config = nullptr;
    EGLint num_configs = 0;
    EGLint attribs[] = { EGL_NONE };
    EGLBoolean choose_res = eglChooseConfig(dpy, attribs, &config, 1, &num_configs);
    assert(choose_res == EGL_TRUE);

    EGLSurface surf = eglCreatePbufferSurface(dpy, config, attribs);
    assert(surf != nullptr);

    EGLContext ctx = eglCreateContext(dpy, config, nullptr, nullptr);
    assert(ctx != nullptr);

    EGLBoolean make_res = eglMakeCurrent(dpy, surf, surf, ctx);
    assert(make_res == EGL_TRUE);
    printf("[Test] eglMakeCurrent successful!\n");

    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    printf("[Test] GL_VENDOR: %s\n", vendor);
    printf("[Test] GL_RENDERER: %s\n", renderer);
    printf("[Test] GL_VERSION: %s\n", version);

    // Test matrix operations
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(10.0f, 20.0f, 0.0f);
    glScalef(2.0f, 2.0f, 1.0f);

    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    printf("[Test] ModelView translation: (%f, %f, %f)\n", mv[12], mv[13], mv[14]);
    assert(std::abs(mv[12] - 10.0f) < 1e-4);
    assert(std::abs(mv[13] - 20.0f) < 1e-4);

    // Test display lists
    GLuint list = glGenLists(1);
    assert(list > 0);
    assert(glIsList(list) == GL_TRUE);

    glNewList(list, GL_COMPILE);
    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    glVertex3f(10.0f, 0.0f, 0.0f);
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    glEnd();
    glEndList();

    // Call display list
    glCallList(list);

    // Clear and draw
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDeleteLists(list, 1);

    // Test texture creation
    GLuint tex = 0;
    glGenTextures(1, &tex);
    assert(tex > 0);
    glBindTexture(GL_TEXTURE_2D, tex);

    uint32_t pixels[4] = { 0xFFFFFFFF, 0xFF0000FF, 0x00FF00FF, 0x0000FFFF };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(100.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(100.0f, 100.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, 100.0f, 0.0f);
    glEnd();

    glDeleteTextures(1, &tex);


    eglSwapBuffers(dpy, surf);

    eglMakeCurrent(dpy, nullptr, nullptr, nullptr);
    eglDestroyContext(dpy, ctx);
    eglDestroySurface(dpy, surf);
    eglTerminate(dpy);

    printf("[Test] All angle_wgpu tests passed successfully!\n");
    return 0;
}
