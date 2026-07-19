#include "Lighting.h"
#include "../Minecraft.World/FloatBuffer.h"
#include "../Minecraft.World/Vec3.h"
#include "angle_wgpu.h"

FloatBuffer *Lighting::lb = new FloatBuffer(16);

void Lighting::turnOff()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_COLOR_MATERIAL);
}

void Lighting::turnOn()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    // FIXME: glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    float a = 0.4f;
    float d = 0.6f;
    float s = 0.0f;

    Vec3 *l = Vec3::newTemp(0.2f, 1.0f, -0.7f)->normalize();
    glLightfv(GL_LIGHT0, GL_POSITION, getBuffer(l->x, l->y, l->z, 0)->_getDataPointer());
    glLightfv(GL_LIGHT0, GL_DIFFUSE, getBuffer(d, d, d, 1)->_getDataPointer());
    glLightfv(GL_LIGHT0, GL_AMBIENT, getBuffer(0.0f, 0.0f, 0.0f, 1.0f)->_getDataPointer());
    glLightfv(GL_LIGHT0, GL_SPECULAR, getBuffer(s, s, s, 1.0f)->_getDataPointer());

    l = Vec3::newTemp(-0.2f, 1.0f, 0.7f)->normalize();
    glLightfv(GL_LIGHT1, GL_POSITION, getBuffer(l->x, l->y, l->z, 0)->_getDataPointer());
    glLightfv(GL_LIGHT1, GL_DIFFUSE, getBuffer(d, d, d, 1)->_getDataPointer());
    glLightfv(GL_LIGHT1, GL_AMBIENT, getBuffer(0.0f, 0.0f, 0.0f, 1.0f)->_getDataPointer());
    glLightfv(GL_LIGHT1, GL_SPECULAR, getBuffer(s, s, s, 1.0f)->_getDataPointer());

    glShadeModel(GL_FLAT);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, getBuffer(a, a, a, 1)->_getDataPointer());
}

FloatBuffer *Lighting::getBuffer(double a, double b, double c, double d)
{
    return getBuffer((float)a, (float)b, (float)c, (float)d);
}

FloatBuffer *Lighting::getBuffer(float a, float b, float c, float d)
{
    lb->clear();
    lb->put(a)->put(b)->put(c)->put(d);
    lb->flip();
    return lb;
}

void Lighting::turnOnGui()
{
    glPushMatrix();
    glRotatef(-30, 0, 1, 0);
    glRotatef(165, 1, 0, 0);
    turnOn();
    glPopMatrix();
}
