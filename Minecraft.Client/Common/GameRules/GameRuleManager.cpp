#include "GameRuleManager.h"
#include "../../../Minecraft.World/File.h"
#include "../../../Minecraft.World/StringHelpers.h"
#include "../../../Minecraft.World/compression.h"
#include "../../StringTable.h"
#include "../DLC/DLCGameRules.h"
#include "../DLC/DLCGameRulesFile.h"
#include "../DLC/DLCGameRulesHeader.h"
#include "../DLC/DLCLocalisationFile.h"
#include "../DLC/DLCPack.h"
#include "ConsoleGameRules.h"
#include "stdafx.h"

const WCHAR *GameRuleManager::wchTagNameA[] =
    {
        L"",                  // Root
        L"MapOptions",        // LevelGenerationOptions
        L"ApplySchematic",    // ApplySchematic
        L"GenerateStructure", // GenerateStructure
        L"GenerateBox",       // GenerateBox
        L"PlaceBlock",        // PlaceBlock
        L"PlaceContainer",    // PlaceContainer
        L"PlaceSpawner",      // PlaceSpawner
        L"BiomeOverride",     // BiomeOverride
        L"StartFeature",      // StartFeature
        L"AddItem",           // AddItem
        L"AddEnchantment",    // AddEnchantment
        L"LevelRules",        // LevelRules
        L"NamedArea",         // NamedArea
        L"UseTile",           // UseTileRule
        L"CollectItem",       // CollectItemRule
        L"CompleteAll",       // CompleteAllRule
        L"UpdatePlayer",      // UpdatePlayerRule
};

const WCHAR *GameRuleManager::wchAttrNameA[] =
    {
        L"descriptionName",  // descriptionName
        L"promptName",       // promptName
        L"dataTag",          // dataTag
        L"enchantmentId",    // enchantmentId
        L"enchantmentLevel", // enchantmentLevel
        L"itemId",           // itemId
        L"quantity",         // quantity
        L"auxValue",         // auxValue
        L"slot",             // slot
        L"name",             // name
        L"food",             // food
        L"health",           // health
        L"tileId",           // tileId
        L"useCoords",        // useCoords
        L"seed",             // seed
        L"flatworld",        // flatworld
        L"filename",         // filename
        L"rot",
        L"data",     // data
        L"block",    // block
        L"entity",   // entity
        L"facing",   // facing
        L"edgeTile", // edgeTile
        L"fillTile", // fillTile
        L"skipAir",  // skipAir
        L"x",        // x
        L"x0",       // x0
        L"x1",       // x1
        L"y",        // y
        L"y0",       // y0
        L"y1",       // y1
        L"z",        // z
        L"z0",       // z0
        L"z1",       // z1
        L"chunkX",   // chunkX
        L"chunkZ",   // chunkZ
        L"yRot",     // yRot
        L"spawnX",   // spawnX
        L"spawnY",   // spawnY
        L"spawnZ",   // spawnZ
        L"orientation",
        L"dimension",
        L"topTileId", // topTileId
        L"biomeId",   // biomeId
        L"feature",   // feature
};

GameRuleManager::GameRuleManager()
{
    m_currentGameRuleDefinitions = NULL;
    m_currentLevelGenerationOptions = NULL;
}

