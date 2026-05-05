#pragma once
#include "IEffectRenderer.h"

class EffectRendererDX12 : public IEffectRenderer
{
public:
    bool Initialize(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    void Render(ID3D12GraphicsCommandList*, CEffectLibrary*, const XMFLOAT4X4&, const XMFLOAT4X4&) override;
    void Release() override;

private:
    ID3D12RootSignature* m_pRootSignature = nullptr;
    ID3D12PipelineState* m_pPipelineState = nullptr;
};