#include "ConsoleInput.h"
#include "stdafx.h"

ConsoleInput::ConsoleInput(const wstring &msg, ConsoleInputSource *source)
{
    this->msg = msg;
    this->source = source;
}
