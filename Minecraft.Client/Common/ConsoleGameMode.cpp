#include "ConsoleGameMode.h"
#include "..\Common\Tutorial\Tutorial.h"
#include "stdafx.h"

ConsoleGameMode::ConsoleGameMode(int iPad, Minecraft *minecraft, ClientConnection *connection)
    : TutorialMode(iPad, minecraft, connection)
{
    tutorial = new Tutorial(iPad);
}
