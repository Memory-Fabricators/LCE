// Real (non-Iggy) port of CMinecraftApp, the top-level game services object.
//
// The original implementation lives in Common/Consoles_App.cpp, which is excluded
// from the SDL3 build because it drags in the proprietary Iggy UI middleware
// (Common/UI/UI.h) and Xbox-only headers (xuiapp.h, xsocialpost.h, XUI/*) that have
// no SDL3 equivalent. This file re-implements every method the SDL3 build actually
// needs, porting the real (platform-portable) logic for real, and leaving only the
// genuinely Iggy/XUI-specific pieces as explicitly-commented no-ops.
#include "stdafx.h"

#include "../../Minecraft.World/AABB.h"
#include "../../Minecraft.World/Container.h"
#include "../../Minecraft.World/DispenserTileEntity.h"
#include "../../Minecraft.World/FurnaceTileEntity.h"
#include "../../Minecraft.World/Inventory.h"
#include "../../Minecraft.World/Level.h"
#include "../../Minecraft.World/Player.h"
#include "../../Minecraft.World/Recipy.h"
#include "../../Minecraft.World/SignTileEntity.h"
#include "../../Minecraft.World/Vec3.h"
#include "../../Minecraft.World/compression.h"
#include "../ArchiveFile.h"
#include "../Common/Consoles_App.h"
#include "../Common/DLC/DLCPack.h"
#include "../EntityRenderDispatcher.h"
#include "../GameMode.h"
#include "../LocalPlayer.h"
#include "../Minecraft.h"
#include "../MinecraftServer.h"
#include "../MultiPlayerLocalPlayer.h"
#include "../Options.h"
#include "../PlayerList.h"
#include "../ServerPlayer.h"
#include "../StringTable.h"
#include "../TexturePackRepository.h"
#include "SDL3_App.h"
#include "SDL3_UIController.h"

#include <sstream>

// ---------------------------------------------------------------------------
// Static data members
//
// Consoles_App.cpp normally provides the out-of-line definitions for these -
// since that file is excluded here, the methods below that odr-use them would
// otherwise leave the linker with new undefined symbols of their own.
// ---------------------------------------------------------------------------
unsigned int CMinecraftApp::m_uiLastSignInData = 0;

const float CMinecraftApp::fSafeZoneX = 64.0f; // 5% of 1280
const float CMinecraftApp::fSafeZoneY = 36.0f; // 5% of 720

int CMinecraftApp::s_iHTMLFontSizesA[eHTMLSize_COUNT] = {20, 13, 20, 26};

unsigned char CMinecraftApp::m_szPNG[8] = {137, 80, 78, 71, 13, 10, 26, 10};

DWORD CMinecraftApp::m_dwContentTypeA[e_Marketplace_MAX] = {0};

unordered_map<PlayerUID, MOJANG_DATA *> CMinecraftApp::MojangData;
unordered_map<int, ULONGLONG> CMinecraftApp::DLCTextures_PackID;
unordered_map<ULONGLONG, DLC_INFO *> CMinecraftApp::DLCInfo_Trial;
unordered_map<ULONGLONG, DLC_INFO *> CMinecraftApp::DLCInfo_Full;
unordered_map<wstring, ULONGLONG> CMinecraftApp::DLCInfo_SkinName;

TIPSTRUCT CMinecraftApp::m_GameTipA[CMinecraftApp::MAX_TIPS_GAMETIP] = {
    {0, IDS_TIPS_GAMETIP_1},
    {0, IDS_TIPS_GAMETIP_2},
    {0, IDS_TIPS_GAMETIP_3},
    {0, IDS_TIPS_GAMETIP_4},
    {0, IDS_TIPS_GAMETIP_5},
    {0, IDS_TIPS_GAMETIP_6},
    {0, IDS_TIPS_GAMETIP_7},
    {0, IDS_TIPS_GAMETIP_8},
    {0, IDS_TIPS_GAMETIP_9},
    {0, IDS_TIPS_GAMETIP_10},
    {0, IDS_TIPS_GAMETIP_11},
    {0, IDS_TIPS_GAMETIP_12},
    {0, IDS_TIPS_GAMETIP_13},
    {0, IDS_TIPS_GAMETIP_14},
    {0, IDS_TIPS_GAMETIP_15},
    {0, IDS_TIPS_GAMETIP_16},
    {0, IDS_TIPS_GAMETIP_17},
    {0, IDS_TIPS_GAMETIP_18},
    {0, IDS_TIPS_GAMETIP_19},
    {0, IDS_TIPS_GAMETIP_20},
    {0, IDS_TIPS_GAMETIP_21},
    {0, IDS_TIPS_GAMETIP_22},
    {0, IDS_TIPS_GAMETIP_23},
    {0, IDS_TIPS_GAMETIP_24},
    {0, IDS_TIPS_GAMETIP_25},
    {0, IDS_TIPS_GAMETIP_26},
    {0, IDS_TIPS_GAMETIP_27},
    {0, IDS_TIPS_GAMETIP_28},
    {0, IDS_TIPS_GAMETIP_29},
    {0, IDS_TIPS_GAMETIP_30},
    {0, IDS_TIPS_GAMETIP_31},
    {0, IDS_TIPS_GAMETIP_32},
    {0, IDS_TIPS_GAMETIP_33},
    {0, IDS_TIPS_GAMETIP_34},
    {0, IDS_TIPS_GAMETIP_35},
    {0, IDS_TIPS_GAMETIP_36},
    {0, IDS_TIPS_GAMETIP_37},
    {0, IDS_TIPS_GAMETIP_38},
    {0, IDS_TIPS_GAMETIP_39},
    {0, IDS_TIPS_GAMETIP_40},
    {0, IDS_TIPS_GAMETIP_41},
    {0, IDS_TIPS_GAMETIP_42},
    {0, IDS_TIPS_GAMETIP_43},
    {0, IDS_TIPS_GAMETIP_44},
    {0, IDS_TIPS_GAMETIP_45},
    {0, IDS_TIPS_GAMETIP_46},
    {0, IDS_TIPS_GAMETIP_47},
    {0, IDS_TIPS_GAMETIP_48},
    {0, IDS_TIPS_GAMETIP_49},
    {0, IDS_TIPS_GAMETIP_50},
};

TIPSTRUCT CMinecraftApp::m_TriviaTipA[CMinecraftApp::MAX_TIPS_TRIVIATIP] = {
    {0, IDS_TIPS_TRIVIA_1},
    {0, IDS_TIPS_TRIVIA_2},
    {0, IDS_TIPS_TRIVIA_3},
    {0, IDS_TIPS_TRIVIA_4},
    {0, IDS_TIPS_TRIVIA_5},
    {0, IDS_TIPS_TRIVIA_6},
    {0, IDS_TIPS_TRIVIA_7},
    {0, IDS_TIPS_TRIVIA_8},
    {0, IDS_TIPS_TRIVIA_9},
    {0, IDS_TIPS_TRIVIA_10},
    {0, IDS_TIPS_TRIVIA_11},
    {0, IDS_TIPS_TRIVIA_12},
    {0, IDS_TIPS_TRIVIA_13},
    {0, IDS_TIPS_TRIVIA_14},
    {0, IDS_TIPS_TRIVIA_15},
    {0, IDS_TIPS_TRIVIA_16},
    {0, IDS_TIPS_TRIVIA_17},
    {0, IDS_TIPS_TRIVIA_18},
    {0, IDS_TIPS_TRIVIA_19},
    {0, IDS_TIPS_TRIVIA_20},
};

Random *CMinecraftApp::TipRandom = new Random();

int CMinecraftApp::TipsSortFunction(const void *a, const void *b)
{
    return ((TIPSTRUCT *)a)->iSortValue - ((TIPSTRUCT *)b)->iSortValue;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
CMinecraftApp::CMinecraftApp()
{
    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        m_eTMSAction[i] = eTMSAction_Idle;
        m_eXuiAction[i] = eAppAction_Idle;
        m_eXuiActionParam[i] = NULL;
        m_bRead_BannedListA[i] = false;
        SetBanListCheck(i, false);
        m_uiOpacityCountDown[i] = 0;
    }
    m_eGlobalXuiAction = eAppAction_Idle;
    m_eGlobalXuiServerAction = eXuiServerAction_Idle;

    m_bResourcesLoaded = false;
    m_bGameStarted = false;
    m_bIsAppPaused = false;
    m_bIntroRunning = false;
    m_eGameMode = eMode_Singleplayer;
    m_bLoadSavesFromFolderEnabled = false;
    m_bWriteSavesToFolderEnabled = false;
    m_bTutorialMode = false;
    m_disconnectReason = DisconnectPacket::eDisconnect_None;
    m_bLiveLinkRequired = false;
    m_bChangingSessionType = false;
    m_bReallyChangingSessionType = false;
    m_bDebugOptions = false;

    m_xuidNotch = INVALID_XUID;
    ZeroMemory(&m_InviteData, sizeof(JoinFromInviteData));

    m_pDLCFileBuffer = NULL;
    m_dwDLCFileSize = 0;
    m_pBannedListFileBuffer = NULL;
    m_dwBannedListFileSize = 0;

    m_bDefaultCapeInstallAttempted = false;
    m_bDLCInstallProcessCompleted = false;
    m_bDLCInstallPending = false;
    m_iTotalDLC = 0;
    m_iTotalDLCInstalled = 0;
    mfTrialPausedTime = 0.0f;
    m_uiAutosaveTimer = 0;
    ZeroMemory(m_pszUniqueMapName, 14);

    m_bNewDLCAvailable = false;
    m_bSeenNewDLCTip = false;
    m_uiGameHostSettings = 0;

    ZeroMemory(m_playerColours, MINECRAFT_NET_MAX_PLAYERS);

    m_iDLCOfferC = 0;
    m_bAllDLCContentRetrieved = true;
    InitializeCriticalSection(&csDLCDownloadQueue);
    m_bAllTMSContentRetrieved = true;
    m_bTickTMSDLCFiles = true;
    InitializeCriticalSection(&csTMSPPDownloadQueue);
    InitializeCriticalSection(&csAdditionalModelParts);
    InitializeCriticalSection(&csAdditionalSkinBoxes);
    InitializeCriticalSection(&csAnimOverrideBitmask);
    InitializeCriticalSection(&csMemFilesLock);
    InitializeCriticalSection(&csMemTPDLock);
    InitializeCriticalSection(&m_saveNotificationCriticalSection);
    m_saveNotificationDepth = 0;

    m_dwRequiredTexturePackID = 0;
    m_bResetNether = false;
    m_bUseDPadForDebug = true;

    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        m_vBannedListA[i] = new vector<PBANNEDLISTDATA>;
    }

    m_mediaArchive = NULL;
    m_stringTable = NULL;

    LocaleAndLanguageInit();
}

