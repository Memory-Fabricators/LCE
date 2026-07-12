// Small Objective-C++ shim so 4J_Render.cpp (plain C++, built with a C++20
// override for Dawn's webgpu_cpp.h) doesn't need Objective-C interop itself.
//
// SDL_Metal_CreateView() creates the CAMetalLayer-backed view but doesn't
// configure the layer beyond defaults - in particular it doesn't set an
// explicit drawableSize/contentsScale/opaque, which on some configurations
// leaves ANGLE rendering into a layer that never actually composites
// anything visible on screen even though every GL call and eglSwapBuffers
// report success (confirmed via screenshot: literal black, not the
// renderer's clear colour, despite hundreds of real draw calls/frame).
#import <QuartzCore/CAMetalLayer.h>

extern "C" void ConfigureMetalLayer(void *layerPtr, int pixelWidth, int pixelHeight, float contentsScale)
{
    CAMetalLayer *layer = (__bridge CAMetalLayer *)layerPtr;
    layer.opaque = YES;
    layer.framebufferOnly = YES;
    layer.contentsScale = contentsScale;
    layer.drawableSize = CGSizeMake(pixelWidth, pixelHeight);
}
