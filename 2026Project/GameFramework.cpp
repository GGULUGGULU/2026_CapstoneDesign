//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "GameFramework.h"
#include "EffectLibrary.h"
#include "ClientNetworkManager.h"

CGameFramework::CGameFramework()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	m_nRtvDescriptorIncrementSize = 0;
	m_nDsvDescriptorIncrementSize = 0;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pScene = NULL;
	m_pPlayer = NULL;
	m_pNetwork = new CNetworkManager();

	m_nStage = 0;
	m_nScore = 0;
	m_nPlayerCurrentSpeed = 0;
	m_nJumpCount = 0;
	m_bJump = false;
	m_bIsStun = false;

	m_fTotalTime = 0.0f;
	m_fFirstJumpTime = 0.0f;
	m_fJumpCurrentTime = 0.0f;
	m_fSecondJumpWindow = 0.35f; // seconds

	// 아이템 + 대시
	m_fBasePlayerMaxSpeed = 0.0f;
	m_fSpeedItemBonus = 0.0f;
	m_fSpeedItemBonusTime = 0.0f;

	m_bNoDashGaugeConsume = false;
	m_fNoDashGaugeConsumeTime = 0.0f;


	m_fDashSpeedBonus = 150.0f;
	m_fCurrentDashGauge = 100.0f;
	m_fMaxDashGauge = 100.0f;
	m_fDashGaugeConsumePerSecond = 45.0f;
	m_fDashGaugeRecoverPerSecond = 25.0f;
	m_fDashGaugeIncreaseAmount = 50.0f;

	m_bIsDashing = false;
	m_bPrevBoosterSyncActive = false;

	m_bDashLocked = false;
	m_fDashLockTime = 0.0f;
	m_bRemoteLockEffectActive = false;
	m_fRemoteLockEffectTime = 0.0f;

	_tcscpy_s(m_pszFrameRate, _T("2026Project ("));

	wcscpy_s(m_szMyPlayerName, L"Player");

	for (int i = 0; i < 4; ++i)
	{
		swprintf_s(m_szPlayerNames[i], L"Player%d", i + 1);
	}

	m_bNameInputActive = false;
	m_fNameCaretTime = 0.0f;

	m_bShowGameMenu = false;


	m_nGameMenuHoveredIndex = -1;
	m_nGameMenuSelectedIndex = -1;
	m_fBGMVolume = 0.5f;
	m_fSFXVolume = 0.5f;

}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();

	CreateD3D11On12Device();
	CreateD2DDevice();
	CreateTextResources();

	CreateRenderTargetView();
	LoadMinimapUIResource();
	LoadDashVignetteResource();
	LoadDashGaugeFrameResource();
	LoadHelpUIResource();
	LoadSpeedometerUIResource();

	LoadLobbyUIResource();
	LoadResultUIResource();
	LoadRoomUIResource();
	LoadCarImages();
	LoadMapImages();
	LoadReadyImage();
	LoadLoadingImage();

	LoadLoadingImage();
	LoadRankingWaitImage();
	LoadGameMenuResource();

	LoadGameMenuResource();


	CreateDepthStencilView();


	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);


	CEffectLibrary::Instance()->Initialize(m_pd3dDevice, m_pd3dCommandList);


	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	WaitForGpuComplete();

	BuildObjectGameStart();

	CEffectLibrary::Instance()->InitializePostProcess(m_pd3dDevice, m_nWndClientWidth, m_nWndClientHeight);
	
	for (int i = 0; i < 2; ++i) {
		m_RoomButtons[i].shape = static_cast<UIButton::ButtonShape>(UIButton::ButtonShape::TRI_LEFT + i);
		m_MapButtons[i].shape = static_cast<UIButton::ButtonShape>(UIButton::ButtonShape::TRI_LEFT + i);
	}

	m_SoundManager.Init();
	m_SoundManager.SetMasterVolume(0.5f);

	m_pVideoPlayer = std::make_unique<CVideoPlayer>();
	m_pVideoPlayer->Initialize(m_hWnd);

	m_bPlayingIntroVideo = m_pVideoPlayer->Play(
		L"Asset/Video/intro.wmv",
		m_nWndClientWidth,
		m_nWndClientHeight
	);


	if (!m_bPlayingIntroVideo)
	{
		m_SoundManager.PlayBGM("Asset/Audio/TRBGM.mp3");
	}

	return(true);
}

//#define _WITH_CREATE_SWAPCHAIN_FOR_HWND
void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1**)&m_pdxgiSwapChain);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain**)&m_pdxgiSwapChain);
#endif
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	//CreateRenderTargetView();
#endif
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT nDXGIFactoryFlags = 0;
#if defined(_DEBUG)
	ID3D12Debug* pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&pd3dDebugController);
	if (pd3dDebugController)
	{
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void**)&m_pdxgiFactory);

	IDXGIAdapter1* pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice))) break;
	}

	if (!pd3dAdapter)
	{
		m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void**)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void**)&m_pd3dDevice);
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_pd3dFence);
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	HRESULT hResult;

	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void**)&m_pd3dCommandQueue);

	//hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)m_d3dCommandAllocators);
	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_d3dCommandAllocators[0].GetAddressOf()));
	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_d3dCommandAllocators[0].Get(), NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dCommandList);
	// hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_d3dCommandAllocators, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_pd3dCommandList);
	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dRtvDescriptorHeap);
	m_nRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dDsvDescriptorHeap);
	m_nDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void**)&m_pd3dDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, &d3dDepthStencilViewDesc, d3dDsvCPUDescriptorHandle);
}

void CGameFramework::ChangeSwapChainState()
{
	WaitForGpuComplete();

	if (m_d2dDeviceContext) m_d2dDeviceContext->SetTarget(nullptr);
	if (m_d3d11DeviceContext) {
		m_d3d11DeviceContext->ClearState();
		m_d3d11DeviceContext->Flush();
	}

	static bool bIsFullScreen = false;
	static RECT rcWindowed;

	bIsFullScreen = !bIsFullScreen;

	if (bIsFullScreen)
	{
		GetWindowRect(m_hWnd, &rcWindowed);

		HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hMonitor, &mi);

		SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(m_hWnd, HWND_TOP,
			mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_NOZORDER | SWP_FRAMECHANGED);
	}
	else
	{
		SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER | WS_VISIBLE);

		SetWindowPos(m_hWnd, HWND_TOP,
			rcWindowed.left, rcWindowed.top,
			rcWindowed.right - rcWindowed.left, rcWindowed.bottom - rcWindowed.top,
			SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	for (int i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_d2dRenderTargets[i].Reset();
		m_wrappedBackBuffers[i].Reset();
		m_d3dSwapChainBackBuffers[i].Reset();
	}
	if (m_pd3dDepthStencilBuffer) { m_pd3dDepthStencilBuffer->Release(); m_pd3dDepthStencilBuffer = NULL; }

	if (m_d3d11DeviceContext) m_d3d11DeviceContext->Flush();

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	HRESULT hr = m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, 0, 0, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_nWndClientWidth = dxgiSwapChainDesc.BufferDesc.Width;
	m_nWndClientHeight = dxgiSwapChainDesc.BufferDesc.Height;


	if (m_pVideoPlayer)
	{
		m_pVideoPlayer->Resize(m_nWndClientWidth, m_nWndClientHeight);
	}

	CreateRenderTargetView();
	CreateDepthStencilView();

	CEffectLibrary::Instance()->ResizePostProcess(m_pd3dDevice, m_nWndClientWidth, m_nWndClientHeight); // (추가)

	float fAspectRatio = (float)m_nWndClientWidth / (float)m_nWndClientHeight;

	if (m_pCamera)
	{
		m_pCamera->SetViewport(0, 0, m_nWndClientWidth, m_nWndClientHeight, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, m_nWndClientWidth, m_nWndClientHeight);
		m_pCamera->GenerateProjectionMatrix(1.01f, 50000.0f, fAspectRatio, 60.0f);
	}

	if (m_pPlayer && m_pPlayer->GetCamera())
	{
		CCamera* pPlayerCamera = m_pPlayer->GetCamera();
		pPlayerCamera->SetViewport(0, 0, m_nWndClientWidth, m_nWndClientHeight, 0.0f, 1.0f);
		pPlayerCamera->SetScissorRect(0, 0, m_nWndClientWidth, m_nWndClientHeight);
		pPlayerCamera->GenerateProjectionMatrix(1.01f, 50000.0f, fAspectRatio, 60.0f);
	}
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	int mouseX = LOWORD(lParam);
	int mouseY = HIWORD(lParam);

	m_ptMousePos.x = mouseX;
	m_ptMousePos.y = mouseY;

	m_ptMousePos.x = LOWORD(lParam);
	m_ptMousePos.y = HIWORD(lParam);



	// m_nStage
	// 0 -> 메인로비 
	// 1 -> 게임으로 들어가는 중간단계
	// -1 -> 대기룸으로 들어가는 중간단계
	// -2 -> 메인대기룸
	// 2 -> 인게임
	// 99 -> 피니시대기
	// 100 -> 게임결과



	if (m_bShowGameMenu)
	{
		m_nGameMenuHoveredIndex = -1;

		for (int i = 0; i < 3; ++i)
		{
			D2D1_RECT_F r = GetGameMenuButtonRect(i);

			if (m_ptMousePos.x >= r.left &&
				m_ptMousePos.x <= r.right &&
				m_ptMousePos.y >= r.top &&
				m_ptMousePos.y <= r.bottom)
			{
				m_nGameMenuHoveredIndex = i;
				break;
			}
		}

		if (nMessageID == WM_LBUTTONDOWN)
		{
			if (m_nGameMenuHoveredIndex == 1)
			{
				D2D1_RECT_F r = GetGameMenuButtonRect(1);

				float boxW = r.right - r.left;
				float boxH = r.bottom - r.top;

				float sliderLeft = r.left + boxW * 0.22f;
				float sliderRight = r.right - boxW * 0.12f;

				float bgmY = r.top + boxH * 0.45f;
				float sfxY = r.top + boxH * 0.72f;

				auto Clamp01 = [](float v)
					{
						if (v < 0.0f) return 0.0f;
						if (v > 1.0f) return 1.0f;
						return v;
					};

				if (m_ptMousePos.y >= bgmY - 20.0f && m_ptMousePos.y <= bgmY + 20.0f)
				{
					m_fBGMVolume = Clamp01((m_ptMousePos.x - sliderLeft) / (sliderRight - sliderLeft));
					m_SoundManager.SetBGMVolume(m_fBGMVolume);
				}
				else if (m_ptMousePos.y >= sfxY - 20.0f && m_ptMousePos.y <= sfxY + 20.0f)
				{
					m_fSFXVolume = Clamp01((m_ptMousePos.x - sliderLeft) / (sliderRight - sliderLeft));
					m_SoundManager.SetSFXVolume(m_fSFXVolume);
					m_SoundManager.PlaySFX("Asset/Audio/Claxon.mp3"); 
				}

				return;
			}

			if (m_nGameMenuHoveredIndex == 0)
			{
				m_bShowGameMenu = false;
			}
			else if (m_nGameMenuHoveredIndex == 2)
			{
				::PostQuitMessage(0);
			}
		}

		return;
	} // 사운드 설정



	if (0 == m_nStage)
	{
		if (m_bIPInputActive)
		{
			if (nMessageID == WM_LBUTTONDOWN)
			{
				D2D1_RECT_F ipRect = GetIPInputRect();

				if (!(m_ptMousePos.x >= ipRect.left && m_ptMousePos.x <= ipRect.right &&
					m_ptMousePos.y >= ipRect.top && m_ptMousePos.y <= ipRect.bottom))
				{
					m_bIPInputActive = false;
				}
			}
			return;
		}

		m_nHoveredButtonIndex = -1;

		for (int i = 0; i < 3; ++i) {
			if (m_LobbyButtons[i].IsMouseOver(m_ptMousePos)) {
				m_nHoveredButtonIndex = i;
				break;
			}
		}

		if (nMessageID == WM_LBUTTONDOWN)
		{
			D2D1_RECT_F nameRect = GetNameInputRect();

			if (m_ptMousePos.x >= nameRect.left && m_ptMousePos.x <= nameRect.right &&
				m_ptMousePos.y >= nameRect.top && m_ptMousePos.y <= nameRect.bottom)
			{
				m_bNameInputActive = true;
				return;
			}
			else
			{
				m_bNameInputActive = false;
			}


			if (m_nHoveredButtonIndex == 0) {
				SaveNameFromEditControl();

				m_nStage = -1;
				ConnectToServer("127.0.0.1");
			}
			else if (m_nHoveredButtonIndex == 1) {
				SaveNameFromEditControl();

				m_bIPInputActive = true;
				m_bNameInputActive = false;
				wcscpy_s(m_wszServerIP, L"");
				
				//ConnectToServer("127.0.0.1");
			}
				//m_nStage = 1; // 임시, 인게임으로 바로 들어가는 경로
			
			else if (m_nHoveredButtonIndex == 2) {
				::PostQuitMessage(0);
			}
		}
		return;
	}
	else if (-2 == m_nStage) {
		if (m_pScene == nullptr) return;

		m_nHoveredButtonIndex = -1;

		for (int i = 0; i < 2; ++i) {
			if (m_RoomButtons[i].IsMouseOver(m_ptMousePos)) {
				m_nHoveredButtonIndex = i;
				break;
			}
		}

		for (int i = 0; i < 2; ++i) {
			if (m_MapButtons[i].IsMouseOver(m_ptMousePos)) {
				m_nHoveredButtonIndex = 10 + i; 
				break;
			}
		}

		for (int i = 0; i < 2; ++i) {
			if (m_REButtons[i].IsMouseOver(m_ptMousePos)) {
				m_nHoveredButtonIndex = 20 + i;
				break;
			}
		}

		if (nMessageID == WM_LBUTTONDOWN)
		{
			bool changed{ false };

			bool isReady{ m_bPlayerReady[m_nMyPlayerId - 1] };

			if (!isReady) {
				// 모델링 왼쪽 버튼 클릭 시
				if (m_nHoveredButtonIndex == 0)
				{
					--m_nSelectedCarIndex;
					if (m_nSelectedCarIndex < 0) m_nSelectedCarIndex = 2; // 3대 기준
					changed = true;
				}
				// 모델링 오른쪽 버튼 클릭 시
				else if (m_nHoveredButtonIndex == 1) {
					++m_nSelectedCarIndex;
					if (m_nSelectedCarIndex > 2) m_nSelectedCarIndex = 0;
					changed = true;
				}

				if (m_bIsHostPlayer) {
					// 맵 왼쪽 버튼 클릭 시
					if (m_nHoveredButtonIndex == 10) {
						--m_nSelectedMapIndex;
						if (m_nSelectedMapIndex < 0) m_nSelectedMapIndex = 1;
						changed = true;
					}
					// 맵 오른쪽 버튼 클릭 시
					else if (m_nHoveredButtonIndex == 11) {
						++m_nSelectedMapIndex;
						if (m_nSelectedMapIndex > 1) m_nSelectedMapIndex = 0;
						changed = true;
					}
				}
			}

			if (m_nHoveredButtonIndex == 20) {
				m_bPlayerReady[m_nMyPlayerId - 1] = !m_bPlayerReady[m_nMyPlayerId - 1];

				if (m_pNetwork && m_pNetwork->IsConnected()) {
					RoomSyncEventNet syncEvent{};
					syncEvent.playerId = m_nMyPlayerId;
					syncEvent.selectedCarIndex = m_nSelectedCarIndex;
					syncEvent.selectedMapIndex = m_nSelectedMapIndex;
					syncEvent.isReady = m_bPlayerReady[m_nMyPlayerId - 1]; // 내 레디상태 담기
					wcscpy_s(syncEvent.playerName, m_szMyPlayerName);
					m_pNetwork->SendRoomSyncEvent(syncEvent);
				}
			}
			else if (m_nHoveredButtonIndex == 21) {
				if (m_pNetwork) m_pNetwork->Shutdown();

				m_nStage = 0;
				m_bIsHostPlayer = false;

				for (int i = 0; i < 4; ++i) {
					m_nPlayerIndices[i] = -1;
					m_bPlayerReady[i] = false;
					swprintf_s(m_szPlayerNames[i], L"Player%d", i + 1);
				}

				if (m_nMyPlayerId >= 1 && m_nMyPlayerId <= 4)
				{
					wcscpy_s(m_szPlayerNames[m_nMyPlayerId - 1], m_szMyPlayerName);
				}
			}

			if (changed && m_pNetwork && m_pNetwork->IsConnected()) {
				RoomSyncEventNet syncEvent{};
				syncEvent.playerId = m_nMyPlayerId;
				syncEvent.selectedCarIndex = m_nSelectedCarIndex;
				syncEvent.selectedMapIndex = m_nSelectedMapIndex;
				syncEvent.isReady = m_bPlayerReady[m_nMyPlayerId - 1];
				wcscpy_s(syncEvent.playerName, m_szMyPlayerName);
				m_pNetwork->SendRoomSyncEvent(syncEvent);

				if (m_nMyPlayerId >= 1 && m_nMyPlayerId <= 4) {
					m_nPlayerIndices[m_nMyPlayerId - 1] = m_nSelectedCarIndex;
				}
			}
			
		}
	}
	else if (100 == m_nStage) {
		if (m_pScene == nullptr) return;

		m_nHoveredButtonIndex = -1;

		if (m_MenuButton.IsMouseOver(m_ptMousePos)) {
			m_nHoveredButtonIndex = 30;
		}

		if (nMessageID == WM_LBUTTONDOWN)
		{
			if (m_pNetwork) {
				m_pNetwork->Shutdown();
			}

			ReleaseObjects();

			m_SoundManager.StopCarEngine();

			BuildObjectGameStart();

			m_nStage = 0;
			m_bIsHostPlayer = false;
			m_bMultiplayerEnabled = false;

			for (int i = 0; i < 4; ++i) {
				m_nPlayerIndices[i] = -1;
				m_bPlayerReady[i] = false;
			}

			m_nScore = 0;
			m_nCurrentLap = 1;
			m_nPassedCheckPoints = 0;
			m_fMyFinalTime = 0.0f;
			m_fTotalTime = 0.0f;
			m_GameTimer.Reset();
		}
	}

	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (m_nStage != 0)
		{
			::SetCapture(hWnd);
			::GetCursorPos(&m_ptOldCursorPos);
		}
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		if (m_nStage != 0)
		{
			::ReleaseCapture();
		}
		break;

	case WM_MOUSEMOVE:
		break;

	default:
		break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{

	if (m_bShowGameMenu)
	{
		if (nMessageID == WM_KEYUP)
		{
			if (wParam == VK_ESCAPE)
			{
				m_bShowGameMenu = false;
				return;
			}

			if (wParam == VK_F9)
			{
				ChangeSwapChainState();
				return;
			}
		}

		return;
	}


	if (m_bPlayingIntroVideo)
	{
		if (nMessageID == WM_KEYDOWN)
		{
			if (wParam == VK_F9)
			{
				ChangeSwapChainState();

				if (m_pVideoPlayer)
					m_pVideoPlayer->Resize(m_nWndClientWidth, m_nWndClientHeight);

				return;
			}

			if (wParam == VK_ESCAPE)
			{
				m_bShowGameMenu = true;
				return;
			}

			FinishIntroVideo();
		}

		return;
	}


	if (m_pScene) m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_SPACE:
		{
			if (lParam & 0x40000000) break;

			if (m_nStage != 2 || !m_pPlayer || !m_pScene) break;

			const float fFirstJumpVelocity = 150.0f;
			const float fSecondJumpVelocity = 200.0f;
			const float fNow = m_GameTimer.GetTotalTime();

			const bool bOnGround = m_pScene->CheckGroundCollision();

			if (m_nJumpCount == 0)
			{
				if (!bOnGround) break;

				m_nJumpCount = 1;
				m_fFirstJumpTime = fNow;

				XMFLOAT3 v = m_pPlayer->GetVelocity();
				v.y = fFirstJumpVelocity;
				m_pPlayer->SetVelocity(v);
			}
			else if (m_nJumpCount == 1)
			{
				if ((fNow - m_fFirstJumpTime) <= m_fSecondJumpWindow)
				{
					m_nJumpCount = 2;

					XMFLOAT3 v = m_pPlayer->GetVelocity();
					v.y = fSecondJumpVelocity;
					m_pPlayer->SetVelocity(v);
				}
			}
		}
		break;
		case VK_CONTROL:
			if (m_eHoldItem != ITEM_NONE) {
				ApplyItemReward(m_eHoldItem);
				m_eHoldItem = ITEM_NONE;
			}
			break;

		case '1':
			m_eHoldItem = ITEM_DASH_POTION;
			break;
		case '2':
			m_eHoldItem = ITEM_MAX_SPEED_UP;
			break;
		case '3':
			m_eHoldItem = ITEM_MAX_DASH_GAUGE_UP;
			break;

		case '4':
			m_eHoldItem = ITEM_LOCK;
			break;

		default:
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_RETURN:
			break;
		case VK_F1:
		case VK_F2:
		case VK_F3:
			m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());

			// 아이템 + 대시 재적용
			m_fBasePlayerMaxSpeed = m_pPlayer->m_fMaxVelocityXZ;
			m_pPlayer->m_fMaxVelocityXZ = GetPlayerEffectiveMaxSpeed();

			break;
		case VK_F5:
			break;
		case VK_F6:
			ConnectToServer("127.0.0.1");
			break;
		case VK_F9:
			ChangeSwapChainState();
			break;
		case 'S':
			m_pPlayer->SetVelocity(XMFLOAT3(0, 0, 0));
			break;

		case 'P':
		{
			XMFLOAT3 effectColor(1.0f, 0.75f, 0.1f);

			XMFLOAT3 effectPos = m_pPlayer->GetPosition();
			PlayAndSyncEffect(EFFECT_TYPE::ITEM1, effectPos, XMFLOAT2(50, 50));
			PlayAndSyncEffect(EFFECT_TYPE::ITEM2, effectPos, XMFLOAT2(50, 50));
			PlayAndSyncEffect(EFFECT_TYPE::ITEM3, effectPos, XMFLOAT2(50, 50));
			PlayAndSyncEffect(EFFECT_TYPE::ITEM4, effectPos, XMFLOAT2(25, 25));
			PlayAndSyncEffect(EFFECT_TYPE::ITEM5, effectPos, XMFLOAT2(50, 50));
			PlayAndSyncEffect(EFFECT_TYPE::ITEM6, effectPos, XMFLOAT2(50, 50));
			PlayAndSyncEffect(EFFECT_TYPE::ITEM7, effectPos, XMFLOAT2(50, 50));
			PlayAndSyncEffect(EFFECT_TYPE::ITEM8, effectPos, XMFLOAT2(25, 25));
			PlayAndSyncEffect(EFFECT_TYPE::ITEM9, effectPos, XMFLOAT2(50, 50));

			PlayAndSyncEffect(EFFECT_TYPE::ITEM10, effectPos, XMFLOAT2(65.f, 65.f), effectColor);
			PlayAndSyncEffect(EFFECT_TYPE::ITEM11, effectPos, XMFLOAT2(120.f, 120.f), effectColor);

			PlayAndSyncEffect(
				EFFECT_TYPE::ITEM11,
				effectPos,
				XMFLOAT2(120.f, 120.f),
				effectColor
			);
			break;
		}
		case 'R':
			m_pPlayer->Rotate(0, 90, 0);
			break;
		case 'U':
			XMFLOAT3 t = m_pPlayer->GetPosition();
			
			m_pPlayer->SetPosition(XMFLOAT3(t.x, t.y + 10, t.z));
			break;
		case 'J':
			XMFLOAT3 t1 = m_pPlayer->GetPosition();

			m_pPlayer->SetPosition(XMFLOAT3(t1.x, t1.y - 10, t1.z));
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}



LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE)
			m_GameTimer.Stop();
		else
			m_GameTimer.Start();
		break;
	}

	case WM_SIZE:
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;

	case WM_CHAR:
		if (!(m_bShowGameMenu))
		{
			if (m_nStage == 0)
			{
				if (m_bNameInputActive) HandleNameCharInput(wParam);
				else if (m_bIPInputActive) HandleIPCharInput(wParam);
			}
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			if (m_nStage == 0)
			{
				if (m_bNameInputActive) { m_bNameInputActive = false; return 0; }
				if (m_bIPInputActive) { m_bIPInputActive = false; return 0; }
			}

			if (!(lParam & 0x40000000))
			{
				m_bNameInputActive = false;

				bool bOpenMenu = !m_bShowGameMenu;

				m_bShowGameMenu = bOpenMenu;

				if (bOpenMenu)
				{
					m_SoundManager.PlaySFX("Asset/Audio/Robot.mp3");
				}
			}
			return 0;
		}

		if (m_nStage == 0 )
		{
			if (m_bNameInputActive) {
				if (wParam == VK_BACK || wParam == VK_RETURN)
				{
					HandleNameCharInput(wParam);
				}
				else if (wParam >= 'A' && wParam <= 'Z')
				{
					bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
					wchar_t ch = shift ? (wchar_t)wParam : (wchar_t)(wParam + 32);
					HandleNameCharInput(ch);
				}
				else if (wParam >= '0' && wParam <= '9')
				{
					HandleNameCharInput(wParam);
				}
			}
			else if (m_bIPInputActive) {
				if (wParam == VK_RETURN)
				{
					m_bIPInputActive = false;
					SaveNameFromEditControl();
					m_nStage = -1;

					char szIP[32];
					WideCharToMultiByte(CP_ACP, 0, m_wszServerIP, -1, szIP, 32, NULL, NULL);

					ConnectToServer(szIP);
					return 0;
				}
				else if (wParam == VK_BACK)
				{
					HandleIPCharInput(wParam);
				}
				else if (wParam >= '0' && wParam <= '9') 
				{
					HandleIPCharInput(wParam);
				}
				else if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9) 
				{
					HandleIPCharInput((wParam - VK_NUMPAD0) + '0');
				}
				else if (wParam == VK_OEM_PERIOD || wParam == VK_DECIMAL) 
				{
					HandleIPCharInput('.');
				}
				return 0;
			}

			return 0;
		}

		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;

	case WM_KEYUP:
	/*	if (wParam == VK_ESCAPE)
		{
			m_bShowGameMenu = false;
			return 0;
		}*/

		if (m_nStage == 0 && m_bNameInputActive)
		{
			return 0;
		}

		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}

	return 0;
}

void CGameFramework::OnDestroy()
{

	if (m_pVideoPlayer)
	{
		m_pVideoPlayer->Stop();
		m_pVideoPlayer.reset();
	}


	if (m_pLobbyD2DBitmap) m_pLobbyD2DBitmap.Reset();
	if (m_pResultD2DBitmap) m_pResultD2DBitmap.Reset();
	if (m_pWICFactory) m_pWICFactory.Reset();
	if (m_pGameMenuD2DBitmap) m_pGameMenuD2DBitmap.Reset();

	ReleaseObjects();

	if (m_pNetwork){
		m_pNetwork->Shutdown();
		delete m_pNetwork;
		m_pNetwork = NULL;
		m_bMultiplayerEnabled = false;
	}

	CEffectLibrary::Instance()->Release();//

	::CloseHandle(m_hFenceEvent);

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_d3dSwapChainBackBuffers[i].Get()) m_d3dSwapChainBackBuffers[i].Reset();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();

	if (m_d3d11DeviceContext) m_d3d11DeviceContext->Release();

	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	if (m_pd3dFence) m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();

#if defined(_DEBUG)
	IDXGIDebug1* pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif
}

void CGameFramework::BuildObjectGameStart()
{
	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);
	
	m_pScene = new CScene();
	
	if (m_pScene) m_pScene->BuildObjectsGameStart(m_pd3dDevice, m_pd3dCommandList);
	
	//CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	CCarPlayer* pCarPlayer;
	switch (m_nSelectedCarIndex)
	{
		case 0: pCarPlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
		case 1: pCarPlayer = new CCar2Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
		case 2: pCarPlayer = new CCar3Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
		default: pCarPlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	}
	pCarPlayer->SetPosition(XMFLOAT3(.0f, .0f, .0f));
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;
	m_pCamera = m_pPlayer->GetCamera();
	
	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	
	WaitForGpuComplete();
	
	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

	m_GameTimer.Reset();
} // 게임 메인 로비

void CGameFramework::BuildObjectGameRoom()
{
	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);

	m_pScene = new CScene();

	if (m_pScene) m_pScene->BuildObjectsGameRoom(m_pd3dDevice, m_pd3dCommandList);

	//CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	CCarPlayer* pCarPlayer;
	switch (m_nSelectedCarIndex)
	{
	case 0: pCarPlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	case 1: pCarPlayer = new CCar2Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	case 2: pCarPlayer = new CCar3Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	default: pCarPlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	}
	pCarPlayer->SetPosition(XMFLOAT3(.0f, .0f, .0f));
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;
	m_pCamera = m_pPlayer->GetCamera();

	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

	m_GameTimer.Reset();
} // 게임 대기 방

void CGameFramework::ReleaseObjects()
{
	if (m_pPlayer) {
		m_pPlayer->Release();
		m_pPlayer = nullptr;
	}

	ReleaseRemotePlayers();

	if (m_pScene) {
		m_pScene->ReleaseObjects();
		delete m_pScene;
		m_pScene = nullptr;
	}

	if (m_pd3dShadowMap) {
		m_pd3dShadowMap->Release();
		m_pd3dShadowMap = nullptr;
	}

	//if (m_pd3dShadowDSVHeap) {
	//	m_pd3dShadowDSVHeap->Release();
	//	m_pd3dShadowDSVHeap = nullptr;
	//}

	//m_pCamera = nullptr;
}

void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene) bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene)
	{
		DWORD dwDirection = 0;
		if (pKeysBuffer[VK_UP] & 0xF0) dwDirection |= DIR_FORWARD;
		if (pKeysBuffer[VK_DOWN] & 0xF0) dwDirection |= DIR_BACKWARD;
		if (pKeysBuffer[VK_LEFT] & 0xF0) dwDirection |= DIR_LEFT;
		if (pKeysBuffer[VK_RIGHT] & 0xF0) dwDirection |= DIR_RIGHT;
		if (pKeysBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
		if (pKeysBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;

		float cxDelta = 0.0f, cyDelta = 0.0f;
		POINT ptCursorPos;
		if (GetCapture() == m_hWnd)
		{
			SetCursor(NULL);
			GetCursorPos(&ptCursorPos);
			cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
			cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
			SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
		}

		if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
		{
			if (cxDelta || cyDelta)
			{
				if (pKeysBuffer[VK_RBUTTON] & 0xF0)
					m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta);
				else
					m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
			}
			if (dwDirection) m_pPlayer->Move(dwDirection, 1.5f, true);
		}
	}
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::ProcessInputGameStage()
{
	if (m_bShowGameMenu)
	{
		if (m_pPlayer)
		{
			m_pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		}
		return;
	}


	static UCHAR pKeysBuffer[256];
	::ZeroMemory(pKeysBuffer, sizeof(pKeysBuffer));

	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene)
		bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);

	const bool bForward = ((pKeysBuffer[VK_UP] & 0xF0) != 0);
	const bool bBackward = ((pKeysBuffer[VK_DOWN] & 0xF0) != 0);
	const bool bLeft = ((pKeysBuffer[VK_LEFT] & 0xF0) != 0);
	const bool bRight = ((pKeysBuffer[VK_RIGHT] & 0xF0) != 0);
	const bool bHasDriveInput = (bForward || bBackward);
	const bool bDashKeyDown = ((::GetAsyncKeyState('Z') & 0x8000) != 0);


	const float fTimeElapsed = m_GameTimer.GetTimeElapsed();

	UpdateDashSystem(fTimeElapsed, bDashKeyDown, bHasDriveInput);

	if (!bProcessedByScene)
	{
		XMFLOAT3 vCurrentVelocity = m_pPlayer->GetVelocity();
		XMVECTOR vVel = XMLoadFloat3(&vCurrentVelocity);

		float fCurrentSpeed = XMVectorGetX(XMVector3Length(vVel));
		float fMaxSpeed = max(1.0f, m_pPlayer->m_fMaxVelocityXZ);

		float fSpeedRatio = fCurrentSpeed / fMaxSpeed;
		if (fSpeedRatio > 1.0f) fSpeedRatio = 1.0f;


		float fMaxSteeringAngle = 35.0f - (15.0f * fSpeedRatio);
		if (m_bIsDashing)
		{
			fMaxSteeringAngle *= 0.8f;
		}

		float fTargetSteering = 0.0f;
		if (bLeft)      fTargetSteering = -fMaxSteeringAngle;
		else if (bRight) fTargetSteering = fMaxSteeringAngle;

		if (m_pPlayer)
		{
			((CCarPlayer*)m_pPlayer)->UpdateSteering(fTargetSteering, fTimeElapsed);
		}

	
		float fSteeringFactor = 1.0f - (fSpeedRatio * 0.55f);
		if (fSteeringFactor < 0.45f) fSteeringFactor = 0.45f;

		if (m_bIsDashing)
		{
			fSteeringFactor *= 0.75f;
		}

		float fBaseTurnSpeed = 150.0f;
		float fTurnSpeed = fBaseTurnSpeed * fSteeringFactor;

		XMFLOAT3 vLook = m_pPlayer->GetLookVector();
		XMVECTOR vForwardDir = XMLoadFloat3(&vLook);

		float fForwardDirectionSpeed = XMVectorGetX(XMVector3Dot(vVel, XMLoadFloat3(&vLook)));
		float fDirMult = (fForwardDirectionSpeed >= 0.0f) ? 1.0f : -1.0f;

		if (fCurrentSpeed > 1.0f)
		{
			if (bLeft)  m_pPlayer->Rotate(0.0f, -fTurnSpeed * fDirMult * fTimeElapsed, 0.0f);
			if (bRight) m_pPlayer->Rotate(0.0f, +fTurnSpeed * fDirMult * fTimeElapsed, 0.0f);
		}

		float fAccelValue = m_bIsDashing ? 1000.0f : 250.0f;
		XMVECTOR vAcceleration = XMVectorZero();

		if (bForward)
		{
			vAcceleration = vForwardDir * fAccelValue;
		}
		else if (bBackward)
		{
			vAcceleration = -vForwardDir * (fAccelValue * 0.5f);
		}

		vVel += vAcceleration * fTimeElapsed;

		float fHorizontalSpeed = XMVectorGetX(XMVector3Length(vVel));

		if (fHorizontalSpeed > 0.0f)
		{
			XMVECTOR vDir = XMVector3Normalize(vVel);

			float fDecel = 0.0f;

			if (bHasDriveInput)
			{
				
				fDecel = 10.0f;
			}
			else
			{
				
				if (fHorizontalSpeed > 250.0f)
					fDecel = 10.0f;
				else if (fHorizontalSpeed > 150.0f)
					fDecel = 10.0f;
				else if (fHorizontalSpeed > 80.0f)
					fDecel = 20.0f;
				else
					fDecel = 20.0f;
			}

			float fDeltaSpeed = fDecel * fTimeElapsed;

			if (fDeltaSpeed > fHorizontalSpeed)
				fDeltaSpeed = fHorizontalSpeed;

			vVel -= vDir * fDeltaSpeed;
		}


		XMVECTOR vRightDir = XMLoadFloat3(&m_pPlayer->GetRightVector());
		float fRightVelocity = XMVectorGetX(XMVector3Dot(vVel, vRightDir));

		float fGripStrength = 8.0f;
		vVel -= vRightDir * fRightVelocity * fGripStrength * fTimeElapsed;

		float fCurrentMaxSpeed = max(1.0f, m_pPlayer->m_fMaxVelocityXZ);
		float fSpeedSq = XMVectorGetX(XMVector3LengthSq(vVel));
		if (fSpeedSq > fCurrentMaxSpeed * fCurrentMaxSpeed)
		{
			vVel = XMVector3Normalize(vVel) * fCurrentMaxSpeed;
		}


		XMStoreFloat3(&vCurrentVelocity, vVel);
		m_pPlayer->SetVelocity(vCurrentVelocity);

		m_nPlayerCurrentSpeed = (int)XMVectorGetX(XMVector3Length(vVel));

		
		if (m_nPlayerCurrentSpeed > 20)
		{
			if (2 < m_cnt)
			{
				XMFLOAT3 pos = m_pPlayer->GetPosition();
				XMFLOAT3 right = m_pPlayer->GetRightVector();
				XMFLOAT3 look = m_pPlayer->GetLookVector();

				
				CEffectLibrary::Instance()->PlayCarDustParticle(
					EFFECT_TYPE::DUST,
					pos,
					right,
					look,
					XMFLOAT2(5, 5),
					XMFLOAT2(10, 20)
				);

			
				if (m_pNetwork && m_pNetwork->IsConnected())
				{
					EffectEventNet ev{};
					ev.effectType = (int)EFFECT_TYPE::DUST;
					ev.action = 0;

					ev.x = pos.x;
					ev.y = pos.y;
					ev.z = pos.z;

					
					ev.lx = look.x;
					ev.ly = look.y;
					ev.lz = look.z;

					
					ev.r = right.x;
					ev.g = right.y;
					ev.b = right.z;

					ev.sx = 5.0f;
					ev.sy = 5.0f;

					m_pNetwork->SendEffectEvent(ev);
				}

				m_cnt = 0;
			}
			else
			{
				++m_cnt;
			}
		}

	}

	m_pPlayer->Update(fTimeElapsed);
}

void CGameFramework::AnimateObjects()
{
	float fTimeElapsed = m_GameTimer.GetTimeElapsed();

	if (m_pScene) m_pScene->AnimateObjects(fTimeElapsed);

	m_pPlayer->Animate(fTimeElapsed, NULL);


	if (m_pPlayer && m_bIsDashing)
	{
		XMFLOAT3 pos = m_pPlayer->GetPosition();
		XMFLOAT3 look = m_pPlayer->GetLookVector();

		CEffectLibrary::Instance()->UpdateLocalBoosterPosition(pos, look);

		if (m_pNetwork && m_pNetwork->IsConnected())
		{
			EffectEventNet ev{};
			ev.effectType = (int)EFFECT_TYPE::BOOSTER;
			ev.action = 3;

			ev.x = pos.x;
			ev.y = pos.y;
			ev.z = pos.z;

			ev.lx = look.x;
			ev.ly = look.y;
			ev.lz = look.z;

			m_pNetwork->SendEffectEvent(ev);
		}
	}
	else if (m_bPrevBoosterSyncActive)
	{
		if (m_pNetwork && m_pNetwork->IsConnected())
		{
			EffectEventNet ev{};
			ev.effectType = (int)EFFECT_TYPE::BOOSTER;
			ev.action = 2;

			m_pNetwork->SendEffectEvent(ev);
		}
	}

	m_bPrevBoosterSyncActive = m_bIsDashing;

	for (auto& info : m_vRemotePlayers)
	{
		if (info.playerID != -1 && info.pPlayer && info.pPlayer->m_bIsActive)
		{
			info.pPlayer->Animate(fTimeElapsed, NULL);
		}
	}

	float speedRatio = (float)m_nPlayerCurrentSpeed / GetPlayerEffectiveMaxSpeed();
	CEffectLibrary::Instance()->SetPlayerSpeedRatio(speedRatio);

	CEffectLibrary::Instance()->Update(fTimeElapsed);
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGameFramework::ShowFrameRate()
{
	m_GameTimer.GetFrameRate(m_pszFrameRate + 13, 37);
	size_t nLength = _tcslen(m_pszFrameRate);
	XMFLOAT3 xmf3Position = m_pPlayer->GetPosition();
	_stprintf_s(m_pszFrameRate + nLength, 70 - nLength, _T("(%2f, %2f, %2f)"), xmf3Position.x, xmf3Position.y, xmf3Position.z);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}

void CGameFramework::BeforeTransformBarrier(D3D12_RESOURCE_BARRIER& d3dResourceBarrier, ID3D12GraphicsCommandList* d3dCommandList)
{
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get();
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
}

void CGameFramework::AfterTransformBarrier(D3D12_RESOURCE_BARRIER& d3dResourceBarrier, ID3D12GraphicsCommandList* d3dCommandList)
{
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
}

void CGameFramework::ClearRTVDSV(ID3D12DescriptorHeap* d3dRtvDescriptorHeap, ID3D12DescriptorHeap* d3dDsvDescriptorHeap, ID3D12GraphicsCommandList* d3dCommandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = d3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	d3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = d3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	d3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);
}

void CGameFramework::SetUIInfo()
{
	m_fTotalTime = m_GameTimer.GetTotalTime();
}

