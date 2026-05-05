#include "stdafx.h"
#include "EffectRendererDX12.h"
#include "EffectLibrary.h"
#include "ParticleSystem.h"
#include "MeshEffect.h"
#include "d3dx12.h"

#include <cstring>

// 쉐이더 컴파일 헬퍼 함수
D3D12_SHADER_BYTECODE CompileShaderHelper(LPCWSTR filename, LPCSTR entrypoint, LPCSTR target)
{
	UINT compileFlags = 0;
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* byteCode = nullptr;
	ID3DBlob* errors = nullptr;

	HRESULT hr = D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint, target, compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
		errors->Release();
	}

	if (FAILED(hr))
	{
		return { nullptr, 0 };
	}

	return { byteCode->GetBufferPointer(), byteCode->GetBufferSize() };
}

CEffectLibrary::CEffectLibrary()
{
	InitializeDefaultEffectConfigs();
	InitializeDefaultMeshConfigs();
}

void CEffectLibrary::InitializeDefaultEffectConfigs()
{
	for (int i = 0; i < (int)EFFECT_TYPE::COUNT; ++i)
	{
		m_EffectConfigs[i] = EffectTypeConfig();
	}

	m_EffectConfigs[(int)EFFECT_TYPE::COLLISION].spread = 20.0f;

	m_EffectConfigs[(int)EFFECT_TYPE::DUST].poolSize = 2000;
	m_EffectConfigs[(int)EFFECT_TYPE::DUST].particleCount = 1;
	m_EffectConfigs[(int)EFFECT_TYPE::DUST].useDepth = true;

	for (int i = (int)EFFECT_TYPE::ITEM1; i <= (int)EFFECT_TYPE::ITEM9; ++i)
	{
		m_EffectConfigs[i].spread = 50.0f;
	}

	m_EffectConfigs[(int)EFFECT_TYPE::BOOSTER].poolSize = 1;
	m_EffectConfigs[(int)EFFECT_TYPE::BOOSTER].particleCount = 50;
	m_EffectConfigs[(int)EFFECT_TYPE::BOOSTER].lifeTime = 999999.0f;
	m_EffectConfigs[(int)EFFECT_TYPE::BOOSTER].loop = true;
	m_EffectConfigs[(int)EFFECT_TYPE::BOOSTER].useDepth = true;

	m_EffectConfigs[(int)EFFECT_TYPE::WIND_EFFECT].poolSize = 1;
	m_EffectConfigs[(int)EFFECT_TYPE::WIND_EFFECT].lifeTime = 999999.0f;
	m_EffectConfigs[(int)EFFECT_TYPE::WIND_EFFECT].loop = true;
}

void CEffectLibrary::InitializeDefaultMeshConfigs()
{
	m_WindMeshConfig.radius = 15.0f;
	m_WindMeshConfig.sliceCount = 20;
	m_WindMeshConfig.stackCount = 20;
	m_WindMeshConfig.textureFiles = {
		L"Asset/DDS_File/noise.dds",
		L"Asset/DDS_File/noise.dds",
		L"Asset/DDS_File/noise.dds"
	};
	m_WindMeshConfig.scale = XMFLOAT3(3.f, 10.5f, 1.5f);
	m_WindMeshConfig.scrollSpeed = XMFLOAT3(0.0f, -6.0f, 0.0f);

	m_BoosterMeshConfig.radius = 2.0f;
	m_BoosterMeshConfig.sliceCount = 20;
	m_BoosterMeshConfig.stackCount = 20;
	m_BoosterMeshConfig.textureFiles = {
		L"Asset/DDS_File/BoosterBase.dds",
		L"Asset/DDS_File/BoosterNoise.dds",
		L"Asset/DDS_File/BoosterMask.dds"
	};
	m_BoosterMeshConfig.color = XMFLOAT3(0.1f, 0.5f, 1.0f);
	m_BoosterMeshConfig.scale = XMFLOAT3(2.f, 10.0f, 2.f);
	m_BoosterMeshConfig.scrollSpeed = XMFLOAT3(0.0f, 6.0f, 0.0f);
}

bool CEffectLibrary::IsValidEffectType(EFFECT_TYPE type) const
{
	int index = (int)type;
	return index >= 0 && index < (int)EFFECT_TYPE::COUNT;
}

bool CEffectLibrary::IsItemEffect(EFFECT_TYPE type) const
{
	return type >= EFFECT_TYPE::ITEM1 && type <= EFFECT_TYPE::ITEM9;
}

bool CEffectLibrary::IsDepthParticleEffect(EFFECT_TYPE type) const
{
	if (!IsValidEffectType(type)) return false;
	return m_EffectConfigs[(int)type].useDepth;
}

float CEffectLibrary::GetConfiguredSpread(EFFECT_TYPE type) const
{
	if (!IsValidEffectType(type)) return 0.0f;
	return m_EffectConfigs[(int)type].spread;
}

float CEffectLibrary::GetConfiguredLifeTime(EFFECT_TYPE type) const
{
	if (!IsValidEffectType(type)) return 2.0f;
	return m_EffectConfigs[(int)type].lifeTime;
}

bool CEffectLibrary::GetConfiguredLoop(EFFECT_TYPE type) const
{
	if (!IsValidEffectType(type)) return false;
	return m_EffectConfigs[(int)type].loop;
}

