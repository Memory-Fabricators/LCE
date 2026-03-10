#pragma once

enum class eFileExtensionType
{
    eFileExtensionType_PNG = 0,
    eFileExtensionType_INF,
    eFileExtensionType_DAT,
};

enum class eTMSFileType
{
    eTMSFileType_MinecraftStore = 0,
    eTMSFileType_TexturePack,
    eTMSFileType_All
};

enum class eTPDFileType
{
    eTPDFileType_Loc = 0,
    eTPDFileType_Icon,
    //	eTPDFileType_Banner,
    eTPDFileType_Comparison,
};

enum class eFont
{
    eFont_European = 0,
    eFont_Korean,
    eFont_Japanese,
    eFont_Chinese,
    eFont_None, // to fallback to nothing
};

enum class eXuiAction
{
    eAppAction_Idle = 0,
    eAppAction_SaveGame,
    eAppAction_SaveGameCapturedThumbnail,
    eAppAction_ExitWorld,
    eAppAction_ExitWorldCapturedThumbnail,
    eAppAction_ExitWorldTrial,
    // eAppAction_ExitGameFatalLoadError,
    eAppAction_Respawn,
    eAppAction_WaitForRespawnComplete,
    eAppAction_PrimaryPlayerSignedOut,
    eAppAction_PrimaryPlayerSignedOutReturned,
    eAppAction_PrimaryPlayerSignedOutReturned_Menus,
    eAppAction_ExitPlayer, // secondary player
    eAppAction_ExitPlayerPreLogin,
    eAppAction_TrialOver,
    eAppAction_ExitTrial,
    eAppAction_WaitForDimensionChangeComplete,
    eAppAction_SocialPost,
    eAppAction_SocialPostScreenshot,
    eAppAction_EthernetDisconnected,
    eAppAction_EthernetDisconnectedReturned,
    eAppAction_EthernetDisconnectedReturned_Menus,
    eAppAction_ExitAndJoinFromInvite,
    eAppAction_DashboardTrialJoinFromInvite,
    eAppAction_ExitAndJoinFromInviteConfirmed,
    eAppAction_JoinFromInvite,
    eAppAction_ChangeSessionType,
    eAppAction_SetDefaultOptions,
    eAppAction_LocalPlayerJoined,
    eAppAction_RemoteServerSave,
    eAppAction_WaitRemoteServerSaveComplete,
    eAppAction_FailedToJoinNoPrivileges,
    eAppAction_AutosaveSaveGame,
    eAppAction_AutosaveSaveGameCapturedThumbnail,
    eAppAction_ProfileReadError,
    eAppAction_DisplayLavaMessage,
    eAppAction_BanLevel,
    eAppAction_LevelInBanLevelList,

    eAppAction_ReloadTexturePack,
    eAppAction_ReloadFont,
    eAppAction_TexturePackRequired, // when the user has joined from invite, but doesn't have the texture pack

#ifdef __ORBIS__
    eAppAction_OptionsSaveNoSpace,
#endif
    eAppAction_DebugText,
};

enum class eTMSAction
{
    eTMSAction_Idle = 0,
    eTMSAction_TMS_RetrieveFiles_Complete,
    eTMSAction_TMSPP_RetrieveFiles_CreateLoad_SignInReturned,
    eTMSAction_TMSPP_RetrieveFiles_RunPlayGame,
    eTMSAction_TMSPP_RetrieveFiles_HelpAndOptions,
    eTMSAction_TMSPP_RetrieveFiles_DLCMain,
    eTMSAction_TMSPP_GlobalFileList,
    eTMSAction_TMSPP_GlobalFileList_Waiting,
    // 	eTMSAction_TMSPP_ConfigFile,
    // 	eTMSAction_TMSPP_ConfigFile_Waiting,
    eTMSAction_TMSPP_UserFileList,
    eTMSAction_TMSPP_UserFileList_Waiting,
    eTMSAction_TMSPP_XUIDSFile,
    eTMSAction_TMSPP_XUIDSFile_Waiting,
    eTMSAction_TMSPP_DLCFile,
    eTMSAction_TMSPP_DLCFile_Waiting,
    eTMSAction_TMSPP_BannedListFile,
    eTMSAction_TMSPP_BannedListFile_Waiting,
    eTMSAction_TMSPP_RetrieveFiles_Complete,
    eTMSAction_TMSPP_DLCFileOnly,
    eTMSAction_TMSPP_RetrieveUserFilelist_DLCFileOnly,
};

