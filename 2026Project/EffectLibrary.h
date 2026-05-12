#pragma once

#include <d3d12.h>
#include <vector>
#include <queue>
#include <string>
#include <memory>

#include "EffectCoreTypes.h"
#include <DirectxMath.h>
#include "DDSTextureLoader12.h"
#include "IEffectRenderer.h"



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
	LOCK_ORBIT,
	COUNT, // ∞≥ºˆ
};

struct ActiveEffect {
	EFFECT_TYPE type;
	bool bActive;
	float fAge;
	float fLifeTime;
	bool bLoop;
	float fSpread;
	bool bUseSpread;

	CParticleSystem* pParticleSys;
	CMeshEffect* pMeshEffect;
};


struct EffectEvent {
	EFFECT_TYPE type;
	XMFLOAT3 position;
	XMFLOAT2 size;
	XMFLOAT3 color;
	float lifeTime;
	bool loop;

	EffectEvent()
		: type(EFFECT_TYPE::COLLISION)
		, position(0.0f, 0.0f, 0.0f)
		, size(1.0f, 1.0f)
		, color(1.0f, 1.0f, 1.0f)
		, lifeTime(-1.0f)
		, loop(false)
	{
	}

	EffectEvent(EFFECT_TYPE effectType, XMFLOAT3 effectPos, XMFLOAT2 effectSize, XMFLOAT3 effectColor = XMFLOAT3(1.0f, 1.0f, 1.0f), float effectLifeTime = -1.0f, bool effectLoop = false)
		: type(effectType)
		, position(effectPos)
		, size(effectSize)
		, color(effectColor)
		, lifeTime(effectLifeTime)
		, loop(effectLoop)
	{
	}
};

struct EffectTypeConfig {
	int poolSize;
	int particleCount;
	float lifeTime;
	float spread;
	bool loop;
	bool useDepth;

	EffectTypeConfig()
		: poolSize(50)
		, particleCount(3)
		, lifeTime(2.0f)
		, spread(0.0f)
		, loop(false)
		, useDepth(false)
	{
	}
};


struct EffectMeshConfig {
	float radius;
	int sliceCount;
	int stackCount;
	std::vector<std::wstring> textureFiles;
	XMFLOAT3 color;
	XMFLOAT3 scale;
	XMFLOAT3 scrollSpeed;

	EffectMeshConfig()
		: radius(1.0f)
		, sliceCount(20)
		, stackCount(20)
		, color(1.0f, 1.0f, 1.0f)
		, scale(1.0f, 1.0f, 1.0f)
		, scrollSpeed(0.0f, 0.0f, 0.0f)
	{
	}
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

	ID3D12RootSignature* GetRootSignature() const { return m_pRootSignature; }
	ID3D12DescriptorHeap* GetSrvHeap() const { return m_pd3dSrvHeap; }

	ID3D12PipelineState* GetParticlePSO() const { return m_pPipelineState; }
	ID3D12PipelineState* GetParticleDepthPSO() const { return m_pParticleDepthPSO; }
	ID3D12PipelineState* GetMeshPSO() const { return m_pMeshEffectPSO; }
	ID3D12PipelineState* GetBoosterPSO() const { return m_pBoosterPSO; }

