#pragma once

// #include "

enum class GameRuleType
{
    Invalid = -1,
    Root = 0, // This is the top level rule that defines a game mode, this is used to generate data for new players

    LevelGenerationOptions,
    ApplySchematic,
    GenerateStructure,
    GenerateBox,
    PlaceBlock,
    PlaceContainer,
    PlaceSpawner,
    BiomeOverride,
    StartFeature,

    AddItem,
    AddEnchantment,

    LevelRules,
    NamedArea,

    UseTileRule,
    CollectItemRule,
    CompleteAllRule,
    UpdatePlayerRule,

    Count
};

enum class GameRuleAttribute
{
    Invalid = -1,

    descriptionName = 0,
    promptName,
    dataTag,

    enchantmentId,
    enchantmentLevel,

    itemId,
    quantity,
    auxValue,
    slot,

    name,

    food,
    health,

    tileId,
    useCoords,

    seed,
    flatworld,

    filename,
    rot,

    data,
    block,
    entity,

    facing,

    edgeTile,
    fillTile,
    skipAir,

    x,
    x0,
    x1,

    y,
    y0,
    y1,

    z,
    z0,
    z1,

    chunkX,
    chunkZ,

    yRot,

    spawnX,
    spawnY,
    spawnZ,

    orientation,
    dimension,

    topTileId,
    biomeId,

    feature,

    Count
};

static void write(DataOutputStream *dos, GameRuleType eType)
{
    dos->writeInt(static_cast<int>(eType));
}

static void write(DataOutputStream *dos, GameRuleAttribute eAttr)
{
    dos->writeInt(static_cast<int>(GameRuleType::Count) + static_cast<int>(eAttr));
}
