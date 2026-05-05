#pragma once
#include "IEffectRenderer.h"

class EffectRendererDX12 : public IEffectRenderer
{
public:
	bool Initialize(void* deviceContext) override;

	void Render(
		EffectRenderContext& context,
		CEffectLibrary* library,
		const EffectMat4& view,
		const EffectMat4& proj
	) override;


    void Release() override;

private:
    ID3D12RootSignature* m_pRootSignature = nullptr;
    ID3D12PipelineState* m_pPipelineState = nullptr;
};