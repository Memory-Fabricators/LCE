#include "StringTable.h"
#include "stdafx.h"

StringTable::StringTable(void)
{
}

// Load string table from a binary blob, filling out with the current localisation data only
StringTable::StringTable(PBYTE pbData, DWORD dwSize)
{
    src = byteArray(pbData, dwSize);

    ByteArrayInputStream bais(src);
    DataInputStream dis(&bais);

    int versionNumber = dis.readInt();
    int languagesCount = dis.readInt();

    vector<pair<wstring, int>> langSizeMap;
    for (int i = 0; i < languagesCount; ++i)
    {
        wstring langId = dis.readUTF();
        int langSize = dis.readInt();

        langSizeMap.push_back(vector<pair<wstring, int>>::value_type(langId, langSize));
    }

    vector<wstring> locales;
    app.getLocale(locales);

    bool foundLang = false;
    std::int64_t bytesToSkip = 0;
    int dataSize = 0;

    //
    for (AUTO_VAR(it_locales, locales.begin());
         it_locales != locales.end() && (!foundLang);
         it_locales++)
    {
        bytesToSkip = 0;

        for (AUTO_VAR(it, langSizeMap.begin()); it != langSizeMap.end(); ++it)
        {
            if (it->first.compare(*it_locales) == 0)
            {
                app.DebugPrintf("StringTable:: Found language '%ls'.\n", it_locales->c_str());
                dataSize = it->second;
                foundLang = true;
                break;
            }

            bytesToSkip += it->second;
        }

        if (!foundLang)
        {
            app.DebugPrintf("StringTable:: Can't find language '%ls'.\n", it_locales->c_str());
        }
    }

    if (foundLang)
    {
        dis.skip(bytesToSkip);

        byteArray langData(dataSize);
        dis.read(langData);

        dis.close();

        ByteArrayInputStream bais2(langData);
        DataInputStream dis2(&bais2);

        // Read the language file for the selected language
        int langVersion = dis2.readInt();

        isStatic = false;    // 4J-JEV: Versions 1 and up could use
        if (langVersion > 0) // integers rather than wstrings as keys.
        {
            isStatic = dis2.readBoolean();
        }

        wstring langId = dis2.readUTF();
        int totalStrings = dis2.readInt();

        if (!isStatic)
        {
            for (int i = 0; i < totalStrings; ++i)
            {
                wstring stringId = dis2.readUTF();
                wstring stringValue = dis2.readUTF();

                m_stringsMap.insert(unordered_map<wstring, wstring>::value_type(stringId, stringValue));
            }
        }
        else
        {
            for (int i = 0; i < totalStrings; ++i)
            {
                m_stringsVec.push_back(dis2.readUTF());
            }
        }
        dis2.close();

        // We can't delete this data in the dtor, so clear the reference
        bais2.reset();
    }
    else
    {
        // No locale in getLocale()'s list (see CMinecraftApp::getLocale())
        // matched an entry actually present in this archive's
        // languages.loc. That list always ends with the eMCLang_enUS/
        // eMCLang_null ("en-EN") universal fallbacks, so reaching here means
        // the archive itself is missing even those - a corrupt or unexpected
        // media archive, not something the game can recover string data
        // from. __debugbreak() used to fire unconditionally in debug
        // builds, turning this into a hard SIGILL crash the moment it
        // happened; log clearly and keep running with no translated
        // strings instead (getString() below already returns L"" when
        // m_stringsMap/m_stringsVec is empty via isStatic's dispatch), the
        // same way a release build has always handled this case.
        app.DebugPrintf("StringTable:: Failed to find any usable language in the archive; "
                        "no strings will be available.\n");

        isStatic = false;
    }

    // We can't delete this data in the dtor, so clear the reference
    bais.reset();
}

StringTable::~StringTable(void)
{
    // delete src.data; TODO 4J-JEV: ?
}

void StringTable::getData(PBYTE *ppData, UINT *pSize)
{
    *ppData = src.data;
    *pSize = src.length;
}

LPCWSTR StringTable::getString(const wstring &id)
{
#ifndef _CONTENT_PACKAGE
    if (isStatic)
    {
        // This table was loaded in "static" mode (integer string ids, see
        // the isStatic branch in the constructor above) - the caller should
        // have used getString(int) instead. This is a caller bug, not a
        // runtime data problem, but it's still not worth an
        // illegal-instruction crash over: log once and fall through to the
        // normal empty-string miss below.
        app.DebugPrintf("StringTable::getString(wstring): called on a static-mode table; "
                        "use getString(int) instead.\n");
        return L"";
    }
#endif

    AUTO_VAR(it, m_stringsMap.find(id));

    if (it != m_stringsMap.end())
    {
        return it->second.c_str();
    }
    else
    {
        return L"";
    }
}

LPCWSTR StringTable::getString(int id)
{
    // The SDL port can load DLC before its locale table has been selected.
    // A missing localized string must not turn into an illegal-instruction
    // debug trap; return the normal empty-string fallback below instead.

    if (id < m_stringsVec.size())
    {
        LPCWSTR pwchString = m_stringsVec.at(id).c_str();
        return pwchString;
    }
    else
    {
        return L"";
    }
}
