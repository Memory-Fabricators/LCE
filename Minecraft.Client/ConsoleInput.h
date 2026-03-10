#pragma once
#include "ConsoleInputSource.h"

class ConsoleInput
{
  public:
    wstring msg;
    ConsoleInputSource *source;

    ConsoleInput(const wstring &msg, ConsoleInputSource *source);
};
