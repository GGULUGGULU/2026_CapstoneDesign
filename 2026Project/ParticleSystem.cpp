#include "EffectPCH.h"
#include "ParticleSystem.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace
{
    std::mt19937& ParticleRandomEngine()
    {
        static std::mt19937 engine(std::random_device{}());
        return engine;
    }

    float RandomFloat(float minimum, float maximum)
    {
        if (maximum < minimum) std::swap(minimum, maximum);
        std::uniform_real_distribution<float> distribution(minimum, maximum);
        return distribution(ParticleRandomEngine());
    }

    float Saturate(float value)
    {
        return (std::max)(0.0f, (std::min)(1.0f, value));
    }

    XMFLOAT3 Lerp3(const XMFLOAT3& a, const XMFLOAT3& b, float t)
    {
        return XMFLOAT3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }

    XMFLOAT2 Lerp2(const XMFLOAT2& a, const XMFLOAT2& b, float t)
    {
        return XMFLOAT2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }
}

CParticleSystem::CParticleSystem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList*, int nMaxParticles)
    : m_xmf3Position(0.0f, 0.0f, 0.0f)
    , m_nMaxParticles((std::max)(1, nMaxParticles))
    , m_nActiveParticles(0)
{
    m_vCpuParticles.resize(m_nMaxParticles);
    XMStoreFloat4x4(&m_xmf4x4World, XMMatrixIdentity());

    UINT stride = sizeof(VS_VB_INSTANCE_PARTICLE);
    UINT bufferSize = stride * m_nMaxParticles;
    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    if (pd3dDevice)
    {
        pd3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pd3dVertexBuffer));
    }

    if (m_pd3dVertexBuffer)
    {
        m_pd3dVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedParticles));
        m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
        m_d3dVertexBufferView.StrideInBytes = stride;
        m_d3dVertexBufferView.SizeInBytes = bufferSize;
    }
}

CParticleSystem::~CParticleSystem()
{
    if (m_pd3dVertexBuffer)
    {
        m_pd3dVertexBuffer->Unmap(0, nullptr);
        m_pd3dVertexBuffer->Release();
        m_pd3dVertexBuffer = nullptr;
    }
}

XMFLOAT3 CParticleSystem::CreateSpawnPosition(const ParticleConfig& config, float spreadRange)
{
    XMFLOAT3 extents = config.spawnExtents;
    if (spreadRange > 0.0f) extents = XMFLOAT3(spreadRange, spreadRange * 0.5f, spreadRange);

    if (config.spawnShape == ParticleSpawnShape::POINT) return XMFLOAT3(0.0f, 0.0f, 0.0f);

    if (config.spawnShape == ParticleSpawnShape::BOX)
    {
        return XMFLOAT3(RandomFloat(-extents.x, extents.x), RandomFloat(-extents.y, extents.y), RandomFloat(-extents.z, extents.z));
    }

    if (config.spawnShape == ParticleSpawnShape::CIRCLE)
    {
        float angle = RandomFloat(0.0f, XM_2PI);
        float radius = std::sqrt(RandomFloat(0.0f, 1.0f)) * (std::max)(extents.x, extents.z);

        return XMFLOAT3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
    }

    XMFLOAT3 direction(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f));
    XMVECTOR vector = XMLoadFloat3(&direction);
    if (XMVectorGetX(XMVector3LengthSq(vector)) < 0.000001f) vector = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    vector = XMVector3Normalize(vector) * std::cbrt(RandomFloat(0.0f, 1.0f));
    XMFLOAT3 normalized;
    XMStoreFloat3(&normalized, vector);
    return XMFLOAT3(normalized.x * extents.x, normalized.y * extents.y, normalized.z * extents.z);
}

XMFLOAT3 CParticleSystem::CreateVelocity(const ParticleConfig& config)
{
    XMFLOAT3 randomDirection(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f));
    XMVECTOR direction = XMLoadFloat3(&config.direction) + XMLoadFloat3(&randomDirection);
    if (XMVectorGetX(XMVector3LengthSq(direction)) < 0.000001f) direction = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    direction = XMVector3Normalize(direction) * RandomFloat(config.minSpeed, config.maxSpeed);
    XMFLOAT3 velocity;
    XMStoreFloat3(&velocity, direction);
    return velocity;
}

void CParticleSystem::InitializeParticle(ParticleCPUData& particle, const XMFLOAT2& size, const XMFLOAT3& color, const ParticleConfig& config, float spreadRange)
{
    particle.m_bIsActive = true;
    particle.m_fAge = 0.0f;
    particle.m_fLifeTime = (std::max)(0.001f, RandomFloat(config.minLifeTime, config.maxLifeTime));
    particle.m_xmf3Position = CreateSpawnPosition(config, spreadRange);
    particle.m_xmf3Velocity = CreateVelocity(config);
    particle.m_xmf3Color = color;
    particle.m_xmf2StartSize = XMFLOAT2(size.x * config.startSize.x, size.y * config.startSize.y);
    particle.m_xmf2EndSize = XMFLOAT2(size.x * config.endSize.x, size.y * config.endSize.y);
    particle.m_fOrbitAngle = RandomFloat(0.0f, XM_2PI);
    particle.m_fOrbitRadius = config.orbitRadius;
    particle.m_fOrbitSpeed = config.orbitSpeed;
}

void CParticleSystem::ResetParticles(const XMFLOAT2& size, const ParticleConfig& config, float spreadRange, const XMFLOAT3& color)
{
    m_LastConfig = config;
    m_xmf2BaseSize = size;
    m_xmf3BaseColor = color;
    m_fSpreadRange = spreadRange;
    m_nActiveParticles = 0;

    for (ParticleCPUData& particle : m_vCpuParticles)
    {
        InitializeParticle(particle, size, color, config, spreadRange);
    }

    UpdateByMotion(0.0f, config);
}

