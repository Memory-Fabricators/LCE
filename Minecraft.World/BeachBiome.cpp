#include "BeachBiome.h"
#include "BiomeDecorator.h"
#include "net.minecraft.world.level.tile.h"
#include "stdafx.h"

BeachBiome::BeachBiome(int id) : Biome(id)
{
    // remove default mob spawn settings
    friendlies.clear();
    friendlies_chicken.clear(); // 4J added
    this->topMaterial = (unsigned char)Tile::sand_Id;
    this->material = (unsigned char)Tile::sand_Id;

    decorator->treeCount = -999;
    decorator->deadBushCount = 0;
    decorator->reedsCount = 0;
    decorator->cactusCount = 0;
}
