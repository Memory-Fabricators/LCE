// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// #include <xtl.h>
// #include <xboxmath.h>

#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__) ") : 4J Warning Msg: "

// use  - #pragma message(__LOC__"Need to do something here")

// #ifndef _XBOX
// #ifdef _CONTENT_PACKAGE
// #define TO_BE_IMPLEMENTED
// #endif
// #endif

#ifdef __PS3__

#include "PS3Maths.h"
#include "Ps3Stubs.h"
#include "Ps3Types.h"

#elif defined __ORBIS__
#define AUTO_VAR(_var, _val) auto _var = _val
#include "OrbisMaths.h"
#include "OrbisStubs.h"
#include "OrbisTypes.h"
#include <fios2.h>
#include <game_live_streaming.h>
#include <kernel.h>
#include <message_dialog.h>
#include <scebase.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define HRESULT_SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#elif defined __PSVITA__
#include "PSVitaMaths.h"
#include "PSVitaStubs.h"
#include "PSVitaTypes.h"
#include <kernel.h>
#include <scebase.h>
#include <stdio.h>
#include <stdlib.h>
#include <touch.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#else
#define AUTO_VAR(_var, _val) auto _var = _val
#include <unordered_map>
#include <unordered_set>
#include <vector>
typedef unsigned __int64 __uint64;
#endif

#ifdef _WINDOWS64
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <malloc.h>
#include <tchar.h>
#include <windows.h>
// TODO: reference additional headers your program requires here
#include <DirectXMath.h>
#include <d3d11.h>
using namespace DirectX;

#define HRESULT_SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)

#endif

#ifdef _DURANGO
#include <DirectXMath.h>
#include <collection.h>
#include <d3d11_x.h>
#include <ppltasks.h>
#include <wrl.h>
#include <xdk.h>
using namespace DirectX;
#include "DurangoStubs.h"
#include <pix.h>
#define HRESULT_SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif

#ifdef _XBOX
#include <xact3.h>
#include <xact3d3.h>
#include <xboxmath.h>
#include <xonline.h>
#include <xtl.h>
#include <xuiapp.h>
typedef XINVITE_INFO INVITE_INFO;
typedef XUID PlayerUID;
typedef XNKID SessionID;
typedef XUID GameSessionUID;
#define HRESULT_SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)

#include "..\Minecraft.Client\xbox\network\extra.h"
#else
#include "extraX64.h"
#endif

#ifdef __PS3__
#include <cell/cell_fs.h>
#include <cell/l10n.h>
#include <cell/pad.h>
#include <cell/rtc.h>
#include <cell/sysmodule.h>
#include <sys/ppu_thread.h>
#include <sys/process.h>
#include <sysutil/sysutil_common.h>
#include <sysutil/sysutil_savedata.h>
#include <sysutil/sysutil_sysparam.h>

#endif

// C RunTime Header Files
#include <stdlib.h>

#include <memory>

#include <algorithm>
#include <deque>
#include <exception>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <assert.h>

#ifdef _XBOX
#include <xonline.h>
#include <xparty.h>
#endif

#include "..\Minecraft.World\ArrayWithLength.h"
#include "..\Minecraft.World\Definitions.h"
#include "..\Minecraft.World\PerformanceTimer.h"
#include "..\Minecraft.World\Random.h"
#include "..\Minecraft.World\SharedConstants.h"
#include "..\Minecraft.World\class.h"
#include "..\Minecraft.World\compression.h"

#ifdef _XBOX
#include "xbox\4JLibs\inc\4J_Input.h"
#include "xbox\4JLibs\inc\4J_Profile.h"
#include "xbox\4JLibs\inc\4J_Render.h"
#include "xbox\4JLibs\inc\4J_Storage.h"
#include "xbox\4JLibs\inc\4J_XTMS.h"
#elif defined(__PS3__)

