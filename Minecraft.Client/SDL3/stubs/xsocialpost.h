#pragma once

typedef unsigned int DWORD;

enum ESocialNetwork
{
    eSocialNetwork_Facebook,
    eSocialNetwork_Twitter,
};

class CSocialManager
{
  public:
    static CSocialManager *Instance();
    void Initialise();
    void Tick();
    bool RefreshPostingCapability();
    bool IsTitleAllowedToPostAnything();
    bool IsTitleAllowedToPostImages();
    bool AreAllUsersAllowedToPostImages();
    bool PostLinkToSocialNetwork(ESocialNetwork eSocialNetwork, DWORD dwUserIndex, bool bUsingKinect);
    bool PostImageToSocialNetwork(ESocialNetwork eSocialNetwork, DWORD dwUserIndex, bool bUsingKinect);
    void SetSocialPostText(const wchar_t *Title, const wchar_t *Caption, const wchar_t *Desc);
};
