#pragma once

#include "ConsoleGameRulesConstants.h"
#include "GameRuleDefinition.h"

class ItemInstance;

class AddEnchantmentRuleDefinition : public GameRuleDefinition
{
  private:
    int m_enchantmentId;
    int m_enchantmentLevel;

  public:
    AddEnchantmentRuleDefinition();

    virtual GameRuleType getActionType()
    {
        return GameRuleType::AddEnchantment;
    }

    virtual void writeAttributes(DataOutputStream *, UINT numAttrs);

    virtual void addAttribute(const wstring &attributeName, const wstring &attributeValue);

    bool enchantItem(shared_ptr<ItemInstance> item);
};
