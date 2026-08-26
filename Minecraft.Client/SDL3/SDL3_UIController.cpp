#include "SDL3_UIController.h"
#include "stdafx.h"

#include "4JLibs/inc/4J_Profile.h"
#include "../Common/Consoles_App.h"
#include "ruffle_bridge.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>

ConsoleUIController ui;

namespace
{
bool IsOk(RuffleBridgeStatus status)
{
    return status == RUFFLE_BRIDGE_OK;
}
}

std::string ConsoleUIController::GetMoviePathForScene(EUIScene scene, int width, int height)
{
    std::string baseName;
    switch (scene)
    {
    case eUIScene_MainMenu:
        baseName = "MainMenu";
        break;
    case eUIScene_PauseMenu:
        baseName = "PauseMenu";
        break;
    case eUIScene_InventoryMenu:
        baseName = "InventoryMenu";
        break;
    case eUIScene_CreativeMenu:
        baseName = "CreativeMenu";
        break;
    case eUIScene_ContainerMenu:
        baseName = "ChestMenu";
        break;
    case eUIScene_LargeContainerMenu:
        baseName = "ChestLargeMenu";
        break;
    case eUIScene_Crafting2x2Menu:
        baseName = "Crafting2x2Menu";
        break;
    case eUIScene_Crafting3x3Menu:
        baseName = "Crafting3x3Menu";
        break;
    case eUIScene_FurnaceMenu:
        baseName = "FurnaceMenu";
        break;
    case eUIScene_DispenserMenu:
        baseName = "TrapMenu";
        break;
    case eUIScene_EnchantingMenu:
        baseName = "EnchantingMenu";
        break;
    case eUIScene_BrewingStandMenu:
        baseName = "BrewingStandMenu";
        break;
    case eUIScene_AnvilMenu:
        baseName = "AnvilMenu";
        break;
    case eUIScene_TradingMenu:
        baseName = "TradingMenu";
        break;
    case eUIScene_SignEntryMenu:
        baseName = "SignEntryMenu";
        break;
    case eUIScene_CreateWorldMenu:
        baseName = "CreateWorldMenu";
        break;
    case eUIScene_LoadOrJoinMenu:
        baseName = "LoadOrJoinMenu";
        break;
    case eUIScene_LoadMenu:
        baseName = "LoadMenu";
        break;
    case eUIScene_JoinMenu:
        baseName = "JoinMenu";
        break;
    case eUIScene_LaunchMoreOptionsMenu:
        baseName = "LaunchMoreOptionsMenu";
        break;
    case eUIScene_FullscreenProgress:
    case eUIScene_ConnectingProgress:
        baseName = "FullscreenProgress";
        break;
    case eUIScene_DLCMainMenu:
        baseName = "DLCMainMenu";
        break;
    case eUIScene_DLCOffersMenu:
        baseName = "DLCOffersMenu";
        break;
    case eUIScene_HelpAndOptionsMenu:
    case eUIScene_SettingsMenu:
        baseName = "SettingsMenu";
        break;
    case eUIScene_SettingsOptionsMenu:
        baseName = "SettingsOptionsMenu";
        break;
    case eUIScene_SettingsAudioMenu:
        baseName = "SettingsAudioMenu";
        break;
    case eUIScene_SettingsControlMenu:
        baseName = "SettingsControlMenu";
        break;
    case eUIScene_SettingsGraphicsMenu:
        baseName = "SettingsGraphicsMenu";
        break;
    case eUIScene_SettingsUIMenu:
        baseName = "SettingsUIMenu";
        break;
    case eUIScene_SkinSelectMenu:
        baseName = "SkinSelectMenu";
        break;
    case eUIScene_LeaderboardsMenu:
        baseName = "LeaderboardMenu";
        break;
    case eUIScene_Credits:
        baseName = "Credits";
        break;
    case eUIScene_DeathMenu:
        baseName = "DeathMenu";
        break;
    case eUIScene_Intro:
        baseName = "Intro";
        break;
    case eUIScene_SaveMessage:
        baseName = "SaveMessage";
        break;
    case eUIScene_HUD:
        baseName = "HUD";
        break;
    default:
        break;
    }

    if (baseName.empty())
    {
        return {};
    }

    // Select resolution suffix
    std::string resSuffix = "720.swf";
    if (height >= 1080)
    {
        resSuffix = "1080.swf";
    }
    else if (height <= 480)
    {
        resSuffix = "480.swf";
    }

    std::string candidate = baseName + resSuffix;
    std::wstring wCandidate(candidate.begin(), candidate.end());
    if (app.hasArchiveFile(wCandidate))
    {
        return candidate;
    }

    // Fallback to 720.swf if 1080/480 is not found
    candidate = baseName + "720.swf";
    wCandidate = std::wstring(candidate.begin(), candidate.end());
    if (app.hasArchiveFile(wCandidate))
    {
        return candidate;
    }

    // Fallback to bare .swf
    candidate = baseName + ".swf";
    wCandidate = std::wstring(candidate.begin(), candidate.end());
    if (app.hasArchiveFile(wCandidate))
    {
        return candidate;
    }

    return {};
}

