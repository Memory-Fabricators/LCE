#pragma once

#include "Layer.h"

class LevelType;

class BiomeInitLayer : public Layer
{
  private:
    BiomeArray startBiomes;

  public:
    BiomeInitLayer(std::int64_t seed, shared_ptr<Layer> parent, LevelType *levelType);
    virtual ~BiomeInitLayer();
    intArray getArea(int xo, int yo, int w, int h);
};
