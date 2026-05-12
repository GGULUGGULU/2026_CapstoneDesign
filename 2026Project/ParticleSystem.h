#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;
struct VS_VB_INSTANCE_PARTICLE
{
	XMFLOAT3 m_xmf3Position;
	XMFLOAT2 m_xmf2Size;
	XMFLOAT3 m_xmf3Color;
};

struct ParticleCPUData
{
	XMFLOAT3 m_xmf3Position;
	XMFLOAT3 m_xmf3Velocity;
	XMFLOAT2 m_xmf2MaxSize;
	XMFLOAT3 m_xmf3Color;
	float m_fAge;
	float m_fLifeTime;
	bool m_bIsActive;
};

class CParticleSystem{
public:
	CParticleSystem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nMaxParticles = 1000);
	virtual ~CParticleSystem();

	void CollisionAnimate(float fTimeElapsed);
	void DustAnimate(float fTimeElapsed, bool flag);
	void ItemAnimate(float fTimeElapsed);
	void BoosterAnimate(float fTimeElapsed);

	void Render(ID3D12GraphicsCommandList* pd3dCommandList);

	void ResetParticles(const XMFLOAT2& size, float fSpreadRange = 10.f, bool flag = false, const XMFLOAT3& color = XMFLOAT3(1,1,1));

	void SetPosition(const XMFLOAT3& pos) { m_xmf3Position = pos; }

	void Clear();

	void ResetLockOrbit(const XMFLOAT2& size, const XMFLOAT3& color);
	void LockOrbitAnimate(float fTimeElapsed);

private:
	XMFLOAT3 m_xmf3Position;
	XMFLOAT4X4 m_xmf4x4World;
	XMFLOAT3 m_xmf3BaseColor = XMFLOAT3(1, 1, 1);

	int m_nMaxParticles;
	int m_nActiveParticles;
	std::vector<ParticleCPUData> m_vCpuParticles;

	ID3D12Resource* m_pd3dVertexBuffer = nullptr;
	ID3D12Resource* m_pd3dVertexUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_d3dVertexBufferView;
	VS_VB_INSTANCE_PARTICLE* m_pMappedParticles = nullptr;

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;

	friend class CEffectLibrary;
};

