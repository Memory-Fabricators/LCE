#pragma once

#include "Layer.h"

class SmoothLayer : public Layer
{
  public:
    SmoothLayer(std::int64_t seedMixup, shared_ptr<Layer> parent);

    virtual intArray getArea(int xo, int yo, int w, int h);
};
