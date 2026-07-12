#include "GiantMobRenderer.h"
#include "stdafx.h"
#include <GL/gl.h>

GiantMobRenderer::GiantMobRenderer(Model *model, float shadow, float _scale) : MobRenderer(model, shadow * _scale)
{
    this->_scale = _scale;
}

void GiantMobRenderer::scale(shared_ptr<Mob> mob, float a)
{
    glScalef(_scale, _scale, _scale);
}
