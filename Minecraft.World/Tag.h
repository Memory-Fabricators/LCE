#pragma once
#include "DataInput.h"
#include "DataOutput.h"
#include <ostream>

class Tag
{
  public:
    static const unsigned char TAG_End = 0;
    static const unsigned char TAG_Byte = 1;
    static const unsigned char TAG_Short = 2;
    static const unsigned char TAG_Int = 3;
    static const unsigned char TAG_Long = 4;
    static const unsigned char TAG_Float = 5;
    static const unsigned char TAG_Double = 6;
    static const unsigned char TAG_Byte_Array = 7;
    static const unsigned char TAG_String = 8;
    static const unsigned char TAG_List = 9;
    static const unsigned char TAG_Compound = 10;
    static const unsigned char TAG_Int_Array = 11;

  private:
    wstring name;

  protected:
    Tag(const wstring &name);

  public:
    virtual void write(DataOutput *dos) = 0;
    virtual void load(DataInput *dis) = 0;
    virtual wstring toString() = 0;
    virtual unsigned char getId() = 0;
    void print(ostream out);
    void print(char *prefix, wostream &out);
    wstring getName();
    Tag *setName(const wstring &name);
    static Tag *readNamedTag(DataInput *dis);
    static void writeNamedTag(Tag *tag, DataOutput *dos);
    static Tag *newTag(unsigned char type, const wstring &name);
    static const wchar_t *getTagName(unsigned char type);
    virtual ~Tag()
    {
    }
    virtual bool equals(Tag *obj); // 4J Brought forward from 1.2
    virtual Tag *copy() = 0;       // 4J Brought foward from 1.2
};
