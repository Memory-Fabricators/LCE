#pragma once

#include "OutputStream.h"

class BufferedOutputStream : public OutputStream
{
  private:
    OutputStream *stream;

  protected:
    std::vector<std::byte> buf; // The internal buffer where data is stored.
    unsigned int count;         // The number of valid bytes in the buffer.

  public:
    BufferedOutputStream(OutputStream *out, int size);
    ~BufferedOutputStream();

    virtual void flush();
    virtual void close();
    virtual void write(std::vector<std::byte> b, unsigned int offset, unsigned int length);
    virtual void write(std::vector<std::byte> b);
    virtual void write(unsigned int b);
};
