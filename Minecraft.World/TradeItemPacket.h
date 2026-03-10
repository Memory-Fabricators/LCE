#pragma once

// 4J ADDED PACKET

;

#include "Packet.h"

#include "PacketListener.h"
#include "stdafx.h"
#include <iostream>

class TradeItemPacket : public Packet, public enable_shared_from_this<TradeItemPacket>
{
  public:
    int containerId;
    int offer;

    TradeItemPacket();
    TradeItemPacket(int containerId, int offer);

    virtual void handle(PacketListener *listener);
    virtual void read(DataInputStream *dis);
    virtual void write(DataOutputStream *dos);
    virtual int getEstimatedSize();

  public:
    static shared_ptr<Packet> create()
    {
        return shared_ptr<Packet>(new TradeItemPacket());
    }
    virtual int getId()
    {
        return 151;
    }
};
