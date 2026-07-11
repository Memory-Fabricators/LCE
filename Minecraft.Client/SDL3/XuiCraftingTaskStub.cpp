#include "../Common/Tutorial/XuiCraftingTask.h"
#include "../stdafx.h"

// TODO: placeholder. XuiCraftingTask.cpp is excluded from the SDL3 build
// because isCompleted() reaches into UIScene_CraftingMenu (Iggy UI). This
// stub just anchors the vtable so tutorial code that constructs
// XuiCraftingTask still links; the crafting tutorial task never reports
// completion until the UI layer is ported.
bool XuiCraftingTask::isCompleted()
{
    return false;
}