	UINT GetSrvIncrementSize() const { return m_nSrvDescriptorIncrementSize; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuStart() const { return m_d3dSrvGpuHandleStart; }



	static CEffectLibrary* Instance();

	void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void Release();

	void Update(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, const XMFLOAT4X4& view, const XMFLOAT4X4& proj);
	void Render(EffectRenderContext& context, const EffectMat4& view, const EffectMat4& proj);

	ActiveEffect* Play(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void PlayCarDustParticle(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT3 right, XMFLOAT3 look, XMFLOAT2 size, XMFLOAT2 offset, XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f));


	void PushEffectEvent(const EffectEvent& eventData);
	void PushEffectEvent(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void PushEffectEvent(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color, float lifeTime, bool loop = false);

	void SetEffectTypeConfig(EFFECT_TYPE type, const EffectTypeConfig& config);
	void SetEffectLifeTime(EFFECT_TYPE type, float lifeTime);
	void SetEffectTextureFileName(EFFECT_TYPE type, const std::wstring& fileName);

	void SetBoosterMeshConfig(const EffectMeshConfig& config);
	void SetWindMeshConfig(const EffectMeshConfig& config);
	void SetBoosterTextureFiles(const std::vector<std::wstring>& textureFiles);
	void SetWindTextureFiles(const std::vector<std::wstring>& textureFiles);

	void StopEffectType(EFFECT_TYPE type);
	void ClearActiveEffects();
	int GetActiveEffectCount() const;
	int GetPooledEffectCount(EFFECT_TYPE type) const;

	void ToggleBooster(bool flag);
	void UpdateBoosterPosition(const XMFLOAT3&, const XMFLOAT3&);

	void ToggleLocalBooster(bool flag);
	void UpdateLocalBoosterPosition(const XMFLOAT3& pos, const XMFLOAT3& lookDir);

	void ToggleRemoteBooster(bool flag);
	void UpdateRemoteBoosterPosition(const XMFLOAT3& pos, const XMFLOAT3& lookDir);




	bool IsDepthParticleEffect(EFFECT_TYPE type) const;

	void UpdateLockOrbitPosition(const XMFLOAT3& position);

private:
	CEffectLibrary();
	~CEffectLibrary() {}

	std::vector<ActiveEffect*> m_vActiveEffects;
	std::vector<ActiveEffect*> m_vEffectPool[(int)EFFECT_TYPE::COUNT];

	ID3D12DescriptorHeap* m_pd3dSrvHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_d3dSrvCpuHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSrvGpuHandleStart;

	std::vector<ID3D12Resource*> m_vTextures;
	std::vector<ID3D12Resource*> m_vUploadBuffers;

	std::wstring m_TextureFileNames[(int)EFFECT_TYPE::COUNT] = {
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
		L"Asset/DDS_File/SpeedLine1.dds",
		L"Asset/DDS_File/LockOrbit.dds"
	};

	UINT m_nSrvDescriptorIncrementSize = 0;

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;
	ID3D12PipelineState* m_pMeshEffectPSO = nullptr;
	ID3D12PipelineState* m_pParticleDepthPSO = nullptr; // »Î∏’¡ˆøÎ
	ID3D12PipelineState* m_pBoosterPSO = nullptr; // ∫ŒΩ∫≈ÕøÎ


	void BuildRootSignature(ID3D12Device* pd3dDevice);
	void BuildPipelineState(ID3D12Device* pd3dDevice);
	void LoadAssets(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	void ReleaseIfInitialized();
	bool InitializeRenderResources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateEffectPools(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateWindEffect(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateBoosterEffect(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateParticleEffectPool(EFFECT_TYPE type, ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nPoolSize, int nParticleCount);

	void InitializeDefaultEffectConfigs();
	void InitializeDefaultMeshConfigs();
	bool IsValidEffectType(EFFECT_TYPE type) const;
	bool IsItemEffect(EFFECT_TYPE type) const;
	
	float GetConfiguredSpread(EFFECT_TYPE type) const;
	float GetConfiguredLifeTime(EFFECT_TYPE type) const;
	bool GetConfiguredLoop(EFFECT_TYPE type) const;

	void ConsumeEffectEvents();
	void UpdateEffectInstance(ActiveEffect* eff, float fTimeElapsed, bool& bIsDead);
	void UpdateParticleEffect(ActiveEffect* eff, float fTimeElapsed);
	void UpdateMeshEffect(ActiveEffect* eff, float fTimeElapsed);
	void RecycleEffect(ActiveEffect* eff);
	void UpdateSpeedLineState(float fTimeElapsed);

	void RenderParticleEffect(ID3D12GraphicsCommandList* pd3dCommandList, ActiveEffect* eff, int& currentPsoType, ID3D12DescriptorHeap** ppParticleHeap);
	void RenderMeshEffect(ID3D12GraphicsCommandList* pd3dCommandList, ActiveEffect* eff, int& currentPsoType);

	//ActiveEffect* m_pBoosterEffect = nullptr;
	//ActiveEffect* m_pWindShieldEffect = nullptr;

	ActiveEffect* m_pLocalBoosterEffect = nullptr;
	ActiveEffect* m_pLocalWindShieldEffect = nullptr;

	ActiveEffect* m_pRemoteBoosterEffect = nullptr;
	ActiveEffect* m_pRemoteWindShieldEffect = nullptr;


	bool m_bSpreadZero = false;

	EffectTypeConfig m_EffectConfigs[(int)EFFECT_TYPE::COUNT];
	EffectMeshConfig m_BoosterMeshConfig;
	EffectMeshConfig m_WindMeshConfig;
	std::queue<EffectEvent> m_qEffectEvents;
	std::unique_ptr<IEffectRenderer> m_pRenderer;

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
	float m_fSpeedLineAccumTime = 0.0f;
	float m_fSpeedLineAngle = 0.0f;
	float m_fSpeedLineScale = 1.0f;
	float m_fSpeedLineAlpha = 0.0f;
	float m_fCurrentPlayerSpeedRatio = 0.0f;

	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dSpeedLineGpuHandle;

public:
	void SetPlayerSpeedRatio(float ratio) { m_fCurrentPlayerSpeedRatio = ratio; }

	const std::vector<ActiveEffect*>& GetActiveEffects() const
	{
		return m_vActiveEffects;
	}

};

