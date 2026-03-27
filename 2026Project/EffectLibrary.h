#pragma once

#include <d3d12.h>
#include <DirectxMath.h>
#include <vector>
#include "DDSTextureLoader12.h"

using namespace DirectX;

class CParticleSystem;
class CMeshEffect;

enum class EFFECT_TYPE
{
	COLLISION, // √Êµπ
	DUST, // »Î∏’¡ˆ
	ITEM1, // æ∆¿Ã≈€ »πµÊ
	ITEM2, // æ∆¿Ã≈€ »πµÊ
	ITEM3, // æ∆¿Ã≈€ »πµÊ
	ITEM4, // æ∆¿Ã≈€ »πµÊ
	ITEM5, // æ∆¿Ã≈€ »πµÊ
	ITEM6, // æ∆¿Ã≈€ »πµÊ
	ITEM7, // æ∆¿Ã≈€ »πµÊ
	ITEM8, // æ∆¿Ã≈€ »πµÊ
	ITEM9, // æ∆¿Ã≈€ »πµÊ
	BOOSTER, // ∫ŒΩ∫≈Õ
	WIND_EFFECT,
	SPEED_LINE,
	COUNT, // ∞≥ºˆ
};

struct ActiveEffect {
	EFFECT_TYPE type;
	bool bActive;
	float fAge;

	CParticleSystem* pParticleSys;
	CMeshEffect* pMeshEffect;
};

struct CB_RADIAL_BLUR {
	float strength;
	float cx;
	float cy;
	float aspect;

	float slSin;     
	float slCos;     
	float slScale;   
	float slAlpha;   
};

class CEffectLibrary
{
public:
	static CEffectLibrary* Instance();

	void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void Release();

	void Update(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, const XMFLOAT4X4& view, const XMFLOAT4X4& proj);

	ActiveEffect* Play(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void PlayCarDustParticle(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT3 right, XMFLOAT3 look, XMFLOAT2 size, XMFLOAT2 offset, XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f));

	void ToggleBooster(bool flag);
	void UpdateBoosterPosition(const XMFLOAT3&, const XMFLOAT3&);

private:
	CEffectLibrary() {}
	~CEffectLibrary() {}

	std::vector<ActiveEffect*> m_vActiveEffects;
	std::vector<ActiveEffect*> m_vEffectPool[(int)EFFECT_TYPE::COUNT];

	ID3D12DescriptorHeap* m_pd3dSrvHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSrvCpuHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSrvGpuHandleStart;

	std::vector<ID3D12Resource*> m_vTextures;
	std::vector<ID3D12Resource*> m_vUploadBuffers;

	const std::wstring m_TextureFileNames[(int)EFFECT_TYPE::COUNT] = {
		L"Asset/DDS_File/WhiteStar1.dds",
		/////////////////////////////////////////////////
		L"Asset/DDS_File/Dust.dds",
		/////////////////////////////////////////////////
		L"Asset/DDS_File/LongPinkRibbon.dds",
		L"Asset/DDS_File/LongRedRibbon.dds",
		L"Asset/DDS_File/LongYellowRibbon.dds",
		L"Asset/DDS_File/PinkCircle.dds",
		L"Asset/DDS_File/PinkTriangle.dds",
		L"Asset/DDS_File/ShortPinkRibbon.dds",
		L"Asset/DDS_File/ShortYellowRibbon.dds",
		L"Asset/DDS_File/YellowCircle.dds",
		L"Asset/DDS_File/YellowTriangle.dds",
		/////////////////////////////////////////////////
		L"Asset/DDS_File/Booster.dds",
		L"Asset/DDS_File/WindShield.dds",
		L"Asset/DDS_File/SpeedLine.dds"
	};

	UINT m_nSrvDescriptorIncrementSize = 0;

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;
	ID3D12PipelineState* m_pMeshEffectPSO = nullptr;

	void BuildRootSignature(ID3D12Device* pd3dDevice);
	void BuildPipelineState(ID3D12Device* pd3dDevice);
	void LoadAssets(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	ActiveEffect* m_pBoosterEffect = nullptr;
	ActiveEffect* m_pWindShieldEffect = nullptr;

	bool m_bSpreadZero = false;

public:
	void InitializePostProcess(ID3D12Device* pd3dDevice, int width, int height);
	void ResizePostProcess(ID3D12Device* pd3dDevice, int width, int height);
	void PrepareSceneRenderTarget(ID3D12GraphicsCommandList* pd3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);
	void RenderRadialBlur(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12Resource* pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, int speed);

private:
	int m_nWidth = 0;
	int m_nHeight = 0;

	ID3D12RootSignature* m_pd3dComputeRootSignature = nullptr;

	ID3D12PipelineState* m_pRadialBlurPSO = nullptr;

	ID3D12Resource* m_pSceneRenderTexture = nullptr;
	ID3D12Resource* m_pBlurTexture = nullptr;

	ID3D12DescriptorHeap* m_pd3dPostProcessRtvHeap = nullptr;
	ID3D12DescriptorHeap* m_pd3dCbvSrvUavHeap = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSceneRtvCpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSrvGpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dUavGpuHandle;

private:
	ID3D12PipelineState* m_pSpeedLinePSO = nullptr;

	float m_fSpeedLineAccumTime = 0.0f;
	float m_fSpeedLineAngle = 0.0f;
	float m_fSpeedLineScale = 1.0f;
	float m_fSpeedLineAlpha = 0.0f;
	float m_fCurrentPlayerSpeedRatio = 0.0f; 

	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSpeedLineGpuHandle; 

public:
	void SetPlayerSpeedRatio(float ratio) { m_fCurrentPlayerSpeedRatio = ratio; }
};

