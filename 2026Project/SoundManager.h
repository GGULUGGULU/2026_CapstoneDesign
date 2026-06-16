#pragma once

#include <string>
#include "miniaudio.h"

class CSoundManager
{
private:
	ma_engine engine;
	ma_sound BGMSound;
	ma_sound carEngineSound;

	bool m_bBGMInitialized{ false };
	bool m_bCarEngineInitialized{ false };

public:
	void Init();
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

