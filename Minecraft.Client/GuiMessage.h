#pragma once
;

class GuiMessage
{
  public:
    wstring string;
    int ticks;
    GuiMessage(const wstring &string);
};