// ---------------------------------------------------------------------------
// Debug output
//
// OutputDebugStringA has no meaning off Windows, so route debug text to
// stderr (and, for the USER_UI channel, into the UI's own debug console when
// one exists) instead.
// ---------------------------------------------------------------------------
void CMinecraftApp::DebugPrintf(const char *szFormat, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, szFormat);
    vsnprintf(buf, sizeof(buf), szFormat, ap);
    va_end(ap);
    fputs(buf, stderr);
}

void CMinecraftApp::DebugPrintf(int user, const char *szFormat, ...)
{
    if (user == USER_NONE)
    {
        return;
    }
    char buf[1024];
    va_list ap;
    va_start(ap, szFormat);
    vsnprintf(buf, sizeof(buf), szFormat, ap);
    va_end(ap);
    // ConsoleUIController has no debug-console equivalent of Iggy's logDebugString
    // yet, so USER_UI just falls through to the same stderr output as every
    // other channel.
    fputs(buf, stderr);
}

// ---------------------------------------------------------------------------
// App state
// ---------------------------------------------------------------------------
bool CMinecraftApp::IsAppPaused()
{
    return m_bIsAppPaused;
}

void CMinecraftApp::SetAppPaused(bool val)
{
    m_bIsAppPaused = val;
}

void CMinecraftApp::HandleButtonPresses()
{
    // The console build's per-pad HandleButtonPresses(int) body is an empty,
    // commented-out debug-profile stress test on every platform - nothing to port.
}

int CMinecraftApp::GetLocalPlayerCount(void)
{
    int iPlayerC = 0;
    Minecraft *pMinecraft = Minecraft::GetInstance();
    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        if (pMinecraft != NULL && pMinecraft->localplayers[i] != NULL)
        {
            iPlayerC++;
        }
    }
    return iPlayerC;
}

void CMinecraftApp::SetAction(int iPad, eXuiAction action, LPVOID param)
{
    if (m_eXuiAction[iPad] == eAppAction_ExitWorldCapturedThumbnail && action != eAppAction_Idle)
    {
        app.DebugPrintf("Invalid change of App action for pad %d from %d to %d, ignoring\n", iPad, m_eXuiAction[iPad], action);
    }
    else
    {
        app.DebugPrintf("Changing App action for pad %d from %d to %d\n", iPad, m_eXuiAction[iPad], action);
        m_eXuiAction[iPad] = action;
        m_eXuiActionParam[iPad] = param;
    }
}

// HandleXuiActions is a ~1600-line dispatcher that drives Iggy/XUI dialog flows
// (confirmation boxes, upsell screens, save-notification dialogs, etc.) entirely
// through the excluded Common/UI subsystem. There is no portable subset to
// extract - every branch ends in an Iggy scene call. Left as a documented no-op
// until the SDL3 UI layer (ConsoleUIController) grows real scene handling.
void CMinecraftApp::HandleXuiActions(void)
{
}

void CMinecraftApp::SetSpecialTutorialCompletionFlag(int iPad, int index)
{
    if (index >= 0 && index < 32 && GameSettingsA[iPad] != NULL)
    {
        GameSettingsA[iPad]->uiSpecialTutorialBitmask |= (1 << index);
    }
}

LPCWSTR CMinecraftApp::GetString(int iID)
{
    if (app.m_stringTable == NULL)
    {
        // Every other platform guarantees the string table loaded successfully
        // by the time anything calls GetString (see loadStringTable()) - same
        // flaky-mounted-volume class of issue as AbstractTexturePack's colours.col
        // read. Degrade to an empty string instead of crashing every caller.
        return L"";
    }
    return app.m_stringTable->getString(iID);
}

// ---------------------------------------------------------------------------
// Menu loading
//
// These all delegate to ConsoleUIController::NavigateToScene, which is the
// real SDL3 UI integration point (Minecraft.Client/SDL3/SDL3_UIController.h).
// The controller itself is still mostly a stub (NavigateToScene currently
// just returns false) - that is tracked separately - but the call sites here
// are real, not placeholders.
// ---------------------------------------------------------------------------
bool CMinecraftApp::LoadInventoryMenu(int iPad, shared_ptr<LocalPlayer> player, bool bNavigateBack)
{
    InventoryScreenInput *initData = new InventoryScreenInput();
    initData->player = player;
    initData->bNavigateBack = bNavigateBack;
    initData->iPad = iPad;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_InventoryMenu, initData);
}

bool CMinecraftApp::LoadCreativeMenu(int iPad, shared_ptr<LocalPlayer> player, bool bNavigateBack)
{
    InventoryScreenInput *initData = new InventoryScreenInput();
    initData->player = player;
    initData->bNavigateBack = bNavigateBack;
    initData->iPad = iPad;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_CreativeMenu, initData);
}

bool CMinecraftApp::LoadCrafting2x2Menu(int iPad, shared_ptr<LocalPlayer> player)
{
    CraftingPanelScreenInput *initData = new CraftingPanelScreenInput();
    initData->player = player;
    initData->iContainerType = RECIPE_TYPE_2x2;
    initData->iPad = iPad;
    initData->x = 0;
    initData->y = 0;
    initData->z = 0;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_Crafting2x2Menu, initData);
}

bool CMinecraftApp::LoadCrafting3x3Menu(int iPad, shared_ptr<LocalPlayer> player, int x, int y, int z)
{
    CraftingPanelScreenInput *initData = new CraftingPanelScreenInput();
    initData->player = player;
    initData->iContainerType = RECIPE_TYPE_3x3;
    initData->iPad = iPad;
    initData->x = x;
    initData->y = y;
    initData->z = z;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_Crafting3x3Menu, initData);
}

bool CMinecraftApp::LoadEnchantingMenu(int iPad, shared_ptr<Inventory> inventory, int x, int y, int z, Level *level)
{
    EnchantingScreenInput *initData = new EnchantingScreenInput();
    initData->inventory = inventory;
    initData->level = level;
    initData->x = x;
    initData->y = y;
    initData->z = z;
    initData->iPad = iPad;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_EnchantingMenu, initData);
}

bool CMinecraftApp::LoadFurnaceMenu(int iPad, shared_ptr<Inventory> inventory, shared_ptr<FurnaceTileEntity> furnace)
{
    FurnaceScreenInput *initData = new FurnaceScreenInput();
    initData->furnace = furnace;
    initData->inventory = inventory;
    initData->iPad = iPad;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_FurnaceMenu, initData);
}

bool CMinecraftApp::LoadBrewingStandMenu(int iPad, shared_ptr<Inventory> inventory, shared_ptr<BrewingStandTileEntity> brewingStand)
{
    BrewingScreenInput *initData = new BrewingScreenInput();
    initData->brewingStand = brewingStand;
    initData->inventory = inventory;
    initData->iPad = iPad;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_BrewingStandMenu, initData);
}

bool CMinecraftApp::LoadContainerMenu(int iPad, shared_ptr<Container> inventory, shared_ptr<Container> container)
{
    ContainerScreenInput *initData = new ContainerScreenInput();
    initData->inventory = inventory;
    initData->container = container;
    initData->iPad = iPad;

    if (app.GetLocalPlayerCount() > 1)
    {
        initData->bSplitscreen = true;
        bool bLargeChest = container->getContainerSize() > 3 * 9;
        return ui.NavigateToScene(iPad, bLargeChest ? eUIScene_LargeContainerMenu : eUIScene_ContainerMenu, initData);
    }

    initData->bSplitscreen = false;
    return ui.NavigateToScene(iPad, eUIScene_ContainerMenu, initData);
}

bool CMinecraftApp::LoadTrapMenu(int iPad, shared_ptr<Container> inventory, shared_ptr<DispenserTileEntity> trap)
{
    TrapScreenInput *initData = new TrapScreenInput();
    initData->inventory = inventory;
    initData->trap = trap;
    initData->iPad = iPad;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_DispenserMenu, initData);
}

bool CMinecraftApp::LoadSignEntryMenu(int iPad, shared_ptr<SignTileEntity> sign)
{
    SignEntryScreenInput *initData = new SignEntryScreenInput();
    initData->sign = sign;
    initData->iPad = iPad;
    bool success = ui.NavigateToScene(iPad, eUIScene_SignEntryMenu, initData);
    delete initData;
    return success;
}

bool CMinecraftApp::LoadRepairingMenu(int iPad, shared_ptr<Inventory> inventory, Level *level, int x, int y, int z)
{
    AnvilScreenInput *initData = new AnvilScreenInput();
    initData->inventory = inventory;
    initData->level = level;
    initData->x = x;
    initData->y = y;
    initData->z = z;
    initData->iPad = iPad;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_AnvilMenu, initData);
}

