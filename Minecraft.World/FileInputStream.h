#pragma once
// 4J Stu - Represents Java standard library class

#include "InputStream.h"
#include <cstdint>
#include <istream>
#include <vector>

class FileInputStream : public InputStream
{
  public:
    FileInputStream(std::ifstream &file);
    virtual ~FileInputStream();
    virtual int read();
    virtual int read(std::vector<std::uint8_t> b);
    virtual int read(std::vector<std::uint8_t> b, unsigned int offset, unsigned int length);
    virtual void close();
    virtual std::int64_t skip(std::int64_t n);

  private:
    std::ifstream &m_fileHandle;
};