void CGameFramework::BuildGameObjects()
{
	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);

	if (m_pPlayer)
	{
		m_pPlayer->Release();
		m_pPlayer = NULL;
	}

	ReleaseRemotePlayers();
	
	if (m_pScene)
	{
		m_pScene->ReleaseObjects();
		delete m_pScene;
		m_pScene = NULL;
	}

	// 맵
	m_pScene = new CScene();
	if (m_pScene) {
		if (0 == m_nSelectedMapIndex) {
			m_pScene->m_nCurrentMapStage = 0;
			m_pScene->BuildGameObjects(m_pd3dDevice, m_pd3dCommandList);
		}
		else if (1 == m_nSelectedMapIndex) {
			m_pScene->m_nCurrentMapStage = 1;
			m_pScene->BuildGameStage2(m_pd3dDevice, m_pd3dCommandList);
		}
	}
	CreateShadowMap();

	//CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	//pCarPlayer->SetPosition(XMFLOAT3(-1938.0f, -180.0f, 188.0f));
	CCarPlayer* pCarPlayer;
	switch (m_nSelectedCarIndex)
	{
	case 0: pCarPlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	case 1: pCarPlayer = new CCar2Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	case 2: pCarPlayer = new CCar3Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	default: pCarPlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	}
	pCarPlayer->SetScale(10.2f, 10.2f, 10.2f);
	m_pScene->ApplyMeshTextures(m_pd3dDevice, m_pd3dCommandList, pCarPlayer);
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;

	m_pPlayer->ComputeNewLocalAABB();
	m_pPlayer->SetGravity(XMFLOAT3(0, -1, 0));

	CreateRemotePlayers();
	ApplyMultiplayerSpawn();
	m_pCamera = m_pPlayer->GetCamera();

	// 아이템 + 대시 

	m_fBasePlayerMaxSpeed = m_pPlayer->m_fMaxVelocityXZ;
	m_fSpeedItemBonus = 0.0f;

	m_fDashSpeedBonus = 150.0f;
	m_fMaxDashGauge = 100.0f;
	m_fCurrentDashGauge = m_fMaxDashGauge;
	m_fDashGaugeConsumePerSecond = 45.0f;
	m_fDashGaugeRecoverPerSecond = 25.0f;
	m_fDashGaugeIncreaseAmount = 50.0f;

	m_bIsDashing = false;
	m_bDashOverheated = false;
	m_fDashOverheatTime = 0.0f;

	m_nPlayerCurrentSpeed = 0;
	m_bPrevBoosterSyncActive = false;

	m_bDashLocked = false;
	m_fDashLockTime = 0.0f;


	m_pPlayer->m_fMaxVelocityXZ = GetPlayerEffectiveMaxSpeed();

	//

	m_SoundManager.PlayCarEngine("Asset/Audio/Engine2.mp3");
	m_SoundManager.SetCarEngineVolume(0.15f);

	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();
	
	//m_GameTimer.Reset();
	//m_bRaceStartDelayStarted = false;
	//m_fRaceStartDelayTime = 0.0f;
	//m_bCountdownSoundPlayed = false;
	if (!m_bMultiplayerEnabled) {
		m_GameTimer.Reset();
		m_bRaceStartDelayStarted = false;
		m_fRaceStartDelayTime = 0.f;
		m_bRaceStarted = true;
	}
	else {
		m_bStartSign = true;
		m_bServerStartSign = false;
		m_bRaceStarted = false;
		m_bRaceStartDelayStarted = false;
		m_nLoadedPlayersCnt = 0;
	}
	m_bRaceStarted = !m_bMultiplayerEnabled;
}

float CGameFramework::GetPlayerEffectiveMaxSpeed() const
{
	float fMaxSpeed = m_fBasePlayerMaxSpeed + m_fSpeedItemBonus;

	if (m_bDashOverheated)
	{
		fMaxSpeed *= 0.2f; // 탈진
	}

	if (m_bIsDashing && !m_bDashOverheated)
	{
		fMaxSpeed += m_fDashSpeedBonus;
	}

	return fMaxSpeed;
}



void CGameFramework::ApplyItemReward(ITEM_TYPE eItemType)
{
	if (!m_pPlayer) return;
	XMFLOAT3 effectPos = m_pPlayer->GetPosition();
	effectPos.y += 20.0f;

	XMFLOAT3 effectColor(1.0f, 1.0f, 1.0f);

switch (eItemType)
{
case ITEM_DASH_POTION:
{
	effectColor = XMFLOAT3(1.0f, 0.2f, 0.2f);

	float beforeGauge = m_fCurrentDashGauge;

	float afterGauge = beforeGauge + 30.0f;

	if (afterGauge > m_fMaxDashGauge)
		afterGauge = m_fMaxDashGauge;

	if (afterGauge > beforeGauge)
	{
		m_fDashPotionFlashStartGauge = beforeGauge;
		m_fDashPotionFlashEndGauge = afterGauge;
		m_fDashPotionFlashTime = m_fDashPotionFlashDuration;
	}
	break;
}

case ITEM_MAX_SPEED_UP:
	// effectColor = XMFLOAT3(0.2f, 1.0f, 0.3f);
	// effectColor = XMFLOAT3(0.2f, 0.7f, 1.0f);
	effectColor = XMFLOAT3(0.2f, 1.0f, 0.3f);
	break;

case ITEM_MAX_DASH_GAUGE_UP:
	// effectColor = XMFLOAT3(0.2f, 0.7f, 1.0f);
	//effectColor = XMFLOAT3(0.2f, 1.0f, 0.3f);
	effectColor = XMFLOAT3(0.2f, 0.7f, 1.0f);
	break;

case ITEM_LOCK:
	effectColor = XMFLOAT3(0.75f, 0.4f, 1.0f);
	break;
}
	PlayAndSyncEffect(EFFECT_TYPE::ITEM10, effectPos, XMFLOAT2(65.f, 65.f), effectColor);
	PlayAndSyncEffect(EFFECT_TYPE::ITEM11, effectPos, XMFLOAT2(120.f, 120.f), effectColor);


	switch (eItemType)
	{
	case ITEM_DASH_POTION:
		// 풀충전-> 30만충전
	{
		m_fCurrentDashGauge += 30.0f;

		if (m_fCurrentDashGauge > m_fMaxDashGauge)
			m_fCurrentDashGauge = m_fMaxDashGauge;

		break;
	}

	case ITEM_MAX_SPEED_UP:
		// 3초 동안 최대 속도 증가
		m_fSpeedItemBonus = 50.0f;
		m_fSpeedItemBonusTime = 3.0f;
		break;

	case ITEM_MAX_DASH_GAUGE_UP:
		// 3초 동안 대시 게이지 소모 없이 대시 가능
		m_bNoDashGaugeConsume = true;
		m_fNoDashGaugeConsumeTime = 3.0f;
		break;

	case ITEM_LOCK:
	{
		const float fLockDuration = 3.0f;
		SendItemEvent(ITEM_LOCK, fLockDuration);
		for (auto& info : m_vRemotePlayers) {
			if (info.playerID != -1 && info.pPlayer && info.pPlayer->m_bIsActive == true) {
				PlayLockEffectOnPlayer(info.pPlayer, fLockDuration);
			}
		}

		m_bRemoteLockEffectActive = true;
		m_fRemoteLockEffectTime = fLockDuration;
	}
	break;

	default:
		break;
	}

	m_pPlayer->m_fMaxVelocityXZ = GetPlayerEffectiveMaxSpeed();

	/*if ((float)m_nPlayerCurrentSpeed > m_pPlayer->m_fMaxVelocityXZ)
		m_nPlayerCurrentSpeed = (int)m_pPlayer->m_fMaxVelocityXZ;*/
}

// 아이템 + 대시 
void CGameFramework::UpdateDashSystem(float fTimeElapsed, bool bDashKeyDown, bool bHasDriveInput)
{
	if (!m_pPlayer)
	{
		m_bIsDashing = false;
		CEffectLibrary::Instance()->ToggleLocalBooster(false);
		return;
	}
	
	if (m_bDashOverheated)
	{
		m_fDashOverheatTime -= fTimeElapsed;

		if (m_fDashOverheatTime <= 0.0f)
		{
			m_fDashOverheatTime = 0.0f;
			m_bDashOverheated = false;
		}
	}


	if (m_fSpeedItemBonusTime > 0.0f)
	{
		m_fSpeedItemBonusTime -= fTimeElapsed;

		if (m_fSpeedItemBonusTime <= 0.0f)
		{
			m_fSpeedItemBonusTime = 0.0f;
			m_fSpeedItemBonus = 0.0f;
		}
	}

	
	if (m_fNoDashGaugeConsumeTime > 0.0f)
	{
		m_fNoDashGaugeConsumeTime -= fTimeElapsed;

		if (m_fNoDashGaugeConsumeTime <= 0.0f)
		{
			m_fNoDashGaugeConsumeTime = 0.0f;
			m_bNoDashGaugeConsume = false;
		}
	}

	if (m_bDashLocked)
	{
		m_fDashLockTime -= fTimeElapsed;

		if (m_fDashLockTime <= 0.0f)
		{
			m_fDashLockTime = 0.0f;
			m_bDashLocked = false;
		}

		m_bIsDashing = false;
		CEffectLibrary::Instance()->ToggleLocalBooster(false);

		if (m_pPlayer)
			m_pPlayer->m_fMaxVelocityXZ = GetPlayerEffectiveMaxSpeed();

		return;
	}



	const bool bCanDash =
		(bDashKeyDown && bHasDriveInput &&
			!m_bDashOverheated &&
			(m_bNoDashGaugeConsume || m_fCurrentDashGauge > 0.0f));

	m_bIsDashing = bCanDash;

	if (m_bIsDashing)
	{
		if (!m_bNoDashGaugeConsume)
		{
			m_fCurrentDashGauge -= (m_fDashGaugeConsumePerSecond * fTimeElapsed);

			if (m_fCurrentDashGauge <= 0.0f)
			{
				m_fCurrentDashGauge = 0.0f;
				m_bIsDashing = false;

				m_bDashOverheated = true;
				m_fDashOverheatTime = 0.8f;

				m_SoundManager.PlaySFX("Asset/Audio/Boom.mp3");
			

		
				XMFLOAT3 v = m_pPlayer->GetVelocity();
				XMVECTOR vel = XMLoadFloat3(&v);

				float speed = XMVectorGetX(XMVector3Length(vel));

				if (speed > 50.0f)
				{
					XMVECTOR dir = XMVector3Normalize(vel);
					vel = dir * 50.0f;
					XMStoreFloat3(&v, vel);
					m_pPlayer->SetVelocity(v);
				}

				m_nPlayerCurrentSpeed = 50;
				m_pPlayer->m_fMaxVelocityXZ = 50.0f;
			}
		}
	}
	else
	{
		if (!m_bDashOverheated)
		{
			m_fCurrentDashGauge += (m_fDashGaugeRecoverPerSecond * fTimeElapsed);

			if (m_fCurrentDashGauge > m_fMaxDashGauge)
				m_fCurrentDashGauge = m_fMaxDashGauge;
		}
	}



	float fTargetMaxSpeed = GetPlayerEffectiveMaxSpeed();

	
	float fCurrentMaxSpeed = m_pPlayer->m_fMaxVelocityXZ;


	const float fMaxSpeedFallRate = 200.0f; //

	if (fCurrentMaxSpeed < fTargetMaxSpeed)
	{
		fCurrentMaxSpeed = fTargetMaxSpeed;
	}
	else if (fCurrentMaxSpeed > fTargetMaxSpeed)
	{
		fCurrentMaxSpeed -= (fMaxSpeedFallRate * fTimeElapsed);
		if (fCurrentMaxSpeed < fTargetMaxSpeed)
			fCurrentMaxSpeed = fTargetMaxSpeed;
	}

	m_pPlayer->m_fMaxVelocityXZ = fCurrentMaxSpeed;


	const float fCurrentSpeedFallRate = 300.0f; //
	if ((float)m_nPlayerCurrentSpeed > m_pPlayer->m_fMaxVelocityXZ)
	{
		m_nPlayerCurrentSpeed -= (int)(fCurrentSpeedFallRate * fTimeElapsed);
		if ((float)m_nPlayerCurrentSpeed < m_pPlayer->m_fMaxVelocityXZ)
			m_nPlayerCurrentSpeed = (int)m_pPlayer->m_fMaxVelocityXZ;
		if (m_nPlayerCurrentSpeed < 0)
			m_nPlayerCurrentSpeed = 0;
	}


	float dashRatio = 0.0f;

	if (m_fMaxDashGauge > 0.0f)
		dashRatio = m_fCurrentDashGauge / m_fMaxDashGauge;

	if (dashRatio < 0.0f) dashRatio = 0.0f;
	if (dashRatio > 1.0f) dashRatio = 1.0f;

	// 대시 게이지 25% 이하부터
	if (dashRatio <= 0.25f)
	{
		m_fDashVignetteAlpha = 1.0f - (dashRatio / 0.25f);

	}
	else
	{
		m_fDashVignetteAlpha = 0.0f;
	}
	CEffectLibrary::Instance()->ToggleLocalBooster(m_bIsDashing);

	//m_pPlayer->m_fMaxVelocityXZ = GetPlayerEffectiveMaxSpeed();

	if (m_fDashPotionFlashTime > 0.0f)
	{
		m_fDashPotionFlashTime -= fTimeElapsed;

		if (m_fDashPotionFlashTime < 0.0f)
			m_fDashPotionFlashTime = 0.0f;
	}


}

void CGameFramework::CollisionProcess()
{
	if (m_bMultiplayerEnabled && m_pPlayer)
	{
		BoundingBox localAABB = m_pPlayer->GetCombinedAABB();
		BoundingBox worldAABB_Local;
		localAABB.Transform(worldAABB_Local, XMLoadFloat4x4(&m_pPlayer->GetWorldMatrix()));

		for (auto& info : m_vRemotePlayers) {
			CPlayer* pTargetPlayer = info.pPlayer;
			if (info.playerID == -1 || !pTargetPlayer || !pTargetPlayer->m_bIsActive) continue;
		
			BoundingBox remoteAABB = pTargetPlayer->GetCombinedAABB();
			BoundingBox worldAABB_Remote;
			remoteAABB.Transform(worldAABB_Remote, XMLoadFloat4x4(&pTargetPlayer->GetWorldMatrix()));

			if (worldAABB_Local.Intersects(worldAABB_Remote))
			{
				XMFLOAT3 localPos = m_pPlayer->GetPosition();
				XMFLOAT3 remotePos = pTargetPlayer->GetPosition();

				XMFLOAT3 pushDir = Vector3::Subtract(localPos, remotePos);
				pushDir.y = 0.0f;

				if (Vector3::Length(pushDir) < 0.001f)
					pushDir = m_pPlayer->GetLookVector();
				else
					pushDir = Vector3::Normalize(pushDir);

				const float fSeparation = 8.0f;

				XMFLOAT3 localNewPos = Vector3::Add(
					localPos,
					Vector3::ScalarProduct(pushDir, fSeparation * 0.25f, false)
				);

				XMFLOAT3 remoteNewPos = Vector3::Add(
					remotePos,
					Vector3::ScalarProduct(pushDir, -fSeparation * 0.75f, false)
				);

				m_pPlayer->SetPosition(localNewPos);
				pTargetPlayer->SetPosition(remoteNewPos);

				m_pPlayer->OnPrepareRender();
				pTargetPlayer->OnPrepareRender();

				XMFLOAT3 localVel = m_pPlayer->GetVelocity();
				float localSpeed = max(120.0f, Vector3::Length(localVel));

				float attackerBouncePower = localSpeed * 0.25f;
				float victimBouncePower = localSpeed * 0.90f;

				XMFLOAT3 localBounceVel =
					Vector3::ScalarProduct(pushDir, attackerBouncePower, false);

				XMFLOAT3 remoteBounceVel =
					Vector3::ScalarProduct(pushDir, -victimBouncePower, false);

				m_pPlayer->SetVelocity(localBounceVel);
				pTargetPlayer->SetVelocity(remoteBounceVel);


				if (m_pNetwork && m_pNetwork->IsConnected())
				{
					CollisionEventNet ev{};

					ev.playerId = m_nMyPlayerId;
					ev.type = 1;
					ev.objectIndex = -1;

					ev.x = remoteNewPos.x;
					ev.y = remoteNewPos.y;
					ev.z = remoteNewPos.z;

					ev.nx = -pushDir.x;
					ev.ny = 0.0f;
					ev.nz = -pushDir.z;

					ev.reboundPower = victimBouncePower;

					m_pNetwork->SendCollisionEvent(ev);
				}

				XMFLOAT3 hitPos = XMFLOAT3(
					(localPos.x + remotePos.x) * 0.5f,
					(localPos.y + remotePos.y) * 0.5f + 10.0f,
					(localPos.z + remotePos.z) * 0.5f
				);

				PlayAndSyncEffect(EFFECT_TYPE::COLLISION, hitPos, XMFLOAT2(50, 50), XMFLOAT3(1, 0, 0));
				PlayAndSyncEffect(EFFECT_TYPE::COLLISION, hitPos, XMFLOAT2(50, 50), XMFLOAT3(0, 1, 0));
				PlayAndSyncEffect(EFFECT_TYPE::COLLISION, hitPos, XMFLOAT2(50, 50), XMFLOAT3(0, 0, 1));

				m_bIsStun = true;
				m_fCollisionCurrentTime = m_fTotalTime;

				m_SoundManager.PlaySFX("Asset/Audio/Collision.mp3");

				return;
			}
		}
	} // 플레이어들끼리 충돌 판정

	if (m_pPlayer) m_pPlayer->OnPrepareRender();

	XMFLOAT3 colDirection;

	bool bOnGround = false;

	if (2 == m_nStage) // 게임 스테이지에서
	{
		bOnGround = m_pScene->CheckGroundCollision();

		if (bOnGround) // 바닥 충돌
		{
			m_pPlayer->SetGravity(XMFLOAT3(0, 0, 0));

			XMFLOAT3 currentVel = m_pPlayer->GetVelocity();
			if (currentVel.y != 0.0f) m_pPlayer->SetVelocity(XMFLOAT3(currentVel.x, 0.0f, currentVel.z));

			m_nJumpCount = 0;
			m_bJump = false;
		}
		else
		{
			// 점프 조정
			// 근데 숫자 바꾸면 2단이 잘 안될 때가 있음.
			m_pPlayer->SetGravity(XMFLOAT3(0, -1.5f, 0));
		}
	}

	if (2 == m_nStage && m_pScene->CheckCollision() && !m_bIsStun)
	{
		CGameObject* pCollidedObject = m_pScene->m_pCollidedObject;

		// 체크포인트
		if (pCollidedObject->m_bIsCheckPoint) {
			int hitIndex = pCollidedObject->m_nCheckPointIndex;

			if (hitIndex == m_nPassedCheckPoints + 1) {
				++m_nPassedCheckPoints;

				if (m_nPassedCheckPoints == m_nTotalCheckPoints) {
					++m_nCurrentLap;
					m_nPassedCheckPoints = 0;
					m_SoundManager.PlaySFX("Asset/Audio/Lap.mp3");
					// 통과 사운드들어가면 좋음
				}// 한바퀴 통과
			}

			return;
		}

		if (pCollidedObject->m_bIsItemBox)
		{
			if (m_bMultiplayerEnabled) {
				m_pScene->m_pCollidedObject->m_bIsActive = false;

				MapItemEventNet ev;
				ev.playerId = m_nMyPlayerId;
				ev.itemIndex = m_pScene->m_nCollidedObjectIndex;
				ev.IsActive = false;
				m_pNetwork->SendMapItemEvent(ev);
			}
			else { // 싱글 플레이
				m_SoundManager.PlaySFX("Asset/Audio/LapSound.mp3");

				XMFLOAT3 vPos = pCollidedObject->GetPosition();

				PlayAndSyncEffect(EFFECT_TYPE::ITEM1, vPos, XMFLOAT2(50, 50));
				PlayAndSyncEffect(EFFECT_TYPE::ITEM2, vPos, XMFLOAT2(50, 50));
				PlayAndSyncEffect(EFFECT_TYPE::ITEM3, vPos, XMFLOAT2(50, 50));
				PlayAndSyncEffect(EFFECT_TYPE::ITEM4, vPos, XMFLOAT2(25, 25));
				PlayAndSyncEffect(EFFECT_TYPE::ITEM5, vPos, XMFLOAT2(50, 50));
				PlayAndSyncEffect(EFFECT_TYPE::ITEM6, vPos, XMFLOAT2(50, 50));
				PlayAndSyncEffect(EFFECT_TYPE::ITEM7, vPos, XMFLOAT2(50, 50));
				PlayAndSyncEffect(EFFECT_TYPE::ITEM8, vPos, XMFLOAT2(25, 25));
				PlayAndSyncEffect(EFFECT_TYPE::ITEM9, vPos, XMFLOAT2(50, 50));

				m_fItemDisplayTimer = 3.0f;
				pCollidedObject->m_fInactiveTime = 0.0f;

				pCollidedObject->Disable();
				++m_nScore;

				int randItem = rand() % 4;


				if (randItem == 0) m_eHoldItem = ITEM_DASH_POTION;
				else if (randItem == 1) m_eHoldItem = ITEM_MAX_SPEED_UP;
				else if (randItem == 2) m_eHoldItem = ITEM_MAX_DASH_GAUGE_UP;
				else if (randItem == 3) m_eHoldItem = ITEM_LOCK;
			}
			
		}
		else if (pCollidedObject->m_bIsRCP) {
			int currentRCP = pCollidedObject->m_nCheckPointIndex;
			if (m_nLastRCPIndex != currentRCP) {
				if (currentRCP == 1) {
					m_pPlayer->Rotate(-18, 0, 0);
				}
				else if (currentRCP == 2) {
					m_pPlayer->Rotate(18, 0, 0); 
				}
				else if (currentRCP == 3) {
					m_pPlayer->Rotate(18, 0, 0); 
				}
				else if (currentRCP == 4) {
					m_pPlayer->Rotate(-18, 0, 0);
				}
				m_nLastRCPIndex = currentRCP;
			}
		}
		else if (pCollidedObject->m_bIsInvisibleWall)
		{
			// 벽에 박을때
			m_SoundManager.PlaySFX("Asset/Audio/Collision.mp3");

			XMFLOAT3 vPos = m_pPlayer->GetPosition();
			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, XMFLOAT3(vPos.x, vPos.y + 10, vPos.z), XMFLOAT2(25, 25), XMFLOAT3(1, 1, 0));
			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, XMFLOAT3(vPos.x + 10, vPos.y, vPos.z), XMFLOAT2(25, 25), XMFLOAT3(1, 0, 1));
			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, XMFLOAT3(vPos.x, vPos.y, vPos.z + 10), XMFLOAT2(25, 25), XMFLOAT3(0, 1, 1));

			XMMATRIX mWallWorld = XMLoadFloat4x4(&pCollidedObject->GetWorldMatrix());
			XMVECTOR vDet;
			XMMATRIX mWallInv = XMMatrixInverse(&vDet, mWallWorld);

			XMVECTOR xvPlayerLocal = XMVector3TransformCoord(XMLoadFloat3(&vPos), mWallInv);
			XMFLOAT3 vPlayerLocal;
			XMStoreFloat3(&vPlayerLocal, xvPlayerLocal);

			BoundingBox wallBox = pCollidedObject->m_pMesh->GetBoundingBox();

			float ratioX = abs(vPlayerLocal.x / wallBox.Extents.x);
			float ratioZ = abs(vPlayerLocal.z / wallBox.Extents.z);

			XMVECTOR xvLocalNormal = XMVectorZero();
			if (ratioX > ratioZ)
			{
				xvLocalNormal = XMVectorSet((vPlayerLocal.x > 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				xvLocalNormal = XMVectorSet(0.0f, 0.0f, (vPlayerLocal.z > 0.0f) ? 1.0f : -1.0f, 0.0f);
			}

			XMVECTOR xvNormal = XMVector3TransformNormal(xvLocalNormal, mWallWorld);
			xvNormal = XMVector3Normalize(xvNormal);

			XMFLOAT3 vVelocity = m_pPlayer->GetVelocity();
			XMVECTOR xvVelocity = XMLoadFloat3(&vVelocity);

			float fDot = XMVectorGetX(XMVector3Dot(xvVelocity, xvNormal));

			if (fDot < 0.0f)
			{
				float pushOutDistance = 4.0f;
				XMFLOAT3 correctedPos = m_pPlayer->GetPosition();
				correctedPos.x += XMVectorGetX(xvNormal) * pushOutDistance;
				correctedPos.z += XMVectorGetZ(xvNormal) * pushOutDistance;
				m_pPlayer->SetPosition(correctedPos);

				XMVECTOR xvReflection = xvVelocity - (2.0f * fDot * xvNormal);

				xvReflection = xvReflection * 0.9f;

				XMStoreFloat3(&vVelocity, xvReflection);
				m_pPlayer->SetVelocity(vVelocity);

				m_nPlayerCurrentSpeed = (int)XMVectorGetX(XMVector3Length(xvReflection));
			}
		}
		else if (pCollidedObject != m_pScene->m_ppGameObjects[38])
		{
			XMFLOAT3 vPos = pCollidedObject->GetPosition();

			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, XMFLOAT3(vPos.x, vPos.y + 10, vPos.z), XMFLOAT2(50, 50), XMFLOAT3(1, 0, 0));
			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, XMFLOAT3(vPos.x, vPos.y + 10, vPos.z), XMFLOAT2(50, 50), XMFLOAT3(0, 1, 0));
			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, XMFLOAT3(vPos.x, vPos.y + 10, vPos.z), XMFLOAT2(50, 50), XMFLOAT3(0, 0, 1));

			pCollidedObject->Disable();

			colDirection = Vector3::Subtract(m_pPlayer->GetPosition(), pCollidedObject->GetPosition());
			colDirection.y = 0.0f;
			colDirection = Vector3::Normalize(colDirection);

			float fReboundPower = m_nPlayerCurrentSpeed;
			XMFLOAT3 velocity = Vector3::ScalarProduct(colDirection, fReboundPower, false);

			m_pPlayer->SetVelocity(velocity);

			m_bIsStun = true;
			m_fCollisionCurrentTime = m_fTotalTime;
		}
		else
		{
			colDirection = Vector3::Subtract(m_pPlayer->GetPosition(), pCollidedObject->GetPosition());
			colDirection.y = 0.0f;
			colDirection = Vector3::Normalize(colDirection);

			float fReboundPower = m_nPlayerCurrentSpeed;
			XMFLOAT3 velocity = Vector3::ScalarProduct(colDirection, fReboundPower, false);

			m_pPlayer->SetVelocity(velocity);

			m_bIsStun = true;
			m_fCollisionCurrentTime = m_fTotalTime;
		}
	}

	if (m_bIsStun && m_fTotalTime - m_fCollisionCurrentTime > 1.0f)
	{
		m_bIsStun = false;
	}

}

