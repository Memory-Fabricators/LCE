#include "StoneTile.h"
#include "stdafx.h"

StoneTile::StoneTile(int id) : Tile(id, Material::stone)
{
}

int StoneTile::getResource(int data, Random *random, int playerBonusLevel)
{
    return Tile::cobblestone_Id;
}
