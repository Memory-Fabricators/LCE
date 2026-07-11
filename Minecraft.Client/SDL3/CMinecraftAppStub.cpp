#include "../Common/Consoles_App.h"
#include "../stdafx.h"

// TODO: placeholder implementation, not a real port.
//
// Common/Consoles_App.cpp implements the entire CMinecraftApp class (game
// settings, DLC/skin bookkeeping, locale, credit text, menu loading, etc.)
// but unconditionally includes Common/UI/UI.h (Iggy) throughout, so it's
// excluded from the SDL3 build (see Minecraft.Client/CMakeLists.txt). Every
// method below is a no-op/trivial-return stand-in purely so the executable
// links - none of this is real game behavior yet. A real port needs to
// separate the non-UI logic (most of what's here) from the handful of
// genuinely Iggy-specific bits and reimplement it for real.

CMinecraftApp::CMinecraftApp()
{
}

wstring CMinecraftApp::getEntityName(eINSTANCEOF type)
{
    return L"";
}

void CMinecraftApp::SetSpecialTutorialCompletionFlag(int iPad, int index)
{
}

LPCWSTR CMinecraftApp::GetGameRulesString(const wstring &key)
{
    return L"";
}

void CMinecraftApp::UpdatePlayerInfo(BYTE networkSmallId, SHORT playerColourIndex, unsigned int playerGamePrivileges)
{
}

unsigned int CMinecraftApp::GetPlayerPrivileges(BYTE networkSmallId)
{
    return 0;
}

void CMinecraftApp::HandleButtonPresses()
{
}

void CMinecraftApp::HandleButtonPresses(int iPad)
{
}

void CMinecraftApp::DebugPrintf(const char *szFormat, ...)
{
}

void CMinecraftApp::DebugPrintfVerbose(bool bVerbose, const char *szFormat, ...)
{
}

void CMinecraftApp::DebugPrintf(int user, const char *szFormat, ...)
{
}

bool CMinecraftApp::IsAppPaused()
{
    return false;
}

int CMinecraftApp::GetLocalPlayerCount(void)
{
    return 1;
}

LPCWSTR CMinecraftApp::GetString(int iID)
{
    return L"";
}

bool CMinecraftApp::LoadInventoryMenu(int iPad, shared_ptr<LocalPlayer> player, bool bNavigateBack)
{
    return false;
}

bool CMinecraftApp::LoadCreativeMenu(int iPad, shared_ptr<LocalPlayer> player, bool bNavigateBack)
{
    return false;
}

bool CMinecraftApp::LoadEnchantingMenu(int iPad, shared_ptr<Inventory> inventory, int x, int y, int z, Level *level)
{
    return false;
}

bool CMinecraftApp::LoadFurnaceMenu(int iPad, shared_ptr<Inventory> inventory, shared_ptr<FurnaceTileEntity> furnace)
{
    return false;
}

bool CMinecraftApp::LoadBrewingStandMenu(int iPad, shared_ptr<Inventory> inventory, shared_ptr<BrewingStandTileEntity> brewingStand)
{
    return false;
}

bool CMinecraftApp::LoadContainerMenu(int iPad, shared_ptr<Container> inventory, shared_ptr<Container> container)
{
    return false;
}

bool CMinecraftApp::LoadTrapMenu(int iPad, shared_ptr<Container> inventory, shared_ptr<DispenserTileEntity> trap)
{
    return false;
}

bool CMinecraftApp::LoadCrafting2x2Menu(int iPad, shared_ptr<LocalPlayer> player)
{
    return false;
}

bool CMinecraftApp::LoadCrafting3x3Menu(int iPad, shared_ptr<LocalPlayer> player, int x, int y, int z)
{
    return false;
}

bool CMinecraftApp::LoadSignEntryMenu(int iPad, shared_ptr<SignTileEntity> sign)
{
    return false;
}

bool CMinecraftApp::LoadRepairingMenu(int iPad, shared_ptr<Inventory> inventory, Level *level, int x, int y, int z)
{
    return false;
}

bool CMinecraftApp::LoadTradingMenu(int iPad, shared_ptr<Inventory> inventory, shared_ptr<Merchant> trader, Level *level)
{
    return false;
}

void CMinecraftApp::SetAction(int iPad, eXuiAction action, LPVOID param)
{
}

unsigned char CMinecraftApp::GetGameSettings(int iPad, eGameSetting eVal)
{
    return 0;
}

unsigned char CMinecraftApp::GetGameSettings(eGameSetting eVal)
{
    return 0;
}

wstring CMinecraftApp::GetPlayerSkinName(int iPad)
{
    return L"";
}

wstring CMinecraftApp::GetPlayerCapeName(int iPad)
{
    return L"";
}

DWORD CMinecraftApp::GetPlayerSkinId(int iPad)
{
    return 0;
}

DWORD CMinecraftApp::GetPlayerCapeId(int iPad)
{
    return 0;
}

DWORD CMinecraftApp::GetAdditionalModelParts(int iPad)
{
    return 0;
}

void CMinecraftApp::ApplyGameSettingsChanged(int iPad)
{
}

unsigned int CMinecraftApp::GetGameSettingsDebugMask(int iPad, bool bOverridePlayer)
{
    return 0;
}

void CMinecraftApp::SetGameSettingsDebugMask(int iPad, unsigned int uiVal)
{
}

bool CMinecraftApp::IsLocalMultiplayerAvailable()
{
    return false;
}

