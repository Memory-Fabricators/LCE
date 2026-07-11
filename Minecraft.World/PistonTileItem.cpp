#include "PistonTileItem.h"
#include "net.minecraft.world.item.h"
#include "net.minecraft.world.level.tile.h"
#include "stdafx.h"

PistonTileItem::PistonTileItem(int id) : TileItem(id)
{
}

int PistonTileItem::getLevelDataForAuxValue(int auxValue)
{
    // return an undefined facing until the setPlacedBy method is called
    return PistonBaseTile::UNDEFINED_FACING;
}
