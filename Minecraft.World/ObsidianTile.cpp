#include "ObsidianTile.h"
#include "stdafx.h"

ObsidianTile::ObsidianTile(int id) : StoneTile(id)
{
}

int ObsidianTile::getResourceCount(Random *random)
{
    return 1;
}

int ObsidianTile::getResource(int data, Random *random, int playerBonusLevel)
{
    return Tile::obsidian_Id;
}