bool CMinecraftApp::LoadTradingMenu(int iPad, shared_ptr<Inventory> inventory, shared_ptr<Merchant> trader, Level *level)
{
    TradingScreenInput *initData = new TradingScreenInput();
    initData->inventory = inventory;
    initData->trader = trader;
    initData->level = level;
    initData->iPad = iPad;
    initData->bSplitscreen = app.GetLocalPlayerCount() > 1;
    return ui.NavigateToScene(iPad, eUIScene_TradingMenu, initData);
}

// ---------------------------------------------------------------------------
// Game settings
// ---------------------------------------------------------------------------
// Consoles_App.cpp's InitGameSettings() (excluded from this build - see the
// file banner above) additionally calls SetDefaultOptions() to seed every
// GAME_SETTINGS field with a sane default. That setup never ran here, which
// left every field - including ucSensitivity - zero-initialised: with
// sensitivity permanently 0, Input.cpp's `GetJoypadStick_RX(iPad) *
// (GetGameSettings(iPad, eGameSetting_Sensitivity_InGame) / 100.0f)` always
// multiplied the mouse-look stick by zero, so mouse-look was silently dead
// while WASD (which doesn't go through this multiplier) worked fine. Seed
// the same defaults directly here; CMinecraftApp::SetGameSettings() (the
// setter Consoles_App.cpp normally provides) isn't ported since nothing else
// in this build needs it yet.
void CMinecraftApp::InitGameSettings()
{
    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        GameSettingsA[i] = (GAME_SETTINGS *)ProfileManager.GetGameDefinedProfileData(i);
        if (GameSettingsA[i] != NULL)
        {
            GameSettingsA[i]->bSettingsChanged = false;

            GameSettingsA[i]->ucMusicVolume = DEFAULT_VOLUME_LEVEL;
            GameSettingsA[i]->ucSoundFXVolume = DEFAULT_VOLUME_LEVEL;
            GameSettingsA[i]->ucGamma = 50;
            GameSettingsA[i]->ucSensitivity = 100;
            GameSettingsA[i]->ucMenuSensitivity = 100;
            GameSettingsA[i]->ucInterfaceOpacity = 80;

            GameSettingsA[i]->usBitmaskValues = 0;
            GameSettingsA[i]->usBitmaskValues |= 1;       // difficulty (bits 0-1) = 1 (easy)
            GameSettingsA[i]->usBitmaskValues |= 0x0004;  // view bob - on
            GameSettingsA[i]->usBitmaskValues |= 0x0008;  // gamertags visible - on
            GameSettingsA[i]->usBitmaskValues |= 0x0200;  // splitscreen gamertags - on
            GameSettingsA[i]->usBitmaskValues |= 0x0400;  // hints - on
            GameSettingsA[i]->usBitmaskValues |= 2 << 11; // autosave = 2 (every 30 minutes)
            GameSettingsA[i]->usBitmaskValues |= 0x8000;  // tooltips - on

            // Push the sensitivity default through to Options::Option::SENSITIVITY
            // the same way ActionGameSettings() does whenever the setting changes.
            ActionGameSettings(i, eGameSetting_Sensitivity_InGame);
        }
    }
}

unsigned char CMinecraftApp::GetGameSettings(eGameSetting eVal)
{
    int iPad = ProfileManager.GetPrimaryPad();
    return GetGameSettings(iPad, eVal);
}

unsigned char CMinecraftApp::GetGameSettings(int iPad, eGameSetting eVal)
{
    if (GameSettingsA[iPad] == NULL)
    {
        return 0;
    }

    switch (eVal)
    {
    case eGameSetting_MusicVolume:
        return GameSettingsA[iPad]->ucMusicVolume;
    case eGameSetting_SoundFXVolume:
        return GameSettingsA[iPad]->ucSoundFXVolume;
    case eGameSetting_Gamma:
        return GameSettingsA[iPad]->ucGamma;
    case eGameSetting_Difficulty:
        return GameSettingsA[iPad]->usBitmaskValues & 0x0003;
    case eGameSetting_Sensitivity_InGame:
        return GameSettingsA[iPad]->ucSensitivity;
    case eGameSetting_ViewBob:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x0004) >> 2;
    case eGameSetting_GamertagsVisible:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x0008) >> 3;
    case eGameSetting_ControlScheme:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x0030) >> 4;
    case eGameSetting_ControlInvertLook:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x0040) >> 6;
    case eGameSetting_ControlSouthPaw:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x0080) >> 7;
    case eGameSetting_SplitScreenVertical:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x0100) >> 8;
    case eGameSetting_Sensitivity_InMenu:
        return GameSettingsA[iPad]->ucMenuSensitivity;
    case eGameSetting_DisplaySplitscreenGamertags:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x0200) >> 9;
    case eGameSetting_Hints:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x0400) >> 10;
    case eGameSetting_Autosave:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x7800) >> 11;
    case eGameSetting_Tooltips:
        return (GameSettingsA[iPad]->usBitmaskValues & 0x8000) >> 15;
    case eGameSetting_InterfaceOpacity:
        return GameSettingsA[iPad]->ucInterfaceOpacity;
    case eGameSetting_Clouds:
        return GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_CLOUDS;
    case eGameSetting_Online:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_ONLINE) >> 1;
    case eGameSetting_InviteOnly:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_INVITEONLY) >> 2;
    case eGameSetting_FriendsOfFriends:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_FRIENDSOFFRIENDS) >> 3;
    case eGameSetting_DisplayUpdateMessage:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_DISPLAYUPDATEMSG) >> 4;
    case eGameSetting_BedrockFog:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_BEDROCKFOG) >> 6;
    case eGameSetting_DisplayHUD:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_DISPLAYHUD) >> 7;
    case eGameSetting_DisplayHand:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_DISPLAYHAND) >> 8;
    case eGameSetting_CustomSkinAnim:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_CUSTOMSKINANIM) >> 9;
    case eGameSetting_DeathMessages:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_DEATHMESSAGES) >> 10;
    case eGameSetting_UISize:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_UISIZE) >> 11;
    case eGameSetting_UISizeSplitscreen:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_UISIZE_SPLITSCREEN) >> 13;
    case eGameSetting_AnimatedCharacter:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_ANIMATEDCHARACTER) >> 15;
    case eGameSetting_PS3_EULA_Read:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_PS3EULAREAD) >> 16;
    case eGameSetting_PSVita_NetworkModeAdhoc:
        return (GameSettingsA[iPad]->uiBitmaskValues & GAMESETTING_PSVITANETWORKMODEADHOC) >> 17;
    }
    return 0;
}