// The server runs on its own thread, so we need to call its actions there rather than where all other Xui actions are performed
// In general these are debugging options
enum class eXuiServerAction
{
    eXuiServerAction_Idle = 0,
    eXuiServerAction_DropItem, // Debug
    eXuiServerAction_SaveGame,
    eXuiServerAction_AutoSaveGame,
    eXuiServerAction_SpawnMob, // Debug
    eXuiServerAction_PauseServer,
    eXuiServerAction_ToggleRain,    // Debug
    eXuiServerAction_ToggleThunder, // Debug
    eXuiServerAction_ServerSettingChanged_Gamertags,
    eXuiServerAction_ServerSettingChanged_Difficulty,
    eXuiServerAction_ExportSchematic, // Debug
    eXuiServerAction_ServerSettingChanged_BedrockFog,
    eXuiServerAction_SetCameraLocation, // Debug
};

enum class eGameSetting
{
    eGameSetting_MusicVolume = 0,
    eGameSetting_SoundFXVolume,
    eGameSetting_Gamma,
    eGameSetting_Difficulty,
    eGameSetting_Sensitivity_InGame,
    eGameSetting_Sensitivity_InMenu,
    eGameSetting_ViewBob,
    eGameSetting_ControlScheme,
    eGameSetting_ControlInvertLook,
    eGameSetting_ControlSouthPaw,
    eGameSetting_SplitScreenVertical,
    eGameSetting_GamertagsVisible,
    // Interim TU 1.6.6
    eGameSetting_Autosave,
    eGameSetting_DisplaySplitscreenGamertags,
    eGameSetting_Hints,
    eGameSetting_InterfaceOpacity,
    eGameSetting_Tooltips,
    // TU5
    eGameSetting_Clouds,
    eGameSetting_Online,
    eGameSetting_InviteOnly,
    eGameSetting_FriendsOfFriends,
    eGameSetting_DisplayUpdateMessage,

    // TU6
    eGameSetting_BedrockFog,
    eGameSetting_DisplayHUD,
    eGameSetting_DisplayHand,

    // TU7
    eGameSetting_CustomSkinAnim,

    // TU9
    eGameSetting_DeathMessages,
    eGameSetting_UISize,
    eGameSetting_UISizeSplitscreen,
    eGameSetting_AnimatedCharacter,

    // PS3
    eGameSetting_PS3_EULA_Read,

    // PSVita
    eGameSetting_PSVita_NetworkModeAdhoc,
};

enum class eGameMode
{
    eMode_Singleplayer,
    eMode_Multiplayer
};

enum class MinecraftColor
{
    NOT_SET,
    Foliage_Evergreen,
    Foliage_Birch,
    Foliage_Default,
    Foliage_Common,
    Foliage_Ocean,
    Foliage_Plains,
    Foliage_Desert,
    Foliage_ExtremeHills,
    Foliage_Forest,
    Foliage_Taiga,
    Foliage_Swampland,
    Foliage_River,
    Foliage_Hell,
    Foliage_Sky,
    Foliage_FrozenOcean,
    Foliage_FrozenRiver,
    Foliage_IcePlains,
    Foliage_IceMountains,
    Foliage_MushroomIsland,
    Foliage_MushroomIslandShore,
    Foliage_Beach,
    Foliage_DesertHills,
    Foliage_ForestHills,
    Foliage_TaigaHills,
    Foliage_ExtremeHillsEdge,
    Foliage_Jungle,
    Foliage_JungleHills,
    Grass_Common,
    Grass_Ocean,
    Grass_Plains,
    Grass_Desert,
    Grass_ExtremeHills,
    Grass_Forest,
    Grass_Taiga,
    Grass_Swampland,
    Grass_River,
    Grass_Hell,
    Grass_Sky,
    Grass_FrozenOcean,
    Grass_FrozenRiver,
    Grass_IcePlains,
    Grass_IceMountains,
    Grass_MushroomIsland,
    Grass_MushroomIslandShore,
    Grass_Beach,
    Grass_DesertHills,
    Grass_ForestHills,
    Grass_TaigaHills,
    Grass_ExtremeHillsEdge,
    Grass_Jungle,
    Grass_JungleHills,
    Water_Ocean,
    Water_Plains,
    Water_Desert,
    Water_ExtremeHills,
    Water_Forest,
    Water_Taiga,
    Water_Swampland,
    Water_River,
    Water_Hell,
    Water_Sky,
    Water_FrozenOcean,
    Water_FrozenRiver,
    Water_IcePlains,
    Water_IceMountains,
    Water_MushroomIsland,
    Water_MushroomIslandShore,
    Water_Beach,
    Water_DesertHills,
    Water_ForestHills,
    Water_TaigaHills,
    Water_ExtremeHillsEdge,
    Water_Jungle,
    Water_JungleHills,