void CEffectLibrary::SetEffectTypeConfig(EFFECT_TYPE type, const EffectTypeConfig& config)
{
	if (!IsValidEffectType(type)) return;
	m_EffectConfigs[(int)type] = config;
}

void CEffectLibrary::SetEffectLifeTime(EFFECT_TYPE type, float lifeTime)
{
	if (!IsValidEffectType(type)) return;
	m_EffectConfigs[(int)type].lifeTime = lifeTime;
}

void CEffectLibrary::SetEffectTextureFileName(EFFECT_TYPE type, const std::wstring& fileName)
{
	if (!IsValidEffectType(type)) return;
	m_TextureFileNames[(int)type] = fileName;
}

void CEffectLibrary::SetBoosterMeshConfig(const EffectMeshConfig& config)
{
	m_BoosterMeshConfig = config;
}

void CEffectLibrary::SetWindMeshConfig(const EffectMeshConfig& config)
{
	m_WindMeshConfig = config;
}

void CEffectLibrary::SetBoosterTextureFiles(const std::vector<std::wstring>& textureFiles)
{
	m_BoosterMeshConfig.textureFiles = textureFiles;
}

void CEffectLibrary::SetWindTextureFiles(const std::vector<std::wstring>& textureFiles)
{
	m_WindMeshConfig.textureFiles = textureFiles;
}

void CEffectLibrary::StopEffectType(EFFECT_TYPE type)
{
	if (!IsValidEffectType(type)) return;

	auto it = m_vActiveEffects.begin();
	while (it != m_vActiveEffects.end())
	{
		ActiveEffect* eff = *it;
		if (eff && eff->type == type)
		{
			if (eff->pMeshEffect) eff->pMeshEffect->SetActive(false);
			if (eff->pParticleSys) eff->pParticleSys->Clear();
			RecycleEffect(eff);
			it = m_vActiveEffects.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void CEffectLibrary::ClearActiveEffects()
{
	auto it = m_vActiveEffects.begin();
	while (it != m_vActiveEffects.end())
	{
		ActiveEffect* eff = *it;
		if (eff)
		{
			if (eff->pMeshEffect) eff->pMeshEffect->SetActive(false);
			if (eff->pParticleSys) eff->pParticleSys->Clear();
			RecycleEffect(eff);
		}
		it = m_vActiveEffects.erase(it);
	}
}

int CEffectLibrary::GetActiveEffectCount() const
{
	return (int)m_vActiveEffects.size();
}

int CEffectLibrary::GetPooledEffectCount(EFFECT_TYPE type) const
{
	if (!IsValidEffectType(type)) return 0;
	return (int)m_vEffectPool[(int)type].size();
}

CEffectLibrary* CEffectLibrary::Instance()
{
	static CEffectLibrary inst;
	return &inst;
}

void CEffectLibrary::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	ReleaseIfInitialized();

	if (!InitializeRenderResources(pd3dDevice, pd3dCommandList))
	{
		OutputDebugStringA("[EffectLibrary] InitializeRenderResources failed.\n");
		return;
	}

	CreateEffectPools(pd3dDevice, pd3dCommandList);

	m_pRenderer = std::make_unique<EffectRendererDX12>();
	m_pRenderer->Initialize(pd3dDevice);
}

void CEffectLibrary::ReleaseIfInitialized()
{
	if (m_pd3dSrvHeap != nullptr)
	{
		Release();
	}
}

bool CEffectLibrary::InitializeRenderResources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (!pd3dDevice || !pd3dCommandList) return false;

	BuildRootSignature(pd3dDevice);
	BuildPipelineState(pd3dDevice);

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = (int)EFFECT_TYPE::COUNT + 3; // 텍스처 수만큼
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;

	HRESULT hr = pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pd3dSrvHeap));
	if (FAILED(hr) || (m_pd3dSrvHeap == nullptr))
	{
		OutputDebugStringA("[EffectLibrary] CreateDescriptorHeap failed.\n");
		return false;
	}

	m_d3dSrvCpuHandleStart = m_pd3dSrvHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dSrvGpuHandleStart = m_pd3dSrvHeap->GetGPUDescriptorHandleForHeapStart();
	m_nSrvDescriptorIncrementSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	LoadAssets(pd3dDevice, pd3dCommandList);
	return true;
}

void CEffectLibrary::CreateEffectPools(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	for (int typeIndex = 0; typeIndex < (int)EFFECT_TYPE::COUNT; ++typeIndex)
	{
		EFFECT_TYPE type = (EFFECT_TYPE)typeIndex;

		if (type == EFFECT_TYPE::WIND_EFFECT)
		{
			CreateWindEffect(pd3dDevice, pd3dCommandList);
			continue;
		}

		if (type == EFFECT_TYPE::BOOSTER)
		{
			CreateBoosterEffect(pd3dDevice, pd3dCommandList);
		}

		const EffectTypeConfig& config = m_EffectConfigs[typeIndex];
		CreateParticleEffectPool(type, pd3dDevice, pd3dCommandList, config.poolSize, config.particleCount);
	}
}