void CGameFramework::CreateD3D11On12Device()
{
	// Create an 11 device wrapped around the 12 device and share 12's command queue.
	ComPtr<ID3D11Device> d3d11Device;
	D3D11On12CreateDevice(
		m_pd3dDevice,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr,
		0,
		reinterpret_cast<IUnknown**>(&m_pd3dCommandQueue),
		1,
		0,
		&d3d11Device,
		&m_d3d11DeviceContext,
		nullptr
	);

	// Query the 11On12 device from the 11 device.
	//d3d11Device.As(&m_d3d11On12Device);
	d3d11Device.Get()->QueryInterface(
		__uuidof(ID3D11On12Device),
		reinterpret_cast<void**>(m_d3d11On12Device.GetAddressOf())
	);
}

void CGameFramework::CreateD2DDevice()
{
	// Create D2D/DWrite components.
	D2D1_FACTORY_OPTIONS d2dFactoryOptions{};
	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, reinterpret_cast<void**>(m_d2dFactory.GetAddressOf()));

	ComPtr<IDXGIDevice> dxgiDevice;
	m_d3d11On12Device.As(&dxgiDevice);
	m_d2dFactory->CreateDevice(dxgiDevice.Get(), m_d2dDevice.GetAddressOf());
	m_d2dDevice->CreateDeviceContext(deviceOptions, m_d2dDeviceContext.GetAddressOf());
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dWriteFactory.GetAddressOf()));

	HRESULT hrCom = CoInitialize(NULL);

	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_pWICFactory));
}

void CGameFramework::CreateRenderTargetView()
{
	// Query the desktop's dpi settings, which will be used to create D2D's render targets.
	float dpiX;
	float dpiY;
#pragma warning(push)
#pragma warning(disable : 4996) // GetDesktopDpi is deprecated.
	m_d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
#pragma warning(pop)

	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpiX,
		dpiY
	);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	for (UINT i = 0; i < m_nSwapChainBuffers; ++i)
	{
		m_pdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_d3dSwapChainBackBuffers[i]));
		m_pd3dDevice->CreateRenderTargetView(m_d3dSwapChainBackBuffers[i].Get(), NULL, rtvHandle);

		// Create a wrapped 11On12 resource of this back buffer. Since we are 
		// rendering all D3D12 content first and then all D2D content, we specify 
		// the In resource state as RENDER_TARGET - because D3D12 will have last 
		// used it in this state - and the Out resource state as PRESENT. When 
		// ReleaseWrappedResources() is called on the 11On12 device, the resource 
		// will be transitioned to the PRESENT state.
		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		m_d3d11On12Device->CreateWrappedResource(
			m_d3dSwapChainBackBuffers[i].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(m_wrappedBackBuffers[i].GetAddressOf())
		);

		// Create a render target for D2D to draw directly to this back buffer.
		ComPtr<IDXGISurface> surface;
		m_wrappedBackBuffers[i].As(&surface);
		m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&bitmapProperties,
			m_d2dRenderTargets[i].GetAddressOf()
		);

		rtvHandle.Offset(m_nRtvDescriptorIncrementSize);

		if (i > 0 && m_d3dCommandAllocators[i] == nullptr)
			m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_d3dCommandAllocators[i].GetAddressOf()));
	}
}

void CGameFramework::CreateTextResources()
{
	m_dWriteFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"en-us",
		m_textTimeFormat.GetAddressOf()
	);

	m_textTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
	m_textTimeFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);


	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Gray),
		m_textTimeBrush.GetAddressOf()
	);

	m_dWriteFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"en-us",
		m_textSpeedFormat.GetAddressOf()
	);

	m_textSpeedFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_textSpeedFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Yellow),
		m_textSpeedBrush.GetAddressOf()
	);

	m_dWriteFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24.0f,
		L"en-us",
		m_textEndTimeFormat.GetAddressOf()
	);


	// 카운트다운용
	m_dWriteFactory->CreateTextFormat(
		L"Arial",
		NULL,
		DWRITE_FONT_WEIGHT_HEAVY,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		140.0f,
		L"ko-kr",
		&m_textCountdownFormat
	);



	float uiScale = min((float)m_nWndClientHeight / 720.0f, 1.5f);
	float fontSize = 45.0f * uiScale;
	m_dWriteFactory->CreateTextFormat(
		L"맑은 고딕",
		NULL,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		L"ko-kr",
		m_textNameTagFormat.GetAddressOf()
	);

	if (m_textNameTagFormat)
	{
		m_textNameTagFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_textNameTagFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::White),
		m_textNameTagBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.55f),
		m_textNameTagBgBrush.GetAddressOf()
	);

	//


	m_textCountdownFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_textCountdownFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Red),
		&m_textCountdownBrush
	);


	m_textEndTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	m_textEndTimeFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Yellow),
		m_textEndTimeBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Blue),
		m_dashGaugeFillBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(0.2f, 0.2f, 0.2f, 0.8f),
		m_dashGaugeBGBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::DarkGray),
		m_dashGaugeBorderBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.3f),
		m_pBtnHoverBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(0.0f, 1.0f, 0.0f, 0.3f),
		m_pTriBtnHoverBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(1, 1, 1, 0.9f),
		m_minimapBorderBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(0, 0, 0, 0.4f),
		m_minimapFrameBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Red),
		m_minimapPlayerBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Yellow),
		m_minimapOtherBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(0, 0, 0, 1.0f),
		m_menuButtonBrush.GetAddressOf()
	);
	//m_dWriteFactory->CreateTextFormat(
	//	L"맑은 고딕",
	//	NULL,
	//	DWRITE_FONT_WEIGHT_BOLD,
	//	DWRITE_FONT_STYLE_NORMAL,
	//	DWRITE_FONT_STRETCH_NORMAL,
	//	22.0f,
	//	L"ko-kr",
	//	m_textNameTagFormat.GetAddressOf()
	//);

	if (m_textNameTagFormat)
	{
		m_textNameTagFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		m_textNameTagFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::White),
		m_textNameTagBrush.GetAddressOf()
	);

	m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.55f),
		m_textNameTagBgBrush.GetAddressOf()
	);


}

