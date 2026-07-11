#pragma once

#include "ChunkSource.h"

class Level;

class SkyIslandRandomLevelSource : public ChunkSource
{
  public:
    SkyIslandRandomLevelSource(Level *level, __int64 seed);

    virtual bool hasChunk(int x, int y);
    virtual LevelChunk *getChunk(int x, int z);
    virtual LevelChunk *create(int x, int z);
    virtual void postProcess(ChunkSource *parent, int x, int z);
    virtual bool save(bool force, ProgressListener *progressListener);
    virtual bool tick();
    virtual bool shouldSave();
    virtual wstring gatherStats();
    virtual vector<Biome::MobSpawnerData *> *getMobsAt(MobCategory *mobCategory, int x, int y, int z);
    virtual TilePos *findNearestMapFeature(Level *level, const wstring &featureName, int x, int y, int z);

  private:
    Level *m_level;
};