    Sky_Ocean,
    Sky_Plains,
    Sky_Desert,
    Sky_ExtremeHills,
    Sky_Forest,
    Sky_Taiga,
    Sky_Swampland,
    Sky_River,
    Sky_Hell,
    Sky_Sky,
    Sky_FrozenOcean,
    Sky_FrozenRiver,
    Sky_IcePlains,
    Sky_IceMountains,
    Sky_MushroomIsland,
    Sky_MushroomIslandShore,
    Sky_Beach,
    Sky_DesertHills,
    Sky_ForestHills,
    Sky_TaigaHills,
    Sky_ExtremeHillsEdge,
    Sky_Jungle,
    Sky_JungleHills,

    Tile_RedstoneDust,
    Tile_RedstoneDustUnlit,
    Tile_RedstoneDustLitMin,
    Tile_RedstoneDustLitMax,
    Tile_StemMin,
    Tile_StemMax,
    Tile_WaterLily,

    Sky_Dawn_Dark,
    Sky_Dawn_Bright,

    Material_None,
    Material_Grass,
    Material_Sand,
    Material_Cloth,
    Material_Fire,
    Material_Ice,
    Material_Metal,
    Material_Plant,
    Material_Snow,
    Material_Clay,
    Material_Dirt,
    Material_Stone,
    Material_Water,
    Material_Wood,
    Material_Emerald,

    Particle_Note_00,
    Particle_Note_01,
    Particle_Note_02,
    Particle_Note_03,
    Particle_Note_04,
    Particle_Note_05,
    Particle_Note_06,
    Particle_Note_07,
    Particle_Note_08,
    Particle_Note_09,
    Particle_Note_10,
    Particle_Note_11,
    Particle_Note_12,
    Particle_Note_13,
    Particle_Note_14,
    Particle_Note_15,
    Particle_Note_16,
    Particle_Note_17,
    Particle_Note_18,
    Particle_Note_19,
    Particle_Note_20,
    Particle_Note_21,
    Particle_Note_22,
    Particle_Note_23,
    Particle_Note_24,

    Particle_NetherPortal,
    Particle_EnderPortal,
    Particle_Smoke,
    Particle_Ender,
    Particle_Explode,
    Particle_HugeExplosion,
    Particle_DripWater,
    Particle_DripLavaStart,
    Particle_DripLavaEnd,
    Particle_EnchantmentTable,
    Particle_DragonBreathMin,
    Particle_DragonBreathMax,
    Particle_Suspend,
    Particle_CritStart,
    Particle_CritEnd,

    Effect_MovementSpeed,
    Effect_MovementSlowDown,
    Effect_DigSpeed,
    Effect_DigSlowdown,
    Effect_DamageBoost,
    Effect_Heal,
    Effect_Harm,
    Effect_Jump,
    Effect_Confusion,
    Effect_Regeneration,
    Effect_DamageResistance,
    Effect_FireResistance,
    Effect_WaterBreathing,
    Effect_Invisiblity,
    Effect_Blindness,
    Effect_NightVision,
    Effect_Hunger,
    Effect_Weakness,
    Effect_Poison,
    Effect_Wither,
    Effect_HealthBoost,
    Effect_Absorption,
    Effect_Saturation,

    Potion_BaseColor,