void CGameFramework::RenderUI()
{
	m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[m_nSwapChainBufferIndex].GetAddressOf(), 1);
	m_d2dDeviceContext->SetTarget(m_d2dRenderTargets[m_nSwapChainBufferIndex].Get());
	m_d2dDeviceContext->BeginDraw();

	if (m_nStage == 1)
	{
		m_d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black, 1.0f));

	}
	if (0 == m_nStage) {
		if (m_pLobbyD2DBitmap)
		{
			D2D1_RECT_F destRect = D2D1::RectF(
				0.0f,
				0.0f,
				(float)m_nWndClientWidth,
				(float)m_nWndClientHeight
			);

			m_d2dDeviceContext->DrawBitmap(
				m_pLobbyD2DBitmap.Get(),
				destRect,
				1.0f,
				D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
			);

			for (int i = 0; i < 3; ++i)
			{
				m_LobbyButtons[i].Update(m_nWndClientWidth, m_nWndClientHeight);

				if (m_nHoveredButtonIndex == i)
				{
					m_d2dDeviceContext->FillRoundedRectangle(
						D2D1::RoundedRect(
							m_LobbyButtons[i].rect,
							15.0f,
							15.0f
						),
						m_pBtnHoverBrush.Get()
					);
				}
			}
		}

		DrawNameInputUI();
		DrawIPInputUI();
	}

	else if (-2 == m_nStage) {
		if (m_pRoomD2DBitmap && m_pCarImages && m_pMapImages)
		{
			D2D1_RECT_F destRect = D2D1::RectF(
				0.0f,
				0.0f,
				(float)m_nWndClientWidth,
				(float)m_nWndClientHeight
			);

			m_d2dDeviceContext->DrawBitmap(
				m_pRoomD2DBitmap.Get(),
				destRect,
				1.0f,
				D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
			);

			if (m_pScene && m_pCarImages[m_nSelectedCarIndex]){
				float boxLeft = m_nWndClientWidth * 0.62f;   // 우측 62% 지점
				float boxTop = m_nWndClientHeight * 0.2f;   // 상단 2% 지점
				float boxRight = m_nWndClientWidth * 0.88f;  // 우측 88% 지점
				float boxBottom = m_nWndClientHeight * 0.45f;// 하단 45% 지점

				D2D1_RECT_F carRect = D2D1::RectF(boxLeft, boxTop, boxRight, boxBottom);

				m_d2dDeviceContext->DrawBitmap(
					m_pCarImages[m_nSelectedCarIndex].Get(),
					carRect,
					1.0f,
					D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
				);
			}

			if (m_pScene && m_pMapImages[m_nSelectedMapIndex]) {
				float boxLeft = m_nWndClientWidth * 0.11f;   
				float boxTop = m_nWndClientHeight * 0.61f;  
				float boxRight = m_nWndClientWidth * 0.50f;  
				float boxBottom = m_nWndClientHeight * 0.96f;

				D2D1_RECT_F mapRect = D2D1::RectF(boxLeft, boxTop, boxRight, boxBottom);

				m_d2dDeviceContext->DrawBitmap(
					m_pMapImages[m_nSelectedMapIndex].Get(),
					mapRect,
					1.0f,
					D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
				);
			}

			// 1,2,3,4 플레이어의 모델 띄우기
			for (int i = 0; i < 4; ++i) {

				if (-1 == m_nPlayerIndices[i]) {
					continue;
				}

				float carBoxRatio[16]{
					0.03, 0.04, 0.24, 0.28,
					0.27, 0.04, 0.49, 0.28,
					0.03, 0.32, 0.24, 0.57,
					0.27, 0.32, 0.49, 0.57
				}; //0123 ->p1, 4567 -> p2, 891011 ->p3, 12131415 ->p4

				float boxLeft = m_nWndClientWidth * carBoxRatio[i*4 + 0];
				float boxTop = m_nWndClientHeight * carBoxRatio[i*4 + 1]; 
				float boxRight = m_nWndClientWidth * carBoxRatio[i*4 + 2];
				float boxBottom = m_nWndClientHeight * carBoxRatio[i*4 + 3];

				D2D1_RECT_F carRect = D2D1::RectF(boxLeft, boxTop, boxRight, boxBottom);

				m_d2dDeviceContext->DrawBitmap(
					m_pCarImages[m_nPlayerIndices[i]].Get(),
					carRect,
					1.0f,
					D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
				);

				if (m_bPlayerReady[i]) {
					m_d2dDeviceContext->DrawBitmap(
						m_pReadyImage.Get(),
						carRect,
						0.5f,
						D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
					);
				}
			}

			for (int i = 0; i < 2; ++i)
			{
				m_RoomButtons[i].Update(m_nWndClientWidth, m_nWndClientHeight);

				if (m_nHoveredButtonIndex == i) {
					if (m_RoomButtons[i].shape == UIButton::ButtonShape::RECT)
					{
						m_d2dDeviceContext->FillRectangle(m_RoomButtons[i].rect, m_pBtnHoverBrush.Get());
					}
					else
					{
						m_d2dFactory->CreatePathGeometry(&m_pPathGeometry);
						m_pPathGeometry->Open(&m_pSink);
						m_pSink->BeginFigure(m_RoomButtons[i].tri.point1, D2D1_FIGURE_BEGIN_FILLED);
						m_pSink->AddLine(m_RoomButtons[i].tri.point2);
						m_pSink->AddLine(m_RoomButtons[i].tri.point3);
						m_pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
						m_pSink->Close();

						m_d2dDeviceContext->FillGeometry(m_pPathGeometry.Get(), m_pTriBtnHoverBrush.Get());		
					}
				}
			}

			for (int i = 0; i < 2; ++i)
			{
				m_MapButtons[i].Update(m_nWndClientWidth, m_nWndClientHeight);

				if (m_nHoveredButtonIndex == i + 10) {
					if (m_MapButtons[i].shape == UIButton::ButtonShape::RECT)
					{
						m_d2dDeviceContext->FillRectangle(m_MapButtons[i].rect, m_pBtnHoverBrush.Get());
					}
					else
					{
						m_d2dFactory->CreatePathGeometry(&m_pPathGeometry);
						m_pPathGeometry->Open(&m_pSink);
						m_pSink->BeginFigure(m_MapButtons[i].tri.point1, D2D1_FIGURE_BEGIN_FILLED);
						m_pSink->AddLine(m_MapButtons[i].tri.point2);
						m_pSink->AddLine(m_MapButtons[i].tri.point3);
						m_pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
						m_pSink->Close();

						m_d2dDeviceContext->FillGeometry(m_pPathGeometry.Get(), m_pTriBtnHoverBrush.Get());
					}
				}
			}

			for (int i = 0; i < 2; ++i)
			{
				m_REButtons[i].Update(m_nWndClientWidth, m_nWndClientHeight);

				if (m_nHoveredButtonIndex == i + 20)
				{
					m_d2dDeviceContext->FillRectangle(m_REButtons[i].rect, m_pBtnHoverBrush.Get());
				}
			}
		}
	}
	else if(2 == m_nStage)
	{
		int minutes = static_cast<int>(m_fTotalTime / 60);
		int seconds = static_cast<int>(m_fTotalTime) % 60;
		int milliseconds = static_cast<int>((m_fTotalTime - (minutes * 60 + seconds)) * 100);
		swprintf_s(m_timeBuffer, 1024, L"%02d:%02d:%02d", minutes, seconds, milliseconds);

		//swprintf_s(m_speedBuffer, 1024, L"%d Km/h", m_nPlayerCurrentSpeed);
		//swprintf_s(m_speedBuffer, 1024, L"%d Km/h  [Res: %d x %d]", m_nPlayerCurrentSpeed, m_nWndClientWidth, m_nWndClientHeight);
		//const wchar_t* pwszNetStatus = L"OFF";
		//if (m_pNetwork)
		//{
		//	if (m_pNetwork->IsHosting())
		//		pwszNetStatus = (m_pNetwork->IsConnected() ? L"HOST CONNECTED" : L"HOST WAITING");
		//	else
		//		pwszNetStatus = (m_pNetwork->IsConnected() ? L"CLIENT CONNECTED" : L"CLIENT DISCONNECTED");
		//}

		//swprintf_s(
		//	m_speedBuffer,
		//	1024,
		//	L"%d Km/h  Dash : %.0f / %.0f  Net : %s  [Res: %d x %d]",
		//	m_nPlayerCurrentSpeed / 2,
		//	m_fCurrentDashGauge,
		//	m_fMaxDashGauge,
		//	pwszNetStatus,
		//	m_nWndClientWidth,
		//	m_nWndClientHeight
		//);
		////////////////////////////////////////////////////////////////////
		swprintf_s(lapBuffer, L"LAP %d / %d", m_nCurrentLap, 3);

		m_d2dDeviceContext->DrawTextW(
			lapBuffer,
			(UINT32)wcslen(lapBuffer),
			m_textTimeFormat.Get(),
			D2D1::RectF((float)m_nWndClientWidth - 200.0f, 50.0f, 
				(float)m_nWndClientHeight - 20.0f, 100.0f),
			m_textSpeedBrush.Get()
		);

		m_d2dDeviceContext->DrawTextW(
			m_timeBuffer,
			wcslen(m_timeBuffer),
			m_textTimeFormat.Get(),
			D2D1::RectF(10.0f, 10.0f,
				(float)m_nWndClientWidth - 10.0f,
				(float)m_nWndClientHeight - 10.0f),
			m_textSpeedBrush.Get()
		);

		int nTotalActivePlayers = (m_pNetwork && m_pNetwork->IsConnected()) ? m_pNetwork->GetCurrentPlayerCount() : 1;
		int nMyRank = 1;

		if (m_bMultiplayerEnabled)
		{
			float fMyDistToNextCP = 999999.0f;
			if (m_pScene && m_pScene->m_ppGameObjects)
			{
				int nextCPIndex = m_nPassedCheckPoints + 1;
				int objectIndex = -1;

				if (m_nSelectedMapIndex == 0) objectIndex = nextCPIndex + 2;
				else if (m_nSelectedMapIndex == 1) objectIndex = nextCPIndex + 1;

				if (objectIndex != -1 && m_pScene->m_ppGameObjects[objectIndex])
				{
					XMFLOAT3 nextCPPos = m_pScene->m_ppGameObjects[objectIndex]->GetPosition();
					XMVECTOR vPlayer = XMLoadFloat3(&m_pPlayer->GetPosition());
					XMVECTOR vCP = XMLoadFloat3(&nextCPPos);
					fMyDistToNextCP = XMVectorGetX(XMVector3Length(vCP - vPlayer));
				}
			}

			for (const auto& info : m_vRemotePlayers)
			{
				if (info.playerID != -1 && info.pPlayer && info.pPlayer->m_bIsActive)
				{
					if (info.currentLap > m_nCurrentLap)
					{
						nMyRank++;
					}
					else if (info.currentLap == m_nCurrentLap)
					{
						if (info.passedCheckpoints > m_nPassedCheckPoints)
						{
							nMyRank++;
						}
						else if (info.passedCheckpoints == m_nPassedCheckPoints)
						{
							if (info.distToNextCP < fMyDistToNextCP)
							{
								nMyRank++;
							}
						}
					}
				}
			}
		}

		wchar_t rankBuffer[64];
		swprintf_s(rankBuffer, 64, L"RANK %d / %d", nMyRank, nTotalActivePlayers);

		m_d2dDeviceContext->DrawTextW(
			rankBuffer,
			(UINT32)wcslen(rankBuffer),
			m_textTimeFormat.Get(),
			D2D1::RectF((float)m_nWndClientWidth - 200.0f, 100.0f,
				(float)m_nWndClientWidth - 20.0f, 150.0f),
			m_textSpeedBrush.Get()
		);
		///////////////////////////////////////////////////////////////////
		DrawSpeedometerUI();

		m_d2dDeviceContext->DrawTextW(
			m_speedBuffer,
			wcslen(m_speedBuffer),
			m_textSpeedFormat.Get(),
			D2D1::RectF(10, 10,
				(float)m_nWndClientWidth - 10.0f,
				(float)m_nWndClientHeight - 10.0f),
			m_textSpeedBrush.Get()
		);

		// 연료통 ui
		float dashFrameWidth = m_nWndClientWidth * 0.30f;
		float dashFrameHeight = dashFrameWidth * (1024.0f / 1536.0f);

		float dashFrameLeft = 25.0f;
		float dashFrameBottom = (float)m_nWndClientHeight +15.0f;
		float dashFrameTop = dashFrameBottom - dashFrameHeight;
		float dashFrameRight = dashFrameLeft + dashFrameWidth;

		D2D1_RECT_F dashFrameRect = D2D1::RectF(
			dashFrameLeft,
			dashFrameTop,
			dashFrameRight,
			dashFrameBottom
		);

		if (m_pDashGaugeFrameBitmap)
		{
			m_d2dDeviceContext->DrawBitmap(
				m_pDashGaugeFrameBitmap.Get(),
				dashFrameRect,
				1.0f,
				D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
			);
		}

		// 게이지바
		float dashLeft = dashFrameLeft + dashFrameWidth * 0.28f;
		float dashRight = dashFrameLeft + dashFrameWidth * 0.72f;
		float dashTop = dashFrameTop + dashFrameHeight * 0.465f;
		float dashBottom = dashFrameTop + dashFrameHeight * 0.585f;

		float dashRatio = 0.0f;

		if (m_fMaxDashGauge > 0.0f)
			dashRatio = m_fCurrentDashGauge / m_fMaxDashGauge;

		if (dashRatio < 0.0f) dashRatio = 0.0f;
		if (dashRatio > 1.0f) dashRatio = 1.0f;

		XMFLOAT3 gaugeColor = GetDashGaugeColor();

		m_dashGaugeFillBrush->SetColor(
			D2D1::ColorF(gaugeColor.x, gaugeColor.y, gaugeColor.z, 1.0f)
		);

		float dashGaugeWidth = dashRight - dashLeft;
		float dashFillRight = dashLeft + (dashGaugeWidth * dashRatio);

		D2D1_RECT_F dashFillRect = D2D1::RectF(
			dashLeft,
			dashTop,
			dashFillRight,
			dashBottom
		);

		m_d2dDeviceContext->FillRectangle(
			&dashFillRect,
			m_dashGaugeFillBrush.Get()
		);

		if (m_fDashPotionFlashTime > 0.0f && m_fMaxDashGauge > 0.0f)
		{
			float startRatio = m_fDashPotionFlashStartGauge / m_fMaxDashGauge;
			float endRatio = m_fDashPotionFlashEndGauge / m_fMaxDashGauge;

			if (startRatio < 0.0f) startRatio = 0.0f;
			if (startRatio > 1.0f) startRatio = 1.0f;
			if (endRatio < 0.0f) endRatio = 0.0f;
			if (endRatio > 1.0f) endRatio = 1.0f;

			float flashLeft = dashLeft + (dashGaugeWidth * startRatio);
			float flashRight = dashLeft + (dashGaugeWidth * endRatio);

			float alpha = m_fDashPotionFlashTime / m_fDashPotionFlashDuration;

			ComPtr<ID2D1SolidColorBrush> flashBrush;

			m_d2dDeviceContext->CreateSolidColorBrush(
				D2D1::ColorF(1.0f, 0.2f, 0.2f, alpha),
				&flashBrush
			);

			D2D1_RECT_F flashRect = D2D1::RectF(
				flashLeft,
				dashTop,
				flashRight,
				dashBottom
			);

			m_d2dDeviceContext->FillRectangle(
				&flashRect,
				flashBrush.Get()
			);
		}



		///////////////////////////
		//float ItemWidth = 60.0f;   // 게이지 너비
		//float ItemHeight = 90.0f; // 게이지 높이
		//float ItemmarginX = 10.0f;      // 좌측 여백
		//float ItemmarginY = 120.0f;      // 하단 여백
		//
		//float Itemleft = m_nWndClientWidth * (0.5f * (-0.97f + 1.0f));
		//float Itemright = m_nWndClientWidth * (0.5f * (-0.78f + 1.0f));
		//float Itemtop = m_nWndClientHeight * (0.5f * (1.0f - 0.90f));
		//float Itembottom = m_nWndClientHeight * (0.5f * (1.0f - 0.50f));
		//
		//D2D1_RECT_F ItemBgRect = D2D1::RectF(Itemleft, Itemtop, Itemright, Itembottom);
		//m_d2dDeviceContext->FillRectangle(&ItemBgRect, m_dashGaugeBGBrush.Get());

		float screenW = (float)m_nWndClientWidth;
		float screenH = (float)m_nWndClientHeight;

			
		float minimapH = screenH * 0.25f;
		float minimapW = minimapH;

		minimapH = max(150.0f, min(350.0f, minimapH));

		if (m_pMinimapBitmaps)
		{
			auto size = m_pMinimapBitmaps[0]->GetSize();
			minimapW = minimapH * (size.width / size.height);
		}

		float margin = screenH * 0.03f;


		D2D1_RECT_F frameRect = D2D1::RectF(
			(float)m_nWndClientWidth - minimapW - margin - 2,
			(float)m_nWndClientHeight - minimapH - margin - 2,
			(float)m_nWndClientWidth - margin + 2,
			(float)m_nWndClientHeight - margin + 2
		);

		D2D1_RECT_F minimapRect = D2D1::RectF(
			(float)m_nWndClientWidth - minimapW - margin,
			(float)m_nWndClientHeight - minimapH - margin,
			(float)m_nWndClientWidth - margin,
			(float)m_nWndClientHeight - margin
		);

			
		m_d2dDeviceContext->FillRoundedRectangle(
			D2D1::RoundedRect(frameRect, 12, 12),
			m_minimapFrameBrush.Get()
		);

			
		if (m_pMinimapBitmaps[m_nSelectedMapIndex])
		{
			m_d2dDeviceContext->DrawBitmap(
				m_pMinimapBitmaps[m_nSelectedMapIndex].Get(),
				minimapRect
			);
		}

		// 미니맵 테두리
		m_d2dDeviceContext->DrawRoundedRectangle(
			D2D1::RoundedRect(frameRect, 12, 12),
			m_minimapBorderBrush.Get(),
			4.0f
		);

		if (m_pPlayer)
		{
			D2D1_POINT_2F pt = WorldToMinimap(
				m_pPlayer->GetPosition(),
				minimapRect
			);

			m_d2dDeviceContext->FillEllipse(
				D2D1::Ellipse(pt, 5, 5),
				m_minimapPlayerBrush.Get()
			);
		}

		if (m_bMultiplayerEnabled) {
			for (auto& info : m_vRemotePlayers) {
				if (info.playerID != -1 && info.pPlayer && info.pPlayer->m_bIsActive) {
					D2D1_POINT_2F remotePt = WorldToMinimap(
						info.pPlayer->GetPosition(),
						minimapRect
					);

					m_d2dDeviceContext->FillEllipse(
						D2D1::Ellipse(remotePt, 5, 5),
						m_minimapOtherBrush.Get()
					);
				}
			}
		}
		
		// 카운트다운
		if (m_nStage == 2 &&
			m_bMultiplayerEnabled &&
			m_pNetwork &&
			m_pNetwork->IsConnected() &&
			!m_bRaceStarted)
		{
			float remain = m_fRaceStartDelayDuration - m_fRaceStartDelayTime;
			int count = (int)ceilf(remain);

			if (count > 0)
			{
				WCHAR text[32];
				swprintf_s(text, L"%d", count);

				D2D1_RECT_F rect = D2D1::RectF(
					0.0f,
					0.0f,
					(float)m_nWndClientWidth,
					(float)m_nWndClientHeight
				);

				m_d2dDeviceContext->DrawText(
					text,
					(UINT32)wcslen(text),
					m_textCountdownFormat.Get(),
					rect,
					m_textCountdownBrush.Get()
					
				);
			}
		}


	
	}

	if (m_nStage == 1)
	{
		DrawLoadingImage();
	}

	if (m_nStage == 99 && m_bShowRankingWaitImage)
	{
		DrawRankingWaitImage();
	}

	else if (100 == m_nStage)
	{
		if (m_pResultD2DBitmap)
		{
			D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight);
			m_d2dDeviceContext->DrawBitmap(m_pResultD2DBitmap.Get(), destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
			OutputDebugStringW(L"나옴");
		}
		wchar_t resultBuffer[512] = L"===== RACE RESULTS =====\n\n";
		wchar_t tempBuffer[128];

		std::uint32_t myId = (m_nMyPlayerId > 0) ? m_nMyPlayerId : 1;
		int myRank = -1;

		for (std::uint32_t i = 0; i < m_FinalRaceResult.playerCount; ++i)
		{
			if (m_FinalRaceResult.playerRecords[i].playerId == myId)
			{
				myRank = i + 1;
				break;
			}
		}

		if (myRank != -1)
		{
			swprintf_s(tempBuffer, 128, L"[Your Rank: %d Place!]\n\n", myRank);
			wcscat_s(resultBuffer, tempBuffer);
		}

		for (std::uint32_t i = 0; i < m_FinalRaceResult.playerCount; ++i)
		{
			const RaceRecordNet& record = m_FinalRaceResult.playerRecords[i];

			const wchar_t* name = record.playerName;

			if (wcslen(name) <= 0)
			{
				int playerIndex = static_cast<int>(record.playerId) - 1;

				if (playerIndex >= 0 && playerIndex < 4 && wcslen(m_szPlayerNames[playerIndex]) > 0)
				{
					name = m_szPlayerNames[playerIndex];
				}
				else
				{
					name = L"Player";
				}
			}

			swprintf_s(
				tempBuffer,
				128,
				L"%d Place - %s : %.2f sec\n",
				i + 1,
				name,
				record.finishTime
			);

			wcscat_s(resultBuffer, tempBuffer);
		}

		m_d2dDeviceContext->DrawTextW(
			resultBuffer,
			wcslen(resultBuffer),
			m_textEndTimeFormat.Get(),
			D2D1::RectF(0.0f, 0.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight),
			m_textEndTimeBrush.Get()
		);

		D2D1_RECT_F destRect = D2D1::RectF(
			0.0f,
			0.0f,
			(float)m_nWndClientWidth,
			(float)m_nWndClientHeight
		);


		m_MenuButton.Update(m_nWndClientWidth, m_nWndClientHeight);

		if (m_nHoveredButtonIndex == 30)
		{
			m_d2dDeviceContext->FillRectangle(m_MenuButton.rect, m_pBtnHoverBrush.Get());
		}
	}

		
		
	if (m_nStage == 2 && m_pDashVignetteBitmap && m_fDashVignetteAlpha > 0.01f)
	{
		D2D1_RECT_F fullScreenRect = D2D1::RectF(
			0.0f,
			0.0f,
			(float)m_nWndClientWidth,
			(float)m_nWndClientHeight
		);

		m_d2dDeviceContext->DrawBitmap(
			m_pDashVignetteBitmap.Get(),
			fullScreenRect,
			m_fDashVignetteAlpha,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		);
	}

	DrawPlayerNameTags();

	if (m_bShowHelpUI)
	{
		DrawHelpUI();
	}

	if (m_bShowGameMenu)
	{
		DrawGameMenuUI();
	}

	m_d2dDeviceContext->EndDraw();
	m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[m_nSwapChainBufferIndex].GetAddressOf(), 1);
	m_d3d11DeviceContext->Flush();
}

void CGameFramework::BuildObjectEnd()
{
	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);

	if (m_pPlayer)
	{
		m_pPlayer->Release();
		m_pPlayer = NULL;
	}
	ReleaseRemotePlayers();
	if (m_pScene)
	{
		m_pScene->ReleaseObjects();
		delete m_pScene;
		m_pScene = NULL;
	}

	m_pScene = new CScene();

	if (m_pScene) m_pScene->BuildObjectsGameEnd(m_pd3dDevice, m_pd3dCommandList);

	//CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	CCarPlayer* pCarPlayer;
	switch (m_nSelectedCarIndex)
	{
	case 0: pCarPlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	case 1: pCarPlayer = new CCar2Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	case 2: pCarPlayer = new CCar3Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	default: pCarPlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
	}
	pCarPlayer->SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;
	m_pCamera = m_pPlayer->GetCamera();

	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();
}

void CGameFramework::CreateShadowMap()
{
	D3D12_RESOURCE_DESC d3dShadowMapDesc = {};
	d3dShadowMapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dShadowMapDesc.Alignment = 0;
	d3dShadowMapDesc.Width = 16384;
	d3dShadowMapDesc.Height = 16384;
	d3dShadowMapDesc.DepthOrArraySize = 1;
	d3dShadowMapDesc.MipLevels = 1;
	d3dShadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	d3dShadowMapDesc.SampleDesc.Count = 1;
	d3dShadowMapDesc.SampleDesc.Quality = 0;
	d3dShadowMapDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dShadowMapDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties = {};
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue = {};
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(
		&d3dHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&d3dShadowMapDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&d3dClearValue,
		__uuidof(ID3D12Resource),
		(void**)&m_pd3dShadowMap
	);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	m_pd3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pd3dShadowDSVHeap));

	m_d3dCPUShadowDSVHandle = m_pd3dShadowDSVHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;

	m_pd3dDevice->CreateDepthStencilView(m_pd3dShadowMap, &dsvDesc, m_d3dCPUShadowDSVHandle);

	if (m_pScene)
	{
		m_pScene->CreateShadowMapSRV(m_pd3dDevice, m_pd3dShadowMap);
	}
}

void CGameFramework::RenderShadowPass()
{
	if (!m_pScene || !m_pd3dShadowMap) return;

	D3D12_RESOURCE_BARRIER toShadowWrite = {};
	toShadowWrite.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toShadowWrite.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	toShadowWrite.Transition.pResource = m_pd3dShadowMap;
	toShadowWrite.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	toShadowWrite.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	toShadowWrite.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_pd3dCommandList->ResourceBarrier(1, &toShadowWrite);

	m_pScene->RenderShadowMap(m_pd3dCommandList, m_d3dCPUShadowDSVHandle);
	for (auto& info : m_vRemotePlayers) {
		if (info.playerID != -1 && info.pPlayer && info.pPlayer->m_bIsActive) {
			info.pPlayer->Render(m_pd3dCommandList, NULL, NULL);
		}
	}

	D3D12_RESOURCE_BARRIER toGenericRead = {};
	toGenericRead.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toGenericRead.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	toGenericRead.Transition.pResource = m_pd3dShadowMap;
	toGenericRead.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	toGenericRead.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	toGenericRead.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_pd3dCommandList->ResourceBarrier(1, &toGenericRead);
}

void CGameFramework::SetMainViewport()
{
	D3D12_VIEWPORT viewport = { 0, 0, (float)m_nWndClientWidth, (float)m_nWndClientHeight, 0.0f, 1.0f };
	D3D12_RECT scissorRect = { 0, 0, m_nWndClientWidth, m_nWndClientHeight };

	m_pd3dCommandList->RSSetViewports(1, &viewport);
	m_pd3dCommandList->RSSetScissorRects(1, &scissorRect);
}


void CGameFramework::SetupPlayerTransform(CPlayer* pPlayer, const XMFLOAT3& xmf3Position, float fYaw)
{
	if (!pPlayer) return;

	pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	pPlayer->SetPosition(xmf3Position);

	float fDeltaYaw = fYaw - pPlayer->GetYaw();
	if (fDeltaYaw > 180.0f) fDeltaYaw -= 360.0f;
	if (fDeltaYaw < -180.0f) fDeltaYaw += 360.0f;

	if (fabsf(fDeltaYaw) > 0.001f)
	{
		pPlayer->Rotate(0.0f, fDeltaYaw, 0.0f);
	}

	pPlayer->OnPrepareRender();
}

void CGameFramework::ApplyMultiplayerSpawn()
{
	if (m_pPlayer)
	{
		XMFLOAT3 xmf3LocalSpawn = XMFLOAT3(0, 0, 0);
		if (0 == m_nSelectedMapIndex) {
			if (m_bMultiplayerEnabled)
			{
				if (m_nMyPlayerId == 1) xmf3LocalSpawn = Map1PlayerSpawnPos[0];
				else if (m_nMyPlayerId == 2) xmf3LocalSpawn = Map1PlayerSpawnPos[1];
				else if (m_nMyPlayerId == 3)xmf3LocalSpawn = Map1PlayerSpawnPos[2];
				else if (m_nMyPlayerId == 4) xmf3LocalSpawn = Map1PlayerSpawnPos[3];
				// 3, 4 번 플레이어 위치 추가해야함
			}
		}
		else if (1 == m_nSelectedMapIndex) {
			if (m_bMultiplayerEnabled)
			{
				if (m_nMyPlayerId == 1) xmf3LocalSpawn = Map2PlayerSpawnPos[0];
				else if (m_nMyPlayerId == 2) xmf3LocalSpawn = Map2PlayerSpawnPos[1];
				else if (m_nMyPlayerId == 3)xmf3LocalSpawn = Map2PlayerSpawnPos[2];
				else if (m_nMyPlayerId == 4) xmf3LocalSpawn = Map2PlayerSpawnPos[3];
				// 3, 4 번 플레이어 위치 추가해야함
			}
		}
		
		SetupPlayerTransform(m_pPlayer, xmf3LocalSpawn, PLAYER_SPAWN_YAW);
		m_nPlayerCurrentSpeed = 0;
	}

	for (auto& info : m_vRemotePlayers) {
		if (info.pPlayer) {
			XMFLOAT3 baseSpawn = (m_nSelectedMapIndex == 0) ? Map1SinglePlayerSpawn : Map2SinglePlayerSpawn;
			SetupPlayerTransform(info.pPlayer, baseSpawn, PLAYER_SPAWN_YAW);
			info.pPlayer->m_bIsActive = false;
			info.yaw = PLAYER_SPAWN_YAW;
		}
	}
}

bool CGameFramework::ConnectToServer(const char* pszAddress, unsigned short port)
{
	if (m_pNetwork) {
		m_pNetwork->Shutdown();
		delete m_pNetwork;
	}

	m_pNetwork = new CNetworkManager(); 

	bool bConnectSuccess = m_pNetwork->ConnectToServer(pszAddress, port);

	if (!bConnectSuccess)
	{
		::MessageBoxW(m_hWnd, L"서버연결 실패", L"연결 실패", MB_OK | MB_ICONERROR);

		m_bMultiplayerEnabled = false;
		return false;
	}

	m_bMultiplayerEnabled = bConnectSuccess;
	m_bIsHostPlayer = false;

	if (m_bMultiplayerEnabled && (m_nStage == 2))
	{
		m_bNeedRemotePlayerInit = true;

		m_bRaceStarted = false;
		m_bRaceStartDelayStarted = false;
		m_fRaceStartDelayTime = 0.0f;
	}

	return(m_bMultiplayerEnabled);
}

PlayerNetState CGameFramework::BuildLocalPlayerState() const
{
	PlayerNetState state{};

	if (!m_pPlayer) return(state);

	const XMFLOAT3 position = m_pPlayer->GetPosition();

	state.playerId = m_nMyPlayerId;
	state.x = position.x;
	state.y = position.y;
	state.z = position.z;
	state.yaw = m_pPlayer->GetYaw();
	state.speed = static_cast<float>(m_nPlayerCurrentSpeed);
	state.stage = static_cast<unsigned int>(m_nStage);
	state.score = static_cast<unsigned int>(m_nScore);
	state.currentLap = static_cast<std::uint32_t>(m_nCurrentLap);
	state.passedCheckpoints = static_cast<std::uint32_t>(m_nPassedCheckPoints);

	float fDistToNext = 999999.0f; 

	if (m_pScene && m_pScene->m_ppGameObjects)
	{
		int nextCPIndex = m_nPassedCheckPoints + 1;
		int objectIndex = -1;

		if (m_nSelectedMapIndex == 0)
		{
			objectIndex = nextCPIndex + 2;
		}
		else if (m_nSelectedMapIndex == 1)
		{
			objectIndex = nextCPIndex + 1;
		}

		if (objectIndex != -1 && m_pScene->m_ppGameObjects[objectIndex])
		{
			XMFLOAT3 nextCPPos = m_pScene->m_ppGameObjects[objectIndex]->GetPosition();

			XMVECTOR vPlayer = XMLoadFloat3(&position);
			XMVECTOR vCP = XMLoadFloat3(&nextCPPos);
			fDistToNext = XMVectorGetX(XMVector3Length(vCP - vPlayer));
		}
	}

	state.distToNextCP = fDistToNext;

	return(state);
}

