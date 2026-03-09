#pragma once
// 4J Stu - Represents Java standard lib abstract

#include "OutputStream.h"
#include <iostream>

class FileOutputStream : public OutputStream
{
  public:
    FileOutputStream(std::ostream &file);
    virtual ~FileOutputStream();
    virtual void write(unsigned int b);
    virtual void write(std::vector<std::uint8_t> b);
    virtual void write(std::vector<std::uint8_t> b, unsigned int offset, unsigned int length);
    virtual void close();
    virtual void flush()
    {
    }

  private:
    std::ostream &m_file;
};
