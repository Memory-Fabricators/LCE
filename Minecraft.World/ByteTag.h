#pragma once
#include "DataInput.h"
#include "DataOutput.h"
#include "Tag.h"

class ByteTag : public Tag
{
  public:
    std::byte data;
    ByteTag(const std::wstring &name) : Tag(name)
    {
    }
    ByteTag(const std::wstring &name, std::byte data) : Tag(name)
    {
        this->data = data;
    }

    void write(DataOutput *dos)
    {
        dos->writeByte(data);
    }
    void load(DataInput *dis, int tagDepth)
    {
        data = dis->readByte();
    }

    std::byte getId()
    {
        return TAG_Byte;
    }
    std::wstring toString()
    {
        static wchar_t buf[32];
        swprintf(buf, 32, L"%d", data);
        return std::wstring(buf);
    }

    bool equals(Tag *obj)
    {
        if (Tag::equals(obj))
        {
            ByteTag *o = (ByteTag *)obj;
            return data == o->data;
        }
        return false;
    }

    Tag *copy()
    {
        return new ByteTag(getName(), data);
    }
};
