#pragma once

#include "..\GameRules\LevelGenerationOptions.h"
#include "DLCFile.h"

class DLCGameRules : public DLCFile
{
  public:
    DLCGameRules(DLCManager::EDLCType type, const wstring &path) : DLCFile(type, path)
    {
    }
};
