#include "../../../stdafx.h"

C4JStorage StorageManager;

void C4JStorage::Init(const wchar_t *, const char *, DWORD, void (*)(LPVOID, bool), LPVOID)
{
}

void C4JStorage::ResetSaveData()
{
}

void C4JStorage::SetSaveTitle(const wchar_t *)
{
}

void C4JStorage::StoreTMSPathName()
{
}

void C4JStorage::SetDLCPackageRoot(const char *)
{
}

void C4JStorage::RegisterMarketplaceCountsCallback(LPVOID, LPVOID)
{
}

void C4JStorage::Tick()
{
}

bool C4JStorage::LoadContent(int, XCONTENT_DATA &)
{
    return true;
}

bool C4JStorage::SaveContent(int, XCONTENT_DATA &, void (*)(LPVOID, bool), LPVOID)
{
    return true;
}

bool C4JStorage::EraseContent(int, XCONTENT_DATA &)
{
    return true;
}

void C4JStorage::GetSaveDetails(int, SAVE_DETAILS *)
{
}

void C4JStorage::GetDeviceDetails(int, DWORD *, void *, int *)
{
}

int C4JStorage::GetStorageDevices(int, bool)
{
    return 1;
}

bool C4JStorage::GetSaveUniqueNumber(INT *piVal)
{
    *piVal = 0;
    return true;
}

unsigned int C4JStorage::GetSaveSize()
{
    return 0;
}

void C4JStorage::GetSaveData(void *pvData, unsigned int *puiBytes)
{
    *puiBytes = 0;
}

PVOID C4JStorage::AllocateSaveData(unsigned int uiBytes)
{
    return new char[uiBytes];
}

bool C4JStorage::GetSaveDisabled()
{
    return false;
}

C4JStorage::ESaveGameState C4JStorage::GetSaveState()
{
    return C4JStorage::ESaveGame_Idle;
}

int C4JStorage::AddSubfile(unsigned int subfileId)
{
    return 0;
}

unsigned int C4JStorage::GetSubfileCount()
{
    return 0;
}

void C4JStorage::GetSubfileDetails(int idx, unsigned int *subfileId, unsigned char **data, unsigned int *sizeOut)
{
    *subfileId = 0;
    *data = NULL;
    *sizeOut = 0;
}

void C4JStorage::ResetSubfiles()
{
}

void C4JStorage::UpdateSubfile(int idx, unsigned char *data, unsigned int size)
{
}

void C4JStorage::SetSaveImages(PBYTE pbThumbnail, DWORD dwThumbnailBytes, PBYTE pbImage, DWORD dwImageBytes, PBYTE pbTextData, DWORD dwTextDataBytes)
{
}

C4JStorage::ESaveGameState C4JStorage::SaveSaveData(int (*Func)(LPVOID, const bool), LPVOID lpParam)
{
    if (Func)
    {
        Func(lpParam, true);
    }
    return C4JStorage::ESaveGame_Idle;
}

C4JStorage::ESaveGameState C4JStorage::SaveSubfiles(int (*Func)(LPVOID, const bool), LPVOID lpParam)
{
    return C4JStorage::ESaveGame_Idle;
}

DWORD C4JStorage::MountInstalledDLC(int, DWORD, int (*)(LPVOID, int, DWORD, DWORD), LPVOID, LPCSTR)
{
    return 0;
}

DWORD C4JStorage::UnmountInstalledDLC(LPCSTR)
{
    return 0;
}

bool C4JStorage::GetSaveUniqueFilename(char *pszName)
{
    if (pszName)
    {
        pszName[0] = '\0';
    }
    return false;
}

C4JStorage::ESaveGameState C4JStorage::DoesSaveExist(bool *pbExists)
{
    if (pbExists)
    {
        *pbExists = false;
    }
    return C4JStorage::ESaveGame_Idle;
}
