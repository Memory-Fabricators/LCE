#pragma once

#include "ArrayWithLength.h"

class LevelType;

#ifndef _CONTENT_PACAKGE
#define _BIOME_OVERRIDE
#endif

class Layer
{
  private:
    std::int64_t seed;

  protected:
    shared_ptr<Layer> parent;

  private:
    std::int64_t rval;
    std::int64_t seedMixup;

  public:
    static LayerArray getDefaultLayers(std::int64_t seed, LevelType *levelType);

    Layer(std::int64_t seedMixup);

    virtual void init(std::int64_t seed);
    virtual void initRandom(std::int64_t x, std::int64_t y);

  protected:
    int nextRandom(int max);

  public:
    virtual intArray getArea(int xo, int yo, int w, int h) = 0;
};
