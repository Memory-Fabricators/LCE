#include "XboxStructureActionGenerateBox.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "../../../Minecraft.World/net.minecraft.world.level.levelgen.structure.h"
#include "GameRules/ConsoleGameRulesConstants.h"
#include "stdafx.h"

XboxStructureActionGenerateBox::XboxStructureActionGenerateBox()
{
    m_x0 = m_y0 = m_z0 = m_x1 = m_y1 = m_z1 = m_edgeTile = m_fillTile = 0;
    m_skipAir = false;
}

void XboxStructureActionGenerateBox::writeAttributes(DataOutputStream *dos, UINT numAttrs)
{
    ConsoleGenerateStructureAction::writeAttributes(dos, numAttrs + 9);

    ::write(dos, GameRuleAttribute::x0);
    dos->writeUTF(_toString(m_x0));
    ::write(dos, GameRuleAttribute::y0);
    dos->writeUTF(_toString(m_y0));
    ::write(dos, GameRuleAttribute::z0);
    dos->writeUTF(_toString(m_z0));

    ::write(dos, GameRuleAttribute::x1);
    dos->writeUTF(_toString(m_x1));
    ::write(dos, GameRuleAttribute::y1);
    dos->writeUTF(_toString(m_y1));
    ::write(dos, GameRuleAttribute::z1);
    dos->writeUTF(_toString(m_z1));

    ::write(dos, GameRuleAttribute::edgeTile);
    dos->writeUTF(_toString(m_edgeTile));
    ::write(dos, GameRuleAttribute::fillTile);
    dos->writeUTF(_toString(m_fillTile));
    ::write(dos, GameRuleAttribute::skipAir);
    dos->writeUTF(_toString(m_skipAir));
}

void XboxStructureActionGenerateBox::addAttribute(const wstring &attributeName, const wstring &attributeValue)
{
    if (attributeName.compare(L"x0") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_x0 = value;
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter x0=%d\n", m_x0);
    }
    else if (attributeName.compare(L"y0") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_y0 = value;
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter y0=%d\n", m_y0);
    }
    else if (attributeName.compare(L"z0") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_z0 = value;
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter z0=%d\n", m_z0);
    }
    else if (attributeName.compare(L"x1") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_x1 = value;
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter x1=%d\n", m_x1);
    }
    else if (attributeName.compare(L"y1") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_y1 = value;
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter y1=%d\n", m_y1);
    }
    else if (attributeName.compare(L"z1") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_z1 = value;
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter z1=%d\n", m_z1);
    }
    else if (attributeName.compare(L"edgeTile") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_edgeTile = value;
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter edgeTile=%d\n", m_edgeTile);
    }
    else if (attributeName.compare(L"fillTile") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_fillTile = value;
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter fillTile=%d\n", m_fillTile);
    }
    else if (attributeName.compare(L"skipAir") == 0)
    {
        if (attributeValue.compare(L"true") == 0)
        {
            m_skipAir = true;
        }
        app.DebugPrintf("XboxStructureActionGenerateBox: Adding parameter skipAir=%s\n", m_skipAir ? "TRUE" : "FALSE");
    }
    else
    {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool XboxStructureActionGenerateBox::generateBoxInLevel(StructurePiece *structure, Level *level, BoundingBox *chunkBB)
{
    app.DebugPrintf("XboxStructureActionGenerateBox - generating a box\n");
    structure->generateBox(level, chunkBB, m_x0, m_y0, m_z0, m_x1, m_y1, m_z1, m_edgeTile, m_fillTile, m_skipAir);
    return true;
}
