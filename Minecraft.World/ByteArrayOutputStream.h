#pragma once

#include "OutputStream.h"
#include <span>

class ByteArrayOutputStream : public OutputStream
{
    // Note - when actually implementing, byteArray will need to grow as data is written
  public:
    std::vector<std::byte> buf; // The buffer where data is stored.

  protected:
    unsigned int count; // The number of valid bytes in the buffer.

  public:
    ByteArrayOutputStream();
    ByteArrayOutputStream(unsigned int size);
    virtual ~ByteArrayOutputStream();

    virtual void flush()
    {
    }
    virtual void write(unsigned int b);
    virtual void write(std::span<std::byte> b);
    virtual void write(std::span<std::byte> b, unsigned int offset, unsigned int length);
    virtual void close();
    virtual std::vector<std::byte> toByteArray();

    void reset()
    {
        count = 0;
    }
    unsigned int size()
    {
        return count;
    }
};
