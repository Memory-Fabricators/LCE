#pragma once

#include <cstdint>
#include <string>
#include <vector>

class InputStream
{
  public:
    virtual ~InputStream()
    {
    }

    virtual int read() = 0;
    virtual int read(std::vector<std::uint8_t> b) = 0;
    virtual int read(std::vector<std::uint8_t> b, unsigned int offset, unsigned int length) = 0;
    virtual void close() = 0;
    virtual std::int64_t skip(std::int64_t n) = 0;

    static InputStream *getResourceAsStream(const std::wstring &fileName);
};
