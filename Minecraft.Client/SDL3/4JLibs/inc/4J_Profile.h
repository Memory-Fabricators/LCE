#pragma once


enum eAwardType
{
    eAwardType_Achievement = 0,
    eAwardType_GamerPic,
    eAwardType_Theme,
    eAwardType_AvatarItem,
};

enum eUpsellType
{
    eUpsellType_Custom = 0,
    eUpsellType_Achievement,
    eUpsellType_GamerPic,
    eUpsellType_Theme,
    eUpsellType_AvatarItem,
};

enum eUpsellResponse
{
    eUpsellResponse_Declined,
    eUpsellResponse_Accepted_NoPurchase,
    eUpsellResponse_Accepted_Purchase,
};

class C_4JProfile
{
  public:
    struct PROFILESETTINGS
    {
        int iYAxisInversion;
        int iControllerSensitivity;
        int iVibration;
        bool bSwapSticks;
    };

    void Initialise(DWORD dwTitleID, DWORD dwOfferID, unsigned short usProfileVersion,
                    UINT uiProfileValuesC, UINT uiProfileSettingsC, DWORD *pdwProfileSettingsA,
                    int iGameDefinedDataSizeX4, unsigned int *puiGameDefinedDataChangedBitmask);
    void SetTrialTextStringTable(class CXuiStringTable *pStringTable, int iAccept, int iReject);
    void SetTrialAwardText(eAwardType eType, int iUnlockTitle, int iUnlockText);
    void SetDebugFullOverride(bool bOverride);
    void SetUpsellCallback(int (*Callback)(LPVOID, eUpsellResponse), LPVOID lpParam);
    bool IsFullVersion();
    void SetProfileReadErrorCallback(void (*Callback)(LPVOID), LPVOID lpParam);
    void SetDefaultOptionsCallback(void (*Callback)(LPVOID, int), LPVOID lpParam);
    void SetOldProfileVersionCallback(void (*Callback)(LPVOID, int, int), LPVOID lpParam);
    void SetSignInChangeCallback(void (*Callback)(LPVOID, DWORD), LPVOID lpParam);
    void SetNotificationsCallback(void (*Callback)(LPVOID, DWORD), LPVOID lpParam);
    void RegisterAward(int iAward, DWORD dwId, eAwardType eType, bool = true, CXuiStringTable * = NULL,
                       int = 0, int = 0, int = 0);
    void RichPresenceInit(int, int);
    void RegisterRichPresenceContext(DWORD);
    void SetRichPresenceContextValue(int, DWORD, DWORD);
    void Tick();
    int GetPrimaryPad();
    void SetLockedProfile(int);
    void AwardAchievement(int, int);
    void CheckAwards();

    bool IsSignedIn(int iPad);
    bool IsSignedInLive(int iPad);
    bool IsGuest(int iPad);
    bool AllowedToPlayMultiplayer(int iPad);
    void GetXUID(int iPad, PlayerUID *pXuid, bool bOnlineXuid);
    char *GetGamertag(int iPad);
    wstring GetDisplayName(int iPad);

    bool IsSystemUIDisplayed();
    UINT RequestSignInUI(bool bFromInvite, bool bLocalGame, bool bNoGuestsAllowed, bool bMultiplayerSignIn, bool bAddUser, int (*Func)(LPVOID, const bool, const int iPad), LPVOID lpParam, int iQuadrant = 0);
    UINT RequestConvertOfflineToGuestUI(int (*Func)(LPVOID, const bool, const int iPad), LPVOID lpParam, int iQuadrant = 0);

    int GetLockedProfile();
    eAwardType GetAwardType(int iAwardNumber);
    bool CanBeAwarded(int iQuadrant, int iAwardNumber);
    void Award(int iQuadrant, int iAwardNumber, bool bForce = false);
    void SetCurrentGameActivity(int iPad, int iNewPresence, bool bSetOthersToIdle = false);
    BOOL AreXUIDSEqual(PlayerUID xuid1, PlayerUID xuid2);
    void WriteToProfile(int iQuadrant, bool bGameDefinedDataChanged = false, bool bOverride5MinuteLimitOnProfileWrites = false);
    void *GetGameDefinedProfileData(int iQuadrant);
    void SetPrimaryPad(int iPad);
    bool QuerySigninStatus(void);
    void AllowedPlayerCreatedContent(int iPad, bool thisQuadrantOnly, BOOL *allAllowed, BOOL *friendsAllowed);
};

extern C_4JProfile ProfileManager;
