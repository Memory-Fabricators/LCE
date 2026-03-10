#pragma once
// 4J Stu - We are not using GZIP compression, so this is just a pass through class

#include "InputStream.h"
#include <cstdint>

class GZIPInputStream : public InputStream
{
  private:
    InputStream *stream;

  public:
    GZIPInputStream(InputStream *out) : stream(out)
    {
    }
    virtual int read()
    {
        return stream->read();
    }
    virtual int read(std::vector<std::byte> b)
    {
        return stream->read(b);
    }
    virtual int read(std::vector<std::byte> b, unsigned int offset, unsigned int length)
    {
        return stream->read(b, offset, length);
    }
    virtual void close()
    {
        return stream->close();
    }
    virtual std::int64_t skip(std::int64_t n)
    {
        return 0;
    }
};
