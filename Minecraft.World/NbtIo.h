#pragma once
#include "CompoundTag.h"
#include "OutputStream.h"

class InputStream;

class NbtIo
{
  public:
    static CompoundTag *readCompressed(InputStream *in);
    static void writeCompressed(CompoundTag *tag, OutputStream *out);
    static CompoundTag *decompress(std::span<std::byte> buffer);
    static std::vector<std::byte> compress(CompoundTag *tag);
    static CompoundTag *read(DataInput *dis);
    static void write(CompoundTag *tag, DataOutput *dos);
};
