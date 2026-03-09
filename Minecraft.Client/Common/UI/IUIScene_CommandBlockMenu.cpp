#include "IUIScene_CommandBlockMenu.h"
#include "../../../Minecraft.World/CustomPayloadPacket.h"
#include "ClientConnection.h"
#include "MultiPlayerLocalPlayer.h"
#include "stdafx.h"

void IUIScene_CommandBlockMenu::Initialise(CommandBlockEntity *commandBlock)
{
    m_commandBlock = commandBlock;
    SetCommand(m_commandBlock->getCommand());
}

void IUIScene_CommandBlockMenu::ConfirmButtonClicked()
{
    ByteArrayOutputStream baos;
    DataOutputStream dos(&baos);

    dos.writeInt(m_commandBlock->x);
    dos.writeInt(m_commandBlock->y);
    dos.writeInt(m_commandBlock->z);
    dos.writeUTF(GetCommand());

    Minecraft::GetInstance()->localplayers[GetPad()]->connection->send(shared_ptr<CustomPayloadPacket>(new CustomPayloadPacket(CustomPayloadPacket::SET_ADVENTURE_COMMAND_PACKET, baos.toByteArray())));
}
