#pragma once
// 4J Stu - Represents Java standard library class

#include "InputStream.h"
#include <cstddef>
#include <span>

class ByteArrayInputStream : public InputStream
{
  protected:
    std::span<std::byte> buf; // An array of bytes that was provided by the creator of the stream.
    unsigned int count;       // The index one greater than the last valid character in the input stream buffer.
    unsigned int mark;        // The currently marked position in the stream.
    unsigned int pos;         // The index of the next character to read from the input stream buffer.

  public:
    ByteArrayInputStream(std::span<std::byte> buf, unsigned int offset, unsigned int length);
    ByteArrayInputStream(std::span<std::byte> buf);
    virtual ~ByteArrayInputStream();
    virtual int read();
    virtual int read(std::span<std::byte> b);
    virtual int read(std::span<std::byte> b, unsigned int offset, unsigned int length);
    virtual void close();
    virtual std::int64_t skip(std::int64_t n);

    // 4J Stu Added - Sometimes we don't want to delete the data on destroying this
    void reset()
    {
        buf = std::span<std::byte>();
        count = 0;
        mark = 0;
        pos = 0;
    }
};
