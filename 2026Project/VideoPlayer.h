#pragma once

#include <windows.h>
#include <string>

class CVideoPlayer
{
public:
	CVideoPlayer();
	~CVideoPlayer();

	bool Initialize(HWND hParentWnd);
	bool Play(const wchar_t* filePath, int width, int height);
	void Stop();

	bool IsPlaying() const;
	bool IsFinished() const;

	void Resize(int width, int height);

private:
	HWND m_hParentWnd = NULL;
	HWND m_hVideoWnd = NULL;

	std::wstring m_alias;
	bool m_bPlaying = false;
};