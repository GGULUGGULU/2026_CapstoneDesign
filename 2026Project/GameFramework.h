#pragma once

#define FRAME_BUFFER_WIDTH		640
#define FRAME_BUFFER_HEIGHT		480

#include <wincodec.h>
#include <memory>

#include "Timer.h"
#include "Player.h"
#include "Scene.h"
#include "ClientNetworkTypes.h"
#include "EffectLibrary.h"
#include "ClientNetworkManager.h"
#include "SoundManager.h"
#include "VideoPlayer.h"

struct UIButton {
	enum ButtonShape {
		RECT,
		TRI_LEFT,
		TRI_RIGHT
	};

	float xRatio, yRatio, wRatio, hRatio; 
	ButtonShape shape{RECT};
	D2D1_RECT_F rect;           
	D2D1_TRIANGLE tri;

	void Update(int screenWidth, int screenHeight) {
		float centerX = screenWidth * xRatio;
		float centerY = screenHeight * yRatio;
		float halfW = (screenWidth * wRatio) * 0.5f;
		float halfH = (screenHeight * hRatio) * 0.5f;
		
		rect = D2D1::RectF(centerX - halfW, centerY - halfH, centerX + halfW, centerY + halfH);
		
		if (shape == ButtonShape::TRI_LEFT) {
			tri.point1 = D2D1::Point2F(rect.left, centerY);           // 왼쪽 끝점
			tri.point2 = D2D1::Point2F(rect.right, rect.top);         // 우측 상단
			tri.point3 = D2D1::Point2F(rect.right, rect.bottom);      // 우측 하단
		}
		else if (shape == ButtonShape::TRI_RIGHT) {
			tri.point1 = D2D1::Point2F(rect.right, centerY);          // 오른쪽 끝점
			tri.point2 = D2D1::Point2F(rect.left, rect.top);          // 좌측 상단
			tri.point3 = D2D1::Point2F(rect.left, rect.bottom);       // 좌측 하단
		}
	}

	float Sign(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3) {
		return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
	}

	bool IsMouseOver(POINT pt) {
		if (shape == ButtonShape::RECT) {
			return (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom);
		}
		else {
			D2D1_POINT_2F mousePt = D2D1::Point2F(static_cast<float>(pt.x), static_cast<float>(pt.y));

			float d1, d2, d3;
			bool has_neg, has_pos;

			d1 = Sign(mousePt, tri.point1, tri.point2);
			d2 = Sign(mousePt, tri.point2, tri.point3);
			d3 = Sign(mousePt, tri.point3, tri.point1);

			has_neg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
			has_pos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);

			return !(has_neg && has_pos);
		}
	}
};

struct RemotePlayerInfo {
	int playerID{ -1 };
	CPlayer* pPlayer{ nullptr };
	float yaw{ 180 };
};