void CMinecraftApp::ActionGameSettings(int iPad, eGameSetting eVal)
{
    Minecraft *pMinecraft = Minecraft::GetInstance();
    switch (eVal)
    {
    case eGameSetting_MusicVolume:
        if (iPad == ProfileManager.GetPrimaryPad())
        {
            pMinecraft->options->set(Options::Option::MUSIC, ((float)GameSettingsA[iPad]->ucMusicVolume) / 100.0f);
        }
        break;
    case eGameSetting_SoundFXVolume:
        if (iPad == ProfileManager.GetPrimaryPad())
        {
            pMinecraft->options->set(Options::Option::SOUND, ((float)GameSettingsA[iPad]->ucSoundFXVolume) / 100.0f);
        }
        break;
    case eGameSetting_Gamma:
        if (iPad == ProfileManager.GetPrimaryPad())
        {
            float fVal = ((float)GameSettingsA[iPad]->ucGamma) * 327.68f;
            RenderManager.UpdateGamma((unsigned short)fVal);
        }
        break;
    case eGameSetting_Difficulty:
        if (iPad == ProfileManager.GetPrimaryPad())
        {
            pMinecraft->options->toggle(Options::Option::DIFFICULTY, GameSettingsA[iPad]->usBitmaskValues & 0x03);
            app.DebugPrintf("Difficulty toggle to %d\n", GameSettingsA[iPad]->usBitmaskValues & 0x03);

            app.SetGameHostOption(eGameHostOption_Difficulty, pMinecraft->options->difficulty);

            bool bInGame = pMinecraft->level != NULL;
            if (bInGame && g_NetworkManager.IsHost() && (iPad == ProfileManager.GetPrimaryPad()))
            {
                app.SetXuiServerAction(iPad, eXuiServerAction_ServerSettingChanged_Difficulty);
            }
        }
        else
        {
            app.DebugPrintf("NOT ACTIONING DIFFICULTY - Primary pad is %d, This pad is %d\n", ProfileManager.GetPrimaryPad(), iPad);
        }
        break;
    case eGameSetting_Sensitivity_InGame:
        pMinecraft->options->set(Options::Option::SENSITIVITY, ((float)GameSettingsA[iPad]->ucSensitivity) / 100.0f);
        break;
    case eGameSetting_ViewBob:
        // Handled per-player straight from GameSettings rather than here - see original comment.
        break;
    case eGameSetting_ControlScheme:
        InputManager.SetJoypadMapVal(iPad, (GameSettingsA[iPad]->usBitmaskValues & 0x30) >> 4);
        break;
    case eGameSetting_ControlInvertLook:
        break;
    case eGameSetting_ControlSouthPaw:
        if (GameSettingsA[iPad]->usBitmaskValues & 0x80)
        {
            InputManager.SetJoypadStickAxisMap(iPad, AXIS_MAP_LX, AXIS_MAP_RX);
            InputManager.SetJoypadStickAxisMap(iPad, AXIS_MAP_LY, AXIS_MAP_RY);
            InputManager.SetJoypadStickAxisMap(iPad, AXIS_MAP_RX, AXIS_MAP_LX);
            InputManager.SetJoypadStickAxisMap(iPad, AXIS_MAP_RY, AXIS_MAP_LY);
            InputManager.SetJoypadStickTriggerMap(iPad, TRIGGER_MAP_0, TRIGGER_MAP_1);
            InputManager.SetJoypadStickTriggerMap(iPad, TRIGGER_MAP_1, TRIGGER_MAP_0);
        }
        else
        {
            InputManager.SetJoypadStickAxisMap(iPad, AXIS_MAP_LX, AXIS_MAP_LX);
            InputManager.SetJoypadStickAxisMap(iPad, AXIS_MAP_LY, AXIS_MAP_LY);
            InputManager.SetJoypadStickAxisMap(iPad, AXIS_MAP_RX, AXIS_MAP_RX);
            InputManager.SetJoypadStickAxisMap(iPad, AXIS_MAP_RY, AXIS_MAP_RY);
            InputManager.SetJoypadStickTriggerMap(iPad, TRIGGER_MAP_0, TRIGGER_MAP_0);
            InputManager.SetJoypadStickTriggerMap(iPad, TRIGGER_MAP_1, TRIGGER_MAP_1);
        }
        break;
    case eGameSetting_SplitScreenVertical:
        if (iPad == ProfileManager.GetPrimaryPad())
        {
            pMinecraft->updatePlayerViewportAssignments();
        }
        break;
    case eGameSetting_GamertagsVisible:
        {
            bool bInGame = pMinecraft->level != NULL;
            if (bInGame && g_NetworkManager.IsHost() && (iPad == ProfileManager.GetPrimaryPad()))
            {
                app.SetGameHostOption(eGameHostOption_Gamertags, ((GameSettingsA[iPad]->usBitmaskValues & 0x0008) != 0) ? 1 : 0);
                app.SetXuiServerAction(iPad, eXuiServerAction_ServerSettingChanged_Gamertags);

                PlayerList *players = MinecraftServer::getInstance()->getPlayerList();
                for (AUTO_VAR(it3, players->players.begin()); it3 != players->players.end(); ++it3)
                {
                    shared_ptr<ServerPlayer> decorationPlayer = *it3;
                    decorationPlayer->setShowOnMaps((app.GetGameHostOption(eGameHostOption_Gamertags) != 0) ? true : false);
                }
            }
        }
        break;
    case eGameSetting_Sensitivity_InMenu:
        break;
    case eGameSetting_DisplaySplitscreenGamertags:
        for (BYTE idx = 0; idx < XUSER_MAX_COUNT; ++idx)
        {
            if (pMinecraft->localplayers[idx] != NULL)
            {
                bool bFullscreen = pMinecraft->localplayers[idx]->m_iScreenSection == C4JRender::VIEWPORT_TYPE_FULLSCREEN;
                ui.DisplayGamertag(idx, !bFullscreen);
            }
        }
        break;
    case eGameSetting_InterfaceOpacity:
        ui.RefreshTooltips(iPad);
        break;
    case eGameSetting_Hints:
        break;
    case eGameSetting_Tooltips:
        ui.SetEnableTooltips(iPad, (GameSettingsA[iPad]->usBitmaskValues & 0x8000) != 0 ? TRUE : FALSE);
        break;
    case eGameSetting_Clouds:
    case eGameSetting_Online:
    case eGameSetting_InviteOnly:
    case eGameSetting_FriendsOfFriends:
        break;
    case eGameSetting_BedrockFog:
        {
            bool bInGame = pMinecraft->level != NULL;
            if (bInGame && g_NetworkManager.IsHost() && (iPad == ProfileManager.GetPrimaryPad()))
            {
                app.SetGameHostOption(eGameHostOption_BedrockFog, GetGameSettings(iPad, eGameSetting_BedrockFog) ? 1 : 0);
                app.SetXuiServerAction(iPad, eXuiServerAction_ServerSettingChanged_BedrockFog);
            }
        }
        break;
    case eGameSetting_DisplayHUD:
    case eGameSetting_DisplayHand:
    case eGameSetting_CustomSkinAnim:
    case eGameSetting_DeathMessages:
    case eGameSetting_UISize:
    case eGameSetting_UISizeSplitscreen:
    case eGameSetting_AnimatedCharacter:
    case eGameSetting_PS3_EULA_Read:
    case eGameSetting_PSVita_NetworkModeAdhoc:
        break;
    }
}

void CMinecraftApp::ApplyGameSettingsChanged(int iPad)
{
    ActionGameSettings(iPad, eGameSetting_MusicVolume);
    ActionGameSettings(iPad, eGameSetting_SoundFXVolume);
    ActionGameSettings(iPad, eGameSetting_Gamma);
    ActionGameSettings(iPad, eGameSetting_Difficulty);
    ActionGameSettings(iPad, eGameSetting_Sensitivity_InGame);
    ActionGameSettings(iPad, eGameSetting_ViewBob);
    ActionGameSettings(iPad, eGameSetting_ControlScheme);
    ActionGameSettings(iPad, eGameSetting_ControlInvertLook);
    ActionGameSettings(iPad, eGameSetting_ControlSouthPaw);
    ActionGameSettings(iPad, eGameSetting_SplitScreenVertical);
    ActionGameSettings(iPad, eGameSetting_GamertagsVisible);
    ActionGameSettings(iPad, eGameSetting_Sensitivity_InMenu);
    ActionGameSettings(iPad, eGameSetting_DisplaySplitscreenGamertags);
    ActionGameSettings(iPad, eGameSetting_Hints);
    ActionGameSettings(iPad, eGameSetting_InterfaceOpacity);
    ActionGameSettings(iPad, eGameSetting_Tooltips);
    ActionGameSettings(iPad, eGameSetting_Clouds);
    ActionGameSettings(iPad, eGameSetting_BedrockFog);
    ActionGameSettings(iPad, eGameSetting_DisplayHUD);
    ActionGameSettings(iPad, eGameSetting_DisplayHand);
    ActionGameSettings(iPad, eGameSetting_CustomSkinAnim);
    ActionGameSettings(iPad, eGameSetting_DeathMessages);
    ActionGameSettings(iPad, eGameSetting_UISize);
    ActionGameSettings(iPad, eGameSetting_UISizeSplitscreen);
    ActionGameSettings(iPad, eGameSetting_AnimatedCharacter);
    ActionGameSettings(iPad, eGameSetting_PS3_EULA_Read);
}

#ifndef _DEBUG_MENUS_ENABLED
unsigned int CMinecraftApp::GetGameSettingsDebugMask(int iPad, bool bOverridePlayer)
{
    return 0;
}

void CMinecraftApp::SetGameSettingsDebugMask(int iPad, unsigned int uiVal)
{
}
#else
unsigned int CMinecraftApp::GetGameSettingsDebugMask(int iPad, bool bOverridePlayer)
{
    if (iPad == -1)
    {
        iPad = ProfileManager.GetPrimaryPad();
    }
    if (iPad < 0)
    {
        iPad = 0;
    }

    shared_ptr<Player> player = Minecraft::GetInstance()->localplayers[iPad];
    if (bOverridePlayer || player == NULL)
    {
        return GameSettingsA[iPad]->uiDebugBitmask;
    }
    return player->GetDebugOptions();
}

void CMinecraftApp::SetGameSettingsDebugMask(int iPad, unsigned int uiVal)
{
    GameSettingsA[iPad]->bSettingsChanged = true;
    GameSettingsA[iPad]->uiDebugBitmask = uiVal;

    shared_ptr<Player> player = Minecraft::GetInstance()->localplayers[iPad];
    if (player)
    {
        Minecraft::GetInstance()->localgameModes[iPad]->handleDebugOptions(uiVal, player);
    }
}
#endif

// ---------------------------------------------------------------------------
// Player skins / capes
// ---------------------------------------------------------------------------
wstring CMinecraftApp::GetPlayerSkinName(int iPad)
{
    return app.getSkinPathFromId(GameSettingsA[iPad]->dwSelectedSkin);
}

DWORD CMinecraftApp::GetPlayerSkinId(int iPad)
{
    DLCPack *Pack = NULL;
    DLCSkinFile *skinFile = NULL;
    DWORD dwSkin = GameSettingsA[iPad]->dwSelectedSkin;
    wchar_t chars[256];

    if (GET_IS_DLC_SKIN_FROM_BITMASK(dwSkin))
    {
        swprintf(chars, 256, L"dlcskin%08d.png", GET_DLC_SKIN_ID_FROM_BITMASK(dwSkin));
        Pack = app.m_dlcManager.getPackContainingSkin(chars);
        if (Pack)
        {
            skinFile = Pack->getSkinFile(chars);
            bool bSkinIsFree = skinFile->getParameterAsBool(DLCManager::e_DLCParamType_Free);
            bool bLicensed = Pack->hasPurchasedFile(DLCManager::e_DLCType_Skin, skinFile->getPath());
            return (bSkinIsFree || bLicensed) ? dwSkin : 0;
        }
    }

    return dwSkin;
}

wstring CMinecraftApp::GetPlayerCapeName(int iPad)
{
    return Player::getCapePathFromId(GameSettingsA[iPad]->dwSelectedCape);
}

DWORD CMinecraftApp::GetPlayerCapeId(int iPad)
{
    return GameSettingsA[iPad]->dwSelectedCape;
}

DWORD CMinecraftApp::getSkinIdFromPath(const wstring &skin)
{
    bool dlcSkin = false;
    unsigned int skinId = 0;

    if (skin.size() >= 14)
    {
        dlcSkin = skin.substr(0, 3).compare(L"dlc") == 0;

        wstring skinValue = skin.substr(7, skin.size());
        skinValue = skinValue.substr(0, skinValue.find_first_of(L'.'));

        std::wstringstream ss;
        if (dlcSkin)
        {
            ss << std::dec << skinValue.c_str();
        }
        else
        {
            ss << std::hex << skinValue.c_str();
        }
        ss >> skinId;

        skinId = MAKE_SKIN_BITMASK(dlcSkin, skinId);
    }
    return skinId;
}

