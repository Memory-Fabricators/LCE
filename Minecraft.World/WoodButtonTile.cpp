#include "WoodButtonTile.h"
#include "net.minecraft.h"
#include "stdafx.h"

WoodButtonTile::WoodButtonTile(int id) : ButtonTile(id, true)
{
}

Icon *WoodButtonTile::getTexture(int face, int data)
{
    return Tile::wood->getTexture(Facing::UP);
}
