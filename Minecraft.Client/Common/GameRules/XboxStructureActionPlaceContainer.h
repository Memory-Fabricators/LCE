#pragma once

#include "XboxStructureActionPlaceBlock.h"

class AddItemRuleDefinition;
class StructurePiece;
class Level;
class BoundingBox;

class XboxStructureActionPlaceContainer : public XboxStructureActionPlaceBlock
{
  private:
    vector<AddItemRuleDefinition *> m_items;

  public:
    XboxStructureActionPlaceContainer();
    ~XboxStructureActionPlaceContainer();

    virtual GameRuleType getActionType()
    {
        return GameRuleType::PlaceContainer;
    }

    virtual void getChildren(vector<GameRuleDefinition *> *children);
    virtual GameRuleDefinition *addChild(GameRuleType ruleType);

    // 4J-JEV: Super class handles attr-facing fine.
    // virtual void writeAttributes(DataOutputStream *dos, UINT numAttributes);

    virtual void addAttribute(const wstring &attributeName, const wstring &attributeValue);

    bool placeContainerInLevel(StructurePiece *structure, Level *level, BoundingBox *chunkBB);
};
