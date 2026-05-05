#pragma once

struct EffectVec2
{
	float x, y;
};

struct EffectVec3
{
	float x, y, z;
};

struct EffectMat4
{
	float m[4][4];
};

struct EffectRenderContext
{
	void* commandContext = nullptr;
};