void GameRuleManager::loadGameRules(DLCPack *pack)
{
    StringTable *strings = NULL;

    if (pack->doesPackContainFile(DLCManager::e_DLCType_LocalisationData, L"languages.loc"))
    {
        DLCLocalisationFile *localisationFile = (DLCLocalisationFile *)pack->getFile(DLCManager::e_DLCType_LocalisationData, L"languages.loc");
        strings = localisationFile->getStringTable();
    }

    int gameRulesCount = pack->getDLCItemsCount(DLCManager::e_DLCType_GameRulesHeader);
    for (int i = 0; i < gameRulesCount; ++i)
    {
        DLCGameRulesHeader *dlcHeader = (DLCGameRulesHeader *)pack->getFile(DLCManager::e_DLCType_GameRulesHeader, i);
        DWORD dSize;
        auto *dData = dlcHeader->getData(dSize);

        LevelGenerationOptions *createdLevelGenerationOptions = new LevelGenerationOptions();
        //	= loadGameRules(dData, dSize); //, strings);

        createdLevelGenerationOptions->setGrSource(dlcHeader);

        readRuleFile(createdLevelGenerationOptions, dData, dSize, strings);

        createdLevelGenerationOptions->setSrc(LevelGenerationOptions::eSrc_fromDLC);

        // createdLevelGenerationOptions->setSrc( LevelGenerationOptions::eSrc_fromDLC );
        dlcHeader->lgo = createdLevelGenerationOptions;
    }

    gameRulesCount = pack->getDLCItemsCount(DLCManager::e_DLCType_GameRules);
    for (int i = 0; i < gameRulesCount; ++i)
    {
        DLCGameRulesFile *dlcFile = (DLCGameRulesFile *)pack->getFile(DLCManager::e_DLCType_GameRules, i);

        DWORD dSize;
        auto *dData = dlcFile->getData(dSize);

        LevelGenerationOptions *createdLevelGenerationOptions = new LevelGenerationOptions();
        //	= loadGameRules(dData, dSize); //, strings);

        createdLevelGenerationOptions->setGrSource(new JustGrSource());
        readRuleFile(createdLevelGenerationOptions, dData, dSize, strings);

        createdLevelGenerationOptions->setSrc(LevelGenerationOptions::eSrc_tutorial);

        // createdLevelGenerationOptions->set_DLCGameRulesFile( dlcFile );

        createdLevelGenerationOptions->setLoadedData();
    }
}

LevelGenerationOptions *GameRuleManager::loadGameRules(unsigned char *dIn, UINT dSize)
{
    LevelGenerationOptions *lgo = new LevelGenerationOptions();
    lgo->setGrSource(new JustGrSource());
    lgo->setSrc(LevelGenerationOptions::eSrc_fromSave);
    loadGameRules(lgo, dIn, dSize);
    lgo->setLoadedData();
    return lgo;
}

// 4J-JEV: Reverse of saveGameRules.
void GameRuleManager::loadGameRules(LevelGenerationOptions *lgo, unsigned char *dIn, UINT dSize)
{
    app.DebugPrintf("GameRuleManager::LoadingGameRules:\n");

    ByteArrayInputStream bais(byteArray(dIn, dSize));
    DataInputStream dis(&bais);

    // Read file header.

    // dis.readInt(); // File Size

    short version = dis.readShort();
    assert(0x1 == version);
    app.DebugPrintf("\tversion=%d.\n", version);

    for (int i = 0; i < 8; i++)
    {
        dis.readByte();
    }

    BYTE compression_type = dis.readByte();

    app.DebugPrintf("\tcompressionType=%d.\n", compression_type);

    UINT compr_len, decomp_len;
    compr_len = dis.readInt();
    decomp_len = dis.readInt();

    app.DebugPrintf("\tcompr_len=%d.\n\tdecomp_len=%d.\n", compr_len, decomp_len);

    // Decompress File Body

    byteArray content(new BYTE[decomp_len], decomp_len),
        compr_content(new BYTE[compr_len], compr_len);
    dis.read(compr_content);

    Compression::getCompression()->SetDecompressionType((Compression::ECompressionTypes)compression_type);
    Compression::getCompression()->DecompressLZXRLE(content.data, &content.length,
                                                    compr_content.data, compr_content.length);
    Compression::getCompression()->SetDecompressionType(SAVE_FILE_PLATFORM_LOCAL);

    dis.close();
    bais.close();

    delete[] compr_content.data;

    ByteArrayInputStream bais2(content);
    DataInputStream dis2(&bais2);

    // Read StringTable.
    byteArray bStringTable;
    bStringTable.length = dis2.readInt();
    bStringTable.data = new BYTE[bStringTable.length];
    dis2.read(bStringTable);
    StringTable *strings = new StringTable(bStringTable.data, bStringTable.length);

    // Read RuleFile.
    byteArray bRuleFile;
    bRuleFile.length = content.length - bStringTable.length;
    bRuleFile.data = new BYTE[bRuleFile.length];
    dis2.read(bRuleFile);

    // 4J-JEV: I don't believe that the path-name is ever used.
    // DLCGameRulesFile *dlcgr = new DLCGameRulesFile(L"__PLACEHOLDER__");
    // dlcgr->addData(bRuleFile.data,bRuleFile.length);

    if (readRuleFile(lgo, bRuleFile.data, bRuleFile.length, strings))
    {
        // Set current gen options and ruleset.
        // createdLevelGenerationOptions->setFromSaveGame(true);
        lgo->setSrc(LevelGenerationOptions::eSrc_fromSave);
        setLevelGenerationOptions(lgo);
        // m_currentGameRuleDefinitions = lgo->getRequiredGameRules();
    }
    else
    {
        delete lgo;
    }

    // delete [] content.data;

    // Close and return.
    dis2.close();
    bais2.close();

    return;
}

