﻿//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"
#include "EffectLibrary.h"
#include "NetworkManager.h"

namespace {
	//const XMFLOAT3 SINGLE_PLAYER_SPAWN = XMFLOAT3(0.0f, 10.0f, 0.0f);
	//const XMFLOAT3 HOST_PLAYER_SPAWN = XMFLOAT3(-35.0f, 10.0f, 0.0f);
	//const XMFLOAT3 CLIENT_PLAYER_SPAWN = XMFLOAT3(35.0f, 10.0f, 0.0f);
	//constexpr float PLAYER_SPAWN_YAW = 180.0f;

	const XMFLOAT3 SINGLE_PLAYER_SPAWN = XMFLOAT3(-1938.0f, -200.0f, 188.0f);
	const XMFLOAT3 HOST_PLAYER_SPAWN = XMFLOAT3(-1980.0f, -200.0f, 188.0f);
	const XMFLOAT3 CLIENT_PLAYER_SPAWN = XMFLOAT3(-1920.0f, -200.0f, 188.0f);
	constexpr float PLAYER_SPAWN_YAW = 0.0f;
};

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
	m_pRemotePlayer = NULL;
	m_pNetwork = NULL;

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

	m_fDashSpeedBonus = 150.0f;
	m_fCurrentDashGauge = 100.0f;
	m_fMaxDashGauge = 100.0f;
	m_fDashGaugeConsumePerSecond = 45.0f;
	m_fDashGaugeRecoverPerSecond = 25.0f;
	m_fDashGaugeIncreaseAmount = 50.0f;

	m_bIsDashing = false;


	_tcscpy_s(m_pszFrameRate, _T("2026Project ("));
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

	CreateDepthStencilView();


	m_pd3dCommandList->Reset(m_d3dCommandAllocators[0].Get(), NULL);


	CEffectLibrary::Instance()->Initialize(m_pd3dDevice, m_pd3dCommandList);


	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	WaitForGpuComplete();

	BuildObjectGameStart();

	CEffectLibrary::Instance()->InitializePostProcess(m_pd3dDevice, m_nWndClientWidth, m_nWndClientHeight);

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

	if (m_nStage == 0)
	{
		m_nHoveredButtonIndex = -1;

		for (int i = 0; i < 3; ++i) {
			if (m_LobbyButtons[i].IsMouseOver(m_ptMousePos)) {
				m_nHoveredButtonIndex = i;
				break;
			}
		}

		if (nMessageID == WM_LBUTTONDOWN)
		{
			if (m_nHoveredButtonIndex == 0) {
				m_nStage = 1;
			}
			else if (m_nHoveredButtonIndex == 1) {
				m_nStage = 1;
			}
			else if (m_nHoveredButtonIndex == 2) {
				::PostQuitMessage(0);
			}
		}
		return;
	}

	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		::ReleaseCapture();
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
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
		default:
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
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
			StartListenServer();
			break;
		case VK_F6:
			ConnectToListenServer("127.0.0.1");
			break;
		case VK_F9:
			ChangeSwapChainState();
			break;
		case 'S':
			m_pPlayer->SetVelocity(XMFLOAT3(0, 0, 0));
			break;

		case 'P':
		{
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
			break;
		}

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
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return(0);
}