void CEffectLibrary::CreateWindEffect(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CMeshEffect* pShield = new CMeshEffect(pd3dDevice, pd3dCommandList);
	pShield->CreateMesh(pd3dDevice, pd3dCommandList, m_WindMeshConfig.radius, m_WindMeshConfig.sliceCount, m_WindMeshConfig.stackCount);
	pShield->CreateTextures(pd3dDevice, pd3dCommandList, m_WindMeshConfig.textureFiles);
	pShield->SetColor(m_WindMeshConfig.color);
	pShield->SetScale(m_WindMeshConfig.scale);
	pShield->SetScrollSpeed(m_WindMeshConfig.scrollSpeed);

	ActiveEffect* pEffect = new ActiveEffect;
	pEffect->type = EFFECT_TYPE::WIND_EFFECT;
	pEffect->bActive = false;
	pEffect->fAge = 0.0f;
	pEffect->fLifeTime = GetConfiguredLifeTime(EFFECT_TYPE::WIND_EFFECT);
	pEffect->bLoop = GetConfiguredLoop(EFFECT_TYPE::WIND_EFFECT);
	pEffect->fSpread = 0.0f;
	pEffect->bUseSpread = false;
	pEffect->pParticleSys = nullptr;
	pEffect->pMeshEffect = pShield;

	m_vActiveEffects.push_back(pEffect);
	m_pWindShieldEffect = pEffect;
}

void CEffectLibrary::CreateBoosterEffect(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	const EffectTypeConfig& boosterConfig = m_EffectConfigs[(int)EFFECT_TYPE::BOOSTER];
	CParticleSystem* pBoosterParticles = new CParticleSystem(pd3dDevice, pd3dCommandList, boosterConfig.particleCount);

	CMeshEffect* pBoosterMesh = new CMeshEffect(pd3dDevice, pd3dCommandList);
	pBoosterMesh->CreateMesh(pd3dDevice, pd3dCommandList, m_BoosterMeshConfig.radius, m_BoosterMeshConfig.sliceCount, m_BoosterMeshConfig.stackCount);
	pBoosterMesh->CreateTextures(pd3dDevice, pd3dCommandList, m_BoosterMeshConfig.textureFiles);
	pBoosterMesh->SetColor(m_BoosterMeshConfig.color);
	pBoosterMesh->SetScale(m_BoosterMeshConfig.scale);
	pBoosterMesh->SetScrollSpeed(m_BoosterMeshConfig.scrollSpeed);

	ActiveEffect* pEffect = new ActiveEffect;
	pEffect->type = EFFECT_TYPE::BOOSTER;
	pEffect->bActive = false;
	pEffect->fAge = 0.0f;
	pEffect->fLifeTime = GetConfiguredLifeTime(EFFECT_TYPE::BOOSTER);
	pEffect->bLoop = GetConfiguredLoop(EFFECT_TYPE::BOOSTER);
	pEffect->fSpread = GetConfiguredSpread(EFFECT_TYPE::BOOSTER);
	pEffect->bUseSpread = (pEffect->fSpread > 0.0001f);
	pEffect->pParticleSys = pBoosterParticles;
	pEffect->pMeshEffect = pBoosterMesh;

	m_vActiveEffects.push_back(pEffect);
	m_pBoosterEffect = pEffect;
}

void CEffectLibrary::CreateParticleEffectPool(EFFECT_TYPE type, ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int nPoolSize, int nParticleCount)
{
	for (int i = 0; i < nPoolSize; ++i)
	{
		CParticleSystem* pSys = new CParticleSystem(pd3dDevice, pd3dCommandList, nParticleCount);

		ActiveEffect* pEffect = new ActiveEffect{
			type,
			false,
			0.0f,
			GetConfiguredLifeTime(type),
			GetConfiguredLoop(type),
			GetConfiguredSpread(type),
			(GetConfiguredSpread(type) > 0.0001f),
			pSys,
			nullptr
		};

		m_vEffectPool[(int)type].push_back(pEffect);
	}
}


void CEffectLibrary::BuildRootSignature(ID3D12Device* pd3dDevice)
{
	CD3DX12_ROOT_PARAMETER rootParameters[3];

	CD3DX12_DESCRIPTOR_RANGE ranges[1];

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 6);
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsConstants(32, 6, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstants(24, 7, 0, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_STATIC_SAMPLER_DESC sampler(
		0,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(3, rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (FAILED(hr))
	{
		if (error) OutputDebugStringA((char*)error->GetBufferPointer());
		OutputDebugStringA("[EffectLibrary] D3D12SerializeRootSignature failed.\n");
		if (signature) signature->Release();
		if (error) error->Release();
		return;
	}
	hr = pd3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
	if (FAILED(hr) || (m_pRootSignature == nullptr))
	{
		OutputDebugStringA("[EffectLibrary] CreateRootSignature failed.\n");
	}

	if (signature) signature->Release();
	if (error) error->Release();
}

void CEffectLibrary::LoadAssets(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_vTextures.resize((int)EFFECT_TYPE::COUNT, nullptr);
	m_vUploadBuffers.resize((int)EFFECT_TYPE::COUNT, nullptr);

	UINT nDescriptorSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE currentCpuHandle = m_d3dSrvCpuHandleStart;

	for (int i = 0; i < (int)EFFECT_TYPE::COUNT; ++i)
	{
		if ((EFFECT_TYPE)i == EFFECT_TYPE::WIND_EFFECT || (EFFECT_TYPE)i == EFFECT_TYPE::BOOSTER)
		{
			currentCpuHandle.ptr += nDescriptorSize;
			continue;
		}

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		if (m_TextureFileNames[i].empty())
		{
			currentCpuHandle.ptr += nDescriptorSize;
			continue;
		}

		HRESULT hr = LoadDDSTextureFromFile(
			pd3dDevice,
			m_TextureFileNames[i].c_str(),
			&m_vTextures[i],
			ddsData,
			subresources
		);

		if (FAILED(hr))
		{
			OutputDebugStringW((L"Failed to load texture: " + m_TextureFileNames[i] + L"\n").c_str());
			currentCpuHandle.ptr += nDescriptorSize;
			continue;
		}

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_vTextures[i], 0, static_cast<UINT>(subresources.size()));

		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		pd3dDevice->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vUploadBuffers[i])
		);

		UpdateSubresources(pd3dCommandList, m_vTextures[i], m_vUploadBuffers[i], 0, 0, static_cast<UINT>(subresources.size()), subresources.data());

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_vTextures[i],
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		pd3dCommandList->ResourceBarrier(1, &barrier);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = m_vTextures[i]->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_vTextures[i]->GetDesc().MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		pd3dDevice->CreateShaderResourceView(m_vTextures[i], &srvDesc, currentCpuHandle);

		currentCpuHandle.ptr += nDescriptorSize;
	}
}

