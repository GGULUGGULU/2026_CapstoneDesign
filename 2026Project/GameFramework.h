#pragma once

#define FRAME_BUFFER_WIDTH		640
#define FRAME_BUFFER_HEIGHT		480

#include <wincodec.h>
#include "Timer.h"
#include "Player.h"
#include "Scene.h"
#include "NetworkTypes.h"
#include "EffectLibrary.h"
#include "NetworkManager.h"
#include "SoundManager.h"

struct UIButton {
	float xRatio, yRatio, wRatio, hRatio; 
	D2D1_RECT_F rect;                     

	void Update(int screenWidth, int screenHeight) {
		float centerX = screenWidth * xRatio;
		float centerY = screenHeight * yRatio;
		float halfW = (screenWidth * wRatio) * 0.5f;
		float halfH = (screenHeight * hRatio) * 0.5f;
		rect = D2D1::RectF(centerX - halfW, centerY - halfH, centerX + halfW, centerY + halfH);
	}

	bool IsMouseOver(POINT pt) {
		return (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom);
	}
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

	bool StartListenServer(unsigned short port = NET_DEFAULT_PORT);
	bool ConnectToListenServer(const char* pszAddress, unsigned short port = NET_DEFAULT_PORT);
	void SyncMultiplayer();
	PlayerNetState BuildLocalPlayerState() const;
	void ApplyRemotePlayerState(const PlayerNetState& state);
	void CreateRemotePlayer();
	void ReleaseRemotePlayer();
	void SetupPlayerTransform(CPlayer* pPlayer, const XMFLOAT3& xmf3Position, float fYaw);
	void ApplyMultiplayerSpawn();
	void PlayAndSyncEffect(EFFECT_TYPE eType, const XMFLOAT3& xmf3Position, const XMFLOAT2& xmf2Size, const XMFLOAT3& xmf3Color = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void SendEffectEvent(EFFECT_TYPE eType, const XMFLOAT3& xmf3Position, const XMFLOAT2& xmf2Size, const XMFLOAT3& xmf3Color = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void ConsumeNetworkEffectEvents();
	void ConsumeNetworkCollisionEvents();



	void LoadLobbyUIResource();
	void LoadResultUIResource();
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
	CPlayer* m_pRemotePlayer = NULL;
	CCamera* m_pCamera = NULL;
	CNetworkManager* m_pNetwork = NULL;
	float						m_fRemotePlayerYaw = 180.0f;
	bool						m_bMultiplayerEnabled = false;
	bool						m_bIsHostPlayer = false;

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

	ComPtr<IWICImagingFactory> m_pWICFactory;
	ComPtr<ID2D1Bitmap> m_pLobbyD2DBitmap;
	ComPtr<ID2D1Bitmap> m_pResultD2DBitmap;

	ID3D12Resource* m_pd3dShadowMap;
	ID3D12DescriptorHeap* m_pd3dShadowDSVHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dCPUShadowDSVHandle;

	// 아이템 + 대시


	// // ===== 미니맵
	ComPtr<ID2D1Bitmap> m_pMinimapBitmap;

	ComPtr<ID2D1SolidColorBrush> m_minimapBorderBrush;
	ComPtr<ID2D1SolidColorBrush> m_minimapPlayerBrush;
	ComPtr<ID2D1SolidColorBrush> m_minimapFrameBrush;

	void LoadMinimapUIResource();
	D2D1_POINT_2F WorldToMinimap(const XMFLOAT3& worldPos, const D2D1_RECT_F& minimapRect);




private:
	enum ITEM_TYPE
	{
		ITEM_NONE = 0,
		ITEM_DASH_POTION,
		ITEM_MAX_SPEED_UP,
		ITEM_MAX_DASH_GAUGE_UP,
		ITEM_LOCK
	};


	ITEM_TYPE GetItemType(CGameObject* pObject) const;
	void ApplyItemReward(ITEM_TYPE eItemType);
	void UpdateDashSystem(float fTimeElapsed, bool bDashKeyDown, bool bHasDriveInput);
	float GetPlayerEffectiveMaxSpeed() const;

	void SendItemEvent(ITEM_TYPE eItemType, float fDuration);
	void ConsumeNetworkItemEvents();
	void ApplyDashLock(float fDuration);


private:
	float m_fBasePlayerMaxSpeed;
	float m_fSpeedItemBonus;

	float m_fDashSpeedBonus;
	float m_fCurrentDashGauge;
	float m_fMaxDashGauge;
	float m_fDashGaugeConsumePerSecond;
	float m_fDashGaugeRecoverPerSecond;
	float m_fDashGaugeIncreaseAmount;

	bool  m_bIsDashing;

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
	int m_nStage{ 0 };
	bool m_bFlag{ false };
	bool m_bIsStun{ false };
	bool m_bJump{ false };
	int m_nScore{ 0 };

	float m_fMyFinalTime{};
	RaceResultNet m_FinalRaceResult{};

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

	// lap
	int m_nCurrentLap = 1;
	int m_nPassedCheckPoints = 0;
	int m_nTotalCheckPoints = 10;
	const int MAX_LAPS = 3;
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

};


