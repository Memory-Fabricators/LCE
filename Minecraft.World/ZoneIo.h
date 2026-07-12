#pragma once
#include "ZonedChunkStorage.h"

class ByteBuffer;

class ZoneIo
{
  private:
    HANDLE channel;
    std::int64_t pos;

  public:
    ZoneIo(HANDLE channel, std::int64_t pos);
    void write(byteArray bb, int size);
    void write(ByteBuffer *bb, int size);
    ByteBuffer *read(int size);

    void flush();
};