wstring CMinecraftApp::getSkinPathFromId(DWORD skinId)
{
    wchar_t chars[256];
    if (GET_IS_DLC_SKIN_FROM_BITMASK(skinId))
    {
        swprintf(chars, 256, L"dlcskin%08d.png", GET_DLC_SKIN_ID_FROM_BITMASK(skinId));
    }
    else
    {
        DWORD ugcSkinIndex = GET_UGC_SKIN_ID_FROM_BITMASK(skinId);
        DWORD defaultSkinIndex = GET_DEFAULT_SKIN_ID_FROM_BITMASK(skinId);
        if (ugcSkinIndex == 0)
        {
            swprintf(chars, 256, L"defskin%08X.png", defaultSkinIndex);
        }
        else
        {
            swprintf(chars, 256, L"ugcskin%08X.png", ugcSkinIndex);
        }
    }
    return chars;
}

vector<ModelPart *> *CMinecraftApp::GetAdditionalModelParts(unsigned int dwSkinID)
{
    EnterCriticalSection(&csAdditionalModelParts);
    vector<ModelPart *> *pvModelParts = NULL;
    if (m_AdditionalModelParts.size() > 0)
    {
        AUTO_VAR(it, m_AdditionalModelParts.find(dwSkinID));
        if (it != m_AdditionalModelParts.end())
        {
            pvModelParts = (*it).second;
        }
    }
    LeaveCriticalSection(&csAdditionalModelParts);
    return pvModelParts;
}

vector<SKIN_BOX *> *CMinecraftApp::GetAdditionalSkinBoxes(unsigned int dwSkinID)
{
    EnterCriticalSection(&csAdditionalSkinBoxes);
    vector<SKIN_BOX *> *pvSkinBoxes = NULL;
    if (m_AdditionalSkinBoxes.size() > 0)
    {
        AUTO_VAR(it, m_AdditionalSkinBoxes.find(dwSkinID));
        if (it != m_AdditionalSkinBoxes.end())
        {
            pvSkinBoxes = (*it).second;
        }
    }
    LeaveCriticalSection(&csAdditionalSkinBoxes);
    return pvSkinBoxes;
}

void CMinecraftApp::SetAdditionalSkinBoxes(unsigned int dwSkinID, SKIN_BOX *SkinBoxA, unsigned int dwSkinBoxC)
{
    EntityRenderer *renderer = EntityRenderDispatcher::instance->getRenderer(eTYPE_PLAYER);
    Model *pModel = renderer->getModel();
    vector<ModelPart *> *pvModelPart = new vector<ModelPart *>;
    vector<SKIN_BOX *> *pvSkinBoxes = new vector<SKIN_BOX *>;

    EnterCriticalSection(&csAdditionalModelParts);
    EnterCriticalSection(&csAdditionalSkinBoxes);

    app.DebugPrintf("*** SetAdditionalSkinBoxes - Inserting model parts for skin %d from array of Skin Boxes\n", dwSkinID & 0x0FFFFFFF);

    for (unsigned int i = 0; i < dwSkinBoxC; i++)
    {
        if (pModel)
        {
            ModelPart *pModelPart = pModel->AddOrRetrievePart(&SkinBoxA[i]);
            pvModelPart->push_back(pModelPart);
            pvSkinBoxes->push_back(&SkinBoxA[i]);
        }
    }

    m_AdditionalModelParts.insert(std::pair<DWORD, vector<ModelPart *> *>(dwSkinID, pvModelPart));
    m_AdditionalSkinBoxes.insert(std::pair<DWORD, vector<SKIN_BOX *> *>(dwSkinID, pvSkinBoxes));

    LeaveCriticalSection(&csAdditionalSkinBoxes);
    LeaveCriticalSection(&csAdditionalModelParts);
}

vector<ModelPart *> *CMinecraftApp::SetAdditionalSkinBoxes(unsigned int dwSkinID, vector<SKIN_BOX *> *pvSkinBoxA)
{
    EntityRenderer *renderer = EntityRenderDispatcher::instance->getRenderer(eTYPE_PLAYER);
    Model *pModel = renderer->getModel();
    vector<ModelPart *> *pvModelPart = new vector<ModelPart *>;

    EnterCriticalSection(&csAdditionalModelParts);
    EnterCriticalSection(&csAdditionalSkinBoxes);
    app.DebugPrintf("*** SetAdditionalSkinBoxes - Inserting model parts for skin %d from array of Skin Boxes\n", dwSkinID & 0x0FFFFFFF);

    for (AUTO_VAR(it, pvSkinBoxA->begin()); it != pvSkinBoxA->end(); ++it)
    {
        if (pModel)
        {
            ModelPart *pModelPart = pModel->AddOrRetrievePart(*it);
            pvModelPart->push_back(pModelPart);
        }
    }

    m_AdditionalModelParts.insert(std::pair<DWORD, vector<ModelPart *> *>(dwSkinID, pvModelPart));
    m_AdditionalSkinBoxes.insert(std::pair<DWORD, vector<SKIN_BOX *> *>(dwSkinID, pvSkinBoxA));

    LeaveCriticalSection(&csAdditionalSkinBoxes);
    LeaveCriticalSection(&csAdditionalModelParts);
    return pvModelPart;
}

unsigned int CMinecraftApp::GetAnimOverrideBitmask(unsigned int dwSkinID)
{
    EnterCriticalSection(&csAnimOverrideBitmask);
    unsigned int uiAnimOverrideBitmask = 0L;
    if (m_AnimOverrides.size() > 0)
    {
        AUTO_VAR(it, m_AnimOverrides.find(dwSkinID));
        if (it != m_AnimOverrides.end())
        {
            uiAnimOverrideBitmask = (*it).second;
        }
    }
    LeaveCriticalSection(&csAnimOverrideBitmask);
    return uiAnimOverrideBitmask;
}

void CMinecraftApp::SetAnimOverrideBitmask(unsigned int dwSkinID, unsigned int uiAnimOverrideBitmask)
{
    EnterCriticalSection(&csAnimOverrideBitmask);
    if (m_AnimOverrides.size() > 0)
    {
        AUTO_VAR(it, m_AnimOverrides.find(dwSkinID));
        if (it != m_AnimOverrides.end())
        {
            LeaveCriticalSection(&csAnimOverrideBitmask);
            return;
        }
    }
    m_AnimOverrides.insert(std::pair<DWORD, unsigned long>(dwSkinID, uiAnimOverrideBitmask));
    LeaveCriticalSection(&csAnimOverrideBitmask);
}

// ---------------------------------------------------------------------------
// Memory textures (skin/cape PNGs held in RAM rather than on disk)
// ---------------------------------------------------------------------------
void CMinecraftApp::AddMemoryTextureFile(const wstring &wName, PBYTE pbData, DWORD dwBytes)
{
    EnterCriticalSection(&csMemFilesLock);
    PMEMDATA pData = NULL;
    AUTO_VAR(it, m_MEM_Files.find(wName));
    if (it != m_MEM_Files.end())
    {
        pData = (*it).second;
        if (pData->dwBytes == 0 && dwBytes != 0)
        {
            if (pData->pbData != NULL)
            {
                delete[] pData->pbData;
            }
            pData->pbData = pbData;
            pData->dwBytes = dwBytes;
        }
        ++pData->ucRefCount;
        LeaveCriticalSection(&csMemFilesLock);
        return;
    }

    pData = (PMEMDATA) new BYTE[sizeof(MEMDATA)];
    ZeroMemory(pData, sizeof(MEMDATA));
    pData->pbData = pbData;
    pData->dwBytes = dwBytes;
    pData->ucRefCount = 1;
    m_MEM_Files[wName] = pData;

    LeaveCriticalSection(&csMemFilesLock);
}

void CMinecraftApp::GetMemFileDetails(const wstring &wName, PBYTE *ppbData, DWORD *pdwBytes)
{
    EnterCriticalSection(&csMemFilesLock);
    AUTO_VAR(it, m_MEM_Files.find(wName));
    if (it != m_MEM_Files.end())
    {
        PMEMDATA pData = (*it).second;
        *ppbData = pData->pbData;
        *pdwBytes = pData->dwBytes;
    }
    LeaveCriticalSection(&csMemFilesLock);
}

bool CMinecraftApp::IsFileInMemoryTextures(const wstring &wName)
{
    bool val = false;
    EnterCriticalSection(&csMemFilesLock);
    AUTO_VAR(it, m_MEM_Files.find(wName));
    if (it != m_MEM_Files.end())
    {
        val = true;
    }
    LeaveCriticalSection(&csMemFilesLock);
    return val;
}

bool CMinecraftApp::DefaultCapeExists()
{
    return IsFileInMemoryTextures(L"Special_Cape.png");
}

// ---------------------------------------------------------------------------
// Media archive / string table / locale
// ---------------------------------------------------------------------------
void CMinecraftApp::loadMediaArchive()
{
    // Consoles_App.cpp only sets a media archive path for the console platforms
    // it targets (PS3/Windows64/Orbis/Durango/PSVita) - there was never an SDL3
    // branch. MediaWindows64.arc is the closest desktop analog checked into the
    // tree (same on-disk format, x64 desktop build) and is reused here rather
    // than inventing a new, non-existent asset name.
    wstring mediapath = L"Common\\Media\\MediaWindows64.arc";
    m_mediaArchive = new ArchiveFile(File(mediapath));
}

void CMinecraftApp::loadStringTable()
{
    if (m_stringTable != NULL)
    {
        delete m_stringTable;
    }
    wstring localisationFile = L"languages.loc";
    if (m_mediaArchive != NULL && m_mediaArchive->hasFile(localisationFile))
    {
        byteArray locFile = m_mediaArchive->getFile(localisationFile);
        m_stringTable = new StringTable(locFile.data, locFile.length);
        delete locFile.data;
    }
    else
    {
        m_stringTable = NULL;
    }
}

