#include "ItemStat.h"
#include "stdafx.h"

ItemStat::ItemStat(int id, const wstring &name, int itemId) : Stat(id, name), itemId(itemId)
{
}

int ItemStat::getItemId()
{
    return itemId;
}