bool ConsoleUIController::LoadScenePlayer(SceneEntry &entry)
{
    std::string movieName = GetMoviePathForScene(entry.scene, m_width, m_height);
    if (movieName.empty())
    {
        fprintf(stderr, "ConsoleUIController: no SWF found for scene %d\n", static_cast<int>(entry.scene));
        return false;
    }

    std::wstring wMovieName(movieName.begin(), movieName.end());
    byteArray swfData = app.getArchiveFile(wMovieName);
    if (swfData.data == nullptr || swfData.length == 0)
    {
        fprintf(stderr, "ConsoleUIController: failed to read SWF data for '%s'\n", movieName.c_str());
        return false;
    }

    std::vector<const char *> searchDirPtrs;
    for (const std::string &dir : m_searchDirs)
    {
        searchDirPtrs.push_back(dir.c_str());
    }

    char url[1024];
    snprintf(url, sizeof(url), "file:///%s", movieName.c_str());
    RuffleBridgePlayer *player = nullptr;
    const RuffleBridgeStatus status = ruffle_bridge_player_create(
        static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height),
        reinterpret_cast<const uint8_t *>(swfData.data), static_cast<size_t>(swfData.length),
        url,
        searchDirPtrs.data(), searchDirPtrs.size(),
        &player);

    delete[] swfData.data;

    if (!IsOk(status) || player == nullptr)
    {
        fprintf(stderr, "ConsoleUIController: failed to create Ruffle player (%d) for '%s'\n",
                static_cast<int>(status), movieName.c_str());
        return false;
    }
    entry.player = player;
    entry.movieName = movieName;
    app.DebugPrintf("ConsoleUIController: Loaded scene %d ('%s')\n", static_cast<int>(entry.scene), movieName.c_str());

    // Initialize scene buttons / controls
    if (entry.scene == eUIScene_PauseMenu)
    {
        const char *buttonNames[] = {
            "Button1", "Button2", "Button3", "Button4", "Button5", "Button6"
        };
        const char *buttonLabels[] = {
            "Resume Game",
            "Help & Options",
            "Leaderboards",
            "Achievements",
            "Save Game",
            "Exit Game",
        };
        for (size_t i = 0; i < 6; ++i)
        {
            RuffleBridgeButton *btn = nullptr;
            if (ruffle_bridge_button_create(buttonNames[i], &btn) == RUFFLE_BRIDGE_OK && btn != nullptr)
            {
                RuffleBridgeStatus st1 = ruffle_bridge_button_init(player, btn, buttonLabels[i], static_cast<int>(i));
                RuffleBridgeStatus st2 = ruffle_bridge_button_set_enabled(player, btn, true);
                app.DebugPrintf("ConsoleUIController: init %s ('%s') -> %d, %d\n", buttonNames[i], buttonLabels[i], (int)st1, (int)st2);
                ruffle_bridge_button_destroy(btn);
            }
        }
    }

    return true;
}

