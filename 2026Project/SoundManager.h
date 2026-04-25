#pragma once

#include <string>
#include "miniaudio.h"

class CSoundManager
{
private:
	ma_engine engine;
	ma_sound BGMSound;
	ma_sound carEngineSound;

public:
	void Init();
	void PlayBGM(const std::string& filepath);
	void PlaySFX(const std::string& filepath);
	void SetMasterVolume(float volume);
	void SetBGMVolume(float volume);

	void PlayCarEngine(const std::string& filepath);
	void SetCarEnginePitch(float pitch);
	void SetCarEngineVolume(float volume);

	void Release();
};

