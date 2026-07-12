#pragma once

#include "Layer.h"

class FuzzyZoomLayer : public Layer
{
  public:
    FuzzyZoomLayer(std::int64_t seedMixup, shared_ptr<Layer> parent);
    intArray getArea(int xo, int yo, int w, int h);

  protected:
    int random(int a, int b);
    int random(int a, int b, int c, int d);

  public:
    static shared_ptr<Layer> zoom(std::int64_t seed, shared_ptr<Layer> sup, int count);
};
