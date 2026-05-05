#pragma once
#include <DirectXMath.h>
#include <d3d12.h>

using namespace DirectX;

class CEffectLibrary;

class IEffectRenderer
{
public:
    virtual ~IEffectRenderer() = default;

    virtual bool Initialize(ID3D12Device*, ID3D12GraphicsCommandList*) = 0;

    virtual void Render(
        ID3D12GraphicsCommandList*,
        CEffectLibrary*,
        const XMFLOAT4X4&,
        const XMFLOAT4X4&
    ) = 0;

    virtual void Release() = 0;
};