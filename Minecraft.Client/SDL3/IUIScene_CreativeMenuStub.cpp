#include "../Common/UI/IUIScene_CreativeMenu.h"
#include "../stdafx.h"

// TODO: placeholder. IUIScene_CreativeMenu.cpp itself has no Iggy dependency
// (it just fills in creative-inventory item lists), but its base class chain
// (IUIScene_AbstractContainerMenu -> UIScene) is part of Common/UI, which is
// excluded wholesale for its Iggy dependency. Real creative-inventory tab
// contents aren't populated until the UI layer is ported.
IUIScene_CreativeMenu::TabSpec **IUIScene_CreativeMenu::specs = NULL;
vector<shared_ptr<ItemInstance>> IUIScene_CreativeMenu::categoryGroups[eCreativeInventoryGroupsCount];

void IUIScene_CreativeMenu::staticCtor()
{
}
