#pragma once

#include "LevelData.h"

class DerivedLevelData : public LevelData
{
  private:
    LevelData *wrapped;

  public:
    DerivedLevelData(LevelData *wrapped);

  protected:
    virtual void setTagData(CompoundTag *tag); // 4J Added

  public:
    CompoundTag *createTag();
    CompoundTag *createTag(vector<shared_ptr<Player>> *players);
    std::int64_t getSeed();
    int getXSpawn();
    int getYSpawn();
    int getZSpawn();
    std::int64_t getTime();
    std::int64_t getSizeOnDisk();
    CompoundTag *getLoadedPlayerTag();
    wstring getLevelName();
    int getVersion();
    std::int64_t getLastPlayed();
    bool isThundering();
    int getThunderTime();
    bool isRaining();
    int getRainTime();
    GameType *getGameType();
    void setSeed(std::int64_t seed);
    void setXSpawn(int xSpawn);
    void setYSpawn(int ySpawn);
    void setZSpawn(int zSpawn);
    void setTime(std::int64_t time);
    void setSizeOnDisk(std::int64_t sizeOnDisk);
    void setLoadedPlayerTag(CompoundTag *loadedPlayerTag);
    void setDimension(int dimension);
    void setSpawn(int xSpawn, int ySpawn, int zSpawn);
    void setLevelName(const wstring &levelName);
    void setVersion(int version);
    void setThundering(bool thundering);
    void setThunderTime(int thunderTime);
    void setRaining(bool raining);
    void setRainTime(int rainTime);
    bool isGenerateMapFeatures();
    void setGameType(GameType *gameType);
    bool isHardcore();
    LevelType *getGenerator();
    void setGenerator(LevelType *generator);
    bool getAllowCommands();
    void setAllowCommands(bool allowCommands);
    bool isInitialized();
    void setInitialized(bool initialized);
    int getXZSize();    // 4J Added
    int getHellScale(); // 4J Addded
};