// 4J-JEV: Reverse of loadGameRules.
void GameRuleManager::saveGameRules(unsigned char **dOut, UINT *dSize)
{
    if (m_currentGameRuleDefinitions == NULL &&
        m_currentLevelGenerationOptions == NULL)
    {
        app.DebugPrintf("GameRuleManager:: Nothing here to save.");
        *dOut = NULL;
        *dSize = 0;
        return;
    }

    app.DebugPrintf("GameRuleManager::saveGameRules:\n");

    // Initialise output stream.
    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);

    // Write header.

    // VERSION NUMBER
    dos.writeShort(0x1); // version_number

    // Write 8 bytes of empty space in case we need them later.
    // Mainly useful for the ones we save embedded in game saves.
    for (UINT i = 0; i < 8; i++)
    {
        dos.writeByte(0x0);
    }

    dos.writeByte(APPROPRIATE_COMPRESSION_TYPE); // m_compressionType

    // -- START COMPRESSED -- //
    ByteArrayOutputStream compr_baos;
    DataOutputStream compr_dos(&compr_baos);

    if (m_currentGameRuleDefinitions == NULL)
    {
        compr_dos.writeInt(0); // numStrings for StringTable
        compr_dos.writeInt(version_number);
        compr_dos.writeByte(Compression::eCompressionType_None); // compression type
        for (int i = 0; i < 2; i++)
        {
            compr_dos.writeByte(0x0); // Padding.
        }
        compr_dos.writeInt(0); // StringLookup.length
        compr_dos.writeInt(0); // SchematicFiles.length
        compr_dos.writeInt(0); // XmlObjects.length
    }
    else
    {
        StringTable *st = m_currentGameRuleDefinitions->getStringTable();

        if (st == NULL)
        {
            app.DebugPrintf("GameRuleManager::saveGameRules: StringTable == NULL!");
        }
        else
        {
            // Write string table.
            byteArray stba;
            m_currentGameRuleDefinitions->getStringTable()->getData(&stba.data, &stba.length);
            compr_dos.writeInt(stba.length);
            compr_dos.write(stba);

            // Write game rule file to second
            // buffer and generate string lookup.
            writeRuleFile(&compr_dos);
        }
    }

    // Compress compr_dos and write to dos.
    byteArray compr_ba(new BYTE[compr_baos.buf.length], compr_baos.buf.length);
    Compression::getCompression()->CompressLZXRLE(compr_ba.data, &compr_ba.length,
                                                  compr_baos.buf.data, compr_baos.buf.length);

    app.DebugPrintf("\tcompr_ba.length=%d.\n\tcompr_baos.buf.length=%d.\n",
                    compr_ba.length, compr_baos.buf.length);

    dos.writeInt(compr_ba.length); // Write length
    dos.writeInt(compr_baos.buf.length);
    dos.write(compr_ba);

    delete[] compr_ba.data;

    compr_dos.close();
    compr_baos.close();
    // -- END COMPRESSED -- //

    // return
    *dSize = baos.buf.length;
    *dOut = baos.buf.data;

    baos.buf.data = NULL;

    dos.close();
    baos.close();
}

