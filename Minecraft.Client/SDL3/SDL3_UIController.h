#pragma once

#include "../../Minecraft.World/SoundTypes.h"
#include "../Common/UI/UIEnums.h"
#include "4JLibs/inc/4J_Storage.h"
#include "SDL3/SDL.h"

class IggyCustomDrawCallbackRegion;
class UIScene;
class GDrawTexture;
class Tutorial;
struct _TutorialPopupInfo;
typedef _TutorialPopupInfo TutorialPopupInfo;

class ConsoleUIController
{
  public:
    void init(void *window, int w, int h);
    void render();
    void tick();
    void shutdown();
    void CheckMenuDisplayed();

    bool IsPauseMenuDisplayed(int pad);
    void UpdateTrialTimer(int pad);
    void ShowTrialTimer(bool show);
    void ReloadSkin();
    bool IsReloadingSkin();
    bool GetMenuDisplayed(int iPad);

    bool NavigateToScene(int iPad, EUIScene scene, void *initData = NULL, EUILayer layer = eUILayer_Scene, EUIGroup group = eUIGroup_PAD);
    bool NavigateBack(int iPad, bool forceUsePad = false, EUIScene eScene = eUIScene_COUNT, EUILayer eLayer = eUILayer_COUNT);
    void CloseUIScenes(int iPad, bool forceIPad = false);
    void CloseAllPlayersScenes();
    bool IsIgnoreAutosaveMenuDisplayed(int iPad);
    void SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed);
    bool IsSceneInStack(int iPad, EUIScene eScene);
    bool IsContainerMenuDisplayed(int iPad);
    bool IsIgnorePlayerJoinMenuDisplayed(int iPad);
    UIScene *GetTopScene(int iPad, EUILayer layer = eUILayer_Scene, EUIGroup group = eUIGroup_PAD);

    void StartReloadSkinThread();
    void CleanUpSkinReload();

    void UpdatePlayerBasePositions();
    void ShowAutosaveCountdownTimer(bool show);
    void UpdateAutosaveCountdownTimer(unsigned int uiSeconds);
    void ShowSavingMessage(unsigned int iPad, C4JStorage::ESavingMessage eVal);

    void SetTooltipText(unsigned int iPad, unsigned int tooltip, int iTextID);
    void SetEnableTooltips(unsigned int iPad, BOOL bVal);
    void ShowTooltip(unsigned int iPad, unsigned int tooltip, bool show);
    void SetTooltips(unsigned int iPad, int iA, int iB = -1, int iX = -1, int iY = -1, int iLT = -1, int iRT = -1, int iLB = -1, int iRB = -1, int iLS = -1, bool forceUpdate = false);
    void EnableTooltip(unsigned int iPad, unsigned int tooltip, bool enable);
    void RefreshTooltips(unsigned int iPad);

    void PlayUISFX(ESoundEffect eSound);

    void DisplayGamertag(unsigned int iPad, bool show);
    void SetSelectedItem(unsigned int iPad, const wstring &name);
    void UpdateSelectedItemPos(unsigned int iPad);

    void HandleDLCMountingComplete();
    void HandleDLCInstalled(int iPad);
    void HandleTMSDLCFileRetrieved(int iPad);
    void HandleTMSBanFileRetrieved(int iPad);
    void HandleInventoryUpdated(int iPad);
    void HandleGameTick();

    void SetTutorialDescription(int iPad, TutorialPopupInfo *info);
    void SetTutorialVisible(int iPad, bool visible);
    bool IsTutorialVisible(int iPad);
    void SetTutorial(int iPad, Tutorial *tutorial);
    void RemoveInteractSceneReference(int iPad, UIScene *scene);

    void SetEmptyQuadrantLogo(int iSection);
    void HideAllGameUIElements();
    void ShowOtherPlayersBaseScene(unsigned int iPad, bool show);

    void SetTrialTimerLimitSecs(unsigned int uiSeconds);
    void ReduceTrialTimerValue();

    bool PressStartPlaying(unsigned int iPad);
    void ShowPressStart(unsigned int iPad);
    void HidePressStart();

    C4JStorage::EMessageResult RequestMessageBox(UINT uiTitle, UINT uiText, UINT *uiOptionA, UINT uiOptionC, DWORD dwPad = 0,
                                                 int (*Func)(LPVOID, int, const C4JStorage::EMessageResult) = NULL, LPVOID lpParam = NULL, C4JStringTable *pStringTable = NULL, WCHAR *pwchFormatString = NULL, DWORD dwFocusButton = 0, bool bIsError = true);
    C4JStorage::EMessageResult RequestContentRestrictedMessageBox(UINT title = -1, UINT message = -1, int iPad = -1, int (*Func)(LPVOID, int, const C4JStorage::EMessageResult) = NULL, LPVOID lpParam = NULL);
    C4JStorage::EMessageResult RequestUGCMessageBox(UINT title = -1, UINT message = -1, int iPad = -1, int (*Func)(LPVOID, int, const C4JStorage::EMessageResult) = NULL, LPVOID lpParam = NULL);

    void SetWinUserIndex(unsigned int iPad);
};

extern ConsoleUIController ui;
