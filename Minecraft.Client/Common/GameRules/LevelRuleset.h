#pragma once

#include "../../StringTable.h"
#include "AABB.h"
#include "CompoundGameRuleDefinition.h"

class NamedAreaRuleDefinition;

class LevelRuleset : public CompoundGameRuleDefinition
{
  private:
    vector<NamedAreaRuleDefinition *> m_areas;
    StringTable *m_stringTable;

  public:
    LevelRuleset();
    ~LevelRuleset();

    virtual void getChildren(vector<GameRuleDefinition *> *children);
    virtual GameRuleDefinition *addChild(GameRuleType ruleType);

    virtual GameRuleType getActionType()
    {
        return GameRuleType::LevelRules;
    }

    void loadStringTable(StringTable *table);
    LPCWSTR getString(const wstring &key);

    AABB *getNamedArea(const wstring &areaName);

    StringTable *getStringTable()
    {
        return m_stringTable;
    }
};
