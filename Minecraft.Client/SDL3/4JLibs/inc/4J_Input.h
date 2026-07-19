#pragma once


#define MAP_STYLE_0 0
#define MAP_STYLE_1 1
#define MAP_STYLE_2 2

#define _360_JOY_BUTTON_A 0x00000001
#define _360_JOY_BUTTON_B 0x00000002
#define _360_JOY_BUTTON_X 0x00000004
#define _360_JOY_BUTTON_Y 0x00000008
#define _360_JOY_BUTTON_START 0x00000010
#define _360_JOY_BUTTON_BACK 0x00000020
#define _360_JOY_BUTTON_RB 0x00000040
#define _360_JOY_BUTTON_LB 0x00000080
#define _360_JOY_BUTTON_RTHUMB 0x00000100
#define _360_JOY_BUTTON_LTHUMB 0x00000200
#define _360_JOY_BUTTON_DPAD_UP 0x00000400
#define _360_JOY_BUTTON_DPAD_DOWN 0x00000800
#define _360_JOY_BUTTON_DPAD_LEFT 0x00001000
#define _360_JOY_BUTTON_DPAD_RIGHT 0x00002000
#define _360_JOY_BUTTON_LSTICK_RIGHT 0x00004000
#define _360_JOY_BUTTON_LSTICK_LEFT 0x00008000
#define _360_JOY_BUTTON_RSTICK_DOWN 0x00010000
#define _360_JOY_BUTTON_RSTICK_UP 0x00020000
#define _360_JOY_BUTTON_RSTICK_RIGHT 0x00040000
#define _360_JOY_BUTTON_RSTICK_LEFT 0x00080000
#define _360_JOY_BUTTON_LSTICK_DOWN 0x00100000
#define _360_JOY_BUTTON_LSTICK_UP 0x00200000
#define _360_JOY_BUTTON_RT 0x00400000
#define _360_JOY_BUTTON_LT 0x00800000

#define AXIS_MAP_LX 0
#define AXIS_MAP_LY 1
#define AXIS_MAP_RX 2
#define AXIS_MAP_RY 3
#define TRIGGER_MAP_0 0
#define TRIGGER_MAP_1 1

enum EKeyboardResult
{
    EKeyboard_Pending,
    EKeyboard_Cancelled,
    EKeyboard_ResultAccept,
    EKeyboard_ResultDecline,
};

typedef struct _STRING_VERIFY_RESPONSE
{
    WORD wNumStrings;
    HRESULT *pStringResult;
} STRING_VERIFY_RESPONSE;

class C_4JInput
{
  public:
    enum EKeyboardMode
    {
        EKeyboardMode_Default,
        EKeyboardMode_Numeric,
        EKeyboardMode_Password,
        EKeyboardMode_Alphabet,
        EKeyboardMode_Full,
        EKeyboardMode_Alphabet_Extended,
        EKeyboardMode_IP_Address,
        EKeyboardMode_Phone
    };

    void Initialise(int iInputStateC, unsigned char ucMapC, unsigned char ucActionC, unsigned char ucMenuActionC);
    void Tick(void);
    void SetDeadzoneAndMovementRange(unsigned int uiDeadzone, unsigned int uiMovementRangeMax);
    void SetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction, unsigned int uiActionVal);
    unsigned int GetGameJoypadMaps(unsigned char ucMap, unsigned char ucAction);
    void SetJoypadMapVal(int iPad, unsigned char ucMap);
    unsigned char GetJoypadMapVal(int iPad);
    void SetJoypadSensitivity(int iPad, float fSensitivity);
    unsigned int GetValue(int iPad, unsigned char ucAction, bool bRepeat = false);
    bool ButtonPressed(int iPad, unsigned char ucAction = 255);
    bool ButtonReleased(int iPad, unsigned char ucAction);
    bool ButtonDown(int iPad, unsigned char ucAction = 255);
    void SetJoypadStickAxisMap(int iPad, unsigned int uiFrom, unsigned int uiTo);
    void SetJoypadStickTriggerMap(int iPad, unsigned int uiFrom, unsigned int uiTo);
    void SetKeyRepeatRate(float fRepeatDelaySecs, float fRepeatRateSecs);
    void SetDebugSequence(const char *chSequenceA, int (*Func)(LPVOID), LPVOID lpParam);
    FLOAT GetIdleSeconds(int iPad);
    bool IsPadConnected(int iPad);
    float GetJoypadStick_LX(int iPad, bool bCheckMenuDisplay = true);
    float GetJoypadStick_LY(int iPad, bool bCheckMenuDisplay = true);
    float GetJoypadStick_RX(int iPad, bool bCheckMenuDisplay = true);
    float GetJoypadStick_RY(int iPad, bool bCheckMenuDisplay = true);
    unsigned char GetJoypadLTrigger(int iPad, bool bCheckMenuDisplay = true);
    unsigned char GetJoypadRTrigger(int iPad, bool bCheckMenuDisplay = true);
    void SetMenuDisplayed(int iPad, bool bVal);
    EKeyboardResult RequestKeyboard(LPCWSTR Title, LPCWSTR Text, DWORD dwPad, UINT uiMaxChars, int (*Func)(LPVOID, const bool), LPVOID lpParam, C_4JInput::EKeyboardMode eMode);
    void GetText(uint16_t *UTF16String);
    bool VerifyStrings(WCHAR **pwStringA, int iStringC, int (*Func)(LPVOID, STRING_VERIFY_RESPONSE *), LPVOID lpParam);
    void CancelQueuedVerifyStrings(int (*Func)(LPVOID, STRING_VERIFY_RESPONSE *), LPVOID lpParam);
    void CancelAllVerifyInProgress(void);
};

extern C_4JInput InputManager;
