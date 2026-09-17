#pragma once

#include <string>
#include "miniaudio.h"

class CSoundManager
{
private:
	friend struct SoundManagerLifecycleTest; 
	ma_engine engine{};
	ma_sound BGMSound{};
	ma_sound carEngineSound{};
	ma_sound_group m_sfxGroup{};
	bool m_bEngineInitialized = false;
	bool m_bSFXGroupInitialized = false;
	float m_fBGMVolume = 1.0f;

	bool m_bBGMInitialized{ false };
	bool m_bCarEngineInitialized{ false };

public:
	CSoundManager() = default;
	~CSoundManager() { Release(); }
	CSoundManager(const CSoundManager&) = delete;
	CSoundManager& operator=(const CSoundManager&) = delete;


	void Init(const ma_engine_config* config = nullptr);
	void PlayLobbyBGM();
	void PlayMapBGM(int mapIndex);
	void PlayBGM(const std::string& filepath);
	void PlaySFX(const std::string& filepath);
	void SetMasterVolume(float volume);
	void SetBGMVolume(float volume);

	void PlayCarEngine(const std::string& filepath);
	void SetCarEnginePitch(float pitch);
	void SetCarEngineVolume(float volume);

	void SetSFXVolume(float volume);
	float GetSFXVolume() const;
	float m_fSFXVolume = 0.5f;


	void StopBGM();
	void StopCarEngine();

	void Release();
};

