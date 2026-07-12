#include "LargeFeature.h"
#include "net.minecraft.world.level.h"
#include "stdafx.h"

const wstring LargeFeature::STRONGHOLD = L"StrongHold";

LargeFeature::LargeFeature()
{
    radius = 8;
    random = new Random();
}

LargeFeature::~LargeFeature()
{
    delete random;
}

void LargeFeature::apply(ChunkSource *ChunkSource, Level *level, int xOffs, int zOffs, byteArray blocks)
{
    int r = radius;
    this->level = level;

    random->setSeed(level->getSeed());
    std::int64_t xScale = random->nextLong();
    std::int64_t zScale = random->nextLong();

    for (int x = xOffs - r; x <= xOffs + r; x++)
    {
        for (int z = zOffs - r; z <= zOffs + r; z++)
        {
            std::int64_t xx = x * xScale;
            std::int64_t zz = z * zScale;
            random->setSeed(xx ^ zz ^ level->getSeed());
            addFeature(level, x, z, xOffs, zOffs, blocks);
        }
    }
}
