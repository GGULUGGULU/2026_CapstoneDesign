#include "stdafx.h"
#include "EffectRendererDX12.h"
#include "EffectLibrary.h"
#include "ParticleSystem.h"
#include "MeshEffect.h"

bool EffectRendererDX12::Initialize(ID3D12Device*, ID3D12GraphicsCommandList*)
{
    return true;
}

void EffectRendererDX12::Render(
    ID3D12GraphicsCommandList* cmd,
    CEffectLibrary* lib,
    const XMFLOAT4X4& view,
    const XMFLOAT4X4& proj)
{
    if (!cmd || !lib) return;

    auto rootSig = lib->GetRootSignature();
    auto heap = lib->GetSrvHeap();

    if (!rootSig || !heap) return;

    cmd->SetGraphicsRootSignature(rootSig);

    XMFLOAT4X4 tMats[2];
    XMStoreFloat4x4(&tMats[0], XMMatrixTranspose(XMLoadFloat4x4(&view)));
    XMStoreFloat4x4(&tMats[1], XMMatrixTranspose(XMLoadFloat4x4(&proj)));
    cmd->SetGraphicsRoot32BitConstants(1, 32, tMats, 0);

    ID3D12DescriptorHeap* heaps[] = { heap };
    cmd->SetDescriptorHeaps(1, heaps);

    auto& effects = lib->GetActiveEffects();

    int currentPsoType = 0;

    for (auto eff : effects)
    {
        if (!eff || !eff->bActive) continue;

        
        if (eff->pParticleSys)
        {
            bool useDepth = lib->IsDepthParticleEffect(eff->type);

            ID3D12PipelineState* pso = useDepth ?
                lib->GetParticleDepthPSO() :
                lib->GetParticlePSO();

            int desired = useDepth ? 3 : 1;

            if (currentPsoType != desired && pso)
            {
                cmd->SetPipelineState(pso);
                currentPsoType = desired;
            }

            auto handle = lib->GetSrvGpuStart();
            handle.ptr += (UINT64)eff->type * lib->GetSrvIncrementSize();

            cmd->SetGraphicsRootDescriptorTable(0, handle);

            eff->pParticleSys->Render(cmd);
        }

       
        if (eff->pMeshEffect)
        {
            ID3D12PipelineState* pso = nullptr;
            int desired = 2;

            if (eff->type == EFFECT_TYPE::BOOSTER)
            {
                pso = lib->GetBoosterPSO();
                desired = 4;
            }
            else
            {
                pso = lib->GetMeshPSO();
            }

            if (currentPsoType != desired && pso)
            {
                cmd->SetPipelineState(pso);
                currentPsoType = desired;
            }

            eff->pMeshEffect->Render(cmd);
        }
    }
}

void EffectRendererDX12::Release() {}