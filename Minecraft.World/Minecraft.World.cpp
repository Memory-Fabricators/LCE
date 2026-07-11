#include "stdafx.h"

#include "Achievements.h"
#include "Biome.h"
#include "BlockReplacements.h"
#include "Enchantment.h"
#include "EnderMan.h"
#include "EntityIO.h"
#include "FurnaceRecipes.h"
#include "HatchetItem.h"
#include "Item.h"
#include "LevelChunk.h"
#include "LevelType.h"
#include "Material.h"
#include "MaterialColor.h"
#include "MineShaftPieces.h"
#include "MobCategory.h"
#include "Packet.h"
#include "PickaxeItem.h"
#include "PigZombie.h"
#include "PotionBrewing.h"
#include "RandomScatteredLargeFeature.h"
#include "Recipes.h"
#include "SharedConstants.h"
#include "ShovelItem.h"
#include "Skeleton.h"
#include "Stats.h"
#include "StrongholdFeature.h"
#include "Tile.h"
#include "TileEntity.h"
#include "VillageFeature.h"
#include "VillagePieces.h"

#include "../Minecraft.Client/ServerLevel.h"
#include "LevelSettings.h"
#include "McRegionChunkStorage.h"
#include "Minecraft.World.h"
#include "SparseDataStorage.h"
#include "SparseLightStorage.h"
#include "Villager.h"

#ifdef _DURANGO
#include "DurangoStats.h"
#else
#include "CommonStats.h"
#endif

void MinecraftWorld_RunStaticCtors()
{
    // The ordering of these static ctors can be important. If they are within statement blocks then
    // DO NOT CHANGE the ordering - 4J Stu

    Packet::staticCtor();

    {
        MaterialColor::staticCtor();
        Material::staticCtor();
        Tile::staticCtor();
        HatchetItem::staticCtor();
        PickaxeItem::staticCtor();
        ShovelItem::staticCtor();
        BlockReplacements::staticCtor();
        Biome::staticCtor();
        Item::staticCtor();
        FurnaceRecipes::staticCtor();
        Recipes::staticCtor();
#ifdef _DURANGO
        GenericStats::setInstance(new DurangoStats());
#else
        GenericStats::setInstance(new CommonStats());
        Stats::staticCtor();
#endif
        // Achievements::staticCtor(); // 4J Stu - This is now called from within the Stats::staticCtor()
        Skeleton::staticCtor();
        PigZombie::staticCtor();
        TileEntity::staticCtor();
        EntityIO::staticCtor();
        MobCategory::staticCtor();

        Item::staticInit();
        LevelChunk::staticCtor();

        LevelType::staticCtor();

        MineShaftPieces::staticCtor();
        StrongholdFeature::staticCtor();
        VillagePieces::Smithy::staticCtor();
        VillageFeature::staticCtor();
        RandomScatteredLargeFeature::staticCtor();
    }
    EnderMan::staticCtor();
    PotionBrewing::staticCtor();
    Enchantment::staticCtor();

    SharedConstants::staticCtor();

    ServerLevel::staticCtor();
    SparseLightStorage::staticCtor();
    CompressedTileStorage::staticCtor();
    SparseDataStorage::staticCtor();
    McRegionChunkStorage::staticCtor();
    Villager::staticCtor();
    GameType::staticCtor();
}