#include "PS3\4JLibs\inc\4J_Input.h"
#include "PS3\4JLibs\inc\4J_Profile.h"
#include "PS3\4JLibs\inc\4J_Render.h"
#include "PS3\4JLibs\inc\4J_Storage.h"
#elif defined _DURANGO
#include "Durango\4JLibs\inc\4J_Input.h"
#include "Durango\4JLibs\inc\4J_Profile.h"
#include "Durango\4JLibs\inc\4J_Render.h"
#include "Durango\4JLibs\inc\4J_Storage.h"
#elif defined _WINDOWS64
#include "Windows64\4JLibs\inc\4J_Input.h"
#include "Windows64\4JLibs\inc\4J_Profile.h"
#include "Windows64\4JLibs\inc\4J_Render.h"
#include "Windows64\4JLibs\inc\4J_Storage.h"
#elif defined __PSVITA__
#include "PSVita\4JLibs\inc\4J_Input.h"
#include "PSVita\4JLibs\inc\4J_Profile.h"
#include "PSVita\4JLibs\inc\4J_Render.h"
#include "PSVita\4JLibs\inc\4J_Storage.h"
#else
#include "Orbis\4JLibs\inc\4J_Input.h"
#include "Orbis\4JLibs\inc\4J_Profile.h"
#include "Orbis\4JLibs\inc\4J_Render.h"
#include "Orbis\4JLibs\inc\4J_Storage.h"
#endif

#include "BufferedImage.h"
#include "ClientConstants.h"
#include "Font.h"
#include "Gui.h"
#include "MemoryTracker.h"
#include "Minecraft.h"
#include "Screen.h"
#include "ScreenSizeCalculator.h"
#include "Textures.h"
#include "stubs.h"

#include "Common\Network\GameNetworkManager.h"

#ifdef _XBOX
#include "Common\XUI\XUI_Helper.h"
#include "Common\XUI\XUI_Scene_Base.h"
#endif

#include "Common\App_defines.h"
#include "Common\UI\UIEnums.h"
#include "Common\UI\UIStructs.h"
// #ifdef _XBOX
#include "Common\App_enums.h"
#include "Common\App_structs.h"
#include "Common\Tutorial\TutorialEnum.h"
// #endif

#include "Common\BuildVer.h"
#include "Common\Consoles_App.h"
#include "Common\Minecraft_Macros.h"

#ifdef _XBOX
#include "XboxMedia\4J_strings.h"
#include "XboxMedia\strings.h"
#include "Xbox\Audio\SoundEngine.h"
#include "Xbox\GameConfig\Minecraft.spa.h"
#include "Xbox\Leaderboards\XboxLeaderboardManager.h"
#include "Xbox\Sentient\DynamicConfigurations.h"
#include "Xbox\Sentient\Include\SenClientStats.h"
#include "Xbox\Sentient\MinecraftTelemetry.h"
#include "Xbox\Sentient\SentientTelemetryCommon.h"
#include "Xbox\Social\SocialManager.h"
#include "Xbox\XML\ATGXmlParser.h"
#include "Xbox\Xbox_App.h"
#include "Xbox\Xbox_UIController.h"

