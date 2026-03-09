#pragma once
#include "ByteArrayTag.h"
#include "ByteTag.h"
#include "DoubleTag.h"
#include "FloatTag.h"
#include "IntArrayTag.h"
#include "IntTag.h"
#include "ListTag.h"
#include "LongTag.h"
#include "ShortTag.h"
#include "StringTag.h"
#include "Tag.h"
#include <cstdint>
#include <vector>

class CompoundTag : public Tag
{
  private:
    unordered_map<wstring, Tag *> tags;

  public:
    CompoundTag() : Tag(L"")
    {
    }
    CompoundTag(const wstring &name) : Tag(name)
    {
    }

    void write(DataOutput *dos)
    {
        AUTO_VAR(itEnd, tags.end());
        for (unordered_map<wstring, Tag *>::iterator it = tags.begin(); it != itEnd; it++)
        {
            Tag::writeNamedTag(it->second, dos);
        }
        dos->writeByte(Tag::TAG_End);
    }

    void load(DataInput *dis, int tagDepth)
    {
        if (tagDepth > MAX_DEPTH)
        {
#ifndef _CONTENT_PACKAGE
            printf("Tried to read NBT tag with too high complexity, depth > %d", MAX_DEPTH);
            __debugbreak();
#endif
            return;
        }
        tags.clear();
        Tag *tag;
        while ((tag = Tag::readNamedTag(dis))->getId() != Tag::TAG_End)
        {
            tags[tag->getName()] = tag;
        }
        delete tag;
    }

    vector<Tag *> *getAllTags() // 4J - was collection
    {
        // 4J - was return tags.values();
        vector<Tag *> *ret = new vector<Tag *>;

        AUTO_VAR(itEnd, tags.end());
        for (unordered_map<wstring, Tag *>::iterator it = tags.begin(); it != itEnd; it++)
        {
            ret->push_back(it->second);
        }
        return ret;
    }

    byte getId()
    {
        return TAG_Compound;
    }

    void put(const std::wstring &name, Tag *tag)
    {
        tags[name] = tag->setName(name);
    }

    void putByte(const std::wstring &name, byte value)
    {
        tags[name] = (new ByteTag(name, value));
    }

    void putShort(const std::wstring &name, short value)
    {
        tags[name] = (new ShortTag(name, value));
    }

    void putInt(const std::wstring &name, int value)
    {
        tags[name] = (new IntTag(name, value));
    }

    void putLong(const std::wstring &name, __int64 value)
    {
        tags[name] = (new LongTag(name, value));
    }

    void putFloat(const std::wstring &name, float value)
    {
        tags[name] = (new FloatTag(name, value));
    }

    void putDouble(const std::wstring &name, double value)
    {
        tags[name] = (new DoubleTag(name, value));
    }

    void putString(const std::wstring &name, const std::wstring &value)
    {
        tags[name] = (new StringTag(name, value));
    }

    void putByteArray(const std::wstring &name, std::vector<std::uint8_t> value)
    {
        tags[name] = (new ByteArrayTag(name, value));
    }

    void putIntArray(const std::wstring &name, std::vector<int> value)
    {
        tags[name] = (new IntArrayTag(name, value));
    }

    void putCompound(const std::wstring &name, CompoundTag *value)
    {
        tags[name] = value->setName(wstring(name));
    }

    void putBoolean(const std::wstring &name, bool val)
    {
        putByte(name, val ? (byte)1 : 0);
    }

    Tag *get(const std::wstring &name)
    {
        AUTO_VAR(it, tags.find(name));
        if (it != tags.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool contains(const std::wstring &name)
    {
        return tags.find(name) != tags.end();
    }

    byte getByte(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (byte)0;
        }
        return ((ByteTag *)tags[name])->data;
    }

    short getShort(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (short)0;
        }
        return ((ShortTag *)tags[name])->data;
    }

    int getInt(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (int)0;
        }
        return ((IntTag *)tags[name])->data;
    }

    __int64 getLong(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (__int64)0;
        }
        return ((LongTag *)tags[name])->data;
    }

    float getFloat(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (float)0;
        }
        return ((FloatTag *)tags[name])->data;
    }

    double getDouble(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (double)0;
        }
        return ((DoubleTag *)tags[name])->data;
    }

    wstring getString(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return wstring(L"");
        }
        return ((StringTag *)tags[name])->data;
    }

    byteArray getByteArray(const wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return byteArray();
        }
        return ((ByteArrayTag *)tags[name])->data;
    }

    intArray getIntArray(const wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return intArray();
        }
        return ((IntArrayTag *)tags[name])->data;
    }

    CompoundTag *getCompound(const wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return new CompoundTag(name);
        }
        return (CompoundTag *)tags[name];
    }

    ListTag<Tag> *getList(const wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return new ListTag<Tag>(name);
        }
        return (ListTag<Tag> *)tags[name];
    }

    bool getBoolean(const wstring &string)
    {
        return getByte(string) != 0;
    }

    void remove(const wstring &name)
    {
        AUTO_VAR(it, tags.find(name));
        if (it != tags.end())
        {
            tags.erase(it);
        }
        // tags.remove(name);
    }

    wstring toString()
    {
        static const int bufSize = 32;
        static wchar_t buf[bufSize];
        swprintf(buf, bufSize, L"%d entries", tags.size());
        return wstring(buf);
    }

    void print(char *prefix, ostream out)
    {
        /*
        Tag::print(prefix, out);
        out << prefix << "{" << endl;

        char *newPrefix = new char[ strlen(prefix) + 4 ];
        strcpy( newPrefix, prefix);
        strcat( newPrefix, "   ");

        AUTO_VAR(itEnd, tags.end());
        for( unordered_map<string, Tag *>::iterator it = tags.begin(); it != itEnd; it++ )
        {
        it->second->print(newPrefix, out);
        }
        delete[] newPrefix;
        out << prefix << "}" << endl;
        */
    }

    bool isEmpty()
    {
        return tags.empty();
    }

    virtual ~CompoundTag()
    {
        AUTO_VAR(itEnd, tags.end());
        for (AUTO_VAR(it, tags.begin()); it != itEnd; it++)
        {
            delete it->second;
        }
    }

    Tag *copy()
    {
        CompoundTag *tag = new CompoundTag(getName());

        AUTO_VAR(itEnd, tags.end());
        for (AUTO_VAR(it, tags.begin()); it != itEnd; it++)
        {
            tag->put((wchar_t *)it->first.c_str(), it->second->copy());
        }
        return tag;
    }

    bool equals(Tag *obj)
    {
        if (Tag::equals(obj))
        {
            CompoundTag *o = (CompoundTag *)obj;

            if (tags.size() == o->tags.size())
            {
                bool equal = true;
                AUTO_VAR(itEnd, tags.end());
                for (AUTO_VAR(it, tags.begin()); it != itEnd; it++)
                {
                    AUTO_VAR(itFind, o->tags.find(it->first));
                    if (itFind == o->tags.end() || !it->second->equals(itFind->second))
                    {
                        equal = false;
                        break;
                    }
                }
                return equal;
                // return tags.entrySet().equals(o.tags.entrySet());
            }
        }
        return false;
    }
};
