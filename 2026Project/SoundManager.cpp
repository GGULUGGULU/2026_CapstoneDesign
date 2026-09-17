#include "stdafx.h"

#define MINIAUDIO_IMPLEMENTATION
#include "SoundManager.h"

namespace
{
    bool AudioSucceeded(ma_result result, const char* operation, const char* path = "")
    {
        if (result == MA_SUCCESS) return true;
        char message[512];
        sprintf_s(message, "[Audio] %s failed (%d): %s\n", operation, result, path);
        OutputDebugStringA(message);
        return false;
    }
}

void CSoundManager::Init(const ma_engine_config* config)
{
    if (m_bEngineInitialized) return;
    if (!AudioSucceeded(ma_engine_init(config, &engine), "engine init")) return;
    m_bEngineInitialized = true;
    if (!AudioSucceeded(ma_sound_group_init(&engine, 0, nullptr, &m_sfxGroup), "SFX group init")) {
        Release();
        return;
    }
    m_bSFXGroupInitialized = true;
    ma_sound_group_set_volume(&m_sfxGroup, m_fSFXVolume);
}

void CSoundManager::PlayLobbyBGM()
{
    PlayBGM("Asset/Audio/TRBGM.mp3");
}

void CSoundManager::PlayMapBGM(int mapIndex)
{

    static const char* const tracks[] = {
        "Asset/Audio/TurboCandyCircuit.mp3",
        "Asset/Audio/CutlassDash.mp3",
        "Asset/Audio/BGM1.mp3",
        "Asset/Audio/BGM2.mp3"
    };
    if (mapIndex < 0 || mapIndex >= 4) {
        StopBGM();
        OutputDebugStringA("[Audio] Invalid map index; BGM stopped.\n");
        return;
    }
    PlayBGM(tracks[mapIndex]);
}

void CSoundManager::PlayBGM(const std::string& filepath)
{

    StopBGM();
    if (!m_bEngineInitialized) return;
    if (!AudioSucceeded(ma_sound_init_from_file(&engine, filepath.c_str(),
        MA_SOUND_FLAG_STREAM, nullptr, nullptr, &BGMSound), "BGM init", filepath.c_str())) return;
    m_bBGMInitialized = true;
    ma_sound_set_looping(&BGMSound, MA_TRUE);
    ma_sound_set_volume(&BGMSound, m_fBGMVolume);
    if (!AudioSucceeded(ma_sound_start(&BGMSound), "BGM start", filepath.c_str())) StopBGM();
}

void CSoundManager::SetSFXVolume(float volume)
{
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    m_fSFXVolume = volume;
    if (m_bSFXGroupInitialized) ma_sound_group_set_volume(&m_sfxGroup, volume);
}

float CSoundManager::GetSFXVolume() const
{
    return m_fSFXVolume;
}

void CSoundManager::PlaySFX(const std::string& filepath)
{
    if (m_bEngineInitialized && m_bSFXGroupInitialized)
        AudioSucceeded(ma_engine_play_sound(&engine, filepath.c_str(), &m_sfxGroup), "SFX play", filepath.c_str());
}

void CSoundManager::SetMasterVolume(float volume)
{
    if (m_bEngineInitialized) ma_engine_set_volume(&engine, volume);
}

void CSoundManager::SetBGMVolume(float volume)
{
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    m_fBGMVolume = volume;
    if (m_bBGMInitialized) ma_sound_set_volume(&BGMSound, volume);
}

void CSoundManager::PlayCarEngine(const std::string& filepath)
{
    StopCarEngine();
    if (!m_bEngineInitialized) return;
    if (!AudioSucceeded(ma_sound_init_from_file(&engine, filepath.c_str(), 0,
        nullptr, nullptr, &carEngineSound), "car engine init", filepath.c_str())) return;
    m_bCarEngineInitialized = true;
    ma_sound_set_looping(&carEngineSound, MA_TRUE);
    if (!AudioSucceeded(ma_sound_start(&carEngineSound), "car engine start", filepath.c_str())) StopCarEngine();
}

void CSoundManager::SetCarEnginePitch(float pitch)
{
    if (m_bCarEngineInitialized) ma_sound_set_pitch(&carEngineSound, pitch);
}

void CSoundManager::SetCarEngineVolume(float volume)
{
    if (m_bCarEngineInitialized) ma_sound_set_volume(&carEngineSound, volume);
}

void CSoundManager::StopBGM()
{
    if (!m_bBGMInitialized) return;
    ma_sound_stop(&BGMSound);
    ma_sound_uninit(&BGMSound);
    m_bBGMInitialized = false;
}

void CSoundManager::StopCarEngine()
{
    if (!m_bCarEngineInitialized) return;
    ma_sound_stop(&carEngineSound);
    ma_sound_uninit(&carEngineSound);
    m_bCarEngineInitialized = false;
}

void CSoundManager::Release()
{
    StopBGM();
    StopCarEngine();
    if (m_bSFXGroupInitialized) {
        ma_sound_group_uninit(&m_sfxGroup);
        m_bSFXGroupInitialized = false;
    }
    if (m_bEngineInitialized) {
        ma_engine_uninit(&engine);
        m_bEngineInitialized = false;
    }
}