int CMinecraftApp::getArchiveFileSize(const wstring &filename)
{
    TexturePack *tPack = NULL;
    Minecraft *pMinecraft = Minecraft::GetInstance();
    if (pMinecraft && pMinecraft->skins)
    {
        tPack = pMinecraft->skins->getSelected();
    }
    if (tPack && tPack->hasData() && tPack->getArchiveFile() && tPack->getArchiveFile()->hasFile(filename))
    {
        return tPack->getArchiveFile()->getFileSize(filename);
    }
    return m_mediaArchive->getFileSize(filename);
}

bool CMinecraftApp::hasArchiveFile(const wstring &filename)
{
    TexturePack *tPack = NULL;
    Minecraft *pMinecraft = Minecraft::GetInstance();
    if (pMinecraft && pMinecraft->skins)
    {
        tPack = pMinecraft->skins->getSelected();
    }
    if (tPack && tPack->hasData() && tPack->getArchiveFile() && tPack->getArchiveFile()->hasFile(filename))
    {
        return true;
    }
    return m_mediaArchive->hasFile(filename);
}

byteArray CMinecraftApp::getArchiveFile(const wstring &filename)
{
    TexturePack *tPack = NULL;
    Minecraft *pMinecraft = Minecraft::GetInstance();
    if (pMinecraft && pMinecraft->skins)
    {
        tPack = pMinecraft->skins->getSelected();
    }
    if (tPack && tPack->hasData() && tPack->getArchiveFile() && tPack->getArchiveFile()->hasFile(filename))
    {
        return tPack->getArchiveFile()->getFile(filename);
    }
    return m_mediaArchive->getFile(filename);
}

wstring CMinecraftApp::getRootPath(DWORD packId, bool allowOverride, bool bAddDataFolder)
{
    // Non-Xbox platforms just resolve straight through to the current
    // directory / DLC folder layout; the TU-data override path is Xbox-only.
    return L"";
}

wstring CMinecraftApp::getFilePath(DWORD packId, wstring filename, bool bAddDataFolder)
{
    return getRootPath(packId, false, true) + filename;
}

// LocaleAndLanguageInit populates the locale-tag lookup table (m_localeA) that
// getLocale() below needs. The original also builds m_xcLangA, a table of
// Xbox XC_LOCALE_* enum values - those constants don't exist off Xbox and
// nothing in the SDL3 port calls get_xcLang(), so that half of the table is
// intentionally omitted.
void CMinecraftApp::LocaleAndLanguageInit()
{
    m_localeA[eMCLang_null] = L"en-EN";
    m_localeA[eMCLang_enUS] = L"en-US";
    m_localeA[eMCLang_enGB] = L"en-GB";
    m_localeA[eMCLang_enIE] = L"en-IE";
    m_localeA[eMCLang_enAU] = L"en-AU";
    m_localeA[eMCLang_enNZ] = L"en-NZ";
    m_localeA[eMCLang_enCA] = L"en-CA";
    m_localeA[eMCLang_jaJP] = L"ja-JP";
    m_localeA[eMCLang_deDE] = L"de-DE";
    m_localeA[eMCLang_deAT] = L"de-AT";
    m_localeA[eMCLang_frFR] = L"fr-FR";
    m_localeA[eMCLang_frCA] = L"fr-CA";
    m_localeA[eMCLang_esES] = L"es-ES";
    m_localeA[eMCLang_esMX] = L"es-MX";
    m_localeA[eMCLang_itIT] = L"it-IT";
    m_localeA[eMCLang_koKR] = L"ko-KR";
    m_localeA[eMCLang_ptPT] = L"pt-PT";
    m_localeA[eMCLang_ptBR] = L"pt-BR";
    m_localeA[eMCLang_ruRU] = L"ru-RU";
    m_localeA[eMCLang_nlNL] = L"nl-NL";
    m_localeA[eMCLang_fiFI] = L"fi-FI";
    m_localeA[eMCLang_plPL] = L"pl-PL";
    m_localeA[eMCLang_trTR] = L"tr-TR";

    m_eMCLangA[L"en-US"] = eMCLang_enUS;
    m_eMCLangA[L"en-GB"] = eMCLang_enGB;
    m_eMCLangA[L"en-IE"] = eMCLang_enIE;
    m_eMCLangA[L"en-AU"] = eMCLang_enAU;
    m_eMCLangA[L"en-NZ"] = eMCLang_enNZ;
    m_eMCLangA[L"en-CA"] = eMCLang_enCA;
    m_eMCLangA[L"ja-JP"] = eMCLang_jaJP;
    m_eMCLangA[L"de-DE"] = eMCLang_deDE;
    m_eMCLangA[L"fr-FR"] = eMCLang_frFR;
    m_eMCLangA[L"es-ES"] = eMCLang_esES;
    m_eMCLangA[L"it-IT"] = eMCLang_itIT;
    m_eMCLangA[L"ko-KR"] = eMCLang_koKR;
    m_eMCLangA[L"pt-PT"] = eMCLang_ptPT;
    m_eMCLangA[L"pt-BR"] = eMCLang_ptBR;
    m_eMCLangA[L"ru-RU"] = eMCLang_ruRU;
    m_eMCLangA[L"nl-NL"] = eMCLang_nlNL;
    m_eMCLangA[L"pl-PL"] = eMCLang_plPL;
    m_eMCLangA[L"tr-TR"] = eMCLang_trTR;
}

// getLocale on consoles walks XGetLanguage()/XGetLocale() against a huge
// Xbox-locale switch, then - in every case, matched or not - unconditionally
// appends eMCLang_enUS and eMCLang_null ("en-EN", the archive's actual
// first/default language entry - see languages.loc) as universal fallbacks
// (see the tail of CMinecraftApp::getLocale() in Consoles_App.cpp). Those
// APIs don't exist here, so this queries SDL's notion of the user's
// preferred locale instead and maps the top hit through m_eMCLangA, but
// still needs the same unconditional en-US/en-EN fallbacks appended
// afterwards: StringTable::StringTable() (see StringTable.cpp) walks this
// list looking for a language actually present in languages.loc and, unlike
// the caller here, has no further fallback of its own if every entry
// misses.
void CMinecraftApp::getLocale(vector<wstring> &vecWstrLocales)
{
    const char *lang = getenv("LANG");
    if (lang && lang[0])
    {
        char langBuf[32] = {};
        snprintf(langBuf, sizeof(langBuf), "%s", lang);
        char *dot = strchr(langBuf, '.');
        if (dot)
        {
            *dot = '\0';
        }
        char *underscore = strchr(langBuf, '_');
        if (underscore)
        {
            *underscore = '-';
        }
        wchar_t tag[32] = {};
        swprintf(tag, 32, L"%hs", langBuf);
        AUTO_VAR(it, m_eMCLangA.find(tag));
        if (it != m_eMCLangA.end())
        {
            vecWstrLocales.push_back(m_localeA[(eMCLang)it->second]);
        }
    }

    vecWstrLocales.push_back(m_localeA[eMCLang_enUS]);
    vecWstrLocales.push_back(m_localeA[eMCLang_null]);
}

// ---------------------------------------------------------------------------
// Xuid special cases / Mojang data
// ---------------------------------------------------------------------------
bool CMinecraftApp::isXuidNotch(PlayerUID xuid)
{
    if (m_xuidNotch != INVALID_XUID && xuid != INVALID_XUID)
    {
        return ProfileManager.AreXUIDSEqual(xuid, m_xuidNotch) == TRUE;
    }
    return false;
}

bool CMinecraftApp::isXuidDeadmau5(PlayerUID xuid)
{
    AUTO_VAR(it, MojangData.find(xuid));
    if (it != MojangData.end())
    {
        MOJANG_DATA *pMojangData = MojangData[xuid];
        if (pMojangData && pMojangData->eXuid == eXUID_Deadmau5)
        {
            return true;
        }
    }
    return false;
}

MOJANG_DATA *CMinecraftApp::GetMojangDataForXuid(PlayerUID xuid)
{
    return MojangData[xuid];
}

bool CMinecraftApp::GetDLCFullOfferIDForSkinID(const wstring &FirstSkin, ULONGLONG *pullVal)
{
    AUTO_VAR(it, DLCInfo_SkinName.find(FirstSkin));
    if (it == DLCInfo_SkinName.end())
    {
        return false;
    }
    *pullVal = (ULONGLONG)it->second;
    return true;
}

// ---------------------------------------------------------------------------
// Invites
// ---------------------------------------------------------------------------
void CMinecraftApp::ProcessInvite(DWORD dwUserIndex, DWORD dwLocalUsersMask, const INVITE_INFO *pInviteInfo)
{
    m_InviteData.dwUserIndex = dwUserIndex;
    m_InviteData.dwLocalUsersMask = dwLocalUsersMask;
    m_InviteData.pInviteInfo = pInviteInfo;
    SetAction(dwUserIndex, eAppAction_ExitAndJoinFromInvite);
}

// ---------------------------------------------------------------------------
// Save notifications / autosave
// ---------------------------------------------------------------------------
void CMinecraftApp::EnterSaveNotificationSection()
{
    EnterCriticalSection(&m_saveNotificationCriticalSection);
    if (m_saveNotificationDepth++ == 0)
    {
        MinecraftServer::getInstance()->broadcastStartSavingPacket();
        if (g_NetworkManager.IsLocalGame() && g_NetworkManager.GetPlayerCount() == 1)
        {
            app.SetXuiServerAction(ProfileManager.GetPrimaryPad(), eXuiServerAction_PauseServer, (void *)TRUE);
        }
    }
    LeaveCriticalSection(&m_saveNotificationCriticalSection);
}

