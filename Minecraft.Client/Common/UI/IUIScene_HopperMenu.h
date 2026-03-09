#pragma once

#include "../../../Minecraft.World/Container.h"
#include "../../../Minecraft.World/Inventory.h"
#include "IUIScene_AbstractContainerMenu.h"

class IUIScene_HopperMenu : public virtual IUIScene_AbstractContainerMenu
{
  public:
    virtual ESceneSection GetSectionAndSlotInDirection(ESceneSection eSection, ETapState eTapDirection, int *piTargetX, int *piTargetY);
    int getSectionStartOffset(ESceneSection eSection);
};