// 4J-JEV: Reverse of readRuleFile.
void GameRuleManager::writeRuleFile(DataOutputStream *dos)
{
    // Write Header
    dos->writeShort(version_number);                    // Version number.
    dos->writeByte(Compression::eCompressionType_None); // compression type
    for (int i = 0; i < 8; i++)
    {
        dos->writeBoolean(false); // Padding.
    }

    // Write string lookup.
    int numStrings = static_cast<int>(GameRuleType::Count) + static_cast<int>(GameRuleType::Count);
    dos->writeInt(numStrings);
    for (int i = 0; i < static_cast<int>(GameRuleType::Count); i++)
    {
        dos->writeUTF(wchTagNameA[i]);
    }
    for (int i = 0; i < static_cast<int>(GameRuleType::Count); i++)
    {
        dos->writeUTF(wchAttrNameA[i]);
    }

    // Write schematic files.
    unordered_map<wstring, ConsoleSchematicFile *> *files;
    files = getLevelGenerationOptions()->getUnfinishedSchematicFiles();
    dos->writeInt(files->size());
    for (AUTO_VAR(it, files->begin()); it != files->end(); it++)
    {
        wstring filename = it->first;
        ConsoleSchematicFile *file = it->second;

        ByteArrayOutputStream fileBaos;
        DataOutputStream fileDos(&fileBaos);
        file->save(&fileDos);

        dos->writeUTF(filename);
        // dos->writeInt(file->m_data.length);
        dos->writeInt(fileBaos.buf.length);
        dos->write((byteArray)fileBaos.buf);

        fileDos.close();
        fileBaos.close();
    }

    // Write xml objects.
    dos->writeInt(2); // numChildren
    m_currentLevelGenerationOptions->write(dos);
    m_currentGameRuleDefinitions->write(dos);
}