void CMinecraftApp::LeaveSaveNotificationSection()
{
    EnterCriticalSection(&m_saveNotificationCriticalSection);
    if (--m_saveNotificationDepth == 0)
    {
        MinecraftServer::getInstance()->broadcastStopSavingPacket();
        if (g_NetworkManager.IsLocalGame() && g_NetworkManager.GetPlayerCount() == 1)
        {
            app.SetXuiServerAction(ProfileManager.GetPrimaryPad(), eXuiServerAction_PauseServer, (void *)FALSE);
        }
    }
    LeaveCriticalSection(&m_saveNotificationCriticalSection);
}

void CMinecraftApp::SetAutosaveTimerTime(void)
{
    m_uiAutosaveTimer = GetTickCount() + GetGameSettings(ProfileManager.GetPrimaryPad(), eGameSetting_Autosave) * 1000 * 60 * 15;
}

bool CMinecraftApp::AutosaveDue(void)
{
    return GetTickCount() > m_uiAutosaveTimer;
}

unsigned int CMinecraftApp::SecondsToAutosave()
{
    return (m_uiAutosaveTimer - GetTickCount()) / 1000;
}

void CMinecraftApp::SetTrialTimerStart(void)
{
    m_fTrialTimerStart = m_Time.fAppTime;
    mfTrialPausedTime = 0.0f;
}

void CMinecraftApp::UpdateTime()
{
    LARGE_INTEGER qwNewTime;
    LARGE_INTEGER qwDeltaTime;

    QueryPerformanceCounter(&qwNewTime);
    qwDeltaTime.QuadPart = qwNewTime.QuadPart - m_Time.qwTime.QuadPart;

    m_Time.qwAppTime.QuadPart += qwDeltaTime.QuadPart;
    m_Time.qwTime.QuadPart = qwNewTime.QuadPart;

    m_Time.fElapsedTime = m_Time.fSecsPerTick * ((FLOAT)(qwDeltaTime.QuadPart));
    m_Time.fAppTime = m_Time.fSecsPerTick * ((FLOAT)(m_Time.qwAppTime.QuadPart));
}

// ---------------------------------------------------------------------------
// Tips
// ---------------------------------------------------------------------------
void CMinecraftApp::InitialiseTips()
{
    ZeroMemory(m_TipIDA, sizeof(UINT) * MAX_TIPS_GAMETIP + MAX_TIPS_TRIVIATIP);

    if (!RenderManager.IsHiDef())
    {
        m_GameTipA[0].uiStringID = IDS_TIPS_GAMETIP_0;
    }

    for (int i = 0; i < MAX_TIPS_TRIVIATIP; i++)
    {
        m_TriviaTipA[i].iSortValue = TipRandom->nextInt();
    }
    qsort(m_TriviaTipA, MAX_TIPS_TRIVIATIP, sizeof(TIPSTRUCT), TipsSortFunction);

    int iCurrentGameTip = 0;
    int iCurrentTriviaTip = 0;

    for (int i = 0; i < MAX_TIPS_GAMETIP + MAX_TIPS_TRIVIATIP; i++)
    {
        if ((i % 3 == 2) && (iCurrentTriviaTip < MAX_TIPS_TRIVIATIP))
        {
            m_TipIDA[i] = m_TriviaTipA[iCurrentTriviaTip++].uiStringID;
        }
        else if (iCurrentGameTip < MAX_TIPS_GAMETIP)
        {
            m_TipIDA[i] = m_GameTipA[iCurrentGameTip++].uiStringID;
        }
        else if (iCurrentTriviaTip < MAX_TIPS_TRIVIATIP)
        {
            m_TipIDA[i] = m_TriviaTipA[iCurrentTriviaTip++].uiStringID;
        }
    }

    m_uiCurrentTip = 0;
}

// ---------------------------------------------------------------------------
// HTML colour / entity name
// ---------------------------------------------------------------------------
int CMinecraftApp::GetHTMLColour(eMinecraftColour colour)
{
    Minecraft *pMinecraft = Minecraft::GetInstance();
    return pMinecraft->skins->getSelected()->getColourTable()->getColour(colour);
}

wstring CMinecraftApp::getEntityName(eINSTANCEOF type)
{
    switch (type)
    {
    case eTYPE_WOLF:
        return app.GetString(IDS_WOLF);
    case eTYPE_CREEPER:
        return app.GetString(IDS_CREEPER);
    case eTYPE_SKELETON:
        return app.GetString(IDS_SKELETON);
    case eTYPE_SPIDER:
        return app.GetString(IDS_SPIDER);
    case eTYPE_ZOMBIE:
        return app.GetString(IDS_ZOMBIE);
    case eTYPE_PIGZOMBIE:
        return app.GetString(IDS_PIGZOMBIE);
    case eTYPE_ENDERMAN:
        return app.GetString(IDS_ENDERMAN);
    case eTYPE_SILVERFISH:
        return app.GetString(IDS_SILVERFISH);
    case eTYPE_CAVESPIDER:
        return app.GetString(IDS_CAVE_SPIDER);
    case eTYPE_GHAST:
        return app.GetString(IDS_GHAST);
    case eTYPE_SLIME:
        return app.GetString(IDS_SLIME);
    case eTYPE_ARROW:
        return app.GetString(IDS_ITEM_ARROW);
    case eTYPE_ENDERDRAGON:
        return app.GetString(IDS_ENDERDRAGON);
    case eTYPE_BLAZE:
        return app.GetString(IDS_BLAZE);
    case eTYPE_LAVASLIME:
        return app.GetString(IDS_LAVA_SLIME);
    case eTYPE_VILLAGERGOLEM:
        return app.GetString(IDS_IRONGOLEM);
    default:
        break;
    };
    return L"";
}

// ---------------------------------------------------------------------------
// DLC credits
// ---------------------------------------------------------------------------
void CMinecraftApp::AddCreditText(LPCWSTR lpStr)
{
    DebugPrintf("ADDING CREDIT - %ls\n", lpStr);
    SCreditTextItemDef *pCreditStruct = new SCreditTextItemDef;
    pCreditStruct->m_eType = eSmallText;
    pCreditStruct->m_iStringID[0] = NO_TRANSLATED_STRING;
    pCreditStruct->m_iStringID[1] = NO_TRANSLATED_STRING;
    pCreditStruct->m_Text = new WCHAR[wcslen(lpStr) + 1];
    wcscpy((WCHAR *)pCreditStruct->m_Text, lpStr);
    vDLCCredits.push_back(pCreditStruct);
}

bool CMinecraftApp::AlreadySeenCreditText(const wstring &wstemp)
{
    for (unsigned int i = 0; i < m_vCreditText.size(); i++)
    {
        if (m_vCreditText.at(i).compare(wstemp) == 0)
        {
            return true;
        }
    }
    m_vCreditText.push_back((WCHAR *)wstemp.c_str());
    return false;
}

// ---------------------------------------------------------------------------
// Game host options
// ---------------------------------------------------------------------------
void CMinecraftApp::SetGameHostOption(eGameHostOption eVal, unsigned int uiVal)
{
    SetGameHostOption(m_uiGameHostSettings, eVal, uiVal);
}

void CMinecraftApp::SetGameHostOption(unsigned int &uiHostSettings, eGameHostOption eVal, unsigned int uiVal)
{
    switch (eVal)
    {
    case eGameHostOption_FriendsOfFriends:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS;
        break;
    case eGameHostOption_Difficulty:
        uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_DIFFICULTY;
        uiHostSettings |= (GAME_HOST_OPTION_BITMASK_DIFFICULTY & uiVal);
        break;
    case eGameHostOption_Gamertags:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_GAMERTAGS : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_GAMERTAGS;
        break;
    case eGameHostOption_GameType:
        uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_GAMETYPE;
        uiHostSettings |= (GAME_HOST_OPTION_BITMASK_GAMETYPE & (uiVal << 4));
        break;
    case eGameHostOption_LevelType:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_LEVELTYPE : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_LEVELTYPE;
        break;
    case eGameHostOption_Structures:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_STRUCTURES : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_STRUCTURES;
        break;
    case eGameHostOption_BonusChest:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_BONUSCHEST : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_BONUSCHEST;
        break;
    case eGameHostOption_HasBeenInCreative:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_BEENINCREATIVE : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_BEENINCREATIVE;
        break;
    case eGameHostOption_PvP:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_PVP : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_PVP;
        break;
    case eGameHostOption_TrustPlayers:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS;
        break;
    case eGameHostOption_TNT:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_TNT : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_TNT;
        break;
    case eGameHostOption_FireSpreads:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_FIRESPREADS : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_FIRESPREADS;
        break;
    case eGameHostOption_CheatsEnabled:
        if (uiVal != 0)
        {
            uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTFLY | GAME_HOST_OPTION_BITMASK_HOSTHUNGER | GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
        }
        else
        {
            uiHostSettings &= ~(GAME_HOST_OPTION_BITMASK_HOSTFLY | GAME_HOST_OPTION_BITMASK_HOSTHUNGER | GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE);
        }
        break;
    case eGameHostOption_HostCanFly:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTFLY : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTFLY;
        break;
    case eGameHostOption_HostCanChangeHunger:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTHUNGER : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTHUNGER;
        break;
    case eGameHostOption_HostCanBeInvisible:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
        break;
    case eGameHostOption_BedrockFog:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_BEDROCKFOG : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_BEDROCKFOG;
        break;
    case eGameHostOption_DisableSaving:
        uiVal != 0 ? uiHostSettings |= GAME_HOST_OPTION_BITMASK_DISABLESAVE : uiHostSettings &= ~GAME_HOST_OPTION_BITMASK_DISABLESAVE;
        break;
    case eGameHostOption_All:
        uiHostSettings = uiVal;
        break;
    }
}

unsigned int CMinecraftApp::GetGameHostOption(eGameHostOption eVal)
{
    return GetGameHostOption(m_uiGameHostSettings, eVal);
}

