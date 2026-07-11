#pragma once

#include "../Common/App_defines.h"
#include "../Common/App_enums.h"
#include "../Common/Consoles_App.h"

class CConsoleMinecraftApp : public CMinecraftApp
{
  public:
    CConsoleMinecraftApp();
    virtual ~CConsoleMinecraftApp()
    {
    }

    void SetRichPresenceContext(int iPad, int contextId);
    void StoreLaunchData();
    void ExitGame();
    void FatalLoadError();
    void CaptureSaveThumbnail();
    void GetSaveThumbnail(PBYTE *pbData, DWORD *pdwSize);
    void ReleaseSaveThumbnail();
    void GetScreenshot(int iPad, PBYTE *pbData, DWORD *pdwSize);
    void TemporaryCreateGameStart();
    int GetLocalTMSFileIndex(WCHAR *wchTMSFile, bool bFilenameIncludesExtension,
                             int eEXT);
    int LoadLocalTMSFile(WCHAR *wchTMSFile);
    int LoadLocalTMSFile(WCHAR *wchTMSFile, int eExt);
    void FreeLocalTMSFiles(int eType);

    void ReadBannedList(int iPad, eTMSAction action = (eTMSAction)0,
                        bool bCallback = false);
    int LoadLocalTMSFile(WCHAR *wchTMSFile, eFileExtensionType eExt);
    void FreeLocalTMSFiles(eTMSFileType eType);
    int GetLocalTMSFileIndex(WCHAR *wchTMSFile, bool bFilenameIncludesExtension,
                             eFileExtensionType eEXT);

    C4JStringTable *GetStringTable()
    {
        return NULL;
    }
};

extern CConsoleMinecraftApp app;
