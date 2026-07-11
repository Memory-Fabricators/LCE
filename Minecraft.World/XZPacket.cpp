#include "XZPacket.h"
#include "InputOutputStream.h"
#include "PacketListener.h"
#include "net.minecraft.world.item.h"
#include "stdafx.h"
#include <iostream>

const int XZPacket::STRONGHOLD = 0;

XZPacket::~XZPacket()
{
}

XZPacket::XZPacket()
{
    action = STRONGHOLD;
    x = 0;
    z = 0;
}

XZPacket::XZPacket(char action, int x, int z)
{
    this->action = action;
    this->x = x;
    this->z = z;
}

void XZPacket::handle(PacketListener *listener)
{
    listener->handleXZ(shared_from_this());
}

void XZPacket::read(DataInputStream *dis) // throws IOException
{
    action = dis->read();
    x = dis->readInt();
    z = dis->readInt();
}

void XZPacket::write(DataOutputStream *dos) // throws IOException
{
    dos->write(action);
    dos->writeInt(x);
    dos->writeInt(z);
}

int XZPacket::getEstimatedSize()
{
    return 10;
}
