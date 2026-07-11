#include "net.minecraft.world.level.h"
#include "stdafx.h"

#include "Sponge.h"

const int RANGE = 2;

Sponge::Sponge(int id) : Tile(id, Material::sponge)
{
}
