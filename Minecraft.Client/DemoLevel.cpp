#include "DemoLevel.h"
#include "../Minecraft.World/LevelSettings.h"
#include "../Minecraft.World/LevelType.h"
#include "../Minecraft.World/net.minecraft.world.level.storage.h"
#include "stdafx.h"

DemoLevel::DemoLevel(shared_ptr<LevelStorage> levelStorage, const wstring &levelName) : Level(levelStorage, levelName, new LevelSettings(DEMO_LEVEL_SEED, GameType::SURVIVAL, false, false, true, LevelType::lvl_normal, 0, 0))
{
}

DemoLevel::DemoLevel(Level *level, Dimension *dimension) : Level(level, dimension)
{
}

void DemoLevel::setInitialSpawn()
{
    levelData->setSpawn(DEMO_SPAWN_X, DEMO_SPAWN_Y, DEMO_SPAWN_Z);
}
