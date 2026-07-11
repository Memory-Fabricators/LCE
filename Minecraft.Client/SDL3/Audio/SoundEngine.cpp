#include "Common/Audio/SoundEngine.h"
#include "../../stdafx.h"

// TODO: this is a no-op stand-in for the SDL3 platform. Common/Audio/SoundEngine.cpp
// is written entirely against the Miles Sound System (RAD Game Tools) with no
// SDL3/OpenAL branch, so it's excluded from the SDL3 build (see Minecraft.Client/
// CMakeLists.txt). This file exists only so the SoundEngine interface has SOME
// implementation to link against until a real OpenAL-backed engine is written.

char SoundEngine::m_szSoundPath[] = {""};
char SoundEngine::m_szMusicPath[] = {""};
char SoundEngine::m_szRedistName[] = {""};
char *SoundEngine::m_szStreamFileA[eStream_Max] = {};

SoundEngine::SoundEngine()
{
}

void SoundEngine::destroy()
{
}

void SoundEngine::play(int, float, float, float, float, float)
{
}

void SoundEngine::playStreaming(const wstring &, float, float, float, float, float, bool)
{
}

void SoundEngine::playUI(int, float, float)
{
}

void SoundEngine::playMusicTick()
{
}

void SoundEngine::updateMusicVolume(float)
{
}

void SoundEngine::updateSystemMusicPlaying(bool)
{
}

void SoundEngine::updateSoundEffectVolume(float)
{
}

void SoundEngine::init(Options *)
{
}

void SoundEngine::tick(shared_ptr<Mob> *, float)
{
}

void SoundEngine::add(const wstring &, File *)
{
}

void SoundEngine::addMusic(const wstring &, File *)
{
}

void SoundEngine::addStreaming(const wstring &, File *)
{
}

char *SoundEngine::ConvertSoundPathToName(const wstring &, bool)
{
    return NULL;
}

bool SoundEngine::isStreamingWavebankReady()
{
    return true;
}

int SoundEngine::getMusicID(int)
{
    return 0;
}

int SoundEngine::getMusicID(const wstring &)
{
    return 0;
}

void SoundEngine::SetStreamingSounds(int, int, int, int, int, int, int)
{
}

void SoundEngine::updateMiles()
{
}

void SoundEngine::playMusicUpdate()
{
}

float SoundEngine::getMasterMusicVolume()
{
    return 1.0f;
}

int SoundEngine::GetRandomishTrack(int iStart, int)
{
    return iStart;
}