void CEffectLibrary::Render(ID3D12GraphicsCommandList* pd3dCommandList, const XMFLOAT4X4& view, const XMFLOAT4X4& proj)
{
	if (m_pRenderer)
	{
		EffectRenderContext context;
		context.commandContext = pd3dCommandList;

		EffectMat4 effectView{};
		EffectMat4 effectProj{};

		memcpy(effectView.m, &view, sizeof(EffectMat4));
		memcpy(effectProj.m, &proj, sizeof(EffectMat4));

		m_pRenderer->Render(context, this, effectView, effectProj);
	}
}


void CEffectLibrary::RenderParticleEffect(ID3D12GraphicsCommandList* pd3dCommandList, ActiveEffect* eff, int& currentPsoType, ID3D12DescriptorHeap** ppParticleHeap)
{
	if (!pd3dCommandList || !eff || !eff->pParticleSys) return;

	bool bUseDepth = IsDepthParticleEffect(eff->type);

	int nDesiredPsoType = bUseDepth ? 3 : 1;
	ID3D12PipelineState* pTargetPSO = bUseDepth ? m_pParticleDepthPSO : m_pPipelineState;

	if (pTargetPSO == nullptr) return;

	if (currentPsoType != nDesiredPsoType)
	{
		pd3dCommandList->SetPipelineState(pTargetPSO);
		currentPsoType = nDesiredPsoType;
	}

	pd3dCommandList->SetDescriptorHeaps(1, ppParticleHeap);

	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = m_d3dSrvGpuHandleStart;
	textureHandle.ptr += (UINT64)eff->type * m_nSrvDescriptorIncrementSize;
	pd3dCommandList->SetGraphicsRootDescriptorTable(0, textureHandle);

	eff->pParticleSys->Render(pd3dCommandList);
}

void CEffectLibrary::RenderMeshEffect(ID3D12GraphicsCommandList* pd3dCommandList, ActiveEffect* eff, int& currentPsoType)
{
	if (!pd3dCommandList || !eff || !eff->pMeshEffect) return;

	if (eff->type == EFFECT_TYPE::BOOSTER)
	{
		if (m_pBoosterPSO == nullptr) return;

		if (currentPsoType != 4)
		{
			pd3dCommandList->SetPipelineState(m_pBoosterPSO);
			currentPsoType = 4;
		}
	}
	else
	{
		if (m_pMeshEffectPSO == nullptr) return;

		if (currentPsoType != 2)
		{
			pd3dCommandList->SetPipelineState(m_pMeshEffectPSO);
			currentPsoType = 2;
		}
	}

	eff->pMeshEffect->Render(pd3dCommandList);
}