void CGameFramework::ApplyRemotePlayerState(const PlayerNetState& state)
{
	if (state.playerId == m_nMyPlayerId) return;
	
	RemotePlayerInfo* pInfo = FindOrAllocateRemotePlayer(state.playerId);
	if (!pInfo || !pInfo->pPlayer) return;

	CPlayer* pTargetPlayer = pInfo->pPlayer;
	
	pTargetPlayer->m_bIsActive = true;
	pTargetPlayer->SetVelocity(XMFLOAT3(0, 0, 0));
	pTargetPlayer->SetPosition(XMFLOAT3(state.x, state.y, state.z));

	float fDeltaYaw = state.yaw - pInfo->yaw;
	if (fDeltaYaw > 180.0f) fDeltaYaw -= 360.0f;
	if (fDeltaYaw < -180.0f) fDeltaYaw += 360.0f;

	if (fabsf(fDeltaYaw) > 0.001f)
	{
		pTargetPlayer->Rotate(0.0f, fDeltaYaw, 0.0f);
	}

	pInfo->yaw = state.yaw;

	pInfo->currentLap = state.currentLap;
	pInfo->passedCheckpoints = state.passedCheckpoints;

	pInfo->distToNextCP = state.distToNextCP;

	pTargetPlayer->OnPrepareRender();
}

void CGameFramework::SyncMultiplayer()
{
	if (!m_pNetwork || !m_pNetwork->IsConnected()) return;

	if (m_nStage <= 0)
	{
		SyncRoom();   
	}
	else if(m_nStage<100)
	{
		SyncInGame();  
	}
}

void CGameFramework::SyncRoom()
{
	m_pNetwork->Update(0.0f, nullptr);

	int assignedId = 0;
	if (m_pNetwork->ConsumeWelomeEvent(assignedId))
	{
		m_nMyPlayerId = assignedId;
		m_bIsHostPlayer = (m_nMyPlayerId == 1);

		if (m_nMyPlayerId >= 1 && m_nMyPlayerId <= 4) {
			m_nPlayerIndices[m_nMyPlayerId - 1] = m_nSelectedCarIndex;
		}

		RoomSyncEventNet syncEvent{};
		syncEvent.playerId = m_nMyPlayerId;
		syncEvent.selectedCarIndex = m_nSelectedCarIndex;
		syncEvent.selectedMapIndex = m_nSelectedMapIndex;
		syncEvent.isReady = m_bPlayerReady[m_nMyPlayerId - 1];
		wcscpy_s(syncEvent.playerName, m_szMyPlayerName);
		m_pNetwork->SendRoomSyncEvent(syncEvent);
	}

	RoomSyncEventNet roomSyncEv{};
	while (m_pNetwork->ConsumeRoomSyncEvent(roomSyncEv))
	{
		if (roomSyncEv.playerId == 1) {
			m_nSelectedMapIndex = roomSyncEv.selectedMapIndex;
		}

		if (roomSyncEv.playerId >= 1 && roomSyncEv.playerId <= 4) {

			if (wcslen(roomSyncEv.playerName) > 0)
			{
				wcscpy_s(m_szPlayerNames[roomSyncEv.playerId - 1], roomSyncEv.playerName);
			}

			if (roomSyncEv.selectedCarIndex == -1) {
				m_nPlayerIndices[roomSyncEv.playerId - 1] = -1;
				m_bPlayerReady[roomSyncEv.playerId - 1] = false;
			}
			else {
				m_nPlayerIndices[roomSyncEv.playerId - 1] = roomSyncEv.selectedCarIndex;
				m_bPlayerReady[roomSyncEv.playerId - 1] = roomSyncEv.isReady;
			}
		}
	}

	int currentTotalPlayers = m_pNetwork->GetCurrentPlayerCount();

	if (m_nLastPlayerCount != currentTotalPlayers)
	{
		m_nLastPlayerCount = currentTotalPlayers;

		if (m_pNetwork->IsConnected() && m_nMyPlayerId >= 1 && m_nMyPlayerId <= 4) {
			RoomSyncEventNet syncEvent{};
			syncEvent.playerId = m_nMyPlayerId;
			syncEvent.selectedCarIndex = m_nSelectedCarIndex;
			syncEvent.selectedMapIndex = m_nSelectedMapIndex;
			syncEvent.isReady = m_bPlayerReady[m_nMyPlayerId - 1];
			wcscpy_s(syncEvent.playerName, m_szMyPlayerName);
			m_pNetwork->SendRoomSyncEvent(syncEvent);
		}
	}

	if (currentTotalPlayers > 0)
	{
		int readyCount = 0;
		for (int i = 0; i < 4; ++i) {
			if (m_nPlayerIndices[i] != -1 && m_bPlayerReady[i]) {
				readyCount++;
			}
		}

		if (readyCount == currentTotalPlayers)
		{
			m_nStage = 1; 
		}
	}
}

void CGameFramework::SyncInGame()
{
	if (!m_pNetwork || !m_pNetwork->IsConnected()) return;

	if (m_bStartSign) {
		LoadCompleteNet loadEv{};
		loadEv.playerId = m_nMyPlayerId;
		m_pNetwork->SendLoadCompleteEvent(loadEv);

		m_bStartSign = false; 

		if (m_bIsHostPlayer) {
			m_nLoadedPlayersCnt++;

			if (m_nLoadedPlayersCnt >= m_pNetwork->GetCurrentPlayerCount()) {
				GameStartSignNet startEv{};
				startEv.startSign = true;
				m_pNetwork->SendGameStartSignal(startEv);
				m_bServerStartSign = true; 
			}
		}
	}

	LoadCompleteNet loadEv;
	while (m_pNetwork->ConsumeLoadCompleteEvent(loadEv)) {
		if (m_bIsHostPlayer) {
			m_nLoadedPlayersCnt++;

			if (m_nLoadedPlayersCnt >= m_pNetwork->GetCurrentPlayerCount()) {
				GameStartSignNet startEv{};
				startEv.startSign = true;
				m_pNetwork->SendGameStartSignal(startEv);
				m_bServerStartSign = true; 
			}
		}
	}

	GameStartSignNet startEv;
	while (m_pNetwork->ConsumeGameStartSignal(startEv)) {
		m_bServerStartSign = true;
	}

	PlayerNetState localState = BuildLocalPlayerState();
	m_pNetwork->Update(0.0f, &localState);

	PlayerNetState remoteState{};
	while (m_pNetwork->ConsumeRemoteState(remoteState))
	{
		ApplyRemotePlayerState(remoteState);
	}

	ConsumeNetworkCollisionEvents();
	ConsumeNetworkEffectEvents();
	ConsumeNetworkItemEvents();
	ConsumeNetworkMapItemEvents();
}

void CGameFramework::PlayAndSyncEffect(EFFECT_TYPE eType, const XMFLOAT3& xmf3Position, const XMFLOAT2& xmf2Size, const XMFLOAT3& xmf3Color)
{
	CEffectLibrary::Instance()->Play(eType, xmf3Position, xmf2Size, xmf3Color);
	SendEffectEvent(eType, xmf3Position, xmf2Size, xmf3Color);
}

void CGameFramework::SendEffectEvent(EFFECT_TYPE eType, const XMFLOAT3& xmf3Position, const XMFLOAT2& xmf2Size, const XMFLOAT3& xmf3Color)
{
	if (!m_pNetwork || !m_pNetwork->IsConnected()) return;
	
	EffectEventNet ev{};
	ev.effectType = static_cast<int>(eType);
	ev.x = xmf3Position.x;
	ev.y = xmf3Position.y;
	ev.z = xmf3Position.z;
	ev.sx = xmf2Size.x;
	ev.sy = xmf2Size.y;
	ev.r = xmf3Color.x;
	ev.g = xmf3Color.y;
	ev.b = xmf3Color.z;

	m_pNetwork->SendEffectEvent(ev);
}

void CGameFramework::ConsumeNetworkEffectEvents()
{
	if (!m_pNetwork || !m_pScene) return;

	EffectEventNet ev{};

	while (m_pNetwork->ConsumeEffectEvent(ev))
	{
		if (ev.effectType == (int)EFFECT_TYPE::BOOSTER)
		{
			if (ev.action == 2)
			{
				CEffectLibrary::Instance()->ToggleRemoteBooster(false);
			}
			else if (ev.action == 1 || ev.action == 3)
			{
				XMFLOAT3 pos(ev.x, ev.y, ev.z);
				XMFLOAT3 look(ev.lx, ev.ly, ev.lz);

				CEffectLibrary::Instance()->ToggleRemoteBooster(true);
				CEffectLibrary::Instance()->UpdateRemoteBoosterPosition(pos, look);
			}
		}
		else if (ev.effectType == (int)EFFECT_TYPE::DUST)
		{
			XMFLOAT3 pos(ev.x, ev.y, ev.z);
			XMFLOAT3 look(ev.lx, ev.ly, ev.lz);

			
			XMFLOAT3 right(ev.r, ev.g, ev.b);

			CEffectLibrary::Instance()->PlayCarDustParticle(
				EFFECT_TYPE::DUST,
				pos,
				right,
				look,
				XMFLOAT2(ev.sx, ev.sy),
				XMFLOAT2(10, 20)
			);
		}
		else
		{
			CEffectLibrary::Instance()->Play(
				static_cast<EFFECT_TYPE>(ev.effectType),
				XMFLOAT3(ev.x, ev.y, ev.z),
				XMFLOAT2(ev.sx, ev.sy),
				XMFLOAT3(ev.r, ev.g, ev.b)
			);
		}
	}
}

void CGameFramework::LoadLobbyUIResource()
{
	HRESULT hr;

	ComPtr<IWICBitmapDecoder> pDecoder;
	hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/GameLobbyRemoved.png", 
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);

	ComPtr<IWICBitmapFrameDecode> pFrame;
	hr = pDecoder->GetFrame(0, &pFrame);

	ComPtr<IWICFormatConverter> pConverter;
	hr = m_pWICFactory->CreateFormatConverter(&pConverter);

	hr = pConverter->Initialize(
		pFrame.Get(),
		GUID_WICPixelFormat32bppPBGRA, 
		WICBitmapDitherTypeNone,
		NULL,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	hr = m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		pConverter.Get(),
		NULL,
		&m_pLobbyD2DBitmap 
	);
}

void CGameFramework::LoadResultUIResource()
{
	ComPtr<IWICBitmapDecoder> pDecoder;
	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/GameResult.png",
		NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder
	);
	ComPtr<IWICBitmapFrameDecode> pFrame;
	pDecoder->GetFrame(0, &pFrame);

	ComPtr<IWICFormatConverter> pConverter;
	m_pWICFactory->CreateFormatConverter(&pConverter);
	pConverter->Initialize(pFrame.Get(), GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeMedianCut);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(pConverter.Get(), NULL, &m_pResultD2DBitmap);
}

void CGameFramework::LoadMinimapUIResource()
{
	const wchar_t* fileNames[2] = {
		L"Asset/image/Minimap1.png",
		L"Asset/image/Minimap2.png",// 추가해야함
	};

	for (int i = 0; i < 2; ++i)
	{
		m_pMinimapBitmaps[i].Reset();
		ComPtr<IWICBitmapDecoder> decoder;
		ComPtr<IWICBitmapFrameDecode> frame;
		ComPtr<IWICFormatConverter> converter;

		HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
			fileNames[i], nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);

		if (FAILED(hr)) continue;

		decoder->GetFrame(0, &frame);
		m_pWICFactory->CreateFormatConverter(&converter);
		converter->Initialize(
			frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
			nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);

		m_d2dDeviceContext->CreateBitmapFromWicBitmap(
			converter.Get(), nullptr, &m_pMinimapBitmaps[i]);
	}
}


void CGameFramework::LoadDashVignetteResource()
{
	if (!m_pWICFactory)
	{
		CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_pWICFactory)
		);
	}

	ComPtr<IWICBitmapDecoder> decoder;
	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/dash_vignette.png",
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&decoder
	);

	if (FAILED(hr)) return;

	ComPtr<IWICBitmapFrameDecode> frame;
	decoder->GetFrame(0, &frame);

	ComPtr<IWICFormatConverter> converter;
	m_pWICFactory->CreateFormatConverter(&converter);

	converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.0f,
		WICBitmapPaletteTypeCustom
	);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		converter.Get(),
		NULL,
		&m_pDashVignetteBitmap
	);
}

void CGameFramework::CreateRemotePlayers()
{
	if (!m_pScene) return;

	m_vRemotePlayers.clear();

	for (int i = 0; i < 4; ++i) {
		//CPlayer* pRemotePlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
		int playerId = i + 1;

		if (playerId == m_nMyPlayerId) continue;
		if (m_bMultiplayerEnabled && -1 == m_nPlayerIndices[i]) continue;;

		int carIdx = (m_nPlayerIndices[i] >= 0) ? m_nPlayerIndices[i] : 0;

		CCarPlayer* pRemotePlayer;
		switch (carIdx)
		{
		case 0: pRemotePlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
		case 1: pRemotePlayer = new CCar2Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
			//case 2: pCarPlayer = new CCar3Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
		default: pRemotePlayer = new CCar1Player(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature()); break;
		}
		pRemotePlayer->SetScale(10.2f, 10.2f, 10.2f);
		m_pScene->ApplyMeshTextures(m_pd3dDevice, m_pd3dCommandList, pRemotePlayer);
		pRemotePlayer->ComputeNewLocalAABB();

		pRemotePlayer->m_bIsActive = false ;
	
		RemotePlayerInfo info;
		info.playerID = playerId;
		info.pPlayer = pRemotePlayer;
		info.yaw = 0.0f;

		m_vRemotePlayers.emplace_back(info);
	}
}

void CGameFramework::ReleaseRemotePlayers()
{
	for (auto& info : m_vRemotePlayers) {
		if (info.pPlayer) {
			info.pPlayer->Release();
			info.pPlayer = nullptr;
		}
	}
	m_vRemotePlayers.clear();
}

RemotePlayerInfo* CGameFramework::FindOrAllocateRemotePlayer(int targetId)
{
	for (auto& info : m_vRemotePlayers) {
		if (info.playerID == targetId) return &info;
	}

	for (auto& info : m_vRemotePlayers) {
		if (info.playerID == -1) {
			info.playerID = targetId;
			return &info;
		}
	}

	return nullptr;
}

D2D1_POINT_2F CGameFramework::WorldToMinimap(
	const XMFLOAT3& worldPos,
	const D2D1_RECT_F& minimapRect)
{
	float worldMinX; 
	float worldMaxX;
	float worldMinZ;
	float worldMaxZ;
	if (0 == m_nSelectedMapIndex) {
		// 스테이지1 사이즈
		worldMinX = -3000.0f; // 맵 사이즈 맞게 조정 
		worldMaxX = 1450.0f;
		worldMinZ = -3100.0f;
		worldMaxZ = 3250.0f;
	}
	else if (1 == m_nSelectedMapIndex) {
		 worldMinX = -9350.0f; // 맵 사이즈 맞게 조정 
		 worldMaxX = -4000.0f;
		 worldMinZ = 2600.0f;
		 worldMaxZ = 7820.0f;
	}
	
	float u = (worldPos.x - worldMinX) / (worldMaxX - worldMinX);
	float v = (worldPos.z - worldMinZ) / (worldMaxZ - worldMinZ);

	u = max(0.0f, min(1.0f, u));
	v = max(0.0f, min(1.0f, v));

	float x = minimapRect.left + u * (minimapRect.right - minimapRect.left);
	float y = minimapRect.top + (1.0f - v) * (minimapRect.bottom - minimapRect.top);

	return D2D1::Point2F(x, y);
}

void CGameFramework::ConsumeNetworkCollisionEvents()
{
	if (!m_pNetwork || !m_pPlayer || !m_pScene) return;

	CollisionEventNet ev{};

	while (m_pNetwork->ConsumeCollisionEvent(ev))
	{
		if (ev.playerId == m_nMyPlayerId) continue;

		RemotePlayerInfo* pInfo = FindOrAllocateRemotePlayer(ev.playerId);

		if (pInfo && pInfo->pPlayer) {
			XMFLOAT3 hitPos(ev.x, ev.y, ev.z);

			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, hitPos, XMFLOAT2(50, 50), XMFLOAT3(1, 0, 0));
			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, hitPos, XMFLOAT2(50, 50), XMFLOAT3(0, 1, 0));
			PlayAndSyncEffect(EFFECT_TYPE::COLLISION, hitPos, XMFLOAT2(50, 50), XMFLOAT3(0, 0, 1));
		}
	}
}

void CGameFramework::FinishIntroVideo()
{
	if (m_pVideoPlayer)
	{
		m_pVideoPlayer->Stop();
	}

	m_bPlayingIntroVideo = false;

	m_SoundManager.PlayBGM("Asset/Audio/TRBGM.mp3");
}

void CGameFramework::CheckMulti(const float& fTimeElapsed)
{
	if (m_bMultiplayerEnabled && m_pNetwork && m_pNetwork->IsConnected() && !m_bRaceStarted)
	{
		if (!m_bServerStartSign) {
			if (m_pPlayer) m_pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
			m_pPlayer->Update(fTimeElapsed);
			return;
		}

		if (!m_bRaceStartDelayStarted)
		{
			m_GameTimer.Reset();

			m_bRaceStartDelayStarted = true;
			m_bRaceStarted = false;
			m_fRaceStartDelayTime = 0.0f;

			if (!m_bCountdownSoundPlayed)
			{
				m_SoundManager.PlaySFX("Asset/Audio/Count.mp3");
				m_bCountdownSoundPlayed = true;
			}

			if (m_pPlayer) m_pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		}

		if (!m_bRaceStarted)
		{
			m_fRaceStartDelayTime += fTimeElapsed;

			UpdateDashSystem(fTimeElapsed, false, false);

			if (m_pPlayer)
			{
				m_pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
				m_pPlayer->Update(fTimeElapsed);
			}

			if (m_fRaceStartDelayTime >= m_fRaceStartDelayDuration)
			{
				m_GameTimer.Reset();
				m_bRaceStarted = true;
			}
		}
	}
}

void CGameFramework::AdjustSound()
{
	// 사운드 피치 조절
	float speedRatio = (float)m_nPlayerCurrentSpeed / GetPlayerEffectiveMaxSpeed();
	if (speedRatio < 0.0f) speedRatio = 0.0f;
	if (speedRatio > 1.0f) speedRatio = 1.0f;

	float targetPitch = 1.0f + (speedRatio * 1.0f);

	if (m_bIsDashing) {
		targetPitch += 0.5f;
	}

	m_SoundManager.SetCarEnginePitch(targetPitch);
}

void CGameFramework::ShowResult()
{
	if (m_pNetwork && m_pNetwork->IsConnected())
	{
		RaceResultNet finalResult;
		if (m_pNetwork->ConsumeRaceResult(finalResult))
		{
			m_FinalRaceResult = finalResult;
			BuildObjectEnd();
			m_nStage = 100;
		}
	}
	else
	{
		m_FinalRaceResult.playerCount = 1;
		m_FinalRaceResult.playerRecords[0].playerId = m_nMyPlayerId > 0 ? m_nMyPlayerId : 1;
		m_FinalRaceResult.playerRecords[0].finishTime = m_fMyFinalTime;
		wcscpy_s(m_FinalRaceResult.playerRecords[0].playerName, m_szMyPlayerName);

		BuildObjectEnd();
		m_nStage = 100;
	}
}

void CGameFramework::CheckResult()
{
	if (m_nCurrentLap > MAX_LAPS && 2 == m_nStage)
	{
		m_fMyFinalTime = m_GameTimer.GetTotalTime();
		m_nStage = 99;

		m_fFinishAfterTime = 0.0f;
		m_bShowRankingWaitImage = false;

		++m_nScore;

		if (m_pNetwork && m_pNetwork->IsConnected())
		{
			// 내 ID와 최종 시간을 담아서 서버로 바로 쏜다.
			RaceRecordNet record{};
			record.playerId = m_nMyPlayerId;
			record.finishTime = m_fMyFinalTime;
			wcscpy_s(record.playerName, m_szMyPlayerName);

			m_pNetwork->SendRaceFinish(record);
		}
	}
}

