#include "StartFeature.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "GameRules/ConsoleGameRulesConstants.h"
#include "stdafx.h"

StartFeature::StartFeature()
{
    m_chunkX = 0;
    m_chunkZ = 0;
    m_feature = StructureFeature::eFeature_Temples;
}

void StartFeature::writeAttributes(DataOutputStream *dos, UINT numAttrs)
{
    GameRuleDefinition::writeAttributes(dos, numAttrs + 3);

    ::write(dos, GameRuleAttribute::chunkX);
    dos->writeUTF(_toString(m_chunkX));
    ::write(dos, GameRuleAttribute::chunkZ);
    dos->writeUTF(_toString(m_chunkZ));
    ::write(dos, GameRuleAttribute::feature);
    dos->writeUTF(_toString((int)m_feature));
}

void StartFeature::addAttribute(const wstring &attributeName, const wstring &attributeValue)
{
    if (attributeName.compare(L"chunkX") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_chunkX = value;
        app.DebugPrintf("StartFeature: Adding parameter chunkX=%d\n", m_chunkX);
    }
    else if (attributeName.compare(L"chunkZ") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_chunkZ = value;
        app.DebugPrintf("StartFeature: Adding parameter chunkZ=%d\n", m_chunkZ);
    }
    else if (attributeName.compare(L"feature") == 0)
    {
        int value = _fromString<int>(attributeValue);
        m_feature = (StructureFeature::EFeatureTypes)value;
        app.DebugPrintf("StartFeature: Adding parameter feature=%d\n", m_feature);
    }
    else
    {
        GameRuleDefinition::addAttribute(attributeName, attributeValue);
    }
}

bool StartFeature::isFeatureChunk(int chunkX, int chunkZ, StructureFeature::EFeatureTypes feature)
{
    return chunkX == m_chunkX && chunkZ == m_chunkZ && feature == m_feature;
}
