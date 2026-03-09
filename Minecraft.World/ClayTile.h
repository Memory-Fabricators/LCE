#pragma once
#include "Material.h"
#include "Tile.h"

class Random;

class ClayTile : public Tile
{
  public:
    ClayTile(int id);
    virtual int getResource(int data, Random *random, int playerBonusLevel);
    virtual int getResourceCount(Random *random);
};