void CGameFramework::FrameAdvance()
{

	m_GameTimer.Tick(0.0f);



	
	static bool bPrevHelpUI = false;

	if (m_nStage == -2 || m_nStage == 2)
	{
		bool bHelpNow = ((::GetAsyncKeyState(VK_TAB) & 0x8000) != 0);

		if (bHelpNow && !bPrevHelpUI)
		{
			m_SoundManager.PlaySFX("Asset/Audio/Robot.mp3");
		}

		m_bShowHelpUI = bHelpNow;
		bPrevHelpUI = bHelpNow;
	}
	else
	{
		m_bShowHelpUI = false;
		bPrevHelpUI = false;
	}

	

	if (m_bPlayingIntroVideo)
	{
	

		if (!m_pVideoPlayer || m_pVideoPlayer->IsFinished())
		{
			FinishIntroVideo();
		}

		return;
	}

	if (m_bPlayingIntroVideo)
	{
		if (GetAsyncKeyState(VK_ESCAPE) & 0x0001)
		{
			::PostQuitMessage(0);
			return;
		}

		if (GetAsyncKeyState(VK_F9) & 0x0001)
		{
			ChangeSwapChainState();
			if (m_pVideoPlayer)
				m_pVideoPlayer->Resize(m_nWndClientWidth, m_nWndClientHeight);
			return;
		}

		if (GetAsyncKeyState(VK_SPACE) & 0x0001) // 스킵
		{
			FinishIntroVideo();
			return;
		}
	}
	// 



	SetUIInfo();
	SyncMultiplayer();
	//if (1 == m_nStage)
	//{
	//	m_pScene->m_nGFStage = m_nStage = 2;

	//	BuildGameObjects();
	//} // 

	if (-1 == m_nStage)
	{
		if (m_pScene)
			m_pScene->m_nGFStage = -2;

		m_nStage = -2;
		BuildObjectGameRoom();
	}

	if (0 == m_nStage)
	{
		ProcessInput();
	}

	else if (1 == m_nStage)
	{
		if (!m_bLoadingPageShown)
		{
			m_bLoadingPageShown = true;
		}
		else
		{
			if (!m_bGameObjectsBuilt)
			{
				BuildGameObjects();
				m_bGameObjectsBuilt = true;
			}

			SyncInGame();

			if (!m_bMultiplayerEnabled || m_bServerStartSign)
			{
				m_nStage = 2;
				m_bGameObjectsBuilt = false;
				m_bLoadingPageShown = false;
			}
		}
	}


	else if (2 == m_nStage)
	{
		CollisionProcess();
		//m_pPlayer->SetGravity(XMFLOAT3(0, 0, 0));
		const float fTimeElapsed = m_GameTimer.GetTimeElapsed();
	
		if (m_bDashLocked && m_pPlayer)
		{
			XMFLOAT3 pos = m_pPlayer->GetPosition();
			pos.y += 0.0f;
			
			CEffectLibrary::Instance()->UpdateLockOrbitPosition(pos);
		}

		if (m_bRemoteLockEffectActive)
		{
			m_fRemoteLockEffectTime -= fTimeElapsed;

			if (m_fRemoteLockEffectTime <= 0.0f)
			{
				m_fRemoteLockEffectTime = 0.0f;
				m_bRemoteLockEffectActive = false;
			}
			else
			{
				for (auto& info : m_vRemotePlayers){
					if (info.playerID != -1 && info.pPlayer && info.pPlayer->m_bIsActive){
						XMFLOAT3 pos = info.pPlayer->GetPosition();
						pos.y += 0.0f;
						CEffectLibrary::Instance()->UpdateLockOrbitPosition(pos);
						break;
					}
				}
			}
		}

		CheckMulti(fTimeElapsed);

		if (!m_bRaceStarted)
		{
		}
		else if (!m_bIsStun)
		{
			ProcessInputGameStage();
		}
		else
		{
			UpdateDashSystem(fTimeElapsed, false, false);
			m_pPlayer->Update(fTimeElapsed);
		}

		AdjustSound();

	}//
	else if (99 == m_nStage) {
		const float fTimeElapsed = m_GameTimer.GetTimeElapsed();
		UpdateDashSystem(fTimeElapsed, false, false);

		XMFLOAT3 vVel = m_pPlayer->GetVelocity();
		XMVECTOR xvVel = XMLoadFloat3(&vVel);
		float fSpeed = XMVectorGetX(XMVector3Length(xvVel));

		if (fSpeed > 0.1f)
		{
			float fDecel = 80.0f * fTimeElapsed;
			if (fSpeed <= fDecel) m_pPlayer->SetVelocity(XMFLOAT3(0, 0, 0));
			else
			{
				xvVel -= XMVector3Normalize(xvVel) * fDecel;
				XMStoreFloat3(&vVel, xvVel);
				m_pPlayer->SetVelocity(vVel);
			}
		}
		m_pPlayer->Update(fTimeElapsed);
	}
	
	if (m_pNetwork)
	{
		m_pNetwork->Update(m_GameTimer.GetTimeElapsed(), NULL);
	}

	CheckResult();
	
	if (99 == m_nStage)
	{
		m_fFinishAfterTime += m_GameTimer.GetTimeElapsed();

		if (m_fFinishAfterTime >= m_fRankingWaitDelay)
		{
			m_bShowRankingWaitImage = true;
			ShowResult();
		}
	}

	if (0 != m_nStage) {
		AnimateObjects();
	}


	HRESULT hResult = m_d3dCommandAllocators[m_nSwapChainBufferIndex].Get()->Reset();
	hResult = m_pd3dCommandList->Reset(m_d3dCommandAllocators[m_nSwapChainBufferIndex].Get(), NULL);

	if (m_bNeedRemotePlayerInit)
	{
		CreateRemotePlayers();
		ApplyMultiplayerSpawn();
		m_bNeedRemotePlayerInit = false;
	}//

	if (2 == m_nStage || 99 == m_nStage)
	{
		RenderShadowPass();
	}

	if (2 == m_nStage || 99 == m_nStage)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		CEffectLibrary::Instance()->PrepareSceneRenderTarget(m_pd3dCommandList, dsvHandle);
		SetMainViewport();

		if (m_pScene) m_pScene->Render(m_pd3dCommandList, m_pCamera);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

		CEffectLibrary::Instance()->RenderRadialBlur(m_pd3dCommandList, m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(), rtvHandle, dsvHandle, m_nPlayerCurrentSpeed);
		m_pd3dCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		SetMainViewport();
		if (m_pScene)
		{
			m_pd3dCommandList->SetGraphicsRootSignature(m_pScene->GetGraphicsRootSignature());

			if (m_pScene->m_pd3dCbvSrvHeap) {
				ID3D12DescriptorHeap* ppHeaps[] = { m_pScene->m_pd3dCbvSrvHeap };
				m_pd3dCommandList->SetDescriptorHeaps(1, ppHeaps);
			}

			if (m_pCamera) m_pCamera->UpdateShaderVariables(m_pd3dCommandList);

			if (m_pScene->m_pd3dcbLights) {
				m_pd3dCommandList->SetGraphicsRootConstantBufferView(2, m_pScene->m_pd3dcbLights->GetGPUVirtualAddress());
			}

			XMMATRIX mLightViewProj = m_pScene->GetShadowLightViewProj();
			XMFLOAT4X4 xmf4x4LightViewProj;
			XMStoreFloat4x4(&xmf4x4LightViewProj, XMMatrixTranspose(mLightViewProj));
			m_pd3dCommandList->SetGraphicsRoot32BitConstants(4, 16, &xmf4x4LightViewProj, 0);
		}

		if (m_pPlayer) m_pPlayer->Render(m_pd3dCommandList, NULL, m_pCamera);
		for (auto& info : m_vRemotePlayers) {
			if (info.playerID != -1 && info.pPlayer && info.pPlayer->m_bIsActive) {
				info.pPlayer->Render(m_pd3dCommandList, NULL, m_pCamera);
			}
		}
		CEffectLibrary::Instance()->Render(m_pd3dCommandList, m_pCamera->GetViewMatrix(), m_pCamera->GetProjectionMatrix());

	}
	else
	{
		D3D12_RESOURCE_BARRIER d3dResourceBarrier;
		BeforeTransformBarrier(d3dResourceBarrier, m_pd3dCommandList);

		ClearRTVDSV(m_pd3dRtvDescriptorHeap, m_pd3dDsvDescriptorHeap, m_pd3dCommandList);
		SetMainViewport();
		if (m_nStage != 1) {


			if (m_pScene) m_pScene->Render(m_pd3dCommandList, m_pCamera);
			if (m_pPlayer) m_pPlayer->Render(m_pd3dCommandList, NULL, m_pCamera);

		}
	}

	CGameObject* pDebugBoxToRender = NULL;
	if (m_pScene && m_pScene->m_bShowWireframeBox && m_pScene->m_pWireframeBoxObject)
	{
		pDebugBoxToRender = m_pScene->m_pWireframeBoxObject;
	}


#ifdef _WITH_PLAYER_TOP
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif
	//if (m_pPlayer) m_pPlayer->Render(m_pd3dCommandList, pDebugBoxToRender, m_pCamera);

	//if (0 == m_nStage) AfterTransformBarrier(d3dResourceBarrier, m_pd3dCommandList);

	hResult = m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	RenderUI();

	hResult = m_pd3dCommandList->Reset(m_d3dCommandAllocators[m_nSwapChainBufferIndex].Get(), NULL);

	if (0 == m_nStage)
	{
		//D3D12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		//	m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(),
		//	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		//m_pd3dCommandList->ResourceBarrier(1, &presentBarrier);
	}
	else if (0 != m_nStage) {
		D3D12_RESOURCE_BARRIER rtBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_pd3dCommandList->ResourceBarrier(1, &rtBarrier);

		if (2 == m_nStage && m_eHoldItem != ITEM_NONE)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			rtvHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

			m_pd3dCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
			SetMainViewport();

			int itemIdx = 0;
			if (m_eHoldItem == ITEM_DASH_POTION) itemIdx = 0;
			else if (m_eHoldItem == ITEM_MAX_SPEED_UP) itemIdx = 1;
			else if (m_eHoldItem == ITEM_MAX_DASH_GAUGE_UP) itemIdx = 2;
			else if (m_eHoldItem == ITEM_LOCK) itemIdx = 3;

			m_pScene->RenderItemUI(
				m_pd3dCommandList,
				itemIdx,
				m_nWndClientWidth,
				m_nWndClientHeight
			);
		}

		D3D12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_pd3dCommandList->ResourceBarrier(1, &presentBarrier);
	}

	m_pd3dCommandList->Close();

	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	WaitForGpuComplete();

#ifdef _WITH_PRESENT_PARAMETERS
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
	m_pdxgiSwapChain->Present(1, 0);
#else
	m_pdxgiSwapChain->Present(0, 0);
#endif
#endif

	MoveToNextFrame();

	ShowFrameRate();
}

void CGameFramework::SendItemEvent(ITEM_TYPE eItemType, float fDuration)
{
	if (!m_pNetwork || !m_pNetwork->IsConnected()) return;

	ItemEventNet ev{};
	ev.itemType = static_cast<int>(eItemType);
	ev.duration = fDuration;

	m_pNetwork->SendItemEvent(ev);
}

void CGameFramework::ApplyDashLock(float fDuration)
{
	m_bDashLocked = true;
	m_fDashLockTime = fDuration;
	m_bIsDashing = false;

	if (m_pPlayer)
		m_pPlayer->m_fMaxVelocityXZ = GetPlayerEffectiveMaxSpeed();

	CEffectLibrary::Instance()->ToggleRemoteBooster(false);

	PlayLockEffectOnPlayer(m_pPlayer, fDuration);
}

void CGameFramework::PlayLockEffectOnPlayer(CPlayer* pTargetPlayer, float fDuration)
{
	if (!pTargetPlayer) return;

	XMFLOAT3 pos = pTargetPlayer->GetPosition();
	pos.y += 0.0f; // 자물쇠 이펙트 위치

	CEffectLibrary::Instance()->PushEffectEvent(
		EFFECT_TYPE::LOCK_ORBIT,
		pos,
		XMFLOAT2(18.0f, 18.0f),
		XMFLOAT3(1.0f, 1.0f, 1.0f),
		fDuration,
		false
	);
}

void CGameFramework::ConsumeNetworkMapItemEvents()
{
	if (!m_pNetwork || !m_pScene || m_pScene->m_ppGameObjects == nullptr) return;

	MapItemEventNet mapItemEv{};
	while (m_pNetwork->ConsumeMapItemEvent(mapItemEv))
	{
		if (mapItemEv.IsActive)
		{
			if (m_pScene->m_ppGameObjects[mapItemEv.itemIndex]) {
				m_pScene->m_ppGameObjects[mapItemEv.itemIndex]->m_bIsActive = true;
			}
		}
		else
		{
			if (m_pScene->m_ppGameObjects[mapItemEv.itemIndex]) {
				m_pScene->m_ppGameObjects[mapItemEv.itemIndex]->m_bIsActive = false;
			}

			if (mapItemEv.playerId == m_nMyPlayerId)
			{
				m_SoundManager.PlaySFX("Asset/Audio/LapSound.mp3");

				if (m_pScene->m_ppGameObjects[mapItemEv.itemIndex])
				{
					XMFLOAT3 vPos = m_pScene->m_ppGameObjects[mapItemEv.itemIndex]->GetPosition();
					PlayAndSyncEffect(EFFECT_TYPE::ITEM1, vPos, XMFLOAT2(50, 50));
					PlayAndSyncEffect(EFFECT_TYPE::ITEM2, vPos, XMFLOAT2(50, 50));
					PlayAndSyncEffect(EFFECT_TYPE::ITEM3, vPos, XMFLOAT2(50, 50));
					PlayAndSyncEffect(EFFECT_TYPE::ITEM4, vPos, XMFLOAT2(25, 25));
					PlayAndSyncEffect(EFFECT_TYPE::ITEM5, vPos, XMFLOAT2(50, 50));
					PlayAndSyncEffect(EFFECT_TYPE::ITEM6, vPos, XMFLOAT2(50, 50));
					PlayAndSyncEffect(EFFECT_TYPE::ITEM7, vPos, XMFLOAT2(50, 50));
					PlayAndSyncEffect(EFFECT_TYPE::ITEM8, vPos, XMFLOAT2(25, 25));
					PlayAndSyncEffect(EFFECT_TYPE::ITEM9, vPos, XMFLOAT2(50, 50));
				}

				++m_nScore;

				int randItem = rand() % 4;
				if (randItem == 0) m_eHoldItem = ITEM_DASH_POTION;
				else if (randItem == 1) m_eHoldItem = ITEM_MAX_SPEED_UP;
				else if (randItem == 2) m_eHoldItem = ITEM_MAX_DASH_GAUGE_UP;
				else if (randItem == 3) m_eHoldItem = ITEM_LOCK;
			}
		}
	}
}


void CGameFramework::ConsumeNetworkItemEvents()
{
	if (!m_pNetwork || !m_pScene) return;

	ItemEventNet ev{};
	while (m_pNetwork->ConsumeItemEvent(ev))
	{
		if (ev.itemType == ITEM_LOCK)
		{
			ApplyDashLock(ev.duration);
		}
	}
}


void CGameFramework::LoadHelpUIResource()
{
	m_pHelpUID2DBitmap.Reset();

	ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter> converter;

	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/help_ui_dummy.png",
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		decoder.GetAddressOf()
	);

	hr = decoder->GetFrame(0, frame.GetAddressOf());

	hr = m_pWICFactory->CreateFormatConverter(converter.GetAddressOf());

	hr = converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		converter.Get(),
		nullptr,
		m_pHelpUID2DBitmap.GetAddressOf()
	);
}

void CGameFramework::LoadRoomUIResource()
{
	m_pRoomD2DBitmap.Reset();

	ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter> converter;

	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/gameRoomEmpty.png",
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		decoder.GetAddressOf()
	);

	hr = decoder->GetFrame(0, frame.GetAddressOf());

	hr = m_pWICFactory->CreateFormatConverter(converter.GetAddressOf());

	hr = converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		converter.Get(),
		nullptr,
		m_pRoomD2DBitmap.GetAddressOf()
	);
}

void CGameFramework::LoadCarImages()
{
	const wchar_t* fileNames[3] = {
		L"Asset/image/Car_01.png",
		L"Asset/image/Car_02.png",
		L"Asset/image/Car_03.png" // 추가해야함
	};

	for (int i = 0; i < 3; ++i)
	{
		m_pCarImages[i].Reset();
		ComPtr<IWICBitmapDecoder> decoder;
		ComPtr<IWICBitmapFrameDecode> frame;
		ComPtr<IWICFormatConverter> converter;

		HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
			fileNames[i], nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);

		if (FAILED(hr)) continue;

		decoder->GetFrame(0, &frame);
		m_pWICFactory->CreateFormatConverter(&converter);
		converter->Initialize(
			frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
			nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);

		m_d2dDeviceContext->CreateBitmapFromWicBitmap(
			converter.Get(), nullptr, &m_pCarImages[i]);
	}
}

void CGameFramework::LoadMapImages()
{
	const wchar_t* fileNames[2] = {
		L"Asset/image/Map1.png",
		L"Asset/image/MAp2.png",
	};

	for (int i = 0; i < 2; ++i)
	{
		m_pMapImages[i].Reset();
		ComPtr<IWICBitmapDecoder> decoder;
		ComPtr<IWICBitmapFrameDecode> frame;
		ComPtr<IWICFormatConverter> converter;

		HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
			fileNames[i], nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);

		if (FAILED(hr)) continue;

		decoder->GetFrame(0, &frame);
		m_pWICFactory->CreateFormatConverter(&converter);
		converter->Initialize(
			frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
			nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);

		m_d2dDeviceContext->CreateBitmapFromWicBitmap(
			converter.Get(), nullptr, &m_pMapImages[i]);
	}
}

void CGameFramework::LoadReadyImage()
{
	m_pReadyImage.Reset();
	ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter> converter;

	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/Ready!.png", nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);


	decoder->GetFrame(0, &frame);
	m_pWICFactory->CreateFormatConverter(&converter);
	converter->Initialize(
		frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
		nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		converter.Get(), nullptr, &m_pReadyImage);

}

D2D1_RECT_F CGameFramework::GetGameMenuImageRect() const
{
	if (!m_pGameMenuD2DBitmap)
		return D2D1::RectF(0, 0, 0, 0);

	float imgW = (float)m_pGameMenuD2DBitmap->GetSize().width;
	float imgH = (float)m_pGameMenuD2DBitmap->GetSize().height;

	float targetH = m_nWndClientHeight * 0.75f;
	float scale = targetH / imgH;
	float targetW = imgW * scale;

	float left = (m_nWndClientWidth - targetW) * 0.5f;
	float top = (m_nWndClientHeight - targetH) * 0.5f;

	return D2D1::RectF(left, top, left + targetW, top + targetH);
}

D2D1_RECT_F CGameFramework::GetGameMenuButtonRect(int index) const
{
	D2D1_RECT_F menu = GetGameMenuImageRect();

	float w = menu.right - menu.left;
	float h = menu.bottom - menu.top;


	const float x1 = 0.045f;
	const float x2 = 0.975f;

	float y1[3] = { 0.260f, 0.420f, 0.720f };
	float y2[3] = { 0.425f, 0.705f, 0.885f };

	return D2D1::RectF(
		menu.left + w * x1,
		menu.top + h * y1[index],
		menu.left + w * x2,
		menu.top + h * y2[index]
	);
}

D2D1_RECT_F CGameFramework::GetIPInputRect() const
{
	float w = 400.0f;
	float h = 60.0f;
	float x = (m_nWndClientWidth - w) * 0.5f;
	float y = (m_nWndClientHeight - h) * 0.5f;

	return D2D1::RectF(x, y, x + w, y + h);
}

void CGameFramework::HandleIPCharInput(WPARAM wParam)
{
	if (m_nStage != 0 || !m_bIPInputActive) return;

	if (wParam == VK_BACK)
	{
		size_t len = wcsnlen_s(m_wszServerIP, 32);
		if (len > 0) m_wszServerIP[len - 1] = L'\0';
		return;
	}

	if ((wParam >= '0' && wParam <= '9') || wParam == '.')
	{
		size_t len = wcsnlen_s(m_wszServerIP, 32);
		if (len >= 15) return;

		m_wszServerIP[len] = static_cast<wchar_t>(wParam);
		m_wszServerIP[len + 1] = L'\0';
	}
}

void CGameFramework::DrawIPInputUI()
{
	if (m_nStage != 0 || !m_bIPInputActive || !m_d2dDeviceContext) return;

	ComPtr<ID2D1SolidColorBrush> dimBrush;
	m_d2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.6f), &dimBrush);
	if (dimBrush)
	{
		m_d2dDeviceContext->FillRectangle(
			D2D1::RectF(0.0f, 0.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight),
			dimBrush.Get()
		);
	}

	D2D1_RECT_F rect = GetIPInputRect();

	if (m_textNameTagBgBrush)
		m_d2dDeviceContext->FillRoundedRectangle(D2D1::RoundedRect(rect, 8.0f, 8.0f), m_textNameTagBgBrush.Get());

	if (m_dashGaugeBorderBrush)
		m_d2dDeviceContext->DrawRoundedRectangle(D2D1::RoundedRect(rect, 8.0f, 8.0f), m_dashGaugeBorderBrush.Get(), 3.0f);

	const wchar_t* label = L"Enter IP Address and Press 'ENTER'";
	m_d2dDeviceContext->DrawTextW(
		label, (UINT32)wcslen(label), m_textNameTagFormat.Get(),
		D2D1::RectF(rect.left - 50.0f, rect.top - 40.0f, rect.right + 50.0f, rect.top - 5.0f), m_textNameTagBrush.Get()
	);

	wchar_t textBuffer[32]{};
	wcscpy_s(textBuffer, 32, m_wszServerIP);

	m_fNameCaretTime += m_GameTimer.GetTimeElapsed();
	if (fmodf(m_fNameCaretTime, 1.0f) < 0.5f)
	{
		wcscat_s(textBuffer, 32, L"|");
	}

	m_d2dDeviceContext->DrawTextW(
		textBuffer, (UINT32)wcslen(textBuffer), m_textNameTagFormat.Get(),
		D2D1::RectF(rect.left + 15.0f, rect.top + 5.0f, rect.right - 15.0f, rect.bottom), m_textNameTagBrush.Get()
	);
}


