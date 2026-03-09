#pragma once
#include "Material.h"
#include "Tile.h"

class Random;

class BookshelfTile : public Tile
{
  public:
    BookshelfTile(int id);

    virtual Icon *getTexture(int face, int data);
    virtual int getResourceCount(Random *random);
    virtual int getResource(int data, Random *random, int playerBonusLevel);
};
