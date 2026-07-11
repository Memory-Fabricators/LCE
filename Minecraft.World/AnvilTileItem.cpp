#include "AnvilTileItem.h"
#include "net.minecraft.world.level.tile.h"
#include "stdafx.h"

AnvilTileItem::AnvilTileItem(Tile *tile) : MultiTextureTileItem(tile->id - 256, tile, (int *)AnvilTile::ANVIL_NAMES, 3)
{
}

int AnvilTileItem::getLevelDataForAuxValue(int auxValue)
{
    return auxValue << 2;
}

unsigned int AnvilTileItem::getDescriptionId(int iData)
{
    int damage = iData >> 2;
    return MultiTextureTileItem::getDescriptionId(damage);
}
