#include "DefaultRenderer.h"
#include "..\Minecraft.World\net.minecraft.world.entity.h"
#include "stdafx.h"

void DefaultRenderer::render(shared_ptr<Entity> entity, double x, double y, double z, float rot, float a)
{
    glPushMatrix();
    // 4J - removed following line as doesn't really make any sense
    //    render(entity->bb, (x-entity->xOld), (y-entity->yOld), (z-entity->zOld));
    glPopMatrix();
}
