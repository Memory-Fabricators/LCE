#pragma once
// 4J Stu - Represents Java standard library class (although we miss out an intermediate inheritance class that we don't care about)

#include "DataInput.h"
#include "InputStream.h"

class DataInputStream : public InputStream, public DataInput
{
  private:
    InputStream *stream;

  public:
    DataInputStream(InputStream *in);
    virtual int read();
    virtual int read(std::span<std::byte> b);
    virtual int read(std::span<std::byte> b, unsigned int offset, unsigned int length);
    virtual void close();
    virtual bool readBoolean();
    virtual std::byte readByte();
    virtual unsigned char readUnsignedByte();
    virtual unsigned short readUnsignedShort();
    virtual wchar_t readChar();
    virtual bool readFully(std::span<std::byte> b);
    virtual bool readFully(std::span<char> b);
    virtual double readDouble();
    virtual float readFloat();
    virtual int readInt();
    virtual std::int64_t readLong();
    virtual short readShort();
    virtual std::wstring readUTF();
    void deleteChildStream();
    virtual int readUTFChar();
    virtual PlayerUID readPlayerUID(); // 4J Added
    virtual std::int64_t skip(std::int64_t n);
    virtual int skipBytes(int n);
};
