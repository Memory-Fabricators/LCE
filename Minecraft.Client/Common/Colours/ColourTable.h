#pragma once

#include "Common/App_enums.h"
#include <cstddef>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

class ColourTable
{
  private:
    unsigned int m_colourValues[static_cast<std::size_t>(MinecraftColor::COUNT)];

    static wchar_t *ColourTableElements[static_cast<std::size_t>(MinecraftColor::COUNT)];
    static std::unordered_map<std::wstring, MinecraftColor> s_colourNamesMap;

  public:
    static void staticCtor();

    ColourTable(std::span<std::byte> pbData, std::size_t dwLength);
    ColourTable(ColourTable *defaultColours, std::span<std::byte> pbData, std::size_t dwLength);

    unsigned int getColour(MinecraftColor id);
    unsigned int getColor(MinecraftColor id)
    {
        return getColour(id);
    }

    void loadColoursFromData(std::vector<std::byte> pbData, std::size_t dwLength);
    void setColour(const std::wstring &colourName, int value);
    void setColour(const std::wstring &colourName, const std::wstring &value);
};
