#pragma once

#include "../../../Minecraft.World/Container.h"
#include "../../../Minecraft.World/EntityHorse.h"
#include "../../../Minecraft.World/Inventory.h"
#include "IUIScene_AbstractContainerMenu.h"

class IUIScene_HorseInventoryMenu : public virtual IUIScene_AbstractContainerMenu
{
  protected:
    shared_ptr<Inventory> m_inventory;
    shared_ptr<Container> m_container;
    shared_ptr<EntityHorse> m_horse;

  public:
    virtual ESceneSection GetSectionAndSlotInDirection(ESceneSection eSection, ETapState eTapDirection, int *piTargetX, int *piTargetY);
    int getSectionStartOffset(ESceneSection eSection);
    bool IsSectionSlotList(ESceneSection eSection);
    bool IsVisible(ESceneSection eSection);
};