    Mob_Creeper_Colour1,
    Mob_Creeper_Colour2,
    Mob_Skeleton_Colour1,
    Mob_Skeleton_Colour2,
    Mob_Spider_Colour1,
    Mob_Spider_Colour2,
    Mob_Zombie_Colour1,
    Mob_Zombie_Colour2,
    Mob_Slime_Colour1,
    Mob_Slime_Colour2,
    Mob_Ghast_Colour1,
    Mob_Ghast_Colour2,
    Mob_PigZombie_Colour1,
    Mob_PigZombie_Colour2,
    Mob_Enderman_Colour1,
    Mob_Enderman_Colour2,
    Mob_CaveSpider_Colour1,
    Mob_CaveSpider_Colour2,
    Mob_Silverfish_Colour1,
    Mob_Silverfish_Colour2,
    Mob_Blaze_Colour1,
    Mob_Blaze_Colour2,
    Mob_LavaSlime_Colour1,
    Mob_LavaSlime_Colour2,
    Mob_Pig_Colour1,
    Mob_Pig_Colour2,
    Mob_Sheep_Colour1,
    Mob_Sheep_Colour2,
    Mob_Cow_Colour1,
    Mob_Cow_Colour2,
    Mob_Chicken_Colour1,
    Mob_Chicken_Colour2,
    Mob_Squid_Colour1,
    Mob_Squid_Colour2,
    Mob_Wolf_Colour1,
    Mob_Wolf_Colour2,
    Mob_MushroomCow_Colour1,
    Mob_MushroomCow_Colour2,
    Mob_Ocelot_Colour1,
    Mob_Ocelot_Colour2,
    Mob_Villager_Colour1,
    Mob_Villager_Colour2,
    Mob_Bat_Colour1,
    Mob_Bat_Colour2,
    Mob_Witch_Colour1,
    Mob_Witch_Colour2,
    Mob_Horse_Colour1,
    Mob_Horse_Colour2,

    Armour_Default_Leather_Colour,

    Under_Water_Clear_Colour,
    Under_Lava_Clear_Colour,
    In_Cloud_Base_Colour,

    Under_Water_Fog_Colour,
    Under_Lava_Fog_Colour,
    In_Cloud_Fog_Colour,

    Default_Fog_Colour,
    Nether_Fog_Colour,
    End_Fog_Colour,

    Sign_Text,
    Map_Text,

    Leash_Light_Colour,
    Leash_Dark_Colour,

    Fire_Overlay,

    eHTMLColor_0,
    eHTMLColor_1,
    eHTMLColor_2,
    eHTMLColor_3,
    eHTMLColor_4,
    eHTMLColor_5,
    eHTMLColor_6,
    eHTMLColor_7,
    eHTMLColor_8,
    eHTMLColor_9,
    eHTMLColor_a,
    eHTMLColor_b,
    eHTMLColor_c,
    eHTMLColor_d,
    eHTMLColor_e,
    eHTMLColor_f,
    eHTMLColor_0_dark,
    eHTMLColor_1_dark,
    eHTMLColor_2_dark,
    eHTMLColor_3_dark,
    eHTMLColor_4_dark,
    eHTMLColor_5_dark,
    eHTMLColor_6_dark,
    eHTMLColor_7_dark,
    eHTMLColor_8_dark,
    eHTMLColor_9_dark,
    eHTMLColor_a_dark,
    eHTMLColor_b_dark,
    eHTMLColor_c_dark,
    eHTMLColor_d_dark,
    eHTMLColor_e_dark,
    eHTMLColor_f_dark,
    eHTMLColor_T1,
    eHTMLColor_T2,
    eHTMLColor_T3,
    eHTMLColor_Black,
    eHTMLColor_White,

    eTextColor_Enchant,
    eTextColor_EnchantFocus,
    eTextColor_EnchantDisabled,
    eTextColor_RenamedItemTitle,

