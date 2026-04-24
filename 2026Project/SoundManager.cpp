#include "stdafx.h"

#define MINIAUDIO_IMPLEMENTATION
#include "SoundManager.h"

void CSoundManager::Init()
{
	ma_engine_init(NULL, &engine);
}

void CSoundManager::PlayBGM(const std::string& filepath)
{
    ma_sound_init_from_file(&engine, filepath.c_str(),
        MA_SOUND_FLAG_STREAM, NULL, NULL, &BGMSound);
    ma_sound_set_looping(&BGMSound, MA_TRUE);
    ma_sound_start(&BGMSound);
}

void CSoundManager::PlaySFX(const std::string& filepath)
{
    ma_engine_play_sound(&engine, filepath.c_str(), NULL);
}

void CSoundManager::SetMasterVolume(float volume)
{
    ma_engine_set_volume(&engine, volume);
}

void CSoundManager::SetBGMVolume(float volume)
{
    ma_sound_set_volume(&BGMSound, volume);
}

void CSoundManager::Release() {
    ma_sound_uninit(&BGMSound);
    ma_engine_uninit(&engine);
}