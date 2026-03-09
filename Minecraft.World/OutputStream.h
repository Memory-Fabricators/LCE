#pragma once
// 4J Stu - Represents Java standard lib abstract

#include <cstdint>
#include <vector>

class OutputStream
{
  public:
    virtual ~OutputStream()
    {
    }

    virtual void write(unsigned int b) = 0;
    virtual void write(std::vector<std::uint8_t> b) = 0;
    virtual void write(std::vector<std::uint8_t> b, unsigned int offset, unsigned int length) = 0;
    virtual void close() = 0;
    virtual void flush() = 0;
};