    // eHTMLColor_0 = 0x000000, //r:0 , g: 0, b: 0, i: 0
    // eHTMLColor_1 = 0x0000aa, //r:0 , g: 0, b: aa, i: 1 // blue, quite dark
    // eHTMLColor_2 = 0x109e10, // Changed by request of Dave //0x00aa00, //r:0 , g: aa, b: 0, i: 2 // green
    // eHTMLColor_3 = 0x109e9e, // Changed by request of Dave //0x00aaaa, //r:0 , g: aa, b: aa, i: 3 // cyan
    // eHTMLColor_4 = 0xaa0000, //r:aa , g: 0, b: 0, i: 4 // red
    // eHTMLColor_5 = 0xaa00aa, //r:aa , g: 0, b: aa, i: 5 // purple
    // eHTMLColor_6 = 0xffaa00, //r:ff , g: aa, b: 0, i: 6 // orange
    // eHTMLColor_7 = 0xaaaaaa, //r:aa , g: aa, b: aa, i: 7 // light gray
    // eHTMLColor_8 = 0x555555, //r:55 , g: 55, b: 55, i: 8 // gray
    // eHTMLColor_9 = 0x5555ff, //r:55 , g: 55, b: ff, i: 9 // blue
    // eHTMLColor_a = 0x55ff55, //r:55 , g: ff, b: 55, i: a // green
    // eHTMLColor_b = 0x55ffff, //r:55 , g: ff, b: ff, i: b // cyan
    // eHTMLColor_c = 0xff5555, //r:ff , g: 55, b: 55, i: c // red pink
    // eHTMLColor_d = 0xff55ff, //r:ff , g: 55, b: ff, i: d // bright pink
    // eHTMLColor_e = 0xffff55, //r:ff , g: ff, b: 55, i: e // yellow
    // eHTMLColor_f = 0xffffff, //r:ff , g: ff, b: ff, i: f
    // eHTMLColor_0_dark = 0x000000, //r:0 , g: 0, b: 0, i: 10
    // eHTMLColor_1_dark = 0x00002a, //r:0 , g: 0, b: 2a, i: 11
    // eHTMLColor_2_dark = 0x002a00, //r:0 , g: 2a, b: 0, i: 12
    // eHTMLColor_3_dark = 0x002a2a, //r:0 , g: 2a, b: 2a, i: 13
    // eHTMLColor_4_dark = 0x2a0000, //r:2a , g: 0, b: 0, i: 14
    // eHTMLColor_5_dark = 0x2a002a, //r:2a , g: 0, b: 2a, i: 15
    // eHTMLColor_6_dark = 0x2a2a00, //r:2a , g: 2a, b: 0, i: 16
    // eHTMLColor_7_dark = 0x2a2a2a, //r:2a , g: 2a, b: 2a, i: 17 // dark gray
    // eHTMLColor_8_dark = 0x151515, //r:15 , g: 15, b: 15, i: 18
    // eHTMLColor_9_dark = 0x15153f, //r:15 , g: 15, b: 3f, i: 19
    // eHTMLColor_a_dark = 0x153f15, //r:15 , g: 3f, b: 15, i: 1a
    // eHTMLColor_b_dark = 0x153f3f, //r:15 , g: 3f, b: 3f, i: 1b
    // eHTMLColor_c_dark = 0x3f1515, //r:3f , g: 15, b: 15, i: 1c // brown
    // eHTMLColor_d_dark = 0x3f153f, //r:3f , g: 15, b: 3f, i: 1d
    // eHTMLColor_e_dark = 0x3f3f15, //r:3f , g: 3f, b: 15, i: 1e
    // eHTMLColor_f_dark = 0x3f3f3f, //r:3f , g: 3f, b: 3f, i: 1f

    COUNT,
};

enum class eDLCContentType
{
    e_DLC_SkinPack = 0,
    e_DLC_TexturePacks,
    e_DLC_MashupPacks,
    e_DLC_Themes,
    e_DLC_AvatarItems,
    e_DLC_Gamerpics,
    e_DLC_MAX_MinecraftStore,
    e_DLC_TexturePackData, // for the icon, banner and text
    e_DLC_MAX,
    e_DLC_NotDefined,
};

enum class eDLCMarketplaceType
{
    e_Marketplace_Content = 0, // skins, texture packs and mashup packs
    e_Marketplace_Themes,
    e_Marketplace_AvatarItems,
    e_Marketplace_Gamerpics,
    e_Marketplace_MAX,
    e_Marketplace_NotDefined,
};

