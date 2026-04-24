#pragma once

#include <string>
#include "miniaudio.h"

class CSoundManager
{
private:
	ma_engine engine;
	ma_sound BGMSound;

public:
	void Init();
	void PlayBGM(const std::string& filepath);
	void PlaySFX(const std::string& filepath);
	void SetMasterVolume(float volume);
	void SetBGMVolume(float volume);
	void Release();
};

