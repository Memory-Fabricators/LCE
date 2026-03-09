#include "StoneButtonTile.h"
#include "net.minecraft.h"
#include "stdafx.h"

StoneButtonTile::StoneButtonTile(int id) : ButtonTile(id, false)
{
}

Icon *StoneButtonTile::getTexture(int face, int data)
{
    return Tile::stone->getTexture(Facing::UP);
}