enum class eDLCContentState
{
    e_DLC_ContentState_Idle = 0,
    e_DLC_ContentState_Retrieving,
    e_DLC_ContentState_Retrieved
};

enum class eTMSContentState
{
    e_TMS_ContentState_Idle = 0,
    e_TMS_ContentState_Queued,
    e_TMS_ContentState_Retrieving,
    e_TMS_ContentState_Retrieved
};

enum class eXUID
{
    eXUID_Undefined = 0,
    eXUID_NoName, // name not needed
    eXUID_Notch,
    eXUID_Carl,
    eXUID_Daniel,
    eXUID_Deadmau5,
    eXUID_DannyBStyle,
    eXUID_JulianClark,
    eXUID_Millionth,
    eXUID_4JPaddy,
    eXUID_4JStuart,
    eXUID_4JDavid,
    eXUID_4JRichard,
    eXUID_4JSteven,
};

enum class _eTerrainFeatureType
{
    eTerrainFeature_None = 0,
    eTerrainFeature_Stronghold,
    eTerrainFeature_Mineshaft,
    eTerrainFeature_Village,
    eTerrainFeature_Ravine,
    eTerrainFeature_NetherFortress,
    eTerrainFeature_StrongholdEndPortal,
    eTerrainFeature_Count
};

// 4J Stu - Whend adding new options you should consider whether having them on should disable achievements, and if so add them to the CanRecordStatsAndAchievements function
// 4J Stu - These options are now saved in save data, so new options can ONLY be added to the end
enum class eGameHostOption
{
    eGameHostOption_Difficulty = 0,
    eGameHostOption_OnlineGame, // Unused
    eGameHostOption_InviteOnly, // Unused
    eGameHostOption_FriendsOfFriends,
    eGameHostOption_Gamertags,
    eGameHostOption_Tutorial, // special case
    eGameHostOption_GameType,
    eGameHostOption_LevelType, // flat or default
    eGameHostOption_Structures,
    eGameHostOption_BonusChest,
    eGameHostOption_HasBeenInCreative,
    eGameHostOption_PvP,
    eGameHostOption_TrustPlayers,
    eGameHostOption_TNT,
    eGameHostOption_FireSpreads,
    eGameHostOption_CheatsEnabled, // special case
    eGameHostOption_HostCanFly,
    eGameHostOption_HostCanChangeHunger,
    eGameHostOption_HostCanBeInvisible,
    eGameHostOption_BedrockFog,
    eGameHostOption_NoHUD,
    eGameHostOption_WorldSize,
    eGameHostOption_All,

    eGameHostOption_DisableSaving,
    eGameHostOption_WasntSaveOwner, // Added for PS3 save transfer, so we can add a nice message in the future instead of the creative mode one

    eGameHostOption_MobGriefing,
    eGameHostOption_KeepInventory,
    eGameHostOption_DoMobSpawning,
    eGameHostOption_DoMobLoot,
    eGameHostOption_DoTileDrops,
    eGameHostOption_NaturalRegeneration,
    eGameHostOption_DoDaylightCycle,
};

// 4J-PB - If any new DLC items are added to the TMSFiles, this array needs updated
#ifdef _XBOX
enum class _TMSFILES
{
    TMS_SP1 = 0,
    TMS_SP2,
    TMS_SP3,
    TMS_SP4,
    TMS_SP5,
    TMS_SP6,
    TMS_SPF,
    TMS_SPB,
    TMS_SPC,
    TMS_SPZ,
    TMS_SPM,
    TMS_SPI,
    TMS_SPG,
    TMS_SPD1,
    TMS_SPSW1,

    TMS_THST,
    TMS_THIR,
    TMS_THGO,
    TMS_THDI,
    TMS_THAW,

    TMS_GPAN,
    TMS_GPCO,
    TMS_GPEN,
    TMS_GPFO,
    TMS_GPTO,
    TMS_GPBA,
    TMS_GPFA,
    TMS_GPME,
    TMS_GPMF,
    TMS_GPMM,
    TMS_GPSE,
    TMS_GPOr,
    TMS_GPMi,
    TMS_GPMB,
    TMS_GPBr,
    TMS_GPM1,
    TMS_GPM2,
    TMS_GPM3,

