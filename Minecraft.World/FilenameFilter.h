#pragma once
;

#include "stdafx.h"

class File;

// 4J Jev, java lirary interface.
class FilenameFilter
{
  public:
    virtual bool accept(File *dir, const wstring &name) = 0;
};
