#include "ContainerClosePacket.h"
#include "InputOutputStream.h"
#include "PacketListener.h"
#include "stdafx.h"
#include <iostream>

ContainerClosePacket::ContainerClosePacket()
{
    containerId = 0;
}

ContainerClosePacket::ContainerClosePacket(int containerId)
{
    this->containerId = containerId;
}

void ContainerClosePacket::handle(PacketListener *listener)
{
    listener->handleContainerClose(shared_from_this());
}

void ContainerClosePacket::read(DataInputStream *dis) // throws IOException
{
    containerId = dis->readByte();
}

void ContainerClosePacket::write(DataOutputStream *dos) // throws IOException
{
    dos->writeByte(containerId);
}

int ContainerClosePacket::getEstimatedSize()
{
    return 1;
}
