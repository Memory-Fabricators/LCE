#pragma once

enum class Award
{
    TakingInventory = 0,
    GettingWood,
    Benchmarking,
    TimeToMine,
    HotTopic,
    AquireHardware,
    TimeToFarm,
    BakeBread,
    TheLie,
    GettingAnUpgrade,
    DeliciousFish,
    OnARail,
    TimeToStrike,
    MonsterHunter,
    CowTipper,
    WhenPigsFly,
    LeaderOfThePack,
    MOARTools,
    DispenseWithThis,
    InToTheNether,

    mine100Blocks,
    kill10Creepers,
    eatPorkChop,
    play100Days,
    arrowKillCreeper,
    socialPost,

#ifndef _XBOX
    // 4J Stu - Does not map to any Xbox achievements
    snipeSkeleton,
    diamonds,
    portal,
    ghast,
    blazeRod,
    potion,
    theEnd,
    winGame,
    enchantments,
    overkill,
    bookcase,
#endif

#ifdef _EXTENDED_ACHIEVEMENTS
    adventuringTime,
    repopulation,
    // porkChop,
    diamondsToYou,
    // passingTheTime,
    // archer,
    theHaggler,
    potPlanter,
    itsASign,
    ironBelly,
    haveAShearfulDay,
    rainbowCollection,
    stayinFrosty,
    chestfulOfCobblestone,
    renewableEnergy,
    musicToMyEars,
    bodyGuard,
    ironMan,
    zombieDoctor,
    lionTamer,
#endif

    Max,
};
