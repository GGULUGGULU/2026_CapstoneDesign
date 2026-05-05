#include "stdafx.h"
#include "EffectRendererDX12.h"
#include "EffectLibrary.h"
#include "ParticleSystem.h"
#include "MeshEffect.h"


bool EffectRendererDX12::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmd)
{
    // 기존 BuildRootSignature / PSO 일부 옮겨도 되고
    return true;
}

void EffectRendererDX12::Render(
    ID3D12GraphicsCommandList* cmd,
    CEffectLibrary* lib,
    const XMFLOAT4X4& view,
    const XMFLOAT4X4& proj)
{
    if (!cmd || !lib) return;

    auto& effects = lib->GetActiveEffects(); // getter 하나 추가 필요

    for (auto eff : effects)
    {
        if (!eff || !eff->bActive) continue;

        if (eff->pParticleSys)
            eff->pParticleSys->Render(cmd);

        if (eff->pMeshEffect)
            eff->pMeshEffect->Render(cmd);
    }
}

void EffectRendererDX12::Release()
{
    if (m_pRootSignature) m_pRootSignature->Release();
    if (m_pPipelineState) m_pPipelineState->Release();
}