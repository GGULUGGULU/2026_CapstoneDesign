// EffectShaders.hlsl
// Effect / post-process shaders only.

struct VSParticle_IN
{
    float3 posW : POSITION;
    float2 sizeW : SIZE;
};

struct VSParticle_OUT
{
    float3 centerW : POSITION;
    float2 sizeW : SIZE;
};

VSParticle_OUT VSParticle(VSParticle_IN input)
{
    VSParticle_OUT output;
    
    output.centerW = input.posW;
    output.sizeW = input.sizeW;
    
    return output;
}

RWTexture2D<float4> gOutputColor : register(u0);

Texture2D<float4> gInputColor : register(t0);
Texture2D<float4> gSpeedLineTex : register(t1);

SamplerState gComputeSampler : register(s0);

cbuffer cbBlurParams : register(b0)
{
    float4 gBlurParams;
    
    float gSpeedLineSin;
    float gSpeedLineCos;
    float gSpeedLineScale;
    float gSpeedLineAlpha;
};

[numthreads(32, 32, 1)]
void CS_RadialBlur(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    uint width, height;
    gOutputColor.GetDimensions(width, height);
    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return;
    
    float strength = gBlurParams.x;
    float2 center = gBlurParams.yz;
    float aspectRatio = gBlurParams.w;

    float2 uv = float2(pixelCoord.x / (float) width, pixelCoord.y / (float) height);
    float2 uvCorrected = uv;
    float2 centerCorrected = center;

    uvCorrected.x *= aspectRatio;
    centerCorrected.x *= aspectRatio;

    float2 dir = uv - center;
    float dist = length(uvCorrected - centerCorrected);

    float4 color = float4(0, 0, 0, 0);
    int nSamples = 8;
    for (int i = 0; i < nSamples; ++i)
    {
        float scale = strength * (i / (float) (nSamples - 1)) * dist;
        float2 sampleUV = uv - (dir * scale);

        int2 sampleCoord = int2(sampleUV.x * width, sampleUV.y * height);
        sampleCoord = clamp(sampleCoord, int2(0, 0), int2(width - 1, height - 1));

        color += gInputColor.Load(int3(sampleCoord, 0));
    }
    color /= nSamples;
    
    if (gSpeedLineAlpha > 0.0f)
    {
        float2 centeredUV = uv - 0.5f;
        centeredUV.x *= aspectRatio;

        float2x2 rotMatrix = float2x2(gSpeedLineCos, -gSpeedLineSin,
                                      gSpeedLineSin, gSpeedLineCos);
        float2 rotatedUV = mul(centeredUV, rotMatrix);

        float2 finalUV = (rotatedUV * 0.4f / gSpeedLineScale) + 0.5f;

        float4 speedColor = gSpeedLineTex.SampleLevel(gComputeSampler, finalUV, 0);

        speedColor.rgb = pow(abs(speedColor.rgb), 1.2f);

        float softAlpha = smoothstep(0.1f, 0.5f, speedColor.a);
        
        float finalAlpha = softAlpha * gSpeedLineAlpha;
        color.rgb = (color.rgb * (1.0f - finalAlpha)) + (speedColor.rgb * finalAlpha);
    }

    gOutputColor[pixelCoord] = color;
}

cbuffer cbParticleCamera : register(b6)
{
    matrix gmtxParticleView;
    matrix gmtxParticleProjection;
};

cbuffer cbParticleWorld : register(b7)
{
    matrix gmtxParticleWorld;
};

Texture2D gParticleTexture : register(t6);

SamplerState gParticleSampler : register(s0);

struct VS_PARTICLE_INPUT
{
    float3 position : POSITION;
    float2 size : TEXCOORD;
    float3 color : COLOR;
};

struct VS_PARTICLE_OUTPUT
{
    float3 positionW : POSITION;
    float2 size : TEXCOORD;
    float3 color : COLOR;
};

struct GS_PARTICLE_OUTPUT
{
    float4 positionH : SV_POSITION;
    float2 uv : TEXCOORD;
    uint primID : SV_PrimitiveID;
    float3 color : COLOR;
};

VS_PARTICLE_OUTPUT VS_Particle(VS_PARTICLE_INPUT input)
{
    VS_PARTICLE_OUTPUT output;
    
    output.positionW = mul(float4(input.position, 1.0f), gmtxParticleWorld).xyz;
    output.size = input.size;
    output.color = input.color;
    
    return output;
}

