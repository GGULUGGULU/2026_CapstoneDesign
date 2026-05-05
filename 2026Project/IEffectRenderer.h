#pragma once

#include "EffectCoreTypes.h"

class CEffectLibrary;

class IEffectRenderer
{
public:
	virtual ~IEffectRenderer() = default;

	virtual bool Initialize(void* deviceContext) = 0;

	virtual void Render(
		EffectRenderContext& context,
		CEffectLibrary* library,
		const EffectMat4& view,
		const EffectMat4& proj
	) = 0;

	virtual void Release() = 0;
};