bool GameRuleManager::readRuleFile(LevelGenerationOptions *lgo, unsigned char *dIn, UINT dSize, StringTable *strings) //(DLCGameRulesFile *dlcFile, StringTable *strings)
{
    bool levelGenAdded = false;
    bool gameRulesAdded = false;
    LevelGenerationOptions *levelGenerator = lgo; // new LevelGenerationOptions();
    LevelRuleset *gameRules = new LevelRuleset();

    // DWORD dwLen = 0;
    // PBYTE pbData = dlcFile->getData(dwLen);
    // byteArray data(pbData,dwLen);

    byteArray data(dIn, dSize);
    ByteArrayInputStream bais(data);
    DataInputStream dis(&bais);

    // Read File.

    // version_number
    std::int64_t version = dis.readShort();
    unsigned char compressionType = 0;
    if (version == 0)
    {
        for (int i = 0; i < 14; i++)
        {
            dis.readByte(); // Read padding.
        }
    }
    else
    {
        compressionType = dis.readByte();

        // Read the spare bytes we inserted for future use
        for (int i = 0; i < 8; ++i)
        {
            dis.readBoolean();
        }
    }

    ByteArrayInputStream *contentBais = NULL;
    DataInputStream *contentDis = NULL;

    if (compressionType == Compression::eCompressionType_None)
    {
        // No compression
        // No need to read buffer size, as we can read the stream as it is;
        app.DebugPrintf("De-compressing game rules with: None\n");
        contentDis = &dis;
    }
    else
    {
        unsigned int uncompressedSize = dis.readInt();
        unsigned int compressedSize = dis.readInt();
        byteArray compressedBuffer(compressedSize);
        dis.read(compressedBuffer);

        byteArray decompressedBuffer = byteArray(uncompressedSize);

        switch (compressionType)
        {
        case Compression::eCompressionType_None:
            memcpy(decompressedBuffer.data, compressedBuffer.data, uncompressedSize);
            break;

        case Compression::eCompressionType_RLE:
            app.DebugPrintf("De-compressing game rules with: RLE\n");
            Compression::getCompression()->Decompress(decompressedBuffer.data, &decompressedBuffer.length, compressedBuffer.data, compressedSize);
            break;

        default:
            app.DebugPrintf("De-compressing game rules.");
#ifndef _CONTENT_PACKAGE
            // A mismatch here means this platform's build is reading
            // game-rule data encoded for a different platform (see
            // APPROPRIATE_COMPRESSION_TYPE in compression.h) - almost
            // certainly a packaging/build config bug rather than something
            // recoverable per-call. Still, an assert()-triggered abort() is
            // a poor way to surface that: log it clearly and keep going with
            // whatever compressionType byte was actually read (best effort,
            // same as a non-debug build already does), instead of crashing
            // the whole process.
            if (compressionType != APPROPRIATE_COMPRESSION_TYPE)
            {
                app.DebugPrintf("GameRuleManager:: compressionType %d does not match this "
                                "platform's expected %d (APPROPRIATE_COMPRESSION_TYPE) - "
                                "data may have been packaged for a different platform.\n",
                                (int)compressionType, (int)APPROPRIATE_COMPRESSION_TYPE);
            }
#endif
            // DLC game-rule packs retain the Windows64 ZLIB+RLE encoding even
            // on SDL. Select it only for this payload; network/chunk data
            // continues to use the normal local LZX+RLE codec.
            Compression *compression = Compression::getCompression();
            Compression::ECompressionTypes previousType = compression->GetDecompressionType();
            compression->SetDecompressionType((Compression::ECompressionTypes)compressionType);
            compression->DecompressLZXRLE(decompressedBuffer.data, &decompressedBuffer.length,
                                          compressedBuffer.data, compressedSize);
            compression->SetDecompressionType(previousType);
            break;
            /* 4J-JEV:
                Each platform has only 1 method of compression, 'compression.h' file deals with it.

                    case Compression::eCompressionType_LZXRLE:
                        app.DebugPrintf("De-compressing game rules with: LZX+RLE\n");
                        Compression::getCompression()->DecompressLZXRLE( decompressedBuffer.data, &uncompressedSize, compressedBuffer.data, compressedSize);
                        break;
                    default:
                        app.DebugPrintf("Invalid compression type %d found\n", compressionType);
                        __debugbreak();

                        delete [] compressedBuffer.data; delete [] decompressedBuffer.data;
                        dis.close(); bais.reset();

                        if(!gameRulesAdded) delete gameRules;
                        return false;
                        */
        };

        delete[] compressedBuffer.data;

        contentBais = new ByteArrayInputStream(decompressedBuffer);
        contentDis = new DataInputStream(contentBais);
    }

    // String-table count is the first field of the decompressed payload. A
    // valid Tutorial.pck has 54 entries; reject corrupt decompression before
    // readUTF can turn a bogus length into unbounded allocations.
    UINT numStrings = contentDis->readInt();
    if (numStrings > 4096)
    {
        app.DebugPrintf("Invalid game-rule string-table count: %u\n", numStrings);
        return false;
    }
    vector<wstring> tagsAndAtts;
    for (UINT i = 0; i < numStrings; i++)
    {
        tagsAndAtts.push_back(contentDis->readUTF());
    }

    unordered_map<int, GameRuleType> tagIdMap;
    for (int type = (int)GameRuleType::Root; type < (int)GameRuleType::Count; ++type)
    {
        for (UINT i = 0; i < numStrings; ++i)
        {
            if (tagsAndAtts[i].compare(wchTagNameA[type]) == 0)
            {
                tagIdMap.insert(unordered_map<int, GameRuleType>::value_type(i, (GameRuleType)type));
                break;
            }
        }
    }

    // 4J-JEV: TODO: As yet unused.
    /*
    unordered_map<int, GameRuleType::EGameRuleAttr> attrIdMap;
    for(int attr = (int)GameRuleType::descriptionName; attr < (int)GameRuleType::Count; ++attr)
    {
        for (UINT i = 0; i < numStrings; i++)
        {
            if (tagsAndAtts[i].compare(wchAttrNameA[attr]) == 0)
            {
                tagIdMap.insert( unordered_map<int, GameRuleType::EGameRuleAttr>::value_type(i , (GameRuleType::EGameRuleAttr)attr) );
                break;
            }
        }
    }*/

    // subfile
    UINT numFiles = contentDis->readInt();
    for (UINT i = 0; i < numFiles; i++)
    {
        wstring sFilename = contentDis->readUTF();
        int length = contentDis->readInt();
        byteArray ba(length);

        contentDis->read(ba);

        levelGenerator->loadSchematicFile(sFilename, ba.data, ba.length);
    }

    LEVEL_GEN_ID lgoID = LEVEL_GEN_ID_NULL;

    // xml objects
    UINT numObjects = contentDis->readInt();
    for (UINT i = 0; i < numObjects; ++i)
    {
        int tagId = contentDis->readInt();
        GameRuleType tagVal = GameRuleType::Invalid;
        AUTO_VAR(it, tagIdMap.find(tagId));
        if (it != tagIdMap.end())
        {
            tagVal = it->second;
        }

        GameRuleDefinition *rule = NULL;

        if (tagVal == GameRuleType::LevelGenerationOptions)
        {
            rule = levelGenerator;
            levelGenAdded = true;
            // m_levelGenerators.addLevelGenerator(L"",levelGenerator);
            lgoID = addLevelGenerationOptions(levelGenerator);
            levelGenerator->loadStringTable(strings);
        }
        else if (tagVal == GameRuleType::LevelRules)
        {
            rule = gameRules;
            gameRulesAdded = true;
            m_levelRules.addLevelRule(L"", gameRules);
            levelGenerator->setRequiredGameRules(gameRules);
            gameRules->loadStringTable(strings);
        }

        readAttributes(contentDis, &tagsAndAtts, rule);
        readChildren(contentDis, &tagsAndAtts, &tagIdMap, rule);
    }

    if (compressionType != 0)
    {
        // Not default
        contentDis->close();
        if (contentBais != NULL)
        {
            delete contentBais;
        }
        delete contentDis;
    }

    dis.close();
    bais.reset();

    // if(!levelGenAdded) { delete levelGenerator; levelGenerator = NULL; }
    if (!gameRulesAdded)
    {
        delete gameRules;
    }

    return true;
    // return levelGenerator;
}

