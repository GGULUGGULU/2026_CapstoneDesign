#include "stdafx.h"
#include "VideoPlayer.h"
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

static void PrintMciError(MCIERROR error)
{
	if (error == 0) return;

	wchar_t msg[512] = {};
	mciGetErrorStringW(error, msg, 512);

	OutputDebugStringW(L"[VideoPlayer MCI Error] ");
	OutputDebugStringW(msg);
	OutputDebugStringW(L"\n");
}

CVideoPlayer::CVideoPlayer()
{
	m_alias = L"IntroVideo";
}

CVideoPlayer::~CVideoPlayer()
{
	Stop();

	if (m_hVideoWnd)
	{
		DestroyWindow(m_hVideoWnd);
		m_hVideoWnd = NULL;
	}
}

bool CVideoPlayer::Initialize(HWND hParentWnd)
{
	m_hParentWnd = hParentWnd;

	m_hVideoWnd = CreateWindowExW(
		0,
		L"STATIC",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, 0, 100, 100,
		m_hParentWnd,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);

	ShowWindow(m_hVideoWnd, SW_SHOW);
	UpdateWindow(m_hVideoWnd);

	return m_hVideoWnd != NULL;
}

bool CVideoPlayer::Play(const wchar_t* filePath, int width, int height)
{
	if (!m_hVideoWnd) return false;

	Stop();
	Resize(width, height);

	wchar_t command[1024] = {};
	MCIERROR err = 0;

	// 1. 파일 열기
	swprintf_s(command, L"open \"%s\" alias %s", filePath, m_alias.c_str());
	err = mciSendStringW(command, NULL, 0, NULL);
	if (err != 0)
	{
		PrintMciError(err);
		OutputDebugStringW(L"[VideoPlayer] open failed\n");
		return false;
	}

	// 2. 영상 출력 윈도우 연결
	swprintf_s(command, L"window %s handle %llu", m_alias.c_str(), (unsigned long long)m_hVideoWnd);
	err = mciSendStringW(command, NULL, 0, NULL);
	if (err != 0)
	{
		PrintMciError(err);
		OutputDebugStringW(L"[VideoPlayer] window handle failed\n");
		return false;
	}

	// 3. 화면 크기 지정
	swprintf_s(command, L"put %s destination at 0 0 %d %d", m_alias.c_str(), width, height);
	err = mciSendStringW(command, NULL, 0, NULL);
	if (err != 0)
	{
		PrintMciError(err);
		OutputDebugStringW(L"[VideoPlayer] put failed\n");
	}

	// 4. 재생
	swprintf_s(command, L"play %s", m_alias.c_str());
	err = mciSendStringW(command, NULL, 0, NULL);
	if (err != 0)
	{
		PrintMciError(err);
		OutputDebugStringW(L"[VideoPlayer] play failed\n");
		return false;
	}

	ShowWindow(m_hVideoWnd, SW_SHOW);
	SetWindowPos(m_hVideoWnd, HWND_TOP, 0, 0, width, height, SWP_SHOWWINDOW);
	UpdateWindow(m_hVideoWnd);

	m_bPlaying = true;
	OutputDebugStringW(L"[VideoPlayer] play success\n");

	return true;
}

void CVideoPlayer::Stop()
{
	if (!m_bPlaying) return;

	wchar_t command[256] = {};

	swprintf_s(command, L"stop %s", m_alias.c_str());
	mciSendStringW(command, NULL, 0, NULL);

	swprintf_s(command, L"close %s", m_alias.c_str());
	mciSendStringW(command, NULL, 0, NULL);

	if (m_hVideoWnd)
		ShowWindow(m_hVideoWnd, SW_HIDE);

	m_bPlaying = false;
}

bool CVideoPlayer::IsPlaying() const
{
	return m_bPlaying;
}

bool CVideoPlayer::IsFinished() const
{
	if (!m_bPlaying) return true;

	wchar_t status[128] = {};
	wchar_t command[256] = {};

	swprintf_s(command, L"status %s mode", m_alias.c_str());
	mciSendStringW(command, status, 128, NULL);

	return (wcscmp(status, L"stopped") == 0);
}

void CVideoPlayer::Resize(int width, int height)
{
	if (!m_hVideoWnd) return;

	SetWindowPos(
		m_hVideoWnd,
		HWND_TOP,
		0, 0,
		width,
		height,
		SWP_SHOWWINDOW
	);
}