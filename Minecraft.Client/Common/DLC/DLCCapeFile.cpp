#include "DLCCapeFile.h"
#include "DLCManager.h"
#include "stdafx.h"

DLCCapeFile::DLCCapeFile(const wstring &path) : DLCFile(DLCManager::e_DLCType_Cape, path)
{
}

void DLCCapeFile::addData(PBYTE pbData, DWORD dwBytes)
{
    app.AddMemoryTextureFile(m_path, pbData, dwBytes);
}
