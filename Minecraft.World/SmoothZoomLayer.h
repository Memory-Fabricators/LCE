#pragma once

#include "Layer.h"

class SmoothZoomLayer : public Layer
{
  public:
    SmoothZoomLayer(std::int64_t seedMixup, shared_ptr<Layer> parent);

    virtual intArray getArea(int xo, int yo, int w, int h);
    static shared_ptr<Layer> zoom(std::int64_t seed, shared_ptr<Layer> sup, int count);
};
