#include "LevelConflictException.h"
#include "stdafx.h"

LevelConflictException::LevelConflictException(const wstring &msg) : RuntimeException(msg)
{
}
