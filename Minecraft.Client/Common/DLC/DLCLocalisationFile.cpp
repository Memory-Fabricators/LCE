#include "DLCLocalisationFile.h"
#include "..\..\StringTable.h"
#include "DLCManager.h"
#include "stdafx.h"

DLCLocalisationFile::DLCLocalisationFile(const wstring &path) : DLCFile(DLCManager::e_DLCType_LocalisationData, path)
{
    m_strings = NULL;
}

void DLCLocalisationFile::addData(PBYTE pbData, DWORD dwBytes)
{
    m_strings = new StringTable(pbData, dwBytes);
}
