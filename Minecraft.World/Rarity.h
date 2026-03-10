#pragma once

#include "Common/App_enums.h"
#include <string>

class Rarity
{
  public:
    static const Rarity *common;
    static const Rarity *uncommon;
    static const Rarity *rare;
    static const Rarity *epic;

    const MinecraftColor color;
    const std::wstring name;

    Rarity(MinecraftColor color, const std::wstring &name);
};