bool CMinecraftApp::StartInstallDLCProcess(int iPad)
{
    return false;
}

void CMinecraftApp::ProcessInvite(DWORD dwUserIndex, DWORD dwLocalUsersMask, const INVITE_INFO *pInviteInfo)
{
}

bool CMinecraftApp::AlreadySeenCreditText(const wstring &wstemp)
{
    return false;
}

bool CMinecraftApp::isXuidNotch(PlayerUID xuid)
{
    return false;
}

bool CMinecraftApp::isXuidDeadmau5(PlayerUID xuid)
{
    return false;
}

void CMinecraftApp::AddMemoryTextureFile(const wstring &wName, PBYTE pbData, DWORD dwBytes)
{
}

void CMinecraftApp::GetMemFileDetails(const wstring &wName, PBYTE *ppbData, DWORD *pdwBytes)
{
    if (ppbData)
    {
        *ppbData = NULL;
    }
    if (pdwBytes)
    {
        *pdwBytes = 0;
    }
}

bool CMinecraftApp::IsFileInMemoryTextures(const wstring &wName)
{
    return false;
}

bool CMinecraftApp::DefaultCapeExists()
{
    return false;
}

void CMinecraftApp::AddCreditText(LPCWSTR lpStr)
{
}

void CMinecraftApp::SetUniqueMapName(char *pszUniqueMapName)
{
}

void CMinecraftApp::SetTrialTimerStart(void)
{
}

int CMinecraftApp::getArchiveFileSize(const wstring &filename)
{
    return 0;
}

bool CMinecraftApp::hasArchiveFile(const wstring &filename)
{
    return false;
}

byteArray CMinecraftApp::getArchiveFile(const wstring &filename)
{
    return byteArray();
}

int CMinecraftApp::GetHTMLColour(eMinecraftColour colour)
{
    return 0xFFFFFF;
}

void CMinecraftApp::AddTerrainFeaturePosition(_eTerrainFeatureType, int, int)
{
}

void CMinecraftApp::ClearTerrainFeaturePosition()
{
}

bool CMinecraftApp::GetTerrainFeaturePosition(_eTerrainFeatureType eType, int *pX, int *pZ)
{
    return false;
}

MOJANG_DATA *CMinecraftApp::GetMojangDataForXuid(PlayerUID xuid)
{
    return NULL;
}

bool CMinecraftApp::GetDLCFullOfferIDForSkinID(const wstring &FirstSkin, ULONGLONG *pullVal)
{
    if (pullVal)
    {
        *pullVal = 0;
    }
    return false;
}

void CMinecraftApp::SetAutosaveTimerTime(void)
{
}

unsigned int CMinecraftApp::SecondsToAutosave()
{
    return 0;
}

bool CMinecraftApp::AutosaveDue(void)
{
    return false;
}

unsigned int CMinecraftApp::GetGameHostOption(eGameHostOption eVal)
{
    return 0;
}

unsigned int CMinecraftApp::GetGameHostOption(unsigned int uiHostSettings, eGameHostOption eVal)
{
    return 0;
}

void CMinecraftApp::SetGameHostOption(eGameHostOption eVal, unsigned int uiVal)
{
}

void CMinecraftApp::SetGameHostOption(unsigned int &uiHostSettings, eGameHostOption eVal, unsigned int uiVal)
{
}

bool CMinecraftApp::CanRecordStatsAndAchievements()
{
    return false;
}

unsigned int CMinecraftApp::CreateImageTextData(PBYTE bTextMetadata, __int64 seed, bool hasSeed, unsigned int uiHostOptions, unsigned int uiTexturePackId)
{
    return 0;
}

void CMinecraftApp::processSchematics(LevelChunk *levelChunk)
{
}

void CMinecraftApp::processSchematicsLighting(LevelChunk *levelChunk)
{
}

void CMinecraftApp::loadDefaultGameRules()
{
}

void CMinecraftApp::setLevelGenerationOptions(LevelGenerationOptions *levelGen)
{
}

void CMinecraftApp::EnterSaveNotificationSection()
{
}

void CMinecraftApp::LeaveSaveNotificationSection()
{
}

vector<ModelPart *> *CMinecraftApp::GetAdditionalModelParts(DWORD dwSkinID)
{
    return NULL;
}

vector<SKIN_BOX *> *CMinecraftApp::GetAdditionalSkinBoxes(DWORD dwSkinID)
{
    return NULL;
}

void CMinecraftApp::SetAdditionalSkinBoxes(DWORD dwSkinID, SKIN_BOX *SkinBoxA, DWORD dwSkinBoxC)
{
}

vector<ModelPart *> *CMinecraftApp::SetAdditionalSkinBoxes(DWORD dwSkinID, vector<SKIN_BOX *> *pvSkinBoxA)
{
    return NULL;
}

unsigned int CMinecraftApp::GetAnimOverrideBitmask(DWORD dwSkinID)
{
    return 0;
}

void CMinecraftApp::SetAnimOverrideBitmask(DWORD dwSkinID, unsigned int uiAnimOverrideBitmask)
{
}

DWORD CMinecraftApp::getSkinIdFromPath(const wstring &skin)
{
    return 0;
}

wstring CMinecraftApp::getSkinPathFromId(DWORD skinId)
{
    return L"";
}

void CMinecraftApp::getLocale(vector<wstring> &vecWstrLocales)
{
}

wstring CMinecraftApp::getFilePath(DWORD packId, wstring filename, bool bAddDataFolder)
{
    return filename;
}
