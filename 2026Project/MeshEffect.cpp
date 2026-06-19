#include "EffectPCH.h"
#include "MeshEffect.h"
#include "DDSTextureLoader12.h"

CMeshEffect::CMeshEffect(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
    m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_xmf3Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

    XMStoreFloat4x4(&m_xmf4x4World, XMMatrixIdentity());
}

CMeshEffect::~CMeshEffect()
{
    if (m_pVertexBuffer) m_pVertexBuffer->Release();
    if (m_pIndexBuffer) m_pIndexBuffer->Release();

    for (auto& tex : m_vTextures) { if (tex) tex->Release(); }
    m_vTextures.clear();

    for (auto& buf : m_vTextureUploadBuffers) { if (buf) buf->Release(); }
    m_vTextureUploadBuffers.clear();

    if (m_pSrvHeap) m_pSrvHeap->Release();
}

void CMeshEffect::CreateMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fRadius, int nSlices, int nStacks)
{
    std::vector<EFFECT_VERTEX> vVertices;
    std::vector<UINT> vIndices;

    float phiStep = (XM_PI / 2.0f) / nStacks;
    float thetaStep = 2.0f * XM_PI / nSlices;

    for (int i = 0; i <= nStacks; ++i)
    {
        float phi = i * phiStep;

        for (int j = 0; j <= nSlices; ++j)
        {
            float theta = j * thetaStep;

            EFFECT_VERTEX v;

            v.position.x = fRadius * sinf(phi) * cosf(theta);
            v.position.y = fRadius * cosf(phi);
            v.position.z = fRadius * sinf(phi) * sinf(theta);

            XMVECTOR n = XMLoadFloat3(&v.position);
            XMStoreFloat3(&v.normal, XMVector3Normalize(n));

            v.uv.x = (float)j / nSlices;
            v.uv.y = (float)i / nStacks;

            vVertices.push_back(v);
        }
    }

    for (int i = 0; i < nStacks; ++i)
    {
        for (int j = 0; j < nSlices; ++j)
        {
            vIndices.push_back(i * (nSlices + 1) + j);
            vIndices.push_back((i + 1) * (nSlices + 1) + j);
            vIndices.push_back(i * (nSlices + 1) + (j + 1));

            vIndices.push_back(i * (nSlices + 1) + (j + 1));
            vIndices.push_back((i + 1) * (nSlices + 1) + j);
            vIndices.push_back((i + 1) * (nSlices + 1) + (j + 1));
        }
    }
    m_nIndices = (UINT)vIndices.size();

    UINT nVertexStride = sizeof(EFFECT_VERTEX);
    UINT nVertexBufferBytes = nVertexStride * (UINT)vVertices.size();

    D3D12_HEAP_PROPERTIES heapPropUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC resDescVertex = CD3DX12_RESOURCE_DESC::Buffer(nVertexBufferBytes);

    pd3dDevice->CreateCommittedResource(
        &heapPropUpload,
        D3D12_HEAP_FLAG_NONE,
        &resDescVertex,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pVertexBuffer)
    );

    void* pVertexDataBegin = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    m_pVertexBuffer->Map(0, &readRange, &pVertexDataBegin);
    memcpy(pVertexDataBegin, vVertices.data(), nVertexBufferBytes);
    m_pVertexBuffer->Unmap(0, nullptr);

    m_d3dVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
    m_d3dVertexBufferView.StrideInBytes = nVertexStride;
    m_d3dVertexBufferView.SizeInBytes = nVertexBufferBytes;

    UINT nIndexBufferBytes = sizeof(UINT) * (UINT)vIndices.size();
    D3D12_RESOURCE_DESC resDescIndex = CD3DX12_RESOURCE_DESC::Buffer(nIndexBufferBytes);

    pd3dDevice->CreateCommittedResource(
        &heapPropUpload,
        D3D12_HEAP_FLAG_NONE,
        &resDescIndex,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pIndexBuffer)
    );

    void* pIndexDataBegin = nullptr;
    m_pIndexBuffer->Map(0, &readRange, &pIndexDataBegin);
    memcpy(pIndexDataBegin, vIndices.data(), nIndexBufferBytes);
    m_pIndexBuffer->Unmap(0, nullptr);

    m_d3dIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
    m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_d3dIndexBufferView.SizeInBytes = nIndexBufferBytes;
}

void CMeshEffect::CreateTextures(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const std::vector<std::wstring>& fileNames)
{
    UINT nTexCount = (UINT)fileNames.size();
    m_vTextures.resize(nTexCount, nullptr);
    m_vTextureUploadBuffers.resize(nTexCount, nullptr);

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = nTexCount;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pSrvHeap));

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pSrvHeap->GetCPUDescriptorHandleForHeapStart();
    UINT nDescriptorSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (UINT i = 0; i < nTexCount; ++i)
    {
        std::unique_ptr<uint8_t[]> ddsData;
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;

        DirectX::LoadDDSTextureFromFile(pd3dDevice, fileNames[i].c_str(), &m_vTextures[i], ddsData, subresources);

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_vTextures[i], 0, static_cast<UINT>(subresources.size()));
        auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        pd3dDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vTextureUploadBuffers[i]));
        UpdateSubresources(pd3dCommandList, m_vTextures[i], m_vTextureUploadBuffers[i], 0, 0, static_cast<UINT>(subresources.size()), subresources.data());

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vTextures[i], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        pd3dCommandList->ResourceBarrier(1, &barrier);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = m_vTextures[i]->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = -1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        pd3dDevice->CreateShaderResourceView(m_vTextures[i], &srvDesc, cpuHandle);
        cpuHandle.ptr += nDescriptorSize; 
    }
}


void CMeshEffect::Update(float fTimeElapsed)
{
    if (!m_bActive) return;

    m_fCurrentTime += fTimeElapsed;

    if (m_bIsDirty) {
        XMMATRIX mScale = XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z);

        XMMATRIX mRot = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(m_xmf3Rotation.x),
            XMConvertToRadians(m_xmf3Rotation.y),
            XMConvertToRadians(m_xmf3Rotation.z)
        );


        XMMATRIX mTrans = XMMatrixTranslation(m_xmf3Position.x, m_xmf3Position.y, m_xmf3Position.z);

        XMMATRIX mWorld = mScale * mRot * mTrans;
        XMStoreFloat4x4(&m_xmf4x4World, mWorld);

        m_bIsDirty = false;
    }
}

void CMeshEffect::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
    if (!m_bActive) return;

    CB_EFFECT_DATA cbData;
    XMStoreFloat4x4(&cbData.m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
    cbData.m_fTime = m_fCurrentTime;
    cbData.m_fScrollSpeed = m_xmf3ScrollSpeed; 
    cbData.m_xmf4Color = m_xmf4Color;

    pd3dCommandList->SetGraphicsRoot32BitConstants(2, sizeof(CB_EFFECT_DATA) / 4, &cbData, 0);

    if (m_pSrvHeap)
    {
        ID3D12DescriptorHeap* ppHeaps[] = { m_pSrvHeap };
        pd3dCommandList->SetDescriptorHeaps(1, ppHeaps);
        pd3dCommandList->SetGraphicsRootDescriptorTable(0, m_pSrvHeap->GetGPUDescriptorHandleForHeapStart());
    }

    pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);
    pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
    pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}