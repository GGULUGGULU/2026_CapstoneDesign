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
	COLLISION, // 충돌
	DUST, // 흙먼지
	ITEM1, // 아이템 획득
	ITEM2, // 아이템 획득
	ITEM3, // 아이템 획득
	ITEM4, // 아이템 획득
	ITEM5, // 아이템 획득
	ITEM6, // 아이템 획득
	ITEM7, // 아이템 획득
	ITEM8, // 아이템 획득
	ITEM9, // 아이템 획득
	BOOSTER, // 부스터
	WIND_EFFECT,
	SPEED_LINE,
	COUNT, // 개수
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

// 이펙트 요청 정보를 하나의 데이터로 묶은 구조체.
// 충돌, 아이템 획득, 네트워크 동기화 등에서 Play()를 직접 호출하지 않고
// 이벤트 큐에 넣어 처리할 수 있도록 하기 위한 범용화 단계입니다.
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

// 메시 기반 이펙트의 리소스와 형태를 외부 설정으로 분리하기 위한 구조체입니다.
// 부스터, 바람 효과처럼 특정 게임에 박혀 있던 텍스처/크기/색/스크롤 값을 변경할 수 있습니다.
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

	// 직접 Play()를 호출하지 않고 이벤트로 이펙트를 요청하는 함수입니다.
	// 네트워크 수신 이벤트나 충돌 이벤트와 연결하기 좋습니다.
	void PushEffectEvent(const EffectEvent& eventData);
	void PushEffectEvent(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color = XMFLOAT3(1.0f, 1.0f, 1.0f));
	void PushEffectEvent(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color, float lifeTime, bool loop = false);

	// Library customization API. Call before Initialize() when changing pool counts or asset paths.
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

	bool IsDepthParticleEffect(EFFECT_TYPE type) const;


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
		L"Asset/DDS_File/SpeedLine1.dds"
	};

	UINT m_nSrvDescriptorIncrementSize = 0;

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;
	ID3D12PipelineState* m_pMeshEffectPSO = nullptr;
	ID3D12PipelineState* m_pParticleDepthPSO = nullptr; // 흙먼지용
	ID3D12PipelineState* m_pBoosterPSO = nullptr; // 부스터용


	void BuildRootSignature(ID3D12Device* pd3dDevice);
	void BuildPipelineState(ID3D12Device* pd3dDevice);
	void LoadAssets(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	// 1단계 범용화: 기존 파일 구조는 유지하되, 내부 역할을 분리합니다.
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

	ActiveEffect* m_pBoosterEffect = nullptr;
	ActiveEffect* m_pWindShieldEffect = nullptr;

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

