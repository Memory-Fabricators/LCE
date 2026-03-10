#pragma once
#include "Tag.h"
#include <vector>

class ByteArrayTag : public Tag
{
  public:
    std::vector<std::byte> data;

    ByteArrayTag(const std::wstring &name) : Tag(name), data({})
    {
    }

    ByteArrayTag(const std::wstring &name, std::vector<std::byte> data) : Tag(name), data(data)
    {
    }

    void write(DataOutput *dos)
    {
        dos->writeInt(data.size());
        dos->write(data);
    }

    void load(DataInput *dis, int tagDepth)
    {
        int length = dis->readInt();

        if (data.data())
        {
            delete[] data.data();
        }
        data = std::vector<std::byte>(length);
        dis->readFully(data);
    }

    std::byte getId()
    {
        return TAG_Byte_Array;
    }

    std::wstring toString()
    {
        static wchar_t buf[32];
        swprintf(buf, 32, L"[%d bytes]", data.size());
        return std::wstring(buf);
    }

    bool equals(Tag *obj)
    {
        if (Tag::equals(obj))
        {
            ByteArrayTag *o = (ByteArrayTag *)obj;
            return ((data.data() == nullptr && o->data.data() == nullptr) || (data.data() != nullptr && data.size() == o->data.size() && memcmp(data.data(), o->data.data(), data.size()) == 0));
        }
        return false;
    }

    Tag *copy()
    {
        std::byte *cp = new std::byte[data.size()];
        std::copy(data.begin(), data.end(), cp);
        return new ByteArrayTag(getName(), std::vector<std::byte>(cp, cp + data.size()));
    }
};
