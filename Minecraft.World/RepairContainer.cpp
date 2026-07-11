#include "RepairContainer.h"
#include "RepairMenu.h"
#include "stdafx.h"

RepairContainer::RepairContainer(RepairMenu *menu, int name, int size) : SimpleContainer(name, size)
{
    m_menu = menu;
}

void RepairContainer::setChanged()
{
    SimpleContainer::setChanged();
    m_menu->slotsChanged(shared_from_this());
}
