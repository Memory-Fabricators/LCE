#pragma once

enum ESen_AudioSettings
{
    eSen_AudioSettings_Undefined = 0,
    eSen_AudioSettings_Off = 1,
    eSen_AudioSettings_On_Default = 2,
    eSen_AudioSettings_On_CustomSetting = 3,
};

enum ESen_CompeteOrCoop
{
    eSen_CompeteOrCoop_Undefined = 0,
    eSen_CompeteOrCoop_Cooperative = 1,
    eSen_CompeteOrCoop_Competitive = 2,
    eSen_CompeteOrCoop_Coop_and_Competitive = 3,
};

enum ESen_DefaultGameControls
{
    eSen_DefaultGameControls_Undefined = 0,
    eSen_DefaultGameControls_Default_controls = 1,
    eSen_DefaultGameControls_Custom_controls = 2,
};

enum ESen_DifficultyLevel
{
    eSen_DifficultyLevel_Undefined = 0,
    eSen_DifficultyLevel_Easiest = 1,
    eSen_DifficultyLevel_Easier = 2,
    eSen_DifficultyLevel_Normal = 3,
    eSen_DifficultyLevel_Harder = 4,
    eSen_DifficultyLevel_Hardest = 5,
};

enum ESen_GameInputType
{
    eSen_GameInputType_Undefined = 0,
    eSen_GameInputType_Xbox_Controller = 1,
    eSen_GameInputType_Gesture = 2,
    eSen_GameInputType_Voice = 3,
    eSen_GameInputType_Voice_and_Gesture_Together = 4,
    eSen_GameInputType_Touch = 5,
    eSen_GameInputType_Keyboard = 6,
    eSen_GameInputType_Mouse = 7,
};

enum ESen_LevelExitStatus
{
    eSen_LevelExitStatus_Undefined = 0,
    eSen_LevelExitStatus_Exited = 1,
    eSen_LevelExitStatus_Succeeded = 2,
    eSen_LevelExitStatus_Failed = 3,
};

enum ESen_License
{
    eSen_License_Undefined = 0,
    eSen_License_Trial_or_Demo = 1,
    eSen_License_Full_Purchased_Title = 2,
};

enum ESen_MediaDestination
{
    ESen_MediaDestination_Undefined = 0,
    ESen_MediaDestination_Kinect_Share = 1,
    ESen_MediaDestination_Facebook = 2,
    ESen_MediaDestination_YouTube = 3,
    ESen_MediaDestination_Other = 4
};

enum ESen_MediaType
{
    eSen_MediaType_Undefined = 0,
    eSen_MediaType_Picture = 1,
    eSen_MediaType_Video = 2,
    eSen_MediaType_Other_UGC = 3,
};

enum ESen_SingleOrMultiplayer
{
    eSen_SingleOrMultiplayer_Undefined = 0,
    eSen_SingleOrMultiplayer_Single_Player = 1,
    eSen_SingleOrMultiplayer_Multiplayer_Local = 2,
    eSen_SingleOrMultiplayer_Multiplayer_Live = 3,
    eSen_SingleOrMultiplayer_Multiplayer_Both_Local_and_Live = 4,
};

enum ESen_FriendOrMatch
{
    eSen_FriendOrMatch_Undefined = 0,
    eSen_FriendOrMatch_Playing_With_Invited_Friends = 1,
    eSen_FriendOrMatch_Playing_With_Match_Made_Opponents = 2,
    eSen_FriendOrMatch_Playing_With_Both_Friends_And_Matched_Opponents = 3,
    eSen_FriendOrMatch_Joined_Through_An_Xbox_Live_Party = 4,
    eSen_FriendOrMatch_Joined_Through_An_In_Game_Party = 5,
};

enum ESen_UpsellID
{
    eSen_UpsellID_Undefined = 0,
    eSen_UpsellID_Full_Version_Of_Game = 1,
    eSet_UpsellID_Skin_DLC = 2,
    eSet_UpsellID_Texture_DLC = 3,
};

enum ESen_UpsellOutcome
{
    eSen_UpsellOutcome_Undefined = 0,
    eSen_UpsellOutcome_Accepted = 1,
    eSen_UpsellOutcome_Declined = 2,
    eSen_UpsellOutcome_Went_To_Guide = 3,
    eSen_UpsellOutcome_Other = 4,
};
