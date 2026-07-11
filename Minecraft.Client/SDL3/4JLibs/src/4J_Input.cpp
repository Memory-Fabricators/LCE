#include "../../../stdafx.h"

C_4JInput InputManager;

void C_4JInput::Initialise(int, unsigned char, unsigned char, unsigned char)
{
}

void C_4JInput::Tick(void)
{
}

void C_4JInput::SetDeadzoneAndMovementRange(unsigned int, unsigned int)
{
}

void C_4JInput::SetGameJoypadMaps(unsigned char, unsigned char, unsigned int)
{
}

unsigned int C_4JInput::GetGameJoypadMaps(unsigned char, unsigned char)
{
    return 0;
}

void C_4JInput::SetJoypadMapVal(int, unsigned char)
{
}

unsigned char C_4JInput::GetJoypadMapVal(int)
{
    return 0;
}

void C_4JInput::SetJoypadSensitivity(int, float)
{
}

unsigned int C_4JInput::GetValue(int, unsigned char, bool)
{
    return 0;
}

bool C_4JInput::ButtonPressed(int, unsigned char)
{
    return false;
}

bool C_4JInput::ButtonReleased(int, unsigned char)
{
    return false;
}

bool C_4JInput::ButtonDown(int, unsigned char)
{
    return false;
}

void C_4JInput::SetJoypadStickAxisMap(int, unsigned int, unsigned int)
{
}

void C_4JInput::SetJoypadStickTriggerMap(int, unsigned int, unsigned int)
{
}

void C_4JInput::SetKeyRepeatRate(float, float)
{
}

void C_4JInput::SetDebugSequence(const char *, int (*)(LPVOID), LPVOID)
{
}

FLOAT C_4JInput::GetIdleSeconds(int)
{
    return 0.0f;
}

bool C_4JInput::IsPadConnected(int iPad)
{
    return iPad == 0;
}

float C_4JInput::GetJoypadStick_LX(int, bool)
{
    return 0.0f;
}

float C_4JInput::GetJoypadStick_LY(int, bool)
{
    return 0.0f;
}

float C_4JInput::GetJoypadStick_RX(int, bool)
{
    return 0.0f;
}

float C_4JInput::GetJoypadStick_RY(int, bool)
{
    return 0.0f;
}

unsigned char C_4JInput::GetJoypadLTrigger(int, bool)
{
    return 0;
}

unsigned char C_4JInput::GetJoypadRTrigger(int, bool)
{
    return 0;
}

void C_4JInput::SetMenuDisplayed(int, bool)
{
}

EKeyboardResult C_4JInput::RequestKeyboard(LPCWSTR, LPCWSTR, DWORD, UINT, int (*)(LPVOID, const bool), LPVOID, C_4JInput::EKeyboardMode)
{
    return EKeyboard_Cancelled;
}

void C_4JInput::GetText(uint16_t *)
{
}

bool C_4JInput::VerifyStrings(WCHAR **, int, int (*)(LPVOID, STRING_VERIFY_RESPONSE *), LPVOID)
{
    return false;
}

void C_4JInput::CancelQueuedVerifyStrings(int (*)(LPVOID, STRING_VERIFY_RESPONSE *), LPVOID)
{
}

void C_4JInput::CancelAllVerifyInProgress(void)
{
}