unsigned int CMinecraftApp::GetGameHostOption(unsigned int uiHostSettings, eGameHostOption eVal)
{
    switch (eVal)
    {
    case eGameHostOption_FriendsOfFriends:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_FRIENDSOFFRIENDS;
    case eGameHostOption_Difficulty:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_DIFFICULTY;
    case eGameHostOption_Gamertags:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_GAMERTAGS;
    case eGameHostOption_GameType:
        return (uiHostSettings & GAME_HOST_OPTION_BITMASK_GAMETYPE) >> 4;
    case eGameHostOption_All:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_ALL;
    case eGameHostOption_Tutorial:
        return (uiHostSettings & GAME_HOST_OPTION_BITMASK_GAMERTAGS) | GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS | GAME_HOST_OPTION_BITMASK_FIRESPREADS | GAME_HOST_OPTION_BITMASK_TNT | GAME_HOST_OPTION_BITMASK_PVP | GAME_HOST_OPTION_BITMASK_STRUCTURES | 1;
    case eGameHostOption_LevelType:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_LEVELTYPE;
    case eGameHostOption_Structures:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_STRUCTURES;
    case eGameHostOption_BonusChest:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_BONUSCHEST;
    case eGameHostOption_HasBeenInCreative:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_BEENINCREATIVE;
    case eGameHostOption_PvP:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_PVP;
    case eGameHostOption_TrustPlayers:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_TRUSTPLAYERS;
    case eGameHostOption_TNT:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_TNT;
    case eGameHostOption_FireSpreads:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_FIRESPREADS;
    case eGameHostOption_CheatsEnabled:
        return uiHostSettings & (GAME_HOST_OPTION_BITMASK_HOSTFLY | GAME_HOST_OPTION_BITMASK_HOSTHUNGER | GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE);
    case eGameHostOption_HostCanFly:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_HOSTFLY;
    case eGameHostOption_HostCanChangeHunger:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_HOSTHUNGER;
    case eGameHostOption_HostCanBeInvisible:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_HOSTINVISIBLE;
    case eGameHostOption_BedrockFog:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_BEDROCKFOG;
    case eGameHostOption_DisableSaving:
        return uiHostSettings & GAME_HOST_OPTION_BITMASK_DISABLESAVE;
    }
    return 0;
}

bool CMinecraftApp::CanRecordStatsAndAchievements()
{
    return !(app.GetGameHostOption(eGameHostOption_HasBeenInCreative) ||
             app.GetGameHostOption(eGameHostOption_HostCanBeInvisible) ||
             app.GetGameHostOption(eGameHostOption_HostCanChangeHunger) ||
             app.GetGameHostOption(eGameHostOption_HostCanFly));
}

// ---------------------------------------------------------------------------
// World seed PNG metadata
// ---------------------------------------------------------------------------
static unsigned int FromBigEndian_Impl(unsigned int uiValue)
{
    return ((uiValue >> 24) & 0x000000ff) | ((uiValue >> 8) & 0x0000ff00) | ((uiValue << 8) & 0x00ff0000) | ((uiValue << 24) & 0xff000000);
}

unsigned int CMinecraftApp::CreateImageTextData(PBYTE bTextMetadata, std::int64_t seed, bool hasSeed, unsigned int uiHostOptions, unsigned int uiTexturePackId)
{
    int iTextMetadataBytes = 0;
    if (hasSeed)
    {
        strcpy((char *)bTextMetadata, "4J_SEED");
        snprintf((char *)&bTextMetadata[8], 42, "%lld", (long long)seed);

        iTextMetadataBytes += 8;
        while (bTextMetadata[iTextMetadataBytes] != 0)
        {
            iTextMetadataBytes++;
        }
        ++iTextMetadataBytes;
    }

    strcpy((char *)&bTextMetadata[iTextMetadataBytes], "4J_HOSTOPTIONS");
    snprintf((char *)&bTextMetadata[iTextMetadataBytes + 15], 9, "%x", uiHostOptions);

    iTextMetadataBytes += 15;
    while (bTextMetadata[iTextMetadataBytes] != 0)
    {
        iTextMetadataBytes++;
    }
    ++iTextMetadataBytes;

    strcpy((char *)&bTextMetadata[iTextMetadataBytes], "4J_TEXTUREPACK");
    snprintf((char *)&bTextMetadata[iTextMetadataBytes + 15], 9, "%x", uiTexturePackId);

    iTextMetadataBytes += 15;
    while (bTextMetadata[iTextMetadataBytes] != 0)
    {
        iTextMetadataBytes++;
    }

    return iTextMetadataBytes;
}

// ---------------------------------------------------------------------------
// Terrain feature positions
// ---------------------------------------------------------------------------
void CMinecraftApp::AddTerrainFeaturePosition(_eTerrainFeatureType eFeatureType, int x, int z)
{
    for (AUTO_VAR(it, m_vTerrainFeatures.begin()); it < m_vTerrainFeatures.end(); ++it)
    {
        FEATURE_DATA *pFeatureData = *it;
        if ((pFeatureData->eTerrainFeature == eFeatureType) && (pFeatureData->x == x) && (pFeatureData->z == z))
        {
            return;
        }
    }

    FEATURE_DATA *pFeatureData = new FEATURE_DATA;
    pFeatureData->eTerrainFeature = eFeatureType;
    pFeatureData->x = x;
    pFeatureData->z = z;
    m_vTerrainFeatures.push_back(pFeatureData);
}

bool CMinecraftApp::GetTerrainFeaturePosition(_eTerrainFeatureType eType, int *pX, int *pZ)
{
    for (AUTO_VAR(it, m_vTerrainFeatures.begin()); it < m_vTerrainFeatures.end(); ++it)
    {
        FEATURE_DATA *pFeatureData = *it;
        if (pFeatureData->eTerrainFeature == eType)
        {
            *pX = pFeatureData->x;
            *pZ = pFeatureData->z;
            return true;
        }
    }
    return false;
}

void CMinecraftApp::ClearTerrainFeaturePosition()
{
    while (m_vTerrainFeatures.size() > 0)
    {
        FEATURE_DATA *pFeatureData = m_vTerrainFeatures.back();
        m_vTerrainFeatures.pop_back();
        delete pFeatureData;
    }
}

// ---------------------------------------------------------------------------
// Player privileges / colours
// ---------------------------------------------------------------------------
void CMinecraftApp::UpdatePlayerInfo(BYTE networkSmallId, SHORT playerColourIndex, unsigned int playerGamePrivileges)
{
    for (unsigned int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i)
    {
        if (m_playerColours[i] == networkSmallId)
        {
            m_playerColours[i] = 0;
            m_playerGamePrivileges[i] = 0;
        }
    }
    if (playerColourIndex >= 0 && playerColourIndex < MINECRAFT_NET_MAX_PLAYERS)
    {
        m_playerColours[playerColourIndex] = networkSmallId;
        m_playerGamePrivileges[playerColourIndex] = playerGamePrivileges;
    }
}

unsigned int CMinecraftApp::GetPlayerPrivileges(BYTE networkSmallId)
{
    unsigned int privileges = 0;
    for (unsigned int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i)
    {
        if (m_playerColours[i] == networkSmallId)
        {
            privileges = m_playerGamePrivileges[i];
            break;
        }
    }
    return privileges;
}

// ---------------------------------------------------------------------------
// Game rules / schematics (delegate straight through to the already-ported
// GameRuleManager)
// ---------------------------------------------------------------------------
void CMinecraftApp::processSchematics(LevelChunk *levelChunk)
{
    m_gameRules.processSchematics(levelChunk);
}

void CMinecraftApp::processSchematicsLighting(LevelChunk *levelChunk)
{
    m_gameRules.processSchematicsLighting(levelChunk);
}

void CMinecraftApp::loadDefaultGameRules()
{
    m_gameRules.loadDefaultGameRules();
}

void CMinecraftApp::setLevelGenerationOptions(LevelGenerationOptions *levelGen)
{
    m_gameRules.setLevelGenerationOptions(levelGen);
}

LPCWSTR CMinecraftApp::GetGameRulesString(const wstring &key)
{
    return m_gameRules.GetGameRulesString(key);
}

// ---------------------------------------------------------------------------
// Local multiplayer availability
// ---------------------------------------------------------------------------
bool CMinecraftApp::IsLocalMultiplayerAvailable()
{
    DWORD connectedControllers = 0;
    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
    {
        if (InputManager.IsPadConnected(i) || ProfileManager.IsSignedIn(i))
        {
            ++connectedControllers;
        }
    }
    return RenderManager.IsHiDef() && connectedControllers > 1;
}

// ---------------------------------------------------------------------------
// DLC install
// ---------------------------------------------------------------------------
bool CMinecraftApp::StartInstallDLCProcess(int iPad)
{
    app.DebugPrintf("--- CMinecraftApp::StartInstallDLCProcess: pad=%i.\n", iPad);

    if ((app.DLCInstallProcessCompleted() == false) && (m_bDLCInstallPending == false))
    {
        app.m_dlcManager.resetUnnamedCorruptCount();
        m_bDLCInstallPending = true;
        m_iTotalDLC = 0;
        m_iTotalDLCInstalled = 0;
        // Console builds kick off StorageManager.GetInstalledDLC(...) here with a
        // callback that walks the platform DLC store; there is no equivalent
        // storefront on this platform yet, so installed DLC discovery is left to
        // whatever populates m_dlcManager directly (see Common/DLC).
        m_bDLCInstallPending = false;
        m_bDLCInstallProcessCompleted = true;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Banned-list unique map name
// ---------------------------------------------------------------------------
void CMinecraftApp::SetUniqueMapName(char *pszUniqueMapName)
{
    memcpy(m_pszUniqueMapName, pszUniqueMapName, 14);
}
