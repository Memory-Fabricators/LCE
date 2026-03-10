#pragma once

#include <string>

class StatFormatter
{
  public:
    virtual std::wstring format(int value) = 0;
};