void CEffectLibrary::BuildPipelineState(ID3D12Device* pd3dDevice)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };

	psoDesc.VS = CompileShaderHelper(L"Shaders.hlsl", "VS_Particle", "vs_5_1");
	psoDesc.GS = CompileShaderHelper(L"Shaders.hlsl", "GS_Particle", "gs_5_1");
	psoDesc.PS = CompileShaderHelper(L"Shaders.hlsl", "PS_Particle", "ps_5_1");

	psoDesc.pRootSignature = m_pRootSignature;

	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState = blendDesc;

	D3D12_DEPTH_STENCIL_DESC depthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState = depthDesc;

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX;

	HRESULT hr = pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState));
	if (FAILED(hr) || (m_pPipelineState == nullptr))
	{
		OutputDebugStringA("[EffectLibrary] CreateGraphicsPipelineState(Particle) failed.\n");
	}


	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	hr = pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pParticleDepthPSO));
	if (FAILED(hr) || (m_pParticleDepthPSO == nullptr))
	{
		OutputDebugStringA("[EffectLibrary] CreateGraphicsPipelineState(Particle Depth) failed.\n");
	}

	// 바람저항효과 PSO 생성
	D3D12_GRAPHICS_PIPELINE_STATE_DESC meshPsoDesc = {};



	D3D12_INPUT_ELEMENT_DESC meshInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	meshPsoDesc.InputLayout = { meshInputLayout, _countof(meshInputLayout) };

	meshPsoDesc.VS = CompileShaderHelper(L"Shaders.hlsl", "VS_WindShield", "vs_5_1");
	meshPsoDesc.PS = CompileShaderHelper(L"Shaders.hlsl", "PS_WindShield", "ps_5_1");
	meshPsoDesc.GS = { nullptr, 0 };

	meshPsoDesc.pRootSignature = m_pRootSignature;

	D3D12_BLEND_DESC meshBlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	meshBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	meshBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	meshBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	meshBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	meshPsoDesc.BlendState = meshBlendDesc;

	D3D12_DEPTH_STENCIL_DESC meshDepthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	meshDepthDesc.DepthEnable = TRUE;
	meshDepthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	meshPsoDesc.DepthStencilState = meshDepthDesc;

	meshPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	meshPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	meshPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	meshPsoDesc.NumRenderTargets = 1;
	meshPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	meshPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	meshPsoDesc.SampleDesc.Count = 1;
	meshPsoDesc.SampleMask = UINT_MAX;

	hr = pd3dDevice->CreateGraphicsPipelineState(&meshPsoDesc, IID_PPV_ARGS(&m_pMeshEffectPSO));
	if (FAILED(hr) || (m_pMeshEffectPSO == nullptr))
	{
		OutputDebugStringA("[EffectLibrary] CreateGraphicsPipelineState(MeshEffect) failed.\n");
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC boosterPsoDesc = meshPsoDesc;

	boosterPsoDesc.VS = CompileShaderHelper(L"Shaders.hlsl", "VS_Booster", "vs_5_1");
	boosterPsoDesc.PS = CompileShaderHelper(L"Shaders.hlsl", "PS_Booster", "ps_5_1");

	D3D12_BLEND_DESC boosterBlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	boosterBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	boosterBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	boosterBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	boosterBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	boosterBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	boosterBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	boosterBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	boosterPsoDesc.BlendState = boosterBlendDesc;

	D3D12_DEPTH_STENCIL_DESC boosterDepthDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	boosterDepthDesc.DepthEnable = TRUE;
	boosterDepthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	boosterDepthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	boosterPsoDesc.DepthStencilState = boosterDepthDesc;

	hr = pd3dDevice->CreateGraphicsPipelineState(&boosterPsoDesc, IID_PPV_ARGS(&m_pBoosterPSO));
	if (FAILED(hr) || (m_pBoosterPSO == nullptr))
	{
		OutputDebugStringA("[EffectLibrary] CreateGraphicsPipelineState(Booster) failed.\n");
	}
}

ActiveEffect* CEffectLibrary::Play(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color)
{
	if (!IsValidEffectType(type)) return nullptr;
	if (m_vEffectPool[(int)type].empty()) return nullptr;

	ActiveEffect* pEffectData = m_vEffectPool[(int)type].back();
	m_vEffectPool[(int)type].pop_back();

	pEffectData->bActive = true;
	pEffectData->fAge = 0.0f;
	pEffectData->type = type;
	pEffectData->fLifeTime = GetConfiguredLifeTime(type);
	pEffectData->bLoop = GetConfiguredLoop(type);
	pEffectData->fSpread = GetConfiguredSpread(type);
	pEffectData->bUseSpread = (pEffectData->fSpread > 0.0001f);

	if (pEffectData->pParticleSys)
	{
		// 파티클 시스템
		pEffectData->pParticleSys->SetPosition(position);

		float fSpread = pEffectData->fSpread;
		pEffectData->bUseSpread = !IsZero(fSpread);

		pEffectData->pParticleSys->ResetParticles(size, fSpread, pEffectData->bUseSpread, color);

	}
	else if (pEffectData->pMeshEffect)
	{
		// 바람저항효과인 경우
		pEffectData->pMeshEffect->SetPosition(position);
		pEffectData->pMeshEffect->SetActive(true);
	}

	m_vActiveEffects.push_back(pEffectData);
	return pEffectData;
}

void CEffectLibrary::PlayCarDustParticle(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT3 right, XMFLOAT3 look, XMFLOAT2 size, XMFLOAT2 offset, XMFLOAT3 color)
{
	XMVECTOR vPos = XMLoadFloat3(&position);
	XMVECTOR vRight = XMVector3Normalize(XMLoadFloat3(&right));
	XMVECTOR vLook = XMVector3Normalize(XMLoadFloat3(&look));

	XMVECTOR vOffsetX = vRight * offset.x;
	XMVECTOR vOffsetZ = vLook * offset.y;

	XMFLOAT3 fl, fr, bl, br;

	XMStoreFloat3(&fl, vPos - vOffsetX + vOffsetZ); // 좌상 
	XMStoreFloat3(&fr, vPos + vOffsetX + vOffsetZ); // 우상 
	XMStoreFloat3(&bl, vPos - vOffsetX - vOffsetZ); // 좌하 
	XMStoreFloat3(&br, vPos + vOffsetX - vOffsetZ); // 우하 

	Play(type, fl, size, color); // 좌상
	Play(type, fr, size, color); // 우상
	Play(type, bl, size, color); // 좌하
	Play(type, br, size, color); // 우하
}


void CEffectLibrary::PushEffectEvent(const EffectEvent& eventData)
{
	m_qEffectEvents.push(eventData);
}

void CEffectLibrary::PushEffectEvent(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color)
{
	m_qEffectEvents.push(EffectEvent(type, position, size, color));
}

void CEffectLibrary::PushEffectEvent(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color, float lifeTime, bool loop)
{
	m_qEffectEvents.push(EffectEvent(type, position, size, color, lifeTime, loop));
}



