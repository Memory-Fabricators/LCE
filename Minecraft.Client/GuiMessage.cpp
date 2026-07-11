#include "GuiMessage.h"
#include "stdafx.h"

GuiMessage::GuiMessage(const wstring &string)
{
    this->string = string;
    ticks = 0;
}
