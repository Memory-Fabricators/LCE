#include "net.minecraft.world.effect.h"
#include "stdafx.h"

InstantenousMobEffect::InstantenousMobEffect(int id, bool isHarmful, eMinecraftColour color) : MobEffect(id, isHarmful, color)
{
}

bool InstantenousMobEffect::isInstantenous()
{
    return true;
}

bool InstantenousMobEffect::isDurationEffectTick(int remainingDuration, int amplification)
{
    return remainingDuration >= 1;
}