void CGameFramework::OnDestroy()
{
	if (m_pLobbyD2DBitmap) m_pLobbyD2DBitmap.Reset();
	if (m_pWICFactory) m_pWICFactory.Reset();

	ReleaseObjects();

	if (m_pNetwork)
	{
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
	
	CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	pCarPlayer->SetPosition(XMFLOAT3(.0f, .0f, .0f));
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;
	m_pCamera = m_pPlayer->GetCamera();
	
	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	
	WaitForGpuComplete();
	
	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();
	if (m_pRemotePlayer) m_pRemotePlayer->ReleaseUploadBuffers();
	
	m_GameTimer.Reset();
	
	LoadLobbyUIResource();
}

void CGameFramework::ReleaseObjects()
{
	if (m_pPlayer) m_pPlayer->Release();
	ReleaseRemotePlayer();

	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;
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
				CEffectLibrary::Instance()->PlayCarDustParticle(
					EFFECT_TYPE::DUST,
					m_pPlayer->GetPosition(),
					m_pPlayer->GetRightVector(),
					m_pPlayer->GetLookVector(),
					XMFLOAT2(5, 5),
					XMFLOAT2(10, 20)
				);
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

	if (m_pPlayer)
	{
		CEffectLibrary::Instance()->UpdateBoosterPosition(
			XMFLOAT3(m_pPlayer->GetPosition().x, m_pPlayer->GetPosition().y, m_pPlayer->GetPosition().z),
			m_pPlayer->GetLookVector()
		);
	}
	if (m_pRemotePlayer && m_pRemotePlayer->m_bIsActive)
	{
		m_pRemotePlayer->Animate(fTimeElapsed, NULL);
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
	ReleaseRemotePlayer();
	if (m_pScene)
	{
		m_pScene->ReleaseObjects();
		delete m_pScene;
		m_pScene = NULL;
	}

	m_pScene = new CScene();
	if (m_pScene) m_pScene->BuildGameObjects(m_pd3dDevice, m_pd3dCommandList);

	CreateShadowMap();

	CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	//pCarPlayer->SetPosition(XMFLOAT3(-1938.0f, -180.0f, 188.0f));
	pCarPlayer->SetScale(10.2f, 10.2f, 10.2f);
	m_pScene->ApplyMeshTextures(m_pd3dDevice, m_pd3dCommandList, pCarPlayer);
	m_pScene->m_pPlayer = m_pPlayer = pCarPlayer;

	m_pPlayer->ComputeNewLocalAABB();
	m_pPlayer->SetGravity(XMFLOAT3(0, -1, 0));

	CreateRemotePlayer();
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
	m_nPlayerCurrentSpeed = 0;

	m_pPlayer->m_fMaxVelocityXZ = GetPlayerEffectiveMaxSpeed();

	//



	m_pd3dCommandList->Close();
	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();
	if (m_pRemotePlayer) m_pRemotePlayer->ReleaseUploadBuffers();

	m_GameTimer.Reset();
}

CGameFramework::ITEM_TYPE CGameFramework::GetItemType(CGameObject* pObject) const
{
	if (!m_pScene || !pObject) return ITEM_NONE;

	if (pObject == m_pScene->m_ppGameObjects[109]) return ITEM_DASH_POTION;
	if (pObject == m_pScene->m_ppGameObjects[110]) return ITEM_MAX_SPEED_UP;
	if (pObject == m_pScene->m_ppGameObjects[111]) return ITEM_MAX_DASH_GAUGE_UP;
	if (pObject == m_pScene->m_ppGameObjects[112]) return ITEM_DASH_POTION; // 중복 배치

	return ITEM_NONE;
}

float CGameFramework::GetPlayerEffectiveMaxSpeed() const
{
	float fMaxSpeed = m_fBasePlayerMaxSpeed + m_fSpeedItemBonus;
	if (m_bIsDashing) fMaxSpeed += m_fDashSpeedBonus;
	return fMaxSpeed;
}

void CGameFramework::ApplyItemReward(ITEM_TYPE eItemType)
{
	if (!m_pPlayer) return;

	switch (eItemType)
	{
	case ITEM_DASH_POTION:
		// 대시포션: 게이지 풀충전
		m_fCurrentDashGauge = m_fMaxDashGauge;
		break;

	case ITEM_MAX_SPEED_UP:
		// 최대 스피드 증가
		m_fSpeedItemBonus += 50.0f;
		break;

	case ITEM_MAX_DASH_GAUGE_UP:
		// 최대 대시 게이지 증가
		m_fMaxDashGauge += m_fDashGaugeIncreaseAmount;
		m_fCurrentDashGauge += m_fDashGaugeIncreaseAmount;
		if (m_fCurrentDashGauge > m_fMaxDashGauge)
			m_fCurrentDashGauge = m_fMaxDashGauge;
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
		CEffectLibrary::Instance()->ToggleBooster(false);
		return;
	}

	const bool bCanDash = (bDashKeyDown && bHasDriveInput && (m_fCurrentDashGauge > 0.0f));
	m_bIsDashing = bCanDash;

	if (m_bIsDashing)
	{
		m_fCurrentDashGauge -= (m_fDashGaugeConsumePerSecond * fTimeElapsed);
		if (m_fCurrentDashGauge <= 0.0f)
		{
			m_fCurrentDashGauge = 0.0f;
			m_bIsDashing = false;
		}
	}
	else
	{
		m_fCurrentDashGauge += (m_fDashGaugeRecoverPerSecond * fTimeElapsed);
		if (m_fCurrentDashGauge > m_fMaxDashGauge)
			m_fCurrentDashGauge = m_fMaxDashGauge;
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

	CEffectLibrary::Instance()->ToggleBooster(m_bIsDashing);
}

void CGameFramework::CollisionProcess()
{
	// 플레이어끼리 충돌 
	if (m_bMultiplayerEnabled && m_pPlayer && m_pRemotePlayer && m_pRemotePlayer->m_bIsActive)
	{
		BoundingBox localAABB = m_pPlayer->GetCombinedAABB();
		BoundingBox worldAABB_Local;
		localAABB.Transform(worldAABB_Local, XMLoadFloat4x4(&m_pPlayer->GetWorldMatrix()));

		BoundingBox remoteAABB = m_pRemotePlayer->GetCombinedAABB();
		BoundingBox worldAABB_Remote;
		remoteAABB.Transform(worldAABB_Remote, XMLoadFloat4x4(&m_pRemotePlayer->GetWorldMatrix()));

		if (worldAABB_Local.Intersects(worldAABB_Remote))
		{
			XMFLOAT3 localPos = m_pPlayer->GetPosition();
			XMFLOAT3 remotePos = m_pRemotePlayer->GetPosition();

			XMFLOAT3 pushDir = Vector3::Subtract(localPos, remotePos);
			pushDir.y = 0.0f;

			float fLen = Vector3::Length(pushDir);
			if (fLen < 0.001f)
			{

				pushDir = XMFLOAT3(1.0f, 0.0f, 0.0f);
			}
			else
			{
				pushDir = Vector3::Normalize(pushDir);
			}

			const float fSeparation = 8.0f;
			XMFLOAT3 localNewPos = Vector3::Add(localPos, Vector3::ScalarProduct(pushDir, fSeparation * 0.5f, false));
			XMFLOAT3 remoteNewPos = Vector3::Add(remotePos, Vector3::ScalarProduct(pushDir, -fSeparation * 0.5f, false));

			m_pPlayer->SetPosition(localNewPos);
			m_pRemotePlayer->SetPosition(remoteNewPos);

			m_pPlayer->OnPrepareRender();
			m_pRemotePlayer->OnPrepareRender();


			XMFLOAT3 localVel = m_pPlayer->GetVelocity();
			XMFLOAT3 remoteVel = m_pRemotePlayer->GetVelocity();

			float localSpeed = max(80.0f, Vector3::Length(localVel));
			float remoteSpeed = max(80.0f, Vector3::Length(remoteVel));
			float reboundPower = max(localSpeed, remoteSpeed) * 0.8f;

			XMFLOAT3 localBounceVel = Vector3::ScalarProduct(pushDir, reboundPower, false);
			XMFLOAT3 remoteBounceVel = Vector3::ScalarProduct(pushDir, -reboundPower, false);

			m_pPlayer->SetVelocity(localBounceVel);
			m_pRemotePlayer->SetVelocity(remoteBounceVel);

			// 충돌 이펙트
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


			return;
		}
	}

	if (m_pPlayer) m_pPlayer->OnPrepareRender();

	XMFLOAT3 colDirection;

	bool bOnGround = false;

	if (2 == m_nStage)
	{
		bOnGround = m_pScene->CheckGroundCollision();

		if (bOnGround)
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

		// 아이템 + 대시
		ITEM_TYPE eItemType = GetItemType(pCollidedObject);

		if (eItemType != ITEM_NONE)
		{
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

			pCollidedObject->Disable();
			++m_nScore;

			int randItem = rand() % 3;

			if (randItem == 0) m_eHoldItem = ITEM_DASH_POTION;
			else if (randItem == 1) m_eHoldItem = ITEM_MAX_SPEED_UP;
			else if (randItem == 2) m_eHoldItem = ITEM_MAX_DASH_GAUGE_UP;
		}
		else if (pCollidedObject->m_bIsInvisibleWall)
		{// 벽에 박을때
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

				xvReflection = xvReflection * 0.3f;

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
}

void CGameFramework::RenderUI()
{
	m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[m_nSwapChainBufferIndex].GetAddressOf(), 1);
	m_d2dDeviceContext->SetTarget(m_d2dRenderTargets[m_nSwapChainBufferIndex].Get());
	m_d2dDeviceContext->BeginDraw();

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
					m_d2dDeviceContext->FillRectangle(m_LobbyButtons[i].rect, m_pBtnHoverBrush.Get());
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
		const wchar_t* pwszNetStatus = L"OFF";
		if (m_pNetwork)
		{
			if (m_pNetwork->IsHosting())
				pwszNetStatus = (m_pNetwork->IsConnected() ? L"HOST CONNECTED" : L"HOST WAITING");
			else
				pwszNetStatus = (m_pNetwork->IsConnected() ? L"CLIENT CONNECTED" : L"CLIENT DISCONNECTED");
		}

		swprintf_s(
			m_speedBuffer,
			1024,
			L"%d Km/h  Dash : %.0f / %.0f  Net : %s  [Res: %d x %d]",
			m_nPlayerCurrentSpeed / 2,
			m_fCurrentDashGauge,
			m_fMaxDashGauge,
			pwszNetStatus,
			m_nWndClientWidth,
			m_nWndClientHeight
		);

		if (4 != m_nScore && 2 == m_nStage)
		{
			m_d2dDeviceContext->DrawTextW(
				m_timeBuffer,
				wcslen(m_timeBuffer),
				m_textTimeFormat.Get(),
				D2D1::RectF(10.0f, 10.0f,
					(float)m_nWndClientWidth - 10.0f,
					(float)m_nWndClientHeight - 10.0f),
				m_textTimeBrush.Get()
			);

			m_d2dDeviceContext->DrawTextW(
				m_speedBuffer,
				wcslen(m_speedBuffer),
				m_textSpeedFormat.Get(),
				D2D1::RectF(10, 10,
					(float)m_nWndClientWidth - 10.0f,
					(float)m_nWndClientHeight - 10.0f),
				m_textSpeedBrush.Get()
			);

			float gaugeWidth = 40.0f;   // 게이지 너비
			float gaugeHeight = 200.0f; // 게이지 높이
			float marginX = 50.0f;      // 좌측 여백
			float marginY = 50.0f;      // 하단 여백

			float left = marginX;
			float bottom = (float)m_nWndClientHeight - marginY;
			float top = bottom - gaugeHeight;
			float right = left + gaugeWidth;

			D2D1_RECT_F bgRect = D2D1::RectF(left, top, right, bottom);
			m_d2dDeviceContext->FillRectangle(&bgRect, m_dashGaugeBGBrush.Get());

			float dashRatio = m_fCurrentDashGauge / m_fMaxDashGauge;
			if (dashRatio < 0.0f) dashRatio = 0.0f;
			if (dashRatio > 1.0f) dashRatio = 1.0f;

			float fillHeight = gaugeHeight * dashRatio;
			float fillTop = bottom - fillHeight;

			D2D1_RECT_F fillRect = D2D1::RectF(left, fillTop, right, bottom);
			m_d2dDeviceContext->FillRectangle(&fillRect, m_dashGaugeFillBrush.Get());

			m_d2dDeviceContext->DrawRectangle(&bgRect, m_dashGaugeBorderBrush.Get(), 2.0f);
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
		}
	}
	else if (100 == m_nStage)
	{
		wchar_t resultBuffer[256];

		std::uint32_t myId = m_bIsHostPlayer ? 1 : 2;
		const wchar_t* myRankStr = (m_FinalRaceResult.firstId == myId) ? L"1st Place! (WINNER)" : L"2nd Place";

		swprintf_s(resultBuffer, 256,
			L"===== RACE RESULTS =====\n\n"
			L"[Your Rank: %s]\n\n"
			L"1st - Player %d : %.2f sec\n"
			L"2nd - Player %d : %.2f sec\n",
			myRankStr,
			m_FinalRaceResult.firstId, m_FinalRaceResult.firstPlaceTime,
			m_FinalRaceResult.secondId, m_FinalRaceResult.secondPlaceTime
		);


		m_d2dDeviceContext->DrawTextW(
			resultBuffer,
			wcslen(resultBuffer),
			m_textEndTimeFormat.Get(),
			D2D1::RectF(0.0f, 0.0f, (float)m_nWndClientWidth, (float)m_nWndClientHeight),
			m_textEndTimeBrush.Get()
		);
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
	ReleaseRemotePlayer();
	if (m_pScene)
	{
		m_pScene->ReleaseObjects();
		delete m_pScene;
		m_pScene = NULL;
	}

	m_pScene = new CScene();

	if (m_pScene) m_pScene->BuildObjectsGameEnd(m_pd3dDevice, m_pd3dCommandList);

	CCarPlayer* pCarPlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
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
	if (m_pRemotePlayer && m_pRemotePlayer->m_bIsActive)
	{
		m_pRemotePlayer->Render(m_pd3dCommandList, NULL, NULL);
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
		const XMFLOAT3& xmf3LocalSpawn = (m_bMultiplayerEnabled ? (m_bIsHostPlayer ? HOST_PLAYER_SPAWN : CLIENT_PLAYER_SPAWN) : SINGLE_PLAYER_SPAWN);
		SetupPlayerTransform(m_pPlayer, xmf3LocalSpawn, PLAYER_SPAWN_YAW);
		m_nPlayerCurrentSpeed = 0;
	}

	if (m_pRemotePlayer)
	{
		const XMFLOAT3& xmf3RemoteSpawn = (m_bIsHostPlayer ? CLIENT_PLAYER_SPAWN : HOST_PLAYER_SPAWN);
		SetupPlayerTransform(m_pRemotePlayer, xmf3RemoteSpawn, PLAYER_SPAWN_YAW);
		m_pRemotePlayer->m_bIsActive = false;
		m_fRemotePlayerYaw = PLAYER_SPAWN_YAW;
	}
}

bool CGameFramework::StartListenServer(unsigned short port)
{
	if (!m_pNetwork) m_pNetwork = new CNetworkManager();
	else m_pNetwork->Shutdown();

	m_bMultiplayerEnabled = m_pNetwork->StartHost(port);
	m_bIsHostPlayer = m_bMultiplayerEnabled;

	if (m_bMultiplayerEnabled && (m_nStage == 2))
	{
		CreateRemotePlayer();
		ApplyMultiplayerSpawn();
	}

	return(m_bMultiplayerEnabled);
}

bool CGameFramework::ConnectToListenServer(const char* pszAddress, unsigned short port)
{
	if (!m_pNetwork) m_pNetwork = new CNetworkManager();
	else m_pNetwork->Shutdown();

	m_bMultiplayerEnabled = m_pNetwork->ConnectToHost(pszAddress, port);
	m_bIsHostPlayer = false;

	if (m_bMultiplayerEnabled && (m_nStage == 2))
	{
		CreateRemotePlayer();
		ApplyMultiplayerSpawn();
	}

	return(m_bMultiplayerEnabled);
}

void CGameFramework::CreateRemotePlayer()
{
	if (!m_pScene || m_pRemotePlayer) return;

	CCarPlayer* pRemotePlayer = new CCarPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature());
	pRemotePlayer->SetScale(10.2f, 10.2f, 10.2f);
	m_pScene->ApplyMeshTextures(m_pd3dDevice, m_pd3dCommandList, pRemotePlayer);
	pRemotePlayer->ComputeNewLocalAABB();
	pRemotePlayer->Rotate(0, 180, 0);
	pRemotePlayer->SetPosition(XMFLOAT3(30.0f, 10.0f, 0.0f));
	pRemotePlayer->OnPrepareRender();
	pRemotePlayer->m_bIsActive = false;

	m_pRemotePlayer = pRemotePlayer;
	m_fRemotePlayerYaw = 180.0f;
}

void CGameFramework::ReleaseRemotePlayer()
{
	if (m_pRemotePlayer)
	{
		m_pRemotePlayer->Release();
		m_pRemotePlayer = NULL;
	}
	m_fRemotePlayerYaw = 180.0f;
}

PlayerNetState CGameFramework::BuildLocalPlayerState() const
{
	PlayerNetState state{};

	if (!m_pPlayer) return(state);

	const XMFLOAT3 position = m_pPlayer->GetPosition();

	state.playerId = (m_bIsHostPlayer) ? 1u : 2u;
	state.x = position.x;
	state.y = position.y;
	state.z = position.z;
	state.yaw = m_pPlayer->GetYaw();
	state.speed = static_cast<float>(m_nPlayerCurrentSpeed);
	state.stage = static_cast<unsigned int>(m_nStage);
	state.score = static_cast<unsigned int>(m_nScore);

	return(state);
}

void CGameFramework::ApplyRemotePlayerState(const PlayerNetState& state)
{
	if (!m_pRemotePlayer) return;

	m_pRemotePlayer->m_bIsActive = true;
	m_pRemotePlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	m_pRemotePlayer->SetPosition(XMFLOAT3(state.x, state.y, state.z));

	float fDeltaYaw = state.yaw - m_fRemotePlayerYaw;
	if (fDeltaYaw > 180.0f) fDeltaYaw -= 360.0f;
	if (fDeltaYaw < -180.0f) fDeltaYaw += 360.0f;

	if (fabsf(fDeltaYaw) > 0.001f)
	{
		m_pRemotePlayer->Rotate(0.0f, fDeltaYaw, 0.0f);
	}

	m_fRemotePlayerYaw = state.yaw;
	m_pRemotePlayer->OnPrepareRender();
}

void CGameFramework::SyncMultiplayer()
{
	if (!m_pNetwork || !m_pPlayer) return;

	PlayerNetState localState = BuildLocalPlayerState();
	m_pNetwork->Update(m_GameTimer.GetTimeElapsed(), &localState);

	PlayerNetState remoteState{};
	while (m_pNetwork->ConsumeRemoteState(remoteState))
	{
		ApplyRemotePlayerState(remoteState);
	}

	ConsumeNetworkEffectEvents();
}


void CGameFramework::PlayAndSyncEffect(EFFECT_TYPE eType, const XMFLOAT3& xmf3Position, const XMFLOAT2& xmf2Size, const XMFLOAT3& xmf3Color)
{
	CEffectLibrary::Instance()->Play(eType, xmf3Position, xmf2Size, xmf3Color);
	SendEffectEvent(eType, xmf3Position, xmf2Size, xmf3Color);
}

void CGameFramework::SendEffectEvent(EFFECT_TYPE eType, const XMFLOAT3& xmf3Position, const XMFLOAT2& xmf2Size, const XMFLOAT3& xmf3Color)
{
	if (!m_pNetwork || !m_pNetwork->IsConnected()) return;
	if (!m_pNetwork->IsHosting()) return;

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
	if (!m_pNetwork) return;

	EffectEventNet ev{};
	while (m_pNetwork->ConsumeEffectEvent(ev))
	{
		CEffectLibrary::Instance()->Play(static_cast<EFFECT_TYPE>(ev.effectType), XMFLOAT3(ev.x, ev.y, ev.z), XMFLOAT2(ev.sx, ev.sy), XMFLOAT3(ev.r, ev.g, ev.b));
	}
}

void CGameFramework::LoadLobbyUIResource()
{
	if (!m_pWICFactory) {
		return;
	}
	if (!m_d2dDeviceContext) {
		return;
	}
	HRESULT hr;

	ComPtr<IWICBitmapDecoder> pDecoder;
	hr = m_pWICFactory->CreateDecoderFromFilename(
		L"Asset/image/GameLobbyRemoved.png", 
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);
	if (FAILED(hr)) {
		return;
	}

	ComPtr<IWICBitmapFrameDecode> pFrame;
	hr = pDecoder->GetFrame(0, &pFrame);
	if (FAILED(hr)) {
		return;
	}

	ComPtr<IWICFormatConverter> pConverter;
	hr = m_pWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) {
		return;
	}

	hr = pConverter->Initialize(
		pFrame.Get(),
		GUID_WICPixelFormat32bppPBGRA, 
		WICBitmapDitherTypeNone,
		NULL,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);
	if (FAILED(hr)) {
		return;
	}

	hr = m_d2dDeviceContext->CreateBitmapFromWicBitmap(
		pConverter.Get(),
		NULL,
		&m_pLobbyD2DBitmap 
	);
	if (FAILED(hr)) {
		return;
	}
}


//#define _WITH_PLAYER_TOP

void CGameFramework::FrameAdvance()
{
	m_GameTimer.Tick(0.0f);

	SetUIInfo();

	if (1 == m_nStage)
	{
		m_pScene->m_nGFStage = m_nStage = 2;

		BuildGameObjects();
	} // 

	if (0 == m_nStage)
	{
		ProcessInput();
	}
	else if (2 == m_nStage)
	{
		CollisionProcess();

		if (!m_bIsStun)
		{
			ProcessInputGameStage();
		}
		else
		{
			const float fTimeElapsed = m_GameTimer.GetTimeElapsed();
			UpdateDashSystem(fTimeElapsed, false, false); // 스턴 중에는 dash 끔 + 게이지 회복
			m_pPlayer->Update(fTimeElapsed);
		}
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

	if ((2 == m_nStage) && m_pNetwork && m_pPlayer)
	{
		SyncMultiplayer();
	}
	else if (m_pNetwork)
	{
		m_pNetwork->Update(m_GameTimer.GetTimeElapsed(), NULL);
	}

	if (4 == m_nScore && 2 == m_nStage)
	{
		m_fMyFinalTime = m_GameTimer.GetTotalTime();
		m_nStage = 99; 
		++m_nScore;    
	
		if (m_pNetwork)
		{
			RaceRecordNet record;
			record.playerId = m_bIsHostPlayer ? 1 : 2;
			record.finishTime = m_fMyFinalTime;
			if (m_pNetwork->IsHosting()) {
				m_pNetwork->AddServerRecord(record);
			}
			else {
				m_pNetwork->SendRaceFinish(record);
			}
		}
	}
	
	if (99 == m_nStage) {
		
		if (m_pNetwork)
		{
			RaceRecordNet finishRecord;
			while (m_pNetwork->ConsumeRaceFinish(finishRecord))
			{
				m_pNetwork->AddServerRecord(finishRecord);
			}

			if (m_pNetwork->IsHosting())
			{
				if (m_pNetwork->HasBothRecords()) 
				{
					RaceResultNet finalResult = m_pNetwork->CalculateRankings();
					m_pNetwork->SendRaceResult(finalResult);
	
					m_FinalRaceResult = finalResult; 
					BuildObjectEnd();
					m_nStage = 100;
				}
			}
			else
			{
				RaceResultNet finalResult;
				if (m_pNetwork->ConsumeRaceResult(finalResult))
				{
					m_FinalRaceResult = finalResult; 
					BuildObjectEnd();
					m_nStage = 100;
				}
			}
		}
		else {
			m_FinalRaceResult.firstId = 1;
			m_FinalRaceResult.firstPlaceTime = m_fMyFinalTime;
			m_FinalRaceResult.secondId = 0;
			m_FinalRaceResult.secondPlaceTime = 0.0f;

			BuildObjectEnd();
			m_nStage = 100;
		}
	}

	if (0 != m_nStage) {
		AnimateObjects();
	}


	HRESULT hResult = m_d3dCommandAllocators[m_nSwapChainBufferIndex].Get()->Reset();
	hResult = m_pd3dCommandList->Reset(m_d3dCommandAllocators[m_nSwapChainBufferIndex].Get(), NULL);

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
		if (m_pRemotePlayer && m_pRemotePlayer->m_bIsActive) m_pRemotePlayer->Render(m_pd3dCommandList, NULL, m_pCamera);
		CEffectLibrary::Instance()->Render(m_pd3dCommandList, m_pCamera->GetViewMatrix(), m_pCamera->GetProjectionMatrix());

	}
	else
	{
		D3D12_RESOURCE_BARRIER d3dResourceBarrier;
		BeforeTransformBarrier(d3dResourceBarrier, m_pd3dCommandList);

		ClearRTVDSV(m_pd3dRtvDescriptorHeap, m_pd3dDsvDescriptorHeap, m_pd3dCommandList);
		SetMainViewport();

		if (m_pScene) m_pScene->Render(m_pd3dCommandList, m_pCamera);
		if (m_pPlayer) m_pPlayer->Render(m_pd3dCommandList, NULL, m_pCamera);
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
		D3D12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_d3dSwapChainBackBuffers[m_nSwapChainBufferIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_pd3dCommandList->ResourceBarrier(1, &presentBarrier);
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

			m_pScene->RenderItemUI(m_pd3dCommandList, itemIdx);
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