    TMS_AH_0001,
    TMS_AH_0002,
    TMS_AH_0003,
    TMS_AH_0004,
    TMS_AH_0005,
    TMS_AH_0006,
    TMS_AH_0007,
    TMS_AH_0008,
    TMS_AH_0009,
    TMS_AH_0010,
    TMS_AH_0011,
    TMS_AH_0012,
    TMS_AH_0013,

    TMS_AT_0001,
    TMS_AT_0002,
    TMS_AT_0003,
    TMS_AT_0004,
    TMS_AT_0005,
    TMS_AT_0006,
    TMS_AT_0007,
    TMS_AT_0008,
    TMS_AT_0009,
    TMS_AT_0010,
    TMS_AT_0011,
    TMS_AT_0012,
    TMS_AT_0013,
    TMS_AT_0014,
    TMS_AT_0015,
    TMS_AT_0016,
    TMS_AT_0017,
    TMS_AT_0018,
    TMS_AT_0019,
    TMS_AT_0020,
    TMS_AT_0021,
    TMS_AT_0022,
    TMS_AT_0023,
    TMS_AT_0024,
    TMS_AT_0025,
    TMS_AT_0026,

    TMS_AP_0001,
    TMS_AP_0002,
    TMS_AP_0003,
    TMS_AP_0004,
    TMS_AP_0005,
    TMS_AP_0006,
    TMS_AP_0007,
    TMS_AP_0009,
    TMS_AP_0010,
    TMS_AP_0011,
    TMS_AP_0012,
    TMS_AP_0013,
    TMS_AP_0014,
    TMS_AP_0015,
    TMS_AP_0016,
    TMS_AP_0017,
    TMS_AP_0018,

    TMS_AP_0019,
    TMS_AP_0020,
    TMS_AP_0021,
    TMS_AP_0022,
    TMS_AP_0023,
    TMS_AP_0024,
    TMS_AP_0025,
    TMS_AP_0026,
    TMS_AP_0027,
    TMS_AP_0028,
    TMS_AP_0029,
    TMS_AP_0030,
    TMS_AP_0031,
    TMS_AP_0032,
    TMS_AP_0033,

    TMS_AA_0001,

    TMS_MPMA,
    TMS_MPMA_DAT,
    TMS_MPSR,
    TMS_MPSR_DAT,
    TMS_MPHA,
    TMS_MPHA_DAT,
    TMS_MPFE,
    TMS_MPFE_DAT,

    TMS_TP01,
    TMS_TP01_DAT,
    TMS_TP02,
    TMS_TP02_DAT,
    TMS_TP04,
    TMS_TP04_DAT,
    TMS_TP05,
    TMS_TP05_DAT,
    TMS_TP06,
    TMS_TP06_DAT,
    TMS_TP07,
    TMS_TP07_DAT,
    TMS_TP08,
    TMS_TP08_DAT,

    TMS_COUNT
};
#endif

enum class EHTMLFontSize
{
    eHTMLSize_Normal,
    eHTMLSize_Splitscreen,
    eHTMLSize_Tutorial,
    eHTMLSize_EndPoem,

    eHTMLSize_COUNT,
};

enum class EControllerActions
{
    ACTION_MENU_A,
    ACTION_MENU_B,
    ACTION_MENU_X,
    ACTION_MENU_Y,
    ACTION_MENU_UP,
    ACTION_MENU_DOWN,
    ACTION_MENU_RIGHT,
    ACTION_MENU_LEFT,
    ACTION_MENU_PAGEUP,
    ACTION_MENU_PAGEDOWN,
    ACTION_MENU_RIGHT_SCROLL,
    ACTION_MENU_LEFT_SCROLL,
    ACTION_MENU_STICK_PRESS,
    ACTION_MENU_OTHER_STICK_PRESS,
    ACTION_MENU_OTHER_STICK_UP,
    ACTION_MENU_OTHER_STICK_DOWN,
    ACTION_MENU_OTHER_STICK_LEFT,
    ACTION_MENU_OTHER_STICK_RIGHT,
    ACTION_MENU_PAUSEMENU,

#ifdef _DURANGO
    ACTION_MENU_GTC_PAUSE,
    ACTION_MENU_GTC_RESUME,
#endif

#ifdef __ORBIS__
    ACTION_MENU_TOUCHPAD_PRESS,
#endif

