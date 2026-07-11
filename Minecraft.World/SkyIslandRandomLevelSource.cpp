#include "SkyIslandRandomLevelSource.h"
#include "net.minecraft.world.level.h"
#include "stdafx.h"

SkyIslandRandomLevelSource::SkyIslandRandomLevelSource(Level *level, __int64 seed)
    : m_level(level)
{
}

bool SkyIslandRandomLevelSource::hasChunk(int x, int y)
{
    return false;
}

LevelChunk *SkyIslandRandomLevelSource::getChunk(int x, int z)
{
    return NULL;
}

LevelChunk *SkyIslandRandomLevelSource::create(int x, int z)
{
    return NULL;
}

void SkyIslandRandomLevelSource::postProcess(ChunkSource *parent, int x, int z)
{
}

bool SkyIslandRandomLevelSource::save(bool force, ProgressListener *progressListener)
{
    return false;
}

bool SkyIslandRandomLevelSource::tick()
{
    return false;
}

bool SkyIslandRandomLevelSource::shouldSave()
{
    return false;
}

wstring SkyIslandRandomLevelSource::gatherStats()
{
    return L"SkyIslandRandomLevelSource";
}

vector<Biome::MobSpawnerData *> *SkyIslandRandomLevelSource::getMobsAt(MobCategory *mobCategory, int x, int y, int z)
{
    return NULL;
}

TilePos *SkyIslandRandomLevelSource::findNearestMapFeature(Level *level, const wstring &featureName, int x, int y, int z)
{
    return NULL;
}
