#pragma once
#include "DataInput.h"
#include "DataOutput.h"
#include <cstddef>
#include <ostream>

class Tag
{
  public:
    static const std::byte TAG_End = static_cast<std::byte>(0);
    static const std::byte TAG_Byte = static_cast<std::byte>(1);
    static const std::byte TAG_Short = static_cast<std::byte>(2);
    static const std::byte TAG_Int = static_cast<std::byte>(3);
    static const std::byte TAG_Long = static_cast<std::byte>(4);
    static const std::byte TAG_Float = static_cast<std::byte>(5);
    static const std::byte TAG_Double = static_cast<std::byte>(6);
    static const std::byte TAG_Byte_Array = static_cast<std::byte>(7);
    static const std::byte TAG_String = static_cast<std::byte>(8);
    static const std::byte TAG_List = static_cast<std::byte>(9);
    static const std::byte TAG_Compound = static_cast<std::byte>(10);
    static const std::byte TAG_Int_Array = static_cast<std::byte>(11);
    static const int MAX_DEPTH = 512;

  private:
    std::wstring name;

  protected:
    Tag(const std::wstring &name);

  public:
    virtual void write(DataOutput *dos) = 0;
    virtual void load(DataInput *dis, int tagDepth) = 0;
    virtual std::wstring toString() = 0;
    virtual std::byte getId() = 0;
    void print(std::ostream &out);
    void print(char *prefix, std::ostream &out);
    std::wstring getName();
    Tag *setName(const std::wstring &name);
    static Tag *readNamedTag(DataInput *dis);
    static Tag *readNamedTag(DataInput *dis, int tagDepth);
    static void writeNamedTag(Tag *tag, DataOutput *dos);
    static Tag *newTag(std::byte type, const std::wstring &name);
    static wchar_t *getTagName(std::byte type);
    virtual ~Tag()
    {
    }
    virtual bool equals(Tag *obj); // 4J Brought forward from 1.2
    virtual Tag *copy() = 0;       // 4J Brought foward from 1.2
};
