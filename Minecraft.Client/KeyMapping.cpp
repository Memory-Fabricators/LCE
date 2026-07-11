#include "KeyMapping.h"
#include "stdafx.h"

KeyMapping::KeyMapping(const wstring &name, int key)
{
    this->name = name;
    this->key = key;
}