void CGameFramework::DrawGameMenuUI()
{
	if (!m_pGameMenuD2DBitmap) return;

	D2D1_RECT_F menuRect = GetGameMenuImageRect();


	for (int i = 0; i < 3; ++i)
	{
		D2D1_RECT_F r = GetGameMenuButtonRect(i);

		m_d2dDeviceContext->FillRoundedRectangle(
			D2D1::RoundedRect(r, 18.0f, 18.0f),
			m_menuButtonBrush.Get()
		);

	
		if (i != 1)
		{
			if (m_nGameMenuSelectedIndex == i || m_nGameMenuHoveredIndex == i)
			{
				m_d2dDeviceContext->FillRoundedRectangle(
					D2D1::RoundedRect(r, 18.0f, 18.0f),
					m_pBtnHoverBrush.Get()
				);
			}
		}
	}


	m_d2dDeviceContext->DrawBitmap(
		m_pGameMenuD2DBitmap.Get(),
		menuRect,
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
	);


	for (int i = 0; i < 3; ++i)
	{
		D2D1_RECT_F r = GetGameMenuButtonRect(i);

		float menuH = menuRect.bottom - menuRect.top;
		float textOffsetY = menuH * 0.055f;
		float textH = menuH * 0.055f;

		if (i == 0)
		{

			D2D1_RECT_F resumeRect = D2D1::RectF(
				r.left,
				r.top + textOffsetY,
				r.right,
				r.top + textOffsetY + textH
			);

			m_d2dDeviceContext->DrawTextW(
				L"RESUME",
				6,
				m_textNameTagFormat.Get(),
				resumeRect,
				m_textNameTagBrush.Get()
			);
		}
		else if (i == 1)
		{
			D2D1_RECT_F titleRect = D2D1::RectF(
				r.left,
				r.top + 30.0f,
				r.right,
				r.top + 70.0f
			);

			m_d2dDeviceContext->DrawTextW(
				L"VOLUME",
				6,
				m_textNameTagFormat.Get(),
				titleRect,
				m_textNameTagBrush.Get()
			);

			float boxW = r.right - r.left;
			float boxH = r.bottom - r.top;

			float sliderLeft = r.left + boxW * 0.22f;
			float sliderRight = r.right - boxW * 0.12f;

			float bgmY = r.top + boxH * 0.45f;
			float sfxY = r.top + boxH * 0.72f;

			D2D1_RECT_F bgmTextRect = D2D1::RectF(
				r.left + boxW * 0.06f,
				bgmY - 18.0f,
				r.left + boxW * 0.20f,
				bgmY + 18.0f
			);

			D2D1_RECT_F sfxTextRect = D2D1::RectF(
				r.left + boxW * 0.06f,
				sfxY - 18.0f,
				r.left + boxW * 0.20f,
				sfxY + 18.0f
			);

			m_d2dDeviceContext->DrawTextW(
				L"BGM",
				3,
				m_textNameTagFormat.Get(),
				bgmTextRect,
				m_textNameTagBrush.Get()
			);

			m_d2dDeviceContext->DrawTextW(
				L"SFX",
				3,
				m_textNameTagFormat.Get(),
				sfxTextRect,
				m_textNameTagBrush.Get()
			);

			D2D1_RECT_F bgmBg = D2D1::RectF(
				sliderLeft,
				bgmY - 8.0f,
				sliderRight,
				bgmY + 8.0f
			);

			D2D1_RECT_F bgmFill = D2D1::RectF(
				sliderLeft,
				bgmY - 8.0f,
				sliderLeft + (sliderRight - sliderLeft) * m_fBGMVolume,
				bgmY + 8.0f
			);

			D2D1_RECT_F sfxBg = D2D1::RectF(
				sliderLeft,
				sfxY - 8.0f,
				sliderRight,
				sfxY + 8.0f
			);

			D2D1_RECT_F sfxFill = D2D1::RectF(
				sliderLeft,
				sfxY - 8.0f,
				sliderLeft + (sliderRight - sliderLeft) * m_fSFXVolume,
				sfxY + 8.0f
			);

			m_d2dDeviceContext->FillRoundedRectangle(
				D2D1::RoundedRect(bgmBg, 8.0f, 8.0f),
				m_dashGaugeBGBrush.Get()
			);

			m_d2dDeviceContext->FillRoundedRectangle(
				D2D1::RoundedRect(bgmFill, 8.0f, 8.0f),
				m_dashGaugeFillBrush.Get()
			);

			m_d2dDeviceContext->FillRoundedRectangle(
				D2D1::RoundedRect(sfxBg, 8.0f, 8.0f),
				m_dashGaugeBGBrush.Get()
			);

			m_d2dDeviceContext->FillRoundedRectangle(
				D2D1::RoundedRect(sfxFill, 8.0f, 8.0f),
				m_dashGaugeFillBrush.Get()
			);
		}
		else if (i == 2)
		{
			D2D1_RECT_F exitRect = D2D1::RectF(
				r.left,
				r.top,
				r.right,
				r.bottom
			);

			m_d2dDeviceContext->DrawTextW(
				L"EXIT",
				4,
				m_textNameTagFormat.Get(),
				exitRect,
				m_textNameTagBrush.Get()
			);
		}
	}
}

void CGameFramework::DrawLoadingImage()
{
	if (!m_pLoadingImage) return;

	D2D1_RECT_F rect = D2D1::RectF(
		0.0f,
		0.0f,
		(float)m_nWndClientWidth,
		(float)m_nWndClientHeight
	);

	m_d2dDeviceContext->DrawBitmap(m_pLoadingImage.Get(), rect);
}

void CGameFramework::LoadLoadingImage()
{
	m_pLoadingImage.Reset();

	ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter> converter;

	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/loading.png",
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		decoder.GetAddressOf()
	);

	if (FAILED(hr)) return;

	decoder->GetFrame(0, frame.GetAddressOf());

	m_pWICFactory->CreateFormatConverter(converter.GetAddressOf());

	converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		converter.Get(),
		nullptr,
		m_pLoadingImage.GetAddressOf()
	);
}

void CGameFramework::LoadGameMenuResource()
{
	m_pGameMenuD2DBitmap.Reset();

	ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter> converter;

	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/GameMenu.png",
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&decoder
	);

	if (FAILED(hr)) return;

	decoder->GetFrame(0, &frame);

	m_pWICFactory->CreateFormatConverter(&converter);

	converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		converter.Get(),
		nullptr,
		&m_pGameMenuD2DBitmap
	);
}



// 도움말 ui 
void CGameFramework::DrawHelpUI()
{
	float screenW = (float)m_nWndClientWidth;
	float screenH = (float)m_nWndClientHeight;


	float helpW = screenW * 0.75f;
	float helpH = screenH * 0.75f;


	float left = (screenW - helpW) * 0.5f;
	float top = (screenH - helpH) * 0.5f;

	D2D1_RECT_F helpRect = D2D1::RectF(
		left,
		top,
		left + helpW,
		top + helpH
	);

	m_d2dDeviceContext->DrawBitmap(
		m_pHelpUID2DBitmap.Get(),
		helpRect
	);
}

// 속도계 ui
void CGameFramework::DrawSpeedometerUI()
{
	if (!m_pSpeedometerBitmap) return;

	//float speedUIWidth = m_nWndClientWidth * 0.32f * 1.4f;
	//float speedUIHeight = speedUIWidth * (768.0f / 1536.0f);
	float speedUIWidth = m_nWndClientWidth * 0.30f;
	float speedUIHeight = speedUIWidth * 0.68f;

	float speedUILeft = ((float)m_nWndClientWidth - speedUIWidth) * 0.5f;
	float speedUITop = (float)m_nWndClientHeight - speedUIHeight + 10.0f;
	float speedUIRight = speedUILeft + speedUIWidth;
	float speedUIBottom = speedUITop + speedUIHeight; // 회전축 위치

	D2D1_RECT_F speedRect = D2D1::RectF(
		speedUILeft,
		speedUITop,
		speedUIRight,
		speedUIBottom
	);

	m_d2dDeviceContext->DrawBitmap(
		m_pSpeedometerBitmap.Get(),
		speedRect,
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
	);

	if (!m_pSpeedNeedleBitmap) return;

	float displaySpeed = (float)m_nPlayerCurrentSpeed / 2.1f;



	float ratio = displaySpeed / 350.0f;
	if (ratio < 0.0f) ratio = 0.0f;
	if (ratio > 1.0f) ratio = 1.0f;


	// 시침
	// float needleOffsetY = speedUIHeight * -0.27f;
	float needleOffsetY = speedUIHeight * -0.10f;

	// float angle = +173.0f + ratio * 220.0f;
	//float angle = -180.0f + ratio * 180.0f;

	float angle = -185.0f + ratio * 182.0f;

	//D2D1_POINT_2F pivot = D2D1::Point2F(
	//	speedUILeft + speedUIWidth * 0.50f,
	//	speedUITop + speedUIHeight * 0.72f + needleOffsetY
	//);
	D2D1_POINT_2F pivot = D2D1::Point2F(
		speedUILeft + speedUIWidth * 0.50f,
		speedUITop + speedUIHeight * 0.64f
	);


	//float needleW = speedUIWidth * 0.25f; // 시침가로길이
	//float needleH = speedUIHeight * 0.25f; // 18


	float needleW = speedUIWidth * 0.3f;
	float needleH = speedUIHeight * 0.3f;

	//float needleOffsetX = 25.0f;
	//float needleOffsetY2 = -10.0f;
	float needleOffsetX = 0.0f;
	float needleOffsetY2 = 0.0f;

	// 시침 중심조절
	float needlePivotOffsetX = needleH * 0.25f;

	D2D1_RECT_F needleRect = D2D1::RectF(
		pivot.x - needlePivotOffsetX + needleOffsetX,
		pivot.y - needleH * 0.5f + needleOffsetY2,
		pivot.x - needlePivotOffsetX + needleW + needleOffsetX,
		pivot.y + needleH * 0.5f + needleOffsetY2
	);

	D2D1_MATRIX_3X2_F oldTransform;
	m_d2dDeviceContext->GetTransform(&oldTransform);

	m_d2dDeviceContext->SetTransform(
		D2D1::Matrix3x2F::Rotation(angle, pivot)
	);

	m_d2dDeviceContext->DrawBitmap(
		m_pSpeedNeedleBitmap.Get(),
		needleRect,
		1.0f,
		D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
	);

	m_d2dDeviceContext->SetTransform(oldTransform);
}


XMFLOAT3 CGameFramework::GetDashGaugeColor() const
{
	// Lock
	if (m_bDashLocked)
		return XMFLOAT3(0.65f, 0.65f, 0.65f); // 회색

	// Item_Speed
	if (m_fSpeedItemBonusTime > 0.0f)
		// return XMFLOAT3(0.2f, 0.7f, 1.0f); // 파란색
		return XMFLOAT3(0.2f, 1.0f, 0.3f); // 초록색

	// Item_Gauge
	if (m_fNoDashGaugeConsumeTime > 0.0f)
		//return XMFLOAT3(0.2f, 1.0f, 0.3f); // 초록색
		return XMFLOAT3(0.2f, 0.7f, 1.0f); // 파란색

	// 기본
	return XMFLOAT3(1.0f, 0.75f, 0.1f); // 노란색
}

void CGameFramework::LoadDashGaugeFrameResource()
{
	ComPtr<IWICBitmapDecoder> decoder;
	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/dash_gauge_frame.png",
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&decoder
	);

	if (FAILED(hr)) return;

	ComPtr<IWICBitmapFrameDecode> frame;
	decoder->GetFrame(0, &frame);

	ComPtr<IWICFormatConverter> converter;
	m_pWICFactory->CreateFormatConverter(&converter);

	converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		converter.Get(),
		NULL,
		&m_pDashGaugeFrameBitmap
	);
}

void CGameFramework::LoadSpeedometerUIResource()
{
	ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter> converter;

	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/speedometer.png",
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&decoder
	);

	if (SUCCEEDED(hr))
	{
		decoder->GetFrame(0, &frame);
		m_pWICFactory->CreateFormatConverter(&converter);

		converter->Initialize(
			frame.Get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.0f,
			WICBitmapPaletteTypeMedianCut
		);

		m_d2dDeviceContext->CreateBitmapFromWicBitmap(
			converter.Get(),
			NULL,
			&m_pSpeedometerBitmap
		);
	}

	decoder.Reset();
	frame.Reset();
	converter.Reset();

	hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/speed_needle.png",
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&decoder
	);

	if (SUCCEEDED(hr))
	{
		decoder->GetFrame(0, &frame);
		m_pWICFactory->CreateFormatConverter(&converter);

		converter->Initialize(
			frame.Get(),
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.0f,
			WICBitmapPaletteTypeMedianCut
		);

		m_d2dDeviceContext->CreateBitmapFromWicBitmap(
			converter.Get(),
			NULL,
			&m_pSpeedNeedleBitmap
		);
	}
}

bool CGameFramework::WorldToScreenPoint(
	const XMFLOAT3& worldPos,
	D2D1_POINT_2F& outScreen)
{
	if (!m_pCamera) return false;

	XMFLOAT4X4 viewMat = m_pCamera->GetViewMatrix();
	XMFLOAT4X4 projMat = m_pCamera->GetProjectionMatrix();

	XMMATRIX view = XMLoadFloat4x4(&viewMat);
	XMMATRIX proj = XMLoadFloat4x4(&projMat);

	XMVECTOR pos = XMVectorSet(
		worldPos.x,
		worldPos.y,
		worldPos.z,
		1.0f);

	XMVECTOR clip = XMVector4Transform(
		pos,
		view * proj);

	float w = XMVectorGetW(clip);

	if (w <= 0.001f)
		return false;

	float x = XMVectorGetX(clip) / w;
	float y = XMVectorGetY(clip) / w;

	outScreen.x =
		(x * 0.5f + 0.5f) * m_nWndClientWidth;

	outScreen.y =
		(-y * 0.5f + 0.5f) * m_nWndClientHeight;

	return true;
}


void CGameFramework::SaveNameFromEditControl()
{
	if (wcslen(m_szMyPlayerName) <= 0)
	{
		wcscpy_s(m_szMyPlayerName, L"Player");
	}

	if (m_nMyPlayerId >= 1 && m_nMyPlayerId <= 4)
	{
		wcscpy_s(m_szPlayerNames[m_nMyPlayerId - 1], m_szMyPlayerName);
	}
}


void CGameFramework::DrawPlayerNameTags()
{
	if (m_nStage != 2) return;
	if (!m_d2dDeviceContext) return;
	if (!m_textNameTagFormat || !m_textNameTagBrush) return;
	if (!m_pPlayer) return;

	const float maxNameTagDistance = 800.0f; // 이름 보이는 거리
	const float maxDistSq = maxNameTagDistance * maxNameTagDistance;

	XMFLOAT3 myPos = m_pPlayer->GetPosition();

	for (auto& info : m_vRemotePlayers)
	{
		if (info.playerID == -1) continue;
		if (!info.pPlayer) continue;
		if (!info.pPlayer->m_bIsActive) continue;
		if (info.playerID == m_nMyPlayerId) continue;

		int index = info.playerID - 1;
		if (index < 0 || index >= 4) continue;

		const wchar_t* name = m_szPlayerNames[index];
		if (wcslen(name) <= 0) continue;

		XMFLOAT3 pos = info.pPlayer->GetPosition();

		XMVECTOR vMyPos = XMLoadFloat3(&myPos);
		XMVECTOR vOtherPos = XMLoadFloat3(&pos);
		float distSq = XMVectorGetX(XMVector3LengthSq(vOtherPos - vMyPos));

		if (distSq > maxDistSq)
			continue;

		pos.y += 20.0f;

		D2D1_POINT_2F screen{};
		if (!WorldToScreenPoint(pos, screen))
			continue;

		float boxW = 160.0f;
		float boxH = 36.0f;

		D2D1_RECT_F rect = D2D1::RectF(
			screen.x - boxW * 0.5f,
			screen.y - boxH * 0.5f,
			screen.x + boxW * 0.5f,
			screen.y + boxH * 0.5f
		);


		m_d2dDeviceContext->DrawTextW(
			name,
			static_cast<UINT32>(wcslen(name)),
			m_textNameTagFormat.Get(),
			rect,
			m_textNameTagBrush.Get()
		);
	}
}

D2D1_RECT_F CGameFramework::GetNameInputRect() const
{
	float uiScale = (float)m_nWndClientHeight / 720.0f;
	uiScale = max(0.9f, min(uiScale, 1.2f));

	float w = 400.0f * uiScale;
	float h = 55.0f * uiScale;

	float x = m_nWndClientWidth * 0.65f + 50.0f;
	float y = m_nWndClientHeight * 0.52f - 80.0f;

	// 창모드 잘림 방지
	float rightMargin = 30.0f;
	if (x + w > m_nWndClientWidth - rightMargin)
	{
		x = m_nWndClientWidth - w - rightMargin;
	}

	return D2D1::RectF(x, y, x + w, y + h);
}

void CGameFramework::HandleNameCharInput(WPARAM wParam)
{
	if (m_nStage != 0) return;
	if (!m_bNameInputActive) return;

	if (wParam == VK_BACK)
	{
		size_t len = wcsnlen_s(m_szMyPlayerName, 16);
		if (len > 0)
			m_szMyPlayerName[len - 1] = L'\0';
		return;
	}

	if (wParam == VK_RETURN || wParam == VK_ESCAPE)
	{
		m_bNameInputActive = false;
		return;
	}

	if (wParam < 32) return;

	size_t len = wcsnlen_s(m_szMyPlayerName, 16);
	if (len >= 15) return;

	m_szMyPlayerName[len] = static_cast<wchar_t>(wParam);
	m_szMyPlayerName[len + 1] = L'\0';
}


void CGameFramework::DrawNameInputUI()
{
	if (m_nStage != 0) return;
	if (!m_d2dDeviceContext) return;

	D2D1_RECT_F rect = GetNameInputRect();

	if (m_textNameTagBgBrush)
	{
		m_d2dDeviceContext->FillRoundedRectangle(
			D2D1::RoundedRect(rect, 8.0f, 8.0f),
			m_textNameTagBgBrush.Get()
		);
	}

	if (m_dashGaugeBorderBrush)
	{
		m_d2dDeviceContext->DrawRoundedRectangle(
			D2D1::RoundedRect(rect, 8.0f, 8.0f),
			m_dashGaugeBorderBrush.Get(),
			m_bNameInputActive ? 3.0f : 1.5f
		);
	}

	const wchar_t* label = L"Nickname";
	m_d2dDeviceContext->DrawTextW(
		label,
		(UINT32)wcslen(label),
		m_textNameTagFormat.Get(),
		D2D1::RectF(rect.left, rect.top - 34.0f, rect.right, rect.top - 4.0f),
		m_textNameTagBrush.Get()
	);

	wchar_t textBuffer[32]{};
	wcscpy_s(textBuffer, 32, m_szMyPlayerName);

	if (m_bNameInputActive)
	{
		m_fNameCaretTime += m_GameTimer.GetTimeElapsed();

		if (fmodf(m_fNameCaretTime, 1.0f) < 0.5f)
		{
			wcscat_s(textBuffer, 32, L"|");
		}
	}

	m_d2dDeviceContext->DrawTextW(
		textBuffer,
		(UINT32)wcslen(textBuffer),
		m_textNameTagFormat.Get(),
		D2D1::RectF(rect.left + 12.0f, rect.top, rect.right - 12.0f, rect.bottom),
		m_textNameTagBrush.Get()
	);
}

void CGameFramework::DrawRankingWaitImage()
{
	if (!m_pRankingWaitImage) return;

	D2D1_RECT_F rect = D2D1::RectF(
		0.0f,
		0.0f,
		(float)m_nWndClientWidth,
		(float)m_nWndClientHeight
	);

	m_d2dDeviceContext->DrawBitmap(m_pRankingWaitImage.Get(), rect);
}

void CGameFramework::LoadRankingWaitImage()
{
	m_pRankingWaitImage.Reset();

	ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter> converter;

	HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/ranking_wait.png",
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		decoder.GetAddressOf()
	);

	if (FAILED(hr)) return;

	decoder->GetFrame(0, frame.GetAddressOf());
	m_pWICFactory->CreateFormatConverter(converter.GetAddressOf());

	converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		converter.Get(),
		nullptr,
		m_pRankingWaitImage.GetAddressOf()
	);
}
