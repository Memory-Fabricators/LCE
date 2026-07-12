#pragma once
#include "ConsoleGameRulesConstants.h"
using namespace std;

#include "LevelGenerators.h"
#include "LevelRules.h"
class LevelGenerationOptions;
class RootGameRulesDefinition;
class LevelChunk;
class DLCPack;
class DLCGameRulesFile;
class DLCGameRulesHeader;
class StringTable;
class GameRuleDefinition;
class DataInputStream;
class DataOutputStream;
class WstringLookup;

#define GAME_RULE_SAVENAME L"requiredGameRules.grf"

// 4J-JEV:
#define LEVEL_GEN_ID int
#define LEVEL_GEN_ID_NULL 0

class GameRuleManager
{
  public:
    static const WCHAR *wchTagNameA[static_cast<int>(GameRuleAttribute::Count)];
    static const WCHAR *wchAttrNameA[static_cast<int>(GameRuleAttribute::Count)];

    static const short version_number = 2;

  private:
    LevelGenerationOptions *m_currentLevelGenerationOptions;
    LevelRuleset *m_currentGameRuleDefinitions;
    LevelGenerators m_levelGenerators;
    LevelRules m_levelRules;

  public:
    GameRuleManager();

    void loadGameRules(DLCPack *);

    LevelGenerationOptions *loadGameRules(unsigned char *dIn, UINT dSize);
    void loadGameRules(LevelGenerationOptions *lgo, unsigned char *dIn, UINT dSize);

    void saveGameRules(unsigned char **dOut, UINT *dSize);

  private:
    LevelGenerationOptions *readHeader(DLCGameRulesHeader *grh);

    void writeRuleFile(DataOutputStream *dos);

  public:
    bool readRuleFile(LevelGenerationOptions *lgo, unsigned char *dIn, UINT dSize, StringTable *strings); //(DLCGameRulesFile *dlcFile, StringTable *strings);

  private:
    void readAttributes(DataInputStream *dis, vector<wstring> *tagsAndAtts, GameRuleDefinition *rule);
    void readChildren(DataInputStream *dis, vector<wstring> *tagsAndAtts, unordered_map<int, GameRuleType> *tagIdMap, GameRuleDefinition *rule);

  public:
    void processSchematics(LevelChunk *levelChunk);
    void processSchematicsLighting(LevelChunk *levelChunk);
    void loadDefaultGameRules();

  private:
    bool loadGameRulesPack(File *path);

    LEVEL_GEN_ID addLevelGenerationOptions(LevelGenerationOptions *);

  public:
    vector<LevelGenerationOptions *> *getLevelGenerators()
    {
        return m_levelGenerators.getLevelGenerators();
    }
    void setLevelGenerationOptions(LevelGenerationOptions *levelGen);
    LevelRuleset *getGameRuleDefinitions()
    {
        return m_currentGameRuleDefinitions;
    }
    LevelGenerationOptions *getLevelGenerationOptions()
    {
        return m_currentLevelGenerationOptions;
    }
    LPCWSTR GetGameRulesString(const wstring &key);

    // 4J-JEV:
    // Properly cleans-up and unloads the current set of gameRules.
    void unloadCurrentGameRules();
};