    ACTION_MENU_OK,
    ACTION_MENU_CANCEL,
    ACTION_MAX_MENU = ACTION_MENU_CANCEL,

    MINECRAFT_ACTION_JUMP,
    MINECRAFT_ACTION_FORWARD,
    MINECRAFT_ACTION_BACKWARD,
    MINECRAFT_ACTION_LEFT,
    MINECRAFT_ACTION_RIGHT,
    MINECRAFT_ACTION_LOOK_LEFT,
    MINECRAFT_ACTION_LOOK_RIGHT,
    MINECRAFT_ACTION_LOOK_UP,
    MINECRAFT_ACTION_LOOK_DOWN,
    MINECRAFT_ACTION_USE,
    MINECRAFT_ACTION_ACTION,
    MINECRAFT_ACTION_LEFT_SCROLL,
    MINECRAFT_ACTION_RIGHT_SCROLL,
    MINECRAFT_ACTION_INVENTORY,
    MINECRAFT_ACTION_PAUSEMENU,
    MINECRAFT_ACTION_DROP,
    MINECRAFT_ACTION_SNEAK_TOGGLE,
    MINECRAFT_ACTION_CRAFTING,
    MINECRAFT_ACTION_RENDER_THIRD_PERSON,
    MINECRAFT_ACTION_GAME_INFO,
    MINECRAFT_ACTION_DPAD_LEFT,
    MINECRAFT_ACTION_DPAD_RIGHT,
    MINECRAFT_ACTION_DPAD_UP,
    MINECRAFT_ACTION_DPAD_DOWN,

    MINECRAFT_ACTION_MAX,

    // These 4 aren't mapped to the input manager directly but are created from the dpad controls if required in Minecraft::run_middle
    // Don't use them with the input manager directly, just through LocalPlayer::ullButtonsPressed
    MINECRAFT_ACTION_SPAWN_CREEPER,
    MINECRAFT_ACTION_CHANGE_SKIN,
    MINECRAFT_ACTION_FLY_TOGGLE,
    MINECRAFT_ACTION_RENDER_DEBUG
};

enum class eMCLang
{
    eMCLang_null = 0,
    eMCLang_enUS,
    eMCLang_enGB,
    eMCLang_enIE,
    eMCLang_enAU,
    eMCLang_enNZ,
    eMCLang_enCA,
    eMCLang_jaJP,
    eMCLang_deDE,
    eMCLang_deAT,
    eMCLang_frFR,
    eMCLang_frCA,
    eMCLang_esES,
    eMCLang_esMX,
    eMCLang_itIT,
    eMCLang_koKR,
    eMCLang_ptPT,
    eMCLang_ptBR,
    eMCLang_ruRU,
    eMCLang_nlNL,
    eMCLang_fiFI,
    eMCLang_svSV,
    eMCLang_daDA,
    eMCLang_noNO,
    eMCLang_plPL,
    eMCLang_trTR,
    eMCLang_elEL,
    eMCLang_csCS,
    eMCLang_zhCHT,
    eMCLang_laLAS,

    eMCLang_zhSG,
    eMCLang_zhCN,
    eMCLang_zhHK,
    eMCLang_zhTW,
    eMCLang_nlBE,
    eMCLang_daDK,
    eMCLang_frBE,
    eMCLang_frCH,
    eMCLang_deCH,
    eMCLang_nbNO,
    eMCLang_enGR,
    eMCLang_enHK,
    eMCLang_enSA,
    eMCLang_enHU,
    eMCLang_enIN,
    eMCLang_enIL,
    eMCLang_enSG,
    eMCLang_enSK,
    eMCLang_enZA,
    eMCLang_enCZ,
    eMCLang_enAE,
    eMCLang_esAR,
    eMCLang_esCL,
    eMCLang_esCO,
    eMCLang_esUS,
    eMCLang_svSE,

    eMCLang_csCZ,
    eMCLang_elGR,
    eMCLang_nnNO,
    eMCLang_skSK,

    eMCLang_hans,
    eMCLang_hant,
};