#elif defined(__PS3__)
#include "Common\Audio\SoundEngine.h"
#include "PS3Media\4J_strings.h"
#include "PS3Media\strings.h"
#include "PS3\GameConfig\Minecraft.spa.h"
#include "PS3\Iggy\gdraw\gdraw_ps3gcm.h"
#include "PS3\Iggy\include\iggy.h"
#include "PS3\PS3_App.h"
#include "PS3\PS3_UIController.h"
#include "PS3\Sentient\DynamicConfigurations.h"
#include "PS3\Sentient\MinecraftTelemetry.h"
#include "PS3\Sentient\SentientTelemetryCommon.h"
#include "PS3\Social\SocialManager.h"
#include "PS3\XML\ATGXmlParser.h"
#include "extraX64client.h"
#elif defined _DURANGO
#include "Common\Audio\SoundEngine.h"
#include "DurangoMedia\4J_strings.h"
#include "DurangoMedia\strings.h"
#include "Durango\Durango_App.h"
#include "Durango\Durango_UIController.h"
#include "Durango\Iggy\gdraw\gdraw_d3d11.h"
#include "Durango\Iggy\include\iggy.h"
#include "Durango\PresenceIds.h"
#include "Durango\Sentient\DynamicConfigurations.h"
#include "Durango\Sentient\MinecraftTelemetry.h"
#include "Durango\Sentient\SentientTelemetryCommon.h"
#include "Durango\Sentient\TelemetryEnum.h"
#include "Durango\Social\SocialManager.h"
#include "Durango\XML\ATGXmlParser.h"
#elif defined _WINDOWS64
#include "Common\Audio\SoundEngine.h"
#include "Windows64Media\strings.h"
#include "Windows64\GameConfig\Minecraft.spa.h"
#include "Windows64\Iggy\gdraw\gdraw_d3d11.h"
#include "Windows64\Iggy\include\iggy.h"
#include "Windows64\Sentient\DynamicConfigurations.h"
#include "Windows64\Sentient\MinecraftTelemetry.h"
#include "Windows64\Sentient\SentientTelemetryCommon.h"
#include "Windows64\Social\SocialManager.h"
#include "Windows64\Windows64_App.h"
#include "Windows64\Windows64_UIController.h"
#include "Windows64\XML\ATGXmlParser.h"
#elif defined __PSVITA__
#include "Common\Audio\SoundEngine.h"
#include "PSVitaMedia\strings.h" // TODO - create PSVita-specific version of this
#include "PSVita\GameConfig\Minecraft.spa.h"
#include "PSVita\Iggy\gdraw\gdraw_psp2.h"
#include "PSVita\Iggy\include\iggy.h"
#include "PSVita\PSVita_App.h"
#include "PSVita\PSVita_UIController.h"
#include "PSVita\Sentient\DynamicConfigurations.h"
#include "PSVita\Sentient\MinecraftTelemetry.h"
#include "PSVita\Sentient\SentientManager.h"
#include "PSVita\Social\SocialManager.h"
#include "PSVita\XML\ATGXmlParser.h"
#else
#include "Common\Audio\SoundEngine.h"
#include "OrbisMedia\4J_strings.h"
#include "OrbisMedia\strings.h"
#include "Orbis\GameConfig\Minecraft.spa.h"
#include "Orbis\Iggy\gdraw\gdraw_orbis.h"
#include "Orbis\Iggy\include\iggy.h"
#include "Orbis\Orbis_App.h"
#include "Orbis\Orbis_UIController.h"
#include "Orbis\Sentient\DynamicConfigurations.h"
#include "Orbis\Sentient\MinecraftTelemetry.h"
#include "Orbis\Sentient\SentientTelemetryCommon.h"
#include "Orbis\XML\ATGXmlParser.h"
#include "Windows64\Social\SocialManager.h"
#endif

#ifdef _XBOX
#include "Common\XUI\XUI_ConnectingProgress.h"
#include "Common\XUI\XUI_CustomMessages.h"
#include "Common\XUI\XUI_FullscreenProgress.h"
#include "Common\XUI\XUI_PauseMenu.h"
#include "Common\XUI\XUI_Scene_CraftingPanel.h"
#include "Common\XUI\XUI_Scene_Inventory_Creative.h"
#include "Common\XUI\XUI_TutorialPopup.h"
#endif
#include "Common\Colours\ColourTable.h"
#include "Common\ConsoleGameMode.h"
#include "Common\Console_Awards_enum.h"
#include "Common\Console_Debug_enum.h"
#include "Common\DLC\DLCManager.h"
#include "Common\DLC\DLCPack.h"
#include "Common\DLC\DLCSkinFile.h"
#include "Common\GameRules\ConsoleGameRules.h"
#include "Common\GameRules\ConsoleSchematicFile.h"
#include "Common\Telemetry\TelemetryManager.h"
#include "Common\Trial\TrialMode.h"
#include "Common\Tutorial\FullTutorialMode.h"
#include "Common\Tutorial\Tutorial.h"
#include "Common\Tutorial\TutorialMode.h"

#ifdef _XBOX
// #include "Xbox\Xbox_App.h"
#elif !defined(__PS3__)
#include "extraX64client.h"
#endif

#ifdef _FINAL_BUILD
#define printf BREAKTHECOMPILE
#define wprintf BREAKTHECOMPILE
#undef OutputDebugString
#define OutputDebugString BREAKTHECOMPILE
#define OutputDebugStringA BREAKTHECOMPILE
#define OutputDebugStringW BREAKTHECOMPILE
#endif

void MemSect(int sect);
