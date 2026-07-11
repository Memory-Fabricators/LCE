#include "SDL3_UIController.h"
#include "stdafx.h"

ConsoleUIController ui;

void ConsoleUIController::init(void *window, int w, int h)
{
}

void ConsoleUIController::render()
{
}

void ConsoleUIController::tick()
{
}

void ConsoleUIController::shutdown()
{
}

void ConsoleUIController::CheckMenuDisplayed()
{
}

bool ConsoleUIController::IsPauseMenuDisplayed(int pad)
{
    return false;
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
    return false;
}

bool ConsoleUIController::NavigateToScene(int iPad, EUIScene scene, void *initData, EUILayer layer, EUIGroup group)
{
    return false;
}

void ConsoleUIController::CloseUIScenes(int iPad, bool forceIPad)
{
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

bool ConsoleUIController::NavigateBack(int iPad, bool forceUsePad, EUIScene eScene, EUILayer eLayer)
{
    return false;
}

void ConsoleUIController::CloseAllPlayersScenes()
{
}

void ConsoleUIController::SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed)
{
}

bool ConsoleUIController::IsSceneInStack(int iPad, EUIScene eScene)
{
    return false;
}

bool ConsoleUIController::IsContainerMenuDisplayed(int iPad)
{
    return false;
}

bool ConsoleUIController::IsIgnorePlayerJoinMenuDisplayed(int iPad)
{
    return false;
}

UIScene *ConsoleUIController::GetTopScene(int iPad, EUILayer layer, EUIGroup group)
{
    return NULL;
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