[maxvertexcount(4)]
void GS_Particle(point VS_PARTICLE_OUTPUT input[1], inout TriangleStream<GS_PARTICLE_OUTPUT> outStream)
{
    float3 vRight = float3(gmtxParticleView._11, gmtxParticleView._21, gmtxParticleView._31);
    float3 vUp = float3(gmtxParticleView._12, gmtxParticleView._22, gmtxParticleView._32);

    float3 p = input[0].positionW;
    float2 halfSize = input[0].size * 0.5f;
    
    float4 v[4];
    v[0] = float4(p + halfSize.x * vRight - halfSize.y * vUp, 1.0f);
    v[1] = float4(p + halfSize.x * vRight + halfSize.y * vUp, 1.0f);
    v[2] = float4(p - halfSize.x * vRight - halfSize.y * vUp, 1.0f);
    v[3] = float4(p - halfSize.x * vRight + halfSize.y * vUp, 1.0f);

    float2 uv[4] =
    {
        float2(1.0f, 1.0f), float2(1.0f, 0.0f),
        float2(0.0f, 1.0f), float2(0.0f, 0.0f)
    };

    GS_PARTICLE_OUTPUT output;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float4 posV = mul(v[i], gmtxParticleView);
        output.positionH = mul(posV, gmtxParticleProjection);
        output.uv = uv[i];
        output.primID = (uint) i;
        output.color = input[0].color;
        
        outStream.Append(output);
    }
}

float4 PS_Particle(GS_PARTICLE_OUTPUT input) : SV_TARGET
{
    float4 texColor = gParticleTexture.Sample(gParticleSampler, input.uv);

    return float4(texColor.rgb * input.color, texColor.a);
}

// ================================================================================

cbuffer cbWindShield : register(b7)
{
    matrix gmtxShieldWorld;
    float gfShieldTime; // 시간
    float3 gvShieldScrollSpeed; // 속도
};

struct VS_SHIELD_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VS_SHIELD_OUTPUT
{
    float4 positionH : SV_POSITION;
    float3 normalW : NORMAL;
    float2 uv : TEXCOORD;
};

VS_SHIELD_OUTPUT VS_WindShield(VS_SHIELD_INPUT input)
{
    VS_SHIELD_OUTPUT output;
    
    float4 posW = mul(float4(input.position, 1.0f), gmtxShieldWorld);

    output.positionH = mul(mul(posW, gmtxParticleView), gmtxParticleProjection);
    
    output.normalW = normalize(mul(input.normal, (float3x3) gmtxShieldWorld));
    
    output.uv = input.uv + (gvShieldScrollSpeed.xy * gfShieldTime);

    return output;
}

float4 PS_WindShield(VS_SHIELD_OUTPUT input) : SV_TARGET
{
    //float4 color = gParticleTexture.Sample(gParticleSampler, input.uv);
    //
    //return float4(color.rgb, color.a * 0.7f);
    float4 color = gParticleTexture.Sample(gParticleSampler, input.uv);
    
    float2 staticUV = input.uv - (gvShieldScrollSpeed.xy * gfShieldTime);
    
    float fadeY = 1.0f - staticUV.y;
    float fadeX = sin(staticUV.x * 3.141592f);
    float edgeFade = fadeY * fadeX;
    
    color.rgb = color.rgb * 1.5f;
    
    float finalAlpha = saturate(color.a * edgeFade * 2.5f);
    
    return float4(color.rgb, finalAlpha);
}

cbuffer cbCamera : register(b6)
{
    matrix gView;
    matrix gProj;
};

cbuffer cbEffect : register(b7)
{
    matrix gWorld;
    float gTime;
    float3 gScrollSpeed;
    float4 gTintColor;
};

Texture2D gBaseMap : register(t6);
Texture2D gNoiseMap : register(t7);
Texture2D gMaskMap : register(t8);

SamplerState gsamLinear : register(s0);

struct VS_BOOSTER_IN
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VS_BOOSTER_OUT
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VS_BOOSTER_OUT VS_Booster(VS_BOOSTER_IN vin)
{
    VS_BOOSTER_OUT vout;
    
    matrix worldViewProj = mul(mul(gWorld, gView), gProj);
    vout.PosH = mul(float4(vin.PosL, 1.0f), worldViewProj);
    
    vout.TexC = vin.TexC;
    
    return vout;
}

float4 PS_Booster(VS_BOOSTER_OUT pin) : SV_Target
{
    float2 baseUV = pin.TexC + (gScrollSpeed.xy * gTime);
    
    float2 noiseUV = pin.TexC + (gScrollSpeed.xy * 0.7f * gTime) + float2(sin(gTime * 5.0f) * 0.05f, 0.0f);
    
    float2 maskUV = pin.TexC;
    
    float baseColor = gBaseMap.Sample(gsamLinear, baseUV).r;
    float noiseColor = gNoiseMap.Sample(gsamLinear, noiseUV).r;
    float maskAlpha = gMaskMap.Sample(gsamLinear, maskUV).r;
    
    float fireIntensity = baseColor * noiseColor * 2.0f;
    
    float finalAlpha = fireIntensity * maskAlpha;
    
    return float4(gTintColor.rgb * fireIntensity, finalAlpha * gTintColor.a);
    //return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