void ConsoleUIController::DestroyScenePlayer(SceneEntry &entry)
{
    if (entry.player != nullptr)
    {
        ruffle_bridge_player_destroy(entry.player);
        entry.player = nullptr;
    }
}

void ConsoleUIController::init(void *window, int w, int h)
{
    m_width = (w > 0) ? w : 1280;
    m_height = (h > 0) ? h : 720;
    m_lastTickSeconds = 0.0;
    m_searchDirs.clear();

    // Search directories for companion SWFs (e.g. skinWin.swf, skin.swf)
    m_searchDirs.push_back("Minecraft.Client/Common/Media");
    m_searchDirs.push_back("Minecraft.Client/Windows64Media/Media");
    m_searchDirs.push_back("Common/Media");
    m_searchDirs.push_back("Windows64Media/Media");
    m_searchDirs.push_back(".");
    const char *extraDir = getenv("LCE_RUFFLE_SWF_EXTRA_DIR");
    if (extraDir != nullptr && *extraDir != '\0')
    {
        m_searchDirs.emplace_back(extraDir);
    }

    m_initialised = true;
    app.DebugPrintf("ConsoleUIController initialized (%dx%d)\n", m_width, m_height);
}

void ConsoleUIController::render()
{
    if (!m_initialised || m_sceneStack.empty())
        return;

    // Render active scenes from bottom to top of the stack
    for (SceneEntry &entry : m_sceneStack)
    {
        if (entry.player != nullptr)
        {
            ruffle_bridge_player_render(entry.player);
        }
    }
}

void ConsoleUIController::tick()
{
    if (!m_initialised || m_sceneStack.empty())
        return;

    static const auto startTime = std::chrono::steady_clock::now();
    const auto nowDuration = std::chrono::steady_clock::now() - startTime;
    const double now = std::chrono::duration<double>(nowDuration).count();
    const double dt = (m_lastTickSeconds > 0.0) ? (now - m_lastTickSeconds) : 0.016;
    m_lastTickSeconds = now;

    for (SceneEntry &entry : m_sceneStack)
    {
        if (entry.player != nullptr)
        {
            ruffle_bridge_player_tick(entry.player, dt);
        }
    }
}

void ConsoleUIController::shutdown()
{
    CloseAllPlayersScenes();
    m_initialised = false;
}

void ConsoleUIController::CheckMenuDisplayed()
{
}

bool ConsoleUIController::IsPauseMenuDisplayed(int pad)
{
    return IsSceneInStack(pad, eUIScene_PauseMenu);
}

void ConsoleUIController::UpdateTrialTimer(int pad)
{
}

void ConsoleUIController::ShowTrialTimer(bool show)
{
}

void ConsoleUIController::ReloadSkin()
{
}

bool ConsoleUIController::IsReloadingSkin()
{
    return false;
}

bool ConsoleUIController::GetMenuDisplayed(int iPad)
{
    return !m_sceneStack.empty();
}

bool ConsoleUIController::NavigateToScene(int iPad, EUIScene scene, void *initData, EUILayer layer, EUIGroup group)
{
    app.DebugPrintf("ConsoleUIController::NavigateToScene: pad=%d, scene=%d\n", iPad, static_cast<int>(scene));

    // Check if scene is already on top of stack
    if (!m_sceneStack.empty() && m_sceneStack.back().scene == scene && m_sceneStack.back().pad == iPad)
    {
        return true;
    }

    SceneEntry entry;
    entry.scene = scene;
    entry.pad = iPad;
    entry.layer = layer;
    entry.group = group;
    entry.initData = initData;

    if (!LoadScenePlayer(entry))
    {
        return false;
    }

    m_sceneStack.push_back(entry);
    return true;
}