void CEffectLibrary::Update(float fTimeElapsed)
{
	ConsumeEffectEvents();

	auto it = m_vActiveEffects.begin();

	while (it != m_vActiveEffects.end())
	{
		ActiveEffect* eff = *it;
		bool bIsDead = false;

		UpdateEffectInstance(eff, fTimeElapsed, bIsDead);

		if (bIsDead)
		{
			RecycleEffect(eff);
			it = m_vActiveEffects.erase(it);
		}
		else
		{
			++it;
		}
	}

	UpdateSpeedLineState(fTimeElapsed);
}

void CEffectLibrary::ConsumeEffectEvents()
{
	while (!m_qEffectEvents.empty())
	{
		EffectEvent eventData = m_qEffectEvents.front();
		m_qEffectEvents.pop();

		ActiveEffect* effect = Play(eventData.type, eventData.position, eventData.size, eventData.color);
		if (effect)
		{
			if (eventData.lifeTime > 0.0f) effect->fLifeTime = eventData.lifeTime;
			effect->bLoop = eventData.loop || GetConfiguredLoop(eventData.type);
		}
	}
}

void CEffectLibrary::UpdateEffectInstance(ActiveEffect* eff, float fTimeElapsed, bool& bIsDead)
{
	if (!eff) return;

	UpdateParticleEffect(eff, fTimeElapsed);
	UpdateMeshEffect(eff, fTimeElapsed);

	if (!eff->bLoop)
	{
		eff->fAge += fTimeElapsed;

		if (eff->fAge > eff->fLifeTime)
		{
			bIsDead = true;
		}
	}
}

void CEffectLibrary::UpdateParticleEffect(ActiveEffect* eff, float fTimeElapsed)
{
	if (!eff || !eff->pParticleSys) return;
	if (m_pPipelineState == nullptr) return;

	if (eff->type == EFFECT_TYPE::BOOSTER)
	{
		if (eff->bActive) eff->pParticleSys->BoosterAnimate(fTimeElapsed);
	}
	else if (eff->type == EFFECT_TYPE::DUST)
	{
		eff->pParticleSys->DustAnimate(fTimeElapsed, eff->bUseSpread);
	}
	else if (IsItemEffect(eff->type))
	{
		eff->pParticleSys->ItemAnimate(fTimeElapsed);
	}
	else
	{
		eff->pParticleSys->CollisionAnimate(fTimeElapsed);
	}
}

void CEffectLibrary::UpdateMeshEffect(ActiveEffect* eff, float fTimeElapsed)
{
	if (!eff || !eff->pMeshEffect) return;

	if (eff->pMeshEffect->IsActive())
	{
		eff->pMeshEffect->Update(fTimeElapsed);
	}
}

void CEffectLibrary::RecycleEffect(ActiveEffect* eff)
{
	if (!eff) return;

	eff->bActive = false;
	m_vEffectPool[(int)eff->type].push_back(eff);
}

void CEffectLibrary::UpdateSpeedLineState(float fTimeElapsed)
{
	m_fSpeedLineAccumTime += fTimeElapsed;

	if (m_fSpeedLineAccumTime > 0.05f)
	{
		m_fSpeedLineAccumTime = 0.0f;
		m_fSpeedLineAngle = ((rand() % 360) * 3.141592f) / 180.0f;
		m_fSpeedLineScale = 0.9f + ((rand() % 20) / 100.0f);
	}

	if (m_fCurrentPlayerSpeedRatio > 0.7f)
	{
		m_fSpeedLineAlpha = (m_fCurrentPlayerSpeedRatio - 0.7f) * 3.33f;
		if (m_fSpeedLineAlpha > 1.0f) m_fSpeedLineAlpha = 1.0f;
	}
	else
	{
		m_fSpeedLineAlpha = 0.0f;
	}
}


void CEffectLibrary::Release()
{
	for (auto& texture : m_vTextures)
	{
		if (texture) texture->Release();
	}
	m_vTextures.clear();

	for (auto& buffer : m_vUploadBuffers)
	{
		if (buffer) buffer->Release();
	}
	m_vUploadBuffers.clear();

	if (m_pd3dSrvHeap) {
		m_pd3dSrvHeap->Release();
		m_pd3dSrvHeap = nullptr;
	}

	if (m_pRootSignature) m_pRootSignature->Release();
	if (m_pPipelineState) m_pPipelineState->Release();
	if (m_pMeshEffectPSO) m_pMeshEffectPSO->Release();
	if (m_pParticleDepthPSO) m_pParticleDepthPSO->Release();
	if (m_pBoosterPSO) m_pBoosterPSO->Release();

	m_pRootSignature = nullptr;
	m_pPipelineState = nullptr;
	m_pMeshEffectPSO = nullptr;
	m_pParticleDepthPSO = nullptr;
	m_pBoosterPSO = nullptr;

	for (auto eff : m_vActiveEffects)
	{
		if (eff->pParticleSys) delete eff->pParticleSys;
		if (eff->pMeshEffect) delete eff->pMeshEffect;
		delete eff;
	}
	m_vActiveEffects.clear();

	for (int i = 0; i < (int)EFFECT_TYPE::COUNT; ++i)
	{
		for (auto eff : m_vEffectPool[i])
		{
			if (eff->pParticleSys) delete eff->pParticleSys;
			if (eff->pMeshEffect) delete eff->pMeshEffect;
			delete eff;
		}
		m_vEffectPool[i].clear();
	}

	m_pBoosterEffect = nullptr;
	m_pWindShieldEffect = nullptr;
	while (!m_qEffectEvents.empty()) m_qEffectEvents.pop();

	if (m_pRadialBlurPSO) { m_pRadialBlurPSO->Release(); m_pRadialBlurPSO = nullptr; }
	if (m_pd3dComputeRootSignature) { m_pd3dComputeRootSignature->Release(); m_pd3dComputeRootSignature = nullptr; }
	if (m_pSceneRenderTexture) { m_pSceneRenderTexture->Release(); m_pSceneRenderTexture = nullptr; }
	if (m_pBlurTexture) { m_pBlurTexture->Release(); m_pBlurTexture = nullptr; }
	if (m_pd3dPostProcessRtvHeap) { m_pd3dPostProcessRtvHeap->Release(); m_pd3dPostProcessRtvHeap = nullptr; }
	if (m_pd3dCbvSrvUavHeap) { m_pd3dCbvSrvUavHeap->Release(); m_pd3dCbvSrvUavHeap = nullptr; }
}

