#pragma once

#include <cstddef>
#include <cstdint>

class Random
{
  private:
    std::int64_t seed;
    bool haveNextNextGaussian;
    double nextNextGaussian;

  protected:
    int next(int bits);

  public:
    Random();
    Random(std::int64_t seed);
    void setSeed(std::int64_t s);
    void nextBytes(std::byte *bytes, unsigned int count);
    double nextDouble();
    double nextGaussian();
    int nextInt();
    int nextInt(int to);
    float nextFloat();
    std::int64_t nextLong();
    bool nextBoolean();
};
