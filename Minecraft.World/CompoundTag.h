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
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

class CompoundTag : public Tag
{
  private:
    std::unordered_map<std::wstring, Tag *> tags;

  public:
    CompoundTag() : Tag(L"")
    {
    }
    CompoundTag(const std::wstring &name) : Tag(name)
    {
    }

    void write(DataOutput *dos)
    {
        auto itEnd = tags.end();
        for (auto it = tags.begin(); it != itEnd; it++)
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
            __builtin_trap();
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

    std::vector<Tag *> *getAllTags() // 4J - was collection
    {
        // 4J - was return tags.values();
        std::vector<Tag *> *ret = new std::vector<Tag *>;

        auto itEnd = tags.end();
        for (auto it = tags.begin(); it != itEnd; it++)
        {
            ret->push_back(it->second);
        }
        return ret;
    }

    std::byte getId()
    {
        return TAG_Compound;
    }

    void put(const std::wstring &name, Tag *tag)
    {
        tags[name] = tag->setName(name);
    }

    void putByte(const std::wstring &name, std::byte value)
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

    void putLong(const std::wstring &name, std::int64_t value)
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

    void putByteArray(const std::wstring &name, std::vector<std::byte> value)
    {
        tags[name] = (new ByteArrayTag(name, value));
    }

    void putIntArray(const std::wstring &name, std::vector<int> value)
    {
        tags[name] = (new IntArrayTag(name, value));
    }

    void putCompound(const std::wstring &name, CompoundTag *value)
    {
        tags[name] = value->setName(std::wstring(name));
    }

    void putBoolean(const std::wstring &name, bool val)
    {
        putByte(name, static_cast<std::byte>(val ? 1 : 0));
    }

    Tag *get(const std::wstring &name)
    {
        auto it = tags.find(name);
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

    std::byte getByte(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return static_cast<std::byte>(0);
        }
        return static_cast<std::byte>(static_cast<ByteTag *>(tags[name])->data);
    }

    short getShort(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (short)0;
        }
        return static_cast<short>(static_cast<ShortTag *>(tags[name])->data);
    }

    int getInt(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (int)0;
        }
        return static_cast<int>(static_cast<IntTag *>(tags[name])->data);
    }

    std::int64_t getLong(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return 0;
        }
        return static_cast<std::int64_t>(static_cast<LongTag *>(tags[name])->data);
    }

    float getFloat(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (float)0;
        }
        return static_cast<float>(static_cast<FloatTag *>(tags[name])->data);
    }

    double getDouble(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return (double)0;
        }
        return static_cast<double>(static_cast<DoubleTag *>(tags[name])->data);
    }

    std::wstring getString(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return std::wstring(L"");
        }
        return static_cast<std::wstring>(static_cast<StringTag *>(tags[name])->data);
    }

    std::vector<std::byte> getByteArray(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return std::vector<std::byte>();
        }
        return static_cast<std::vector<std::byte>>(static_cast<ByteArrayTag *>(tags[name])->data);
    }

    std::vector<int> getIntArray(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return std::vector<int>();
        }
        return static_cast<std::vector<int>>(static_cast<IntArrayTag *>(tags[name])->data);
    }

    CompoundTag *getCompound(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return new CompoundTag(name);
        }
        return (CompoundTag *)tags[name];
    }

    ListTag<Tag> *getList(const std::wstring &name)
    {
        if (tags.find(name) == tags.end())
        {
            return new ListTag<Tag>(name);
        }
        return (ListTag<Tag> *)tags[name];
    }

    bool getBoolean(const std::wstring &string)
    {
        return getByte(string) != static_cast<std::byte>(0);
    }

    void remove(const std::wstring &name)
    {
        auto it = tags.find(name);
        if (it != tags.end())
        {
            tags.erase(it);
        }
        // tags.remove(name);
    }

    std::wstring toString()
    {
        static const int bufSize = 32;
        static wchar_t buf[bufSize];
        swprintf(buf, bufSize, L"%d entries", tags.size());
        return std::wstring(buf);
    }

    void print(char *prefix, std::ostream &out)
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
        auto itEnd = tags.end();
        for (auto it = tags.begin(); it != itEnd; it++)
        {
            delete it->second;
        }
    }

    Tag *copy()
    {
        CompoundTag *tag = new CompoundTag(getName());

        auto itEnd = tags.end();
        for (auto it = tags.begin(); it != itEnd; it++)
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
                auto itEnd = tags.end();
                for (auto it = tags.begin(); it != itEnd; it++)
                {
                    auto itFind = o->tags.find(it->first);
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