void CEffectLibrary::ToggleBooster(bool flag)
{
	if (m_pWindShieldEffect && m_pWindShieldEffect->pMeshEffect)
	{
		m_pWindShieldEffect->pMeshEffect->SetActive(flag);
	}

	if (m_pBoosterEffect)
	{
		m_pBoosterEffect->bActive = flag;

		if (m_pBoosterEffect->pMeshEffect)
		{
			m_pBoosterEffect->pMeshEffect->SetActive(flag);
		}

		if (!flag && m_pBoosterEffect->pParticleSys)
		{
			m_pBoosterEffect->pParticleSys->Clear();
		}
	}
}

void CEffectLibrary::UpdateBoosterPosition(const XMFLOAT3& pos, const XMFLOAT3& lookDir)
{
	XMVECTOR vLook = XMLoadFloat3(&lookDir);
	vLook = XMVector3Normalize(vLook);

	if (m_pWindShieldEffect && m_pWindShieldEffect->pMeshEffect)
	{
		//XMVECTOR vFrontPos = XMLoadFloat3(&pos) - (vLook * 150.0f);
		XMVECTOR vFrontPos = XMLoadFloat3(&pos) - (vLook * 50.0f);
		XMFLOAT3 fFrontPos;
		XMStoreFloat3(&fFrontPos, vFrontPos);

		m_pWindShieldEffect->pMeshEffect->SetPosition(fFrontPos);

		float yaw = XMConvertToDegrees(atan2(lookDir.x, lookDir.z));

		float yVal = lookDir.y;
		if (yVal > 1.0f) yVal = 1.0f;
		if (yVal < -1.0f) yVal = -1.0f;
		float pitch = XMConvertToDegrees(asin(yVal));

		XMFLOAT3 rot = XMFLOAT3(90.0f - pitch, yaw, 0.0f);
		m_pWindShieldEffect->pMeshEffect->SetRotation(rot);
	}

	if (m_pBoosterEffect)
	{
		XMVECTOR vRearPos = XMLoadFloat3(&pos) - (vLook * 40.0f); // 부스터 생성위치
		XMFLOAT3 fRearPos;
		XMStoreFloat3(&fRearPos, vRearPos);

		if (m_pBoosterEffect->pParticleSys)
		{
			m_pBoosterEffect->pParticleSys->SetPosition(fRearPos);
		}

		if (m_pBoosterEffect->pMeshEffect)
		{
			m_pBoosterEffect->pMeshEffect->SetPosition(fRearPos);

			XMFLOAT3 backDir = XMFLOAT3(-lookDir.x, -lookDir.y, -lookDir.z);

			float yaw = XMConvertToDegrees(atan2(backDir.x, backDir.z));

			float yVal = backDir.y;
			if (yVal > 1.0f) yVal = 1.0f;
			if (yVal < -1.0f) yVal = -1.0f;
			float pitch = XMConvertToDegrees(asin(yVal));

			XMFLOAT3 rot = XMFLOAT3(90.0f - pitch, yaw, 0.0f);
			m_pBoosterEffect->pMeshEffect->SetRotation(rot);
		}
	}
}

void CEffectLibrary::InitializePostProcess(ID3D12Device* pd3dDevice, int width, int height)
{
	m_nWidth = width;
	m_nHeight = height;

	CD3DX12_ROOT_PARAMETER pd3dRootParameters[4];
	pd3dRootParameters[0].InitAsConstants(8, 0); // b0

	CD3DX12_DESCRIPTOR_RANGE srvRange[1];
	srvRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	pd3dRootParameters[1].InitAsDescriptorTable(1, srvRange); // t0

	CD3DX12_DESCRIPTOR_RANGE uavRange[1];
	uavRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	pd3dRootParameters[2].InitAsDescriptorTable(1, uavRange); // u0

	CD3DX12_DESCRIPTOR_RANGE speedLineRange[1];
	speedLineRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	pd3dRootParameters[3].InitAsDescriptorTable(1, speedLineRange); // t1 (속도선 텍스처)

	CD3DX12_STATIC_SAMPLER_DESC sampler(
		0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	CD3DX12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	d3dRootSignatureDesc.Init(4, pd3dRootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_pd3dComputeRootSignature);

	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pd3dComputeRootSignature;
	psoDesc.CS = CompileShaderHelper(L"Shaders.hlsl", "CS_RadialBlur", "cs_5_1");
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	pd3dDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pRadialBlurPSO));

	ResizePostProcess(pd3dDevice, width, height);
}

