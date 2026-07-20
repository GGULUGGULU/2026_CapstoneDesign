#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

enum class ParticleMotion
{
    LINEAR,
    ORBIT,
    EMITTER,
    COLLISION = LINEAR,
    DUST = LINEAR,
    ITEM = LINEAR,
    BOOSTER = EMITTER
};

enum class ParticleSpawnShape
{
    POINT,
    BOX,
    SPHERE,
    CIRCLE
};

enum class ParticleBlendMode
{
    ADDITIVE,
    ALPHA
};

struct ParticleConfig
{
    ParticleMotion motion;
    ParticleSpawnShape spawnShape;
    ParticleBlendMode blendMode;
    XMFLOAT3 acceleration;
    XMFLOAT3 direction;
    XMFLOAT3 spawnExtents;
    XMFLOAT2 startSize;
    XMFLOAT2 endSize;
    XMFLOAT3 startColor;
    XMFLOAT3 endColor;
    float gravity;
    float minSpeed;
    float maxSpeed;
    float minLifeTime;
    float maxLifeTime;
    float orbitRadius;
    float orbitSpeed;
    float orbitHeight;
    int emissionCount;
    bool moveX;
    bool moveY;
    bool moveZ;
    bool shrink;
    bool useSpread;

    ParticleConfig()
        : motion(ParticleMotion::LINEAR)
        , spawnShape(ParticleSpawnShape::BOX)
        , blendMode(ParticleBlendMode::ADDITIVE)
        , acceleration(0.0f, -19.6f, 0.0f)
        , direction(0.0f, 1.0f, 0.0f)
        , spawnExtents(1.0f, 1.0f, 1.0f)
        , startSize(1.0f, 1.0f)
        , endSize(0.0f, 0.0f)
        , startColor(1.0f, 1.0f, 1.0f)
        , endColor(1.0f, 1.0f, 1.0f)
        , gravity(-19.6f)
        , minSpeed(10.0f)
        , maxSpeed(50.0f)
        , minLifeTime(0.5f)
        , maxLifeTime(1.5f)
        , orbitRadius(30.0f)
        , orbitSpeed(2.0f)
        , orbitHeight(35.0f)
        , emissionCount(5)
        , moveX(true)
        , moveY(true)
        , moveZ(true)
        , shrink(true)
        , useSpread(false)
    {
    }
};

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
    XMFLOAT3 m_xmf3Color;
    XMFLOAT2 m_xmf2StartSize;
    XMFLOAT2 m_xmf2EndSize;
    float m_fAge;
    float m_fLifeTime;
    float m_fOrbitAngle;
    float m_fOrbitRadius;
    float m_fOrbitSpeed;
    bool m_bIsActive;
};

class CParticleSystem
{
public:
    CParticleSystem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nMaxParticles = 1000);
    virtual ~CParticleSystem();

    void UpdateByMotion(float fTimeElapsed, const ParticleConfig& config);
    void Render(ID3D12GraphicsCommandList* pd3dCommandList);
    void ResetParticles(const XMFLOAT2& size, const ParticleConfig& config, float fSpreadRange = 10.0f, const XMFLOAT3& color = XMFLOAT3(1.0f, 1.0f, 1.0f));
    void ResetLockOrbit(const XMFLOAT2& size, const XMFLOAT3& color, const ParticleConfig& config);
    void SetPosition(const XMFLOAT3& pos) { m_xmf3Position = pos; }
    void Clear();

private:
    XMFLOAT3 CreateSpawnPosition(const ParticleConfig& config, float spreadRange);
    XMFLOAT3 CreateVelocity(const ParticleConfig& config);
    void InitializeParticle(ParticleCPUData& particle, const XMFLOAT2& size, const XMFLOAT3& color, const ParticleConfig& config, float spreadRange);
    void EmitParticles(const ParticleConfig& config);
    void WriteParticleToGpu(const ParticleCPUData& particle, const ParticleConfig& config);

    XMFLOAT3 m_xmf3Position;
    XMFLOAT4X4 m_xmf4x4World;
    XMFLOAT3 m_xmf3BaseColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
    XMFLOAT2 m_xmf2BaseSize = XMFLOAT2(1.0f, 1.0f);
    ParticleConfig m_LastConfig;
    float m_fSpreadRange = 0.0f;
    int m_nMaxParticles;
    int m_nActiveParticles;
    std::vector<ParticleCPUData> m_vCpuParticles;
    ID3D12Resource* m_pd3dVertexBuffer = nullptr;
    ID3D12Resource* m_pd3dVertexUploadBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW m_d3dVertexBufferView{};
    VS_VB_INSTANCE_PARTICLE* m_pMappedParticles = nullptr;

    friend class CEffectLibrary;
};