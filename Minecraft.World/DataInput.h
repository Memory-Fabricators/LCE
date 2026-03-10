#pragma once

#include "PlayerUID.h"
#include <cstdint>
#include <span>
class DataInput
{
  public:
    virtual int read() = 0;
    virtual int read(std::span<std::byte> b) = 0;
    virtual int read(std::span<std::byte> b, unsigned int offset, unsigned int length) = 0;
    virtual bool readBoolean() = 0;
    virtual std::byte readByte() = 0;
    virtual unsigned char readUnsignedByte() = 0;
    virtual bool readFully(std::span<std::byte> a) = 0;
    virtual double readDouble() = 0;
    virtual float readFloat() = 0;
    virtual int readInt() = 0;
    virtual std::int64_t readLong() = 0;
    virtual short readShort() = 0;
    virtual wchar_t readChar() = 0;
    virtual std::wstring readUTF() = 0;
    virtual PlayerUID readPlayerUID() = 0; // 4J Added
    virtual int skipBytes(int n) = 0;
};
