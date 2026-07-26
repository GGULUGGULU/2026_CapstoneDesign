// Shaders.hlsl
// Game rendering shaders only.

Texture2D gAlbedoTexture : register(t2); // t1 -> t2수정


struct MATERIAL
{
    float4 m_cAmbient;
    float4 m_cDiffuse;
    float4 m_cSpecular; //a = power
    float4 m_cEmissive;
};

cbuffer cbCameraInfo : register(b1)
{
    matrix gmtxView : packoffset(c0);
    matrix gmtxProjection : packoffset(c4);
    float3 gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
    matrix gmtxGameObject : packoffset(c0);
    MATERIAL gMaterial : packoffset(c4);
};

#include "Light.hlsl"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

//#define _WITH_VERTEX_LIGHTING
Texture2D gBillboardTexture : register(t0); // 나무/수풀 텍스처

SamplerState gSampler : register(s0); // 텍스처 샘플러
SamplerState gShadowSampler : register(s1);

Texture2D gShadowMap : register(t4);

cbuffer cbShadowLightInfo : register(b5)
{
    matrix gmtxLightViewProj : packoffset(c0);
};

struct VS_LIGHTING_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VS_LIGHTING_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
        
    
    float2 uv : TEXCOORD0;
    
    float4 positionLight : TEXCOORD1;
    
};

struct VS_SHADOW_INPUT
{
    float3 position : POSITION;
};

struct VS_SHADOW_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_SHADOW_OUTPUT VS_Shadow(VS_SHADOW_INPUT input)
{
    VS_SHADOW_OUTPUT output;
    
    float4 worldPos = mul(float4(input.position, 1.0f), gmtxGameObject);
    
    output.position = mul(worldPos, gmtxLightViewProj);

    return output;
}

VS_LIGHTING_OUTPUT VSLighting(VS_LIGHTING_INPUT input)
{
    VS_LIGHTING_OUTPUT output;

    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
    output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
    output.uv = input.uv;
        
#ifdef _WITH_VERTEX_LIGHTING
	output.normalW = normalize(output.normalW);
	output.color = Lighting(output.positionW, output.normalW);
#endif
    matrix T = matrix(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    );
    output.positionLight = mul(float4(output.positionW, 1.0f), mul(gmtxLightViewProj, T));
    
    return (output);
}

float4 PSLighting(VS_LIGHTING_OUTPUT input) : SV_TARGET
{
    input.normalW = normalize(input.normalW);
    float4 color = Lighting(input.positionW, input.normalW);

    float3 shadowPos = input.positionLight.xyz / input.positionLight.w;
    float shadowDepth = gShadowMap.Sample(gShadowSampler, shadowPos.xy).r;
    float currentDepth = shadowPos.z;

    float bias = 0.001f;

    float shadowFactor = 1.0f;
    
    if (shadowPos.x >= 0.0f && shadowPos.x <= 1.0f && shadowPos.y >= 0.0f && shadowPos.y <= 1.0f)
    {
        if (currentDepth - bias > shadowDepth)
        {
            shadowFactor = 0.4f;
        }
    }
    float4 lightingColor = Lighting(input.positionW, input.normalW);

    //lightingColor.rgb += gMaterial.m_cAmbient.rgb;
    //너무밝아서 주석처리
    
    float4 texColor = gAlbedoTexture.Sample(gSampler, input.uv);
    clip(texColor.a - 0.1f);
    texColor.rgb = pow(texColor.rgb, 2.2f);
    
    float4 finalColor = lightingColor * texColor;

    //return float4(finalColor.rgb * shadowFactor, finalColor.a);
    float3 resultRGB = finalColor.rgb * shadowFactor;
    return float4(pow(resultRGB, 1.0f / 2.2f), finalColor.a);
    // 감마보정
}

struct VS_DIFFUSED_INPUT
{
    float3 position : POSITION;
};


struct VS_DIFFUSED_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};


VS_DIFFUSED_OUTPUT VSDiffused(VS_DIFFUSED_INPUT input)
{
    VS_DIFFUSED_OUTPUT output;
    
    float3 positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);

    output.position = mul(mul(float4(positionW, 1.0f), gmtxView), gmtxProjection);

    output.color = gMaterial.m_cDiffuse;
    
    return (output);
}


float4 PSDiffused(VS_DIFFUSED_OUTPUT input) : SV_TARGET
{
    return (input.color);
}
// ================================================================================

///////////////////////////////////////////////////////////////
TextureCube gCubeMap : register(t5);

struct VS_SKYBOX_INPUT
{
    float3 position : POSITION;
};

struct VS_SKYBOX_OUTPUT
{
    float4 position : SV_POSITION;
    float3 localPos : POSITION;
};

VS_SKYBOX_OUTPUT VS_Skybox(VS_SKYBOX_INPUT input)
{
    VS_SKYBOX_OUTPUT output;
    
    output.localPos = input.position;
    
    matrix viewNoTranslate = gmtxView;
    viewNoTranslate._41 = 0.0f;
    viewNoTranslate._42 = 0.0f;
    viewNoTranslate._43 = 0.0f;
    
    float4 posW = mul(float4(input.position, 1.0f), gmtxGameObject);
    
    float4 posV = mul(posW, viewNoTranslate);
    float4 posH = mul(posV, gmtxProjection);
    
    output.position = posH.xyww;

    return output;
}

float4 PS_Skybox(VS_SKYBOX_OUTPUT input) : SV_TARGET
{
    return gCubeMap.Sample(gSampler, input.localPos);
}

Texture2D gUITexture : register(t7);

struct VS_UI_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VS_UI_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_UI_OUTPUT VS_UI_Main(VS_UI_INPUT input)
{
    VS_UI_OUTPUT output;
    
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    
    return output;
}

float4 PS_UI_Main(VS_UI_OUTPUT input) : SV_TARGET
{
    float4 color = gUITexture.Sample(gSampler, input.uv);
    
    return color;
}