LevelGenerationOptions *GameRuleManager::readHeader(DLCGameRulesHeader *grh)
{
    LevelGenerationOptions *out =
        new LevelGenerationOptions();

    out->setSrc(LevelGenerationOptions::eSrc_fromDLC);
    out->setGrSource(grh);
    addLevelGenerationOptions(out);

    return out;
}

void GameRuleManager::readAttributes(DataInputStream *dis, vector<wstring> *tagsAndAtts, GameRuleDefinition *rule)
{
    int numAttrs = dis->readInt();
    for (UINT att = 0; att < numAttrs; ++att)
    {
        int attID = dis->readInt();
        wstring value = dis->readUTF();

        if (rule != NULL)
        {
            rule->addAttribute(tagsAndAtts->at(attID), value);
        }
    }
}

void GameRuleManager::readChildren(DataInputStream *dis, vector<wstring> *tagsAndAtts, unordered_map<int, GameRuleType> *tagIdMap, GameRuleDefinition *rule)
{
    int numChildren = dis->readInt();
    for (UINT child = 0; child < numChildren; ++child)
    {
        int tagId = dis->readInt();
        GameRuleType tagVal = GameRuleType::Invalid;
        AUTO_VAR(it, tagIdMap->find(tagId));
        if (it != tagIdMap->end())
        {
            tagVal = it->second;
        }

        GameRuleDefinition *childRule = NULL;
        if (rule != NULL)
        {
            childRule = rule->addChild(tagVal);
        }

        readAttributes(dis, tagsAndAtts, childRule);
        readChildren(dis, tagsAndAtts, tagIdMap, childRule);
    }
}

void GameRuleManager::processSchematics(LevelChunk *levelChunk)
{
    if (getLevelGenerationOptions() != NULL)
    {
        LevelGenerationOptions *levelGenOptions = getLevelGenerationOptions();
        levelGenOptions->processSchematics(levelChunk);
    }
}

void GameRuleManager::processSchematicsLighting(LevelChunk *levelChunk)
{
    if (getLevelGenerationOptions() != NULL)
    {
        LevelGenerationOptions *levelGenOptions = getLevelGenerationOptions();
        levelGenOptions->processSchematicsLighting(levelChunk);
    }
}

void GameRuleManager::loadDefaultGameRules()
{
#ifdef _XBOX
#ifdef _TU_BUILD
    wstring fileRoot = L"UPDATE:\\res\\GameRules\\Tutorial.pck";
#else
    wstring fileRoot = L"GAME:\\res\\TitleUpdate\\GameRules\\Tutorial.pck";
#endif
    File packedTutorialFile(fileRoot);
    if (loadGameRulesPack(&packedTutorialFile))
    {
        m_levelGenerators.getLevelGenerators()->at(0)->setWorldName(app.GetString(IDS_PLAY_TUTORIAL));
        // m_levelGenerators.getLevelGenerators()->at(0)->setDefaultSaveName(L"Tutorial");
        m_levelGenerators.getLevelGenerators()->at(0)->setDefaultSaveName(app.GetString(IDS_TUTORIALSAVENAME));
    }

#ifndef _CONTENT_PACKAGE
    // 4J Stu - Remove these just now
    // File testRulesPath(L"GAME:\\GameRules");
    // vector<File *> *packFiles = testRulesPath.listFiles();

    // for(AUTO_VAR(it,packFiles->begin()); it != packFiles->end(); ++it)
    //{
    //	loadGameRulesPack(*it);
    // }
    // delete packFiles;
#endif

#else // _XBOX

    wstring fpTutorial = L"Tutorial.pck";
    if (app.getArchiveFileSize(fpTutorial) >= 0)
    {
        DLCPack *pack = new DLCPack(L"", 0xffffffff);
        DWORD dwFilesProcessed = 0;
        if (app.m_dlcManager.readDLCDataFile(dwFilesProcessed, fpTutorial, pack, true))
        {
            app.m_dlcManager.addPack(pack);
            m_levelGenerators.getLevelGenerators()->at(0)->setWorldName(app.GetString(IDS_PLAY_TUTORIAL));
            m_levelGenerators.getLevelGenerators()->at(0)->setDefaultSaveName(app.GetString(IDS_TUTORIALSAVENAME));
        }
        else
        {
            delete pack;
        }
    }
    /*StringTable *strings = new StringTable(baStrings.data, baStrings.length);
    LevelGenerationOptions *lgo = new LevelGenerationOptions();
    lgo->setGrSource( new JustGrSource() );
    lgo->setSrc( LevelGenerationOptions::eSrc_tutorial );
    readRuleFile(lgo, tutorial.data, tutorial.length, strings);
    lgo->setLoadedData();*/

#endif
}

bool GameRuleManager::loadGameRulesPack(File *path)
{
    bool success = false;
#ifdef _XBOX
    if (path->exists())
    {
        DLCPack *pack = new DLCPack(L"", 0xffffffff);
        DWORD dwFilesProcessed = 0;
        if (app.m_dlcManager.readDLCDataFile(dwFilesProcessed, path->getPath(), pack))
        {
            app.m_dlcManager.addPack(pack);
            success = true;
        }
        else
        {
            delete pack;
        }
    }
#endif
    return success;
}

void GameRuleManager::setLevelGenerationOptions(LevelGenerationOptions *levelGen)
{
    m_currentGameRuleDefinitions = NULL;
    m_currentLevelGenerationOptions = levelGen;

    if (m_currentLevelGenerationOptions != NULL && m_currentLevelGenerationOptions->requiresGameRules())
    {
        m_currentGameRuleDefinitions = m_currentLevelGenerationOptions->getRequiredGameRules();
    }

    if (m_currentLevelGenerationOptions != NULL)
    {
        m_currentLevelGenerationOptions->reset_start();
    }
}

LPCWSTR GameRuleManager::GetGameRulesString(const wstring &key)
{
    if (m_currentGameRuleDefinitions != NULL && !key.empty())
    {
        return m_currentGameRuleDefinitions->getString(key);
    }
    else
    {
        return L"";
    }
}

LEVEL_GEN_ID GameRuleManager::addLevelGenerationOptions(LevelGenerationOptions *lgo)
{
    vector<LevelGenerationOptions *> *lgs = m_levelGenerators.getLevelGenerators();

    for (int i = 0; i < lgs->size(); i++)
    {
        if (lgs->at(i) == lgo)
        {
            return i;
        }
    }

    lgs->push_back(lgo);
    return lgs->size() - 1;
}

void GameRuleManager::unloadCurrentGameRules()
{
    if (m_currentLevelGenerationOptions != NULL)
    {
        if (m_currentGameRuleDefinitions != NULL && m_currentLevelGenerationOptions->isFromSave())
        {
            m_levelRules.removeLevelRule(m_currentGameRuleDefinitions);
        }

        if (m_currentLevelGenerationOptions->isFromSave())
        {
            m_levelGenerators.removeLevelGenerator(m_currentLevelGenerationOptions);

            delete m_currentLevelGenerationOptions;
        }
        else if (m_currentLevelGenerationOptions->isFromDLC())
        {
            m_currentLevelGenerationOptions->reset_finish();
        }
    }

    m_currentGameRuleDefinitions = NULL;
    m_currentLevelGenerationOptions = NULL;
}