bool ConsoleUIController::NavigateBack(int iPad, bool forceUsePad, EUIScene eScene, EUILayer eLayer)
{
    if (m_sceneStack.empty())
    {
        return false;
    }

    if (eScene != eUIScene_COUNT)
    {
        for (auto it = m_sceneStack.rbegin(); it != m_sceneStack.rend(); ++it)
        {
            if (it->scene == eScene && (it->pad == iPad || forceUsePad || iPad == -1))
            {
                DestroyScenePlayer(*it);
                m_sceneStack.erase(std::next(it).base());
                return true;
            }
        }
        return false;
    }

    SceneEntry &top = m_sceneStack.back();
    DestroyScenePlayer(top);
    m_sceneStack.pop_back();
    return true;
}

void ConsoleUIController::NavigateToHomeMenu()
{
    CloseAllPlayersScenes();
    NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_MainMenu);
}

void ConsoleUIController::CloseUIScenes(int iPad, bool forceIPad)
{
    for (auto it = m_sceneStack.begin(); it != m_sceneStack.end();)
    {
        if (it->pad == iPad || forceIPad || iPad == -1)
        {
            DestroyScenePlayer(*it);
            it = m_sceneStack.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ConsoleUIController::CloseAllPlayersScenes()
{
    for (SceneEntry &entry : m_sceneStack)
    {
        DestroyScenePlayer(entry);
    }
    m_sceneStack.clear();
}

bool ConsoleUIController::IsIgnoreAutosaveMenuDisplayed(int iPad)
{
    return false;
}

void ConsoleUIController::UpdatePlayerBasePositions()
{
}

void ConsoleUIController::ShowAutosaveCountdownTimer(bool show)
{
}

void ConsoleUIController::UpdateAutosaveCountdownTimer(unsigned int uiSeconds)
{
}

void ConsoleUIController::SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed)
{
}

bool ConsoleUIController::IsSceneInStack(int iPad, EUIScene eScene)
{
    for (const SceneEntry &entry : m_sceneStack)
    {
        if (entry.scene == eScene && (entry.pad == iPad || iPad == -1))
        {
            return true;
        }
    }
    return false;
}

bool ConsoleUIController::IsContainerMenuDisplayed(int iPad)
{
    return IsSceneInStack(iPad, eUIScene_ContainerMenu) ||
           IsSceneInStack(iPad, eUIScene_LargeContainerMenu) ||
           IsSceneInStack(iPad, eUIScene_InventoryMenu) ||
           IsSceneInStack(iPad, eUIScene_CreativeMenu) ||
           IsSceneInStack(iPad, eUIScene_Crafting2x2Menu) ||
           IsSceneInStack(iPad, eUIScene_Crafting3x3Menu) ||
           IsSceneInStack(iPad, eUIScene_FurnaceMenu) ||
           IsSceneInStack(iPad, eUIScene_DispenserMenu) ||
           IsSceneInStack(iPad, eUIScene_EnchantingMenu) ||
           IsSceneInStack(iPad, eUIScene_BrewingStandMenu) ||
           IsSceneInStack(iPad, eUIScene_AnvilMenu) ||
           IsSceneInStack(iPad, eUIScene_TradingMenu);
}

bool ConsoleUIController::IsIgnorePlayerJoinMenuDisplayed(int iPad)
{
    return false;
}

UIScene *ConsoleUIController::GetTopScene(int iPad, EUILayer layer, EUIGroup group)
{
    return nullptr;
}

void ConsoleUIController::StartReloadSkinThread()
{
}

void ConsoleUIController::CleanUpSkinReload()
{
}

void ConsoleUIController::ShowSavingMessage(unsigned int iPad, C4JStorage::ESavingMessage eVal)
{
}

void ConsoleUIController::SetTooltipText(unsigned int iPad, unsigned int tooltip, int iTextID)
{
}

void ConsoleUIController::SetEnableTooltips(unsigned int iPad, BOOL bVal)
{
}

void ConsoleUIController::ShowTooltip(unsigned int iPad, unsigned int tooltip, bool show)
{
}

void ConsoleUIController::SetTooltips(unsigned int iPad, int iA, int iB, int iX, int iY, int iLT, int iRT, int iLB, int iRB, int iLS, bool forceUpdate)
{
}

void ConsoleUIController::EnableTooltip(unsigned int iPad, unsigned int tooltip, bool enable)
{
}

void ConsoleUIController::RefreshTooltips(unsigned int iPad)
{
}

void ConsoleUIController::PlayUISFX(ESoundEffect eSound)
{
}

void ConsoleUIController::DisplayGamertag(unsigned int iPad, bool show)
{
}

void ConsoleUIController::SetSelectedItem(unsigned int iPad, const wstring &name)
{
}

void ConsoleUIController::UpdateSelectedItemPos(unsigned int iPad)
{
}

void ConsoleUIController::HandleDLCMountingComplete()
{
}

void ConsoleUIController::HandleDLCInstalled(int iPad)
{
}

void ConsoleUIController::HandleTMSDLCFileRetrieved(int iPad)
{
}

void ConsoleUIController::HandleTMSBanFileRetrieved(int iPad)
{
}

void ConsoleUIController::HandleInventoryUpdated(int iPad)
{
}

void ConsoleUIController::HandleGameTick()
{
}

void ConsoleUIController::SetTutorialDescription(int iPad, TutorialPopupInfo *info)
{
}

void ConsoleUIController::SetTutorialVisible(int iPad, bool visible)
{
}

bool ConsoleUIController::IsTutorialVisible(int iPad)
{
    return false;
}

void ConsoleUIController::SetTutorial(int iPad, Tutorial *tutorial)
{
}

void ConsoleUIController::RemoveInteractSceneReference(int iPad, UIScene *scene)
{
}

void ConsoleUIController::SetEmptyQuadrantLogo(int iSection)
{
}

void ConsoleUIController::HideAllGameUIElements()
{
}

void ConsoleUIController::ShowOtherPlayersBaseScene(unsigned int iPad, bool show)
{
}

void ConsoleUIController::SetTrialTimerLimitSecs(unsigned int uiSeconds)
{
}

void ConsoleUIController::ReduceTrialTimerValue()
{
}

bool ConsoleUIController::PressStartPlaying(unsigned int iPad)
{
    return true;
}

void ConsoleUIController::ShowPressStart(unsigned int iPad)
{
}

void ConsoleUIController::HidePressStart()
{
}

C4JStorage::EMessageResult ConsoleUIController::RequestMessageBox(UINT uiTitle, UINT uiText, UINT *uiOptionA, UINT uiOptionC, DWORD dwPad,
                                                                  int (*Func)(LPVOID, int, const C4JStorage::EMessageResult), LPVOID lpParam, C4JStringTable *pStringTable, WCHAR *pwchFormatString, DWORD dwFocusButton, bool bIsError)
{
    return C4JStorage::EMessageResult(0);
}

C4JStorage::EMessageResult ConsoleUIController::RequestContentRestrictedMessageBox(UINT title, UINT message, int iPad, int (*Func)(LPVOID, int, const C4JStorage::EMessageResult), LPVOID lpParam)
{
    return C4JStorage::EMessageResult(0);
}

C4JStorage::EMessageResult ConsoleUIController::RequestUGCMessageBox(UINT title, UINT message, int iPad, int (*Func)(LPVOID, int, const C4JStorage::EMessageResult), LPVOID lpParam)
{
    return C4JStorage::EMessageResult(0);
}

void ConsoleUIController::SetWinUserIndex(unsigned int iPad)
{
}