void CEffectLibrary::ResizePostProcess(ID3D12Device* pd3dDevice, int width, int height)
{
	if (m_pSceneRenderTexture) { m_pSceneRenderTexture->Release(); m_pSceneRenderTexture = nullptr; }
	if (m_pBlurTexture) { m_pBlurTexture->Release(); m_pBlurTexture = nullptr; }
	if (m_pd3dPostProcessRtvHeap) { m_pd3dPostProcessRtvHeap->Release(); m_pd3dPostProcessRtvHeap = nullptr; }
	if (m_pd3dCbvSrvUavHeap) { m_pd3dCbvSrvUavHeap->Release(); m_pd3dCbvSrvUavHeap = nullptr; }

	m_nWidth = width;
	m_nHeight = height;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0 };
	pd3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pd3dPostProcessRtvHeap));
	m_d3dSceneRtvCpuHandle = m_pd3dPostProcessRtvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
	pd3dDevice->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&m_pd3dCbvSrvUavHeap));

	UINT nIncrementSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_pd3dCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();

	m_d3dSrvGpuHandle = m_pd3dCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE clearVal = { DXGI_FORMAT_R8G8B8A8_UNORM, { 0.0f, 0.125f, 0.3f, 1.0f } };

	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &clearVal, IID_PPV_ARGS(&m_pSceneRenderTexture)
	);

	pd3dDevice->CreateRenderTargetView(m_pSceneRenderTexture, NULL, m_d3dSceneRtvCpuHandle);
	pd3dDevice->CreateShaderResourceView(m_pSceneRenderTexture, NULL, cpuHandle);
	cpuHandle.ptr += nIncrementSize;

	m_d3dUavGpuHandle = m_d3dSrvGpuHandle;
	m_d3dUavGpuHandle.ptr += nIncrementSize;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, NULL, IID_PPV_ARGS(&m_pBlurTexture)
	);

	pd3dDevice->CreateUnorderedAccessView(m_pBlurTexture, NULL, NULL, cpuHandle);
	cpuHandle.ptr += nIncrementSize;

	m_d3dSpeedLineGpuHandle = m_d3dUavGpuHandle;
	m_d3dSpeedLineGpuHandle.ptr += nIncrementSize;

	D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = m_d3dSrvCpuHandleStart;
	srcHandle.ptr += ((UINT64)EFFECT_TYPE::SPEED_LINE) * m_nSrvDescriptorIncrementSize;

	pd3dDevice->CopyDescriptorsSimple(1, cpuHandle, srcHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void CEffectLibrary::PrepareSceneRenderTarget(ID3D12GraphicsCommandList* pd3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
	D3D12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pSceneRenderTexture,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	pd3dCommandList->ResourceBarrier(1, &toRT);

	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	pd3dCommandList->ClearRenderTargetView(m_d3dSceneRtvCpuHandle, pfClearColor, 0, NULL);
	pd3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	pd3dCommandList->OMSetRenderTargets(1, &m_d3dSceneRtvCpuHandle, TRUE, &dsvHandle);
}

void CEffectLibrary::RenderRadialBlur(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12Resource* pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, int speed)
{
	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_pSceneRenderTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_pBlurTexture, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pd3dCommandList->ResourceBarrier(2, barriers);

	pd3dCommandList->SetComputeRootSignature(m_pd3dComputeRootSignature);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvUavHeap);
	pd3dCommandList->SetComputeRootDescriptorTable(1, m_d3dSrvGpuHandle); // t0
	pd3dCommandList->SetComputeRootDescriptorTable(2, m_d3dUavGpuHandle); // u0
	pd3dCommandList->SetComputeRootDescriptorTable(3, m_d3dSpeedLineGpuHandle); // t1

	// 속도 
	float maxSpeed = 300.0f;
	float speedRatio = max(0.0f, min(1.0f, (float)speed / maxSpeed));

	float slAlpha = 0.0f;
	if (speedRatio > 0.7f) {
		slAlpha = min(1.0f, (speedRatio - 0.7f) * 3.33f);
	}

	CB_RADIAL_BLUR cbData;
	cbData.strength = (speed > 0) ? min((float)speed * 0.0005f, 0.15f) : 0.0f;
	cbData.cx = 0.5f; cbData.cy = 0.5f;
	cbData.aspect = (float)m_nWidth / (float)m_nHeight;

	cbData.slSin = sinf(m_fSpeedLineAngle);
	cbData.slCos = cosf(m_fSpeedLineAngle);
	cbData.slScale = m_fSpeedLineScale;
	cbData.slAlpha = slAlpha;

	pd3dCommandList->SetComputeRoot32BitConstants(0, 8, &cbData, 0);

	pd3dCommandList->SetPipelineState(m_pRadialBlurPSO);
	pd3dCommandList->Dispatch((m_nWidth + 15) / 16, (m_nHeight + 15) / 16, 1);

	D3D12_RESOURCE_BARRIER copyBarriers[2];
	copyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	copyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_pBlurTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	pd3dCommandList->ResourceBarrier(2, copyBarriers);

	pd3dCommandList->CopyResource(pBackBuffer, m_pBlurTexture);

	D3D12_RESOURCE_BARRIER toRTBack = CD3DX12_RESOURCE_BARRIER::Transition(pBackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pd3dCommandList->ResourceBarrier(1, &toRTBack);

	pd3dCommandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);
}