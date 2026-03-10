#pragma once
#include "GuiComponent.h"
#include "Minecraft.h"
#include <chrono>
#include <string>

class Achievement;
class ItemRenderer;

class AchievementPopup : public GuiComponent
{
  private:
    Minecraft *mc;
    int width, height;

    std::wstring title;
    std::wstring desc;
    Achievement *ach;
    std::chrono::steady_clock::time_point startTime;
    ItemRenderer *ir;
    bool isHelper;

  public:
    AchievementPopup(Minecraft *mc);
    void popup(Achievement *ach);
    void permanent(Achievement *ach);

  private:
    void prepareWindow();

  public:
    void render();
};
