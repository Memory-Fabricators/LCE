#pragma once

#include <chrono>
#include <string>

class PerformanceTimer
{
  private:
    std::chrono::steady_clock::time_point m_qwStartTime;
    float m_fSecsPerTick;

  public:
    PerformanceTimer();
    void Reset();
    void PrintElapsedTime(const std::wstring &description);
};