void CParticleSystem::ResetLockOrbit(const XMFLOAT2& size, const XMFLOAT3& color, const ParticleConfig& config)
{
    ParticleConfig orbitConfig = config;
    orbitConfig.motion = ParticleMotion::ORBIT;
    orbitConfig.spawnShape = ParticleSpawnShape::POINT;
    orbitConfig.startSize = XMFLOAT2(1.0f, 1.0f);
    orbitConfig.endSize = XMFLOAT2(1.0f, 1.0f);
    orbitConfig.minLifeTime = (std::max)(orbitConfig.minLifeTime, 999999.0f);
    orbitConfig.maxLifeTime = orbitConfig.minLifeTime;
    ResetParticles(size, orbitConfig, 0.0f, color);

    int count = (std::max)(1, m_nMaxParticles);
    for (int index = 0; index < count; ++index)
    {
        ParticleCPUData& particle = m_vCpuParticles[index];
        particle.m_fOrbitAngle = XM_2PI * static_cast<float>(index) / static_cast<float>(count);
        particle.m_fOrbitRadius = orbitConfig.orbitRadius;
        particle.m_fOrbitSpeed = orbitConfig.orbitSpeed;
    }

    UpdateByMotion(0.0f, orbitConfig);
}

void CParticleSystem::EmitParticles(const ParticleConfig& config)
{
    int remaining = (std::max)(0, config.emissionCount);
    for (ParticleCPUData& particle : m_vCpuParticles)
    {
        if (remaining == 0) break;
        if (particle.m_bIsActive) continue;
        InitializeParticle(particle, m_xmf2BaseSize, m_xmf3BaseColor, config, m_fSpreadRange);
        --remaining;
    }
}

void CParticleSystem::WriteParticleToGpu(const ParticleCPUData& particle, const ParticleConfig& config)
{
    if (!m_pMappedParticles || m_nActiveParticles >= m_nMaxParticles) return;
    float ratio = Saturate(particle.m_fAge / (std::max)(0.001f, particle.m_fLifeTime));
    XMFLOAT2 size = Lerp2(particle.m_xmf2StartSize, particle.m_xmf2EndSize, ratio);
    if (!config.shrink) size = particle.m_xmf2StartSize;
    XMFLOAT3 color = Lerp3(config.startColor, config.endColor, ratio);
    color.x *= particle.m_xmf3Color.x;
    color.y *= particle.m_xmf3Color.y;
    color.z *= particle.m_xmf3Color.z;
    m_pMappedParticles[m_nActiveParticles].m_xmf3Position = particle.m_xmf3Position;
    m_pMappedParticles[m_nActiveParticles].m_xmf2Size = size;
    m_pMappedParticles[m_nActiveParticles].m_xmf3Color = color;
    ++m_nActiveParticles;
}

void CParticleSystem::UpdateByMotion(float fTimeElapsed, const ParticleConfig& config)
{
    m_LastConfig = config;
    if (config.motion == ParticleMotion::EMITTER) EmitParticles(config);
    m_nActiveParticles = 0;

    for (ParticleCPUData& particle : m_vCpuParticles)
    {
        if (!particle.m_bIsActive) continue;
        particle.m_fAge += fTimeElapsed;

        if (config.motion != ParticleMotion::ORBIT && particle.m_fAge >= particle.m_fLifeTime)
        {
            particle.m_bIsActive = false;
            continue;
        }

        if (config.motion == ParticleMotion::ORBIT)
        {
            particle.m_fOrbitAngle += particle.m_fOrbitSpeed * fTimeElapsed;
            particle.m_xmf3Position.x = std::cos(particle.m_fOrbitAngle) * particle.m_fOrbitRadius;
            particle.m_xmf3Position.y = config.orbitHeight;
            particle.m_xmf3Position.z = -std::sin(particle.m_fOrbitAngle) * particle.m_fOrbitRadius;
        }
        else
        {
            XMFLOAT3 acceleration = config.acceleration;
            acceleration.y += config.gravity - config.acceleration.y;
            particle.m_xmf3Velocity.x += acceleration.x * fTimeElapsed;
            particle.m_xmf3Velocity.y += acceleration.y * fTimeElapsed;
            particle.m_xmf3Velocity.z += acceleration.z * fTimeElapsed;
            if (config.moveX) particle.m_xmf3Position.x += particle.m_xmf3Velocity.x * fTimeElapsed;
            if (config.moveY) particle.m_xmf3Position.y += particle.m_xmf3Velocity.y * fTimeElapsed;
            if (config.moveZ) particle.m_xmf3Position.z += particle.m_xmf3Velocity.z * fTimeElapsed;
        }

        WriteParticleToGpu(particle, config);
    }
}

void CParticleSystem::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
    if (!pd3dCommandList || !m_pd3dVertexBuffer || m_nActiveParticles == 0) return;
    XMFLOAT4X4 world;
    XMStoreFloat4x4(&world, XMMatrixTranspose(XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z)));
    pd3dCommandList->SetGraphicsRoot32BitConstants(2, 16, &world, 0);
    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    pd3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);
    pd3dCommandList->DrawInstanced(m_nActiveParticles, 1, 0, 0);
}

void CParticleSystem::Clear()
{
    for (ParticleCPUData& particle : m_vCpuParticles) particle.m_bIsActive = false;
    m_nActiveParticles = 0;
}