class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateDepthStencilView();

	void ChangeSwapChainState();

	void BuildObjectGameStart();
	void BuildObjectGameRoom();
	void ReleaseObjects();

	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);


	//
	void ShowFrameRate();
	void BeforeTransformBarrier(D3D12_RESOURCE_BARRIER& d3dResourceBarrier, ID3D12GraphicsCommandList* d3dCommandList);
	void AfterTransformBarrier(D3D12_RESOURCE_BARRIER& d3dResourceBarrier, ID3D12GraphicsCommandList* d3dCommandList);
	void ClearRTVDSV(ID3D12DescriptorHeap* d3dRtvDescriptorHeap, ID3D12DescriptorHeap* d3dDsvDescriptorHeap, ID3D12GraphicsCommandList* d3dCommandList);
	void SetUIInfo();
	void BuildGameObjects();
	void CollisionProcess();
	void ProcessInputGameStage();
	void CreateD3D11On12Device();
	void CreateD2DDevice();
	void CreateRenderTargetView();
	void CreateTextResources();
	void RenderUI();
	void BuildObjectEnd();

	void CreateShadowMap();
	void RenderShadowPass();
	void SetMainViewport();

	bool StartServer(unsigned short port = NET_DEFAULT_PORT);
	bool ConnectToServer(const char* pszAddress, unsigned short port = NET_DEFAULT_PORT);
	void SyncMultiplayer();
	void SyncRoom();
	void SyncInGame();
	void SyncResult();
	PlayerNetState BuildLocalPlayerState() const;
	void ApplyRemotePlayerState(const PlayerNetState& state);
	
	void SetupPlayerTransform(CPlayer* pPlayer, const XMFLOAT3& xmf3Position, float fYaw);
	void ApplyMultiplayerSpawn();
	void PlayAndSyncEffect(EFFECT_TYPE eType, const XMFLOAT3& xmf3Position, const XMFLOAT2& xmf2Size, const XMFLOAT3& xmf3Color = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void SendEffectEvent(EFFECT_TYPE eType, const XMFLOAT3& xmf3Position, const XMFLOAT2& xmf2Size, const XMFLOAT3& xmf3Color = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void ConsumeNetworkEffectEvents();
	void ConsumeNetworkCollisionEvents();

	void CheckMulti(const float&);
	void AdjustSound();
	void ShowResult();
	void CheckResult();

	void LoadLobbyUIResource();
	void LoadResultUIResource();
	void LoadDashVignetteResource();
	void LoadRoomUIResource();
	void LoadCarImages();
	void LoadMapImages();
	void LoadReadyImage();

	void CreateRemotePlayers();
	void ReleaseRemotePlayers();
	RemotePlayerInfo* FindOrAllocateRemotePlayer(int targetId);

	//
private:
	HINSTANCE					m_hInstance;
	HWND						m_hWnd;

	int							m_nWndClientWidth;
	int							m_nWndClientHeight;

	IDXGIFactory4* m_pdxgiFactory = NULL;
	IDXGISwapChain3* m_pdxgiSwapChain = NULL;
	ID3D12Device* m_pd3dDevice = NULL;

	bool						m_bMsaa4xEnable = false;
	UINT						m_nMsaa4xQualityLevels = 0;

	static const UINT			m_nSwapChainBuffers = 2;
	UINT						m_nSwapChainBufferIndex;

	ID3D12DescriptorHeap* m_pd3dRtvDescriptorHeap = NULL;
	UINT						m_nRtvDescriptorIncrementSize;

	ID3D12Resource* m_pd3dDepthStencilBuffer = NULL;
	ID3D12DescriptorHeap* m_pd3dDsvDescriptorHeap = NULL;
	UINT						m_nDsvDescriptorIncrementSize;

	ID3D12CommandQueue* m_pd3dCommandQueue = NULL;
	ID3D12GraphicsCommandList* m_pd3dCommandList = NULL;

	ID3D12Fence* m_pd3dFence = NULL;
	UINT64						m_nFenceValues[m_nSwapChainBuffers];
	HANDLE						m_hFenceEvent;

	CSoundManager m_SoundManager;

#if defined(_DEBUG)
	ID3D12Debug* m_pd3dDebugController;
#endif

	CGameTimer					m_GameTimer;

	CScene* m_pScene = NULL;
	CPlayer* m_pPlayer = NULL;
	CCamera* m_pCamera = NULL;
	CNetworkManager* m_pNetwork = NULL;
	float						m_fRemotePlayerYaw = 180.0f;
	bool						m_bMultiplayerEnabled = false;
	bool						m_bIsHostPlayer = false;

	int m_nMyPlayerId{ 0 };

	POINT						m_ptOldCursorPos;

	_TCHAR						m_pszFrameRate[80];


	ID3D11DeviceContext* m_d3d11DeviceContext;
	ComPtr<ID3D11On12Device> m_d3d11On12Device;
	ComPtr<ID2D1Factory3> m_d2dFactory;
	ComPtr<ID2D1Device> m_d2dDevice;
	ComPtr<ID2D1DeviceContext> m_d2dDeviceContext;
	ComPtr<IDWriteFactory> m_dWriteFactory;

	ComPtr<ID3D11Resource> m_wrappedBackBuffers[m_nSwapChainBuffers];
	ComPtr<ID2D1Bitmap1> m_d2dRenderTargets[m_nSwapChainBuffers];

	ComPtr<ID3D12Resource> m_d3dSwapChainBackBuffers[m_nSwapChainBuffers];
	ComPtr<ID3D12CommandAllocator> m_d3dCommandAllocators[m_nSwapChainBuffers];

	ComPtr<IDWriteTextFormat> m_textTimeFormat;  // 글꼴, 크기, 정렬
	ComPtr<ID2D1SolidColorBrush> m_textTimeBrush; // 글자 색상

	ComPtr<IDWriteTextFormat> m_textSpeedFormat;  // 글꼴, 크기, 정렬
	ComPtr<ID2D1SolidColorBrush> m_textSpeedBrush; // 글자 색상

	ComPtr<IDWriteTextFormat> m_textEndTimeFormat;  // 글꼴, 크기, 정렬
	ComPtr<ID2D1SolidColorBrush> m_textEndTimeBrush; // 글자 색상

	ComPtr<ID2D1SolidColorBrush> m_dashGaugeFillBrush; // 대시게이지 색상
	ComPtr<ID2D1SolidColorBrush> m_dashGaugeBGBrush; // 대시게이지 배경 색상
	ComPtr<ID2D1SolidColorBrush> m_dashGaugeBorderBrush; // 대시게이지 경계선 색상

	ComPtr<ID2D1SolidColorBrush> m_pBtnHoverBrush;
	ComPtr<ID2D1SolidColorBrush> m_pTriBtnHoverBrush;

	ComPtr<IWICImagingFactory> m_pWICFactory;
	ComPtr<ID2D1Bitmap> m_pLobbyD2DBitmap;
	ComPtr<ID2D1Bitmap> m_pResultD2DBitmap;
	ComPtr<ID2D1Bitmap> m_pDashVignetteBitmap;
	ComPtr<ID2D1Bitmap> m_pRoomD2DBitmap;
	ComPtr<ID2D1Bitmap> m_pCarImages[3];
	ComPtr<ID2D1Bitmap> m_pMapImages[2];
	ComPtr<ID2D1Bitmap> m_pReadyImage;
	

	float m_fDashVignetteAlpha = 0.0f;

	ID3D12Resource* m_pd3dShadowMap;
	ID3D12DescriptorHeap* m_pd3dShadowDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dCPUShadowDSVHandle;

	// 아이템 + 대시


	// // ===== 미니맵
	ComPtr<ID2D1Bitmap> m_pMinimapBitmaps[2];

	ComPtr<ID2D1SolidColorBrush> m_minimapBorderBrush;
	ComPtr<ID2D1SolidColorBrush> m_minimapPlayerBrush;
	ComPtr<ID2D1SolidColorBrush> m_minimapFrameBrush;
	ComPtr<ID2D1SolidColorBrush> m_minimapOtherBrush;

	void LoadMinimapUIResource();
	D2D1_POINT_2F WorldToMinimap(const XMFLOAT3& worldPos, const D2D1_RECT_F& minimapRect);


	// 도움말ui
	bool m_bShowHelpUI = false;
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_pHelpUID2DBitmap;
	void LoadHelpUIResource();
	void DrawHelpUI();

	// 영상

	std::unique_ptr<CVideoPlayer> m_pVideoPlayer;
	void FinishIntroVideo();
	bool m_bPlayingIntroVideo = true;
	
	std::vector<RemotePlayerInfo> m_vRemotePlayers;
	bool m_bNeedRemotePlayerInit{ false };

private:
	enum ITEM_TYPE
	{
		ITEM_NONE = 0,
		ITEM_DASH_POTION,
		ITEM_MAX_SPEED_UP,
		ITEM_MAX_DASH_GAUGE_UP,
		ITEM_LOCK
	};

	void ApplyItemReward(ITEM_TYPE eItemType);
	void UpdateDashSystem(float fTimeElapsed, bool bDashKeyDown, bool bHasDriveInput);
	float GetPlayerEffectiveMaxSpeed() const;

	void SendItemEvent(ITEM_TYPE eItemType, float fDuration);
	void ConsumeNetworkItemEvents();
	void ApplyDashLock(float fDuration);
	void PlayLockEffectOnPlayer(CPlayer* pTargetPlayer, float fDuration);

	void ConsumeNetworkMapItemEvents();

private:
	float m_fBasePlayerMaxSpeed;
	float m_fSpeedItemBonus;

	float m_fSpeedItemBonusTime = 0.0f;

	bool  m_bNoDashGaugeConsume = false;
	float m_fNoDashGaugeConsumeTime = 0.0f;

	float m_fDashSpeedBonus;
	float m_fCurrentDashGauge;
	float m_fMaxDashGauge;
	float m_fDashGaugeConsumePerSecond;
	float m_fDashGaugeRecoverPerSecond;
	float m_fDashGaugeIncreaseAmount;

	bool  m_bIsDashing;
	bool  m_bDashOverheated = false;
	float m_fDashOverheatTime = 0.0f;
	bool m_bPrevBoosterSyncActive = false;


public:
	// UI
	TCHAR m_timeBuffer[1024];
	float m_fTotalTime{ 0.f };
	float m_fCollisionCurrentTime{ 0.f };
	float m_fJumpCurrentTime{ 0.f };

	TCHAR m_speedBuffer[1024];
	int m_nPlayerCurrentSpeed{ 0 };

	float m_fItemDisplayTimer = 0.0f;
	ITEM_TYPE m_eHoldItem = ITEM_NONE;

	// stage
	XMFLOAT3 Map1PlayerSpawnPos[4]{
		{-1980.0f, -200.0f, 188.0f}, // 1
		{-1920.0f, -200.0f, 188.0f}, // 2
		{-1860.0f, -200.0f, 188.0f}, // 3
		{-1800.0f, -200.0f, 188.0f}, // 4
	};

	XMFLOAT3 Map2PlayerSpawnPos[4]{
		{-7600, -2260, 5530}, // 1
		{-7520, -2260, 5530}, // 2
		{-7440, -2260, 5530}, // 3
		{-7360, -2260, 5530}, // 4
	};

	XMFLOAT3 Map1SinglePlayerSpawn{ -1938.0f, -200.0f, 188.0f };
	XMFLOAT3 Map2SinglePlayerSpawn{ -7600, -2260, 5530 };
	
	float PLAYER_SPAWN_YAW = 0.0f;

	int m_nStage{ 0 };
	bool m_bFlag{ false };
	bool m_bIsStun{ false };
	bool m_bJump{ false };
	int m_nScore{ 0 };

	float m_fMyFinalTime{};
	RaceResultNet m_FinalRaceResult{};

	bool m_bStartSign{ false };
	bool m_bServerStartSign{ false };
	int m_nLoadedPlayersCnt{0};
	// jump
	int   m_nJumpCount = 0;
	float m_fSecondJumpWindow = 0.35f;   // 2단 범위
	float m_fFirstJumpTime = 0.0f;
	unsigned m_cnt{ 0 };

	POINT m_ptMousePos;
	int m_nHoveredButtonIndex{ -1 };

	UIButton m_LobbyButtons[3] = {
		// 방 만들기
		{ 0.7908f+0.01, 0.5962f, 0.2523f, 0.0986f },
		// 방 들어가기
		{ 0.7908f+0.01, 0.7692f, 0.2523f, 0.0962f },
		// 게임 종료
		{ 0.7908f+0.01, 0.9282f, 0.2523f, 0.0968f }
	};

	UIButton m_RoomButtons[2]{
		{0.5137+0.04, 0.2494+0.07, 0.0778, 0.1412}, //0 왼쪽 화살표
		{0.9113+0.036, 0.2494+0.07, 0.071, 0.1412} //1 오른쪽 화살표
	};

	UIButton m_MapButtons[2]{
		{0.055, 0.8, 0.071, 0.1412}, //0 맵 왼쪽 화살표
		{0.563, 0.8, 0.071, 0.1412}  //1 맵 오른쪽 화살표
	};

	UIButton m_REButtons[2]{
		{0.8035, 0.6923, 0.3, 0.1634}, //Ready!
		{0.8035, 0.8966, 0.3, 0.1634} //EXIT
	};

	UIButton m_MenuButton{
		0.50, 0.9227, 0.3713, 0.1033
	};

	ComPtr<ID2D1PathGeometry> m_pPathGeometry;
	ComPtr<ID2D1GeometrySink> m_pSink;
	
	int m_nSelectedCarIndex{ 0 };
	int m_nSelectedMapIndex{ 0 };
	int m_nPlayerIndices[4]{ -1, -1, -1, -1 };
	bool m_bPlayerReady[4]{ false, false, false, false };
	int m_nLastPlayerCount{ 0 };

	// lap
	int m_nCurrentLap = 1;
	int m_nPassedCheckPoints = 0;
	int m_nTotalCheckPoints = 10;
	const int MAX_LAPS = 1;
	WCHAR lapBuffer[64];


	//카운트다운
	bool  m_bRaceStartDelayStarted = false;
	bool  m_bRaceStarted = true;
	float m_fRaceStartDelayTime = 0.0f;
	float m_fRaceStartDelayDuration = 5.0f;
	bool m_bCountdownSoundPlayed = false;

	ComPtr<IDWriteTextFormat> m_textCountdownFormat;
	ComPtr<ID2D1SolidColorBrush> m_textCountdownBrush;


	bool  m_bDashLocked = false;
	float m_fDashLockTime = 0.0f;

	bool  m_bRemoteLockEffectActive = false;
	float m_fRemoteLockEffectTime = 0.0f;


	//

	float m_fDashPotionFlashTime = 0.0f;
	float m_fDashPotionFlashDuration = 0.5f;
	float m_fDashPotionFlashStartGauge = 0.0f;
	float m_fDashPotionFlashEndGauge = 0.0f;

	XMFLOAT3 GetDashGaugeColor() const;
};