#include "net.minecraft.world.level.biome.h"
#include "stdafx.h"

PlainsBiome::PlainsBiome(int id) : Biome(id)
{
    decorator->treeCount = -999;
    decorator->flowerCount = 4;
    decorator->grassCount = 10;
}
