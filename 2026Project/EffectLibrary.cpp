#include "stdafx.h"
#include "EffectLibrary.h"
#include "ParticleSystem.h"
#include "MeshEffect.h"
#include "d3dx12.h"

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

CEffectLibrary* CEffectLibrary::Instance()
{
	static CEffectLibrary inst;
	return &inst;
}

void CEffectLibrary::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pd3dSrvHeap != nullptr)
	{
		Release();
	}
	BuildRootSignature(pd3dDevice);
	BuildPipelineState(pd3dDevice);

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = (int)EFFECT_TYPE::COUNT; // 텍스처 수만큼
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;

	HRESULT hr = pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_pd3dSrvHeap));
	if (FAILED(hr) || (m_pd3dSrvHeap == nullptr))
	{
		OutputDebugStringA("[EffectLibrary] CreateDescriptorHeap failed.\n");
		return;
	}

	m_d3dSrvCpuHandleStart = m_pd3dSrvHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dSrvGpuHandleStart = m_pd3dSrvHeap->GetGPUDescriptorHandleForHeapStart();

	m_nSrvDescriptorIncrementSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	LoadAssets(pd3dDevice, pd3dCommandList);

	for (int typeIndex = 0; typeIndex < (int)EFFECT_TYPE::COUNT; ++typeIndex)
	{
		// 바람저항효과
		if ((EFFECT_TYPE)typeIndex == EFFECT_TYPE::WIND_EFFECT)
		{
			CMeshEffect* pShield = new CMeshEffect(pd3dDevice, pd3dCommandList);
			pShield->CreateMesh(pd3dDevice, pd3dCommandList, 15.0f, 20, 20);
			//pShield->CreateProceduralTexture(pd3dDevice, pd3dCommandList);
			//pShield->CreateTexture(pd3dDevice, pd3dCommandList, L"Asset/DDS_File/WindShield.dds");
			pShield->CreateTexture(pd3dDevice, pd3dCommandList, L"Asset/DDS_File/noise.dds");
			pShield->SetScale(XMFLOAT3(1.5f, 15.5f, 1.5f));
			//pShield->SetPosition(XMFLOAT3(0, 0, -30));

			ActiveEffect* pEffect = new ActiveEffect;
			pEffect->type = (EFFECT_TYPE)typeIndex;
			pEffect->bActive = false;
			pEffect->pParticleSys = nullptr;
			pEffect->pMeshEffect = pShield;

			m_vActiveEffects.push_back(pEffect);
			m_pWindShieldEffect = pEffect;

			continue;
		}
		else if ((EFFECT_TYPE)typeIndex == EFFECT_TYPE::BOOSTER)
		{
			// 부스터
			CParticleSystem* pBoosterParticles = new CParticleSystem(pd3dDevice, pd3dCommandList, 50);

			ActiveEffect* pEffect = new ActiveEffect;
			pEffect->type = EFFECT_TYPE::BOOSTER;
			pEffect->pParticleSys = pBoosterParticles;
			pEffect->pMeshEffect = nullptr;
			pEffect->bActive = false;

			m_vActiveEffects.push_back(pEffect);
			m_pBoosterEffect = pEffect;
		}

		// 기타 파티클 생성
		int nPoolSize = 50;
		int nParticleCount = 3;

		// 흙먼지
		if ((EFFECT_TYPE)typeIndex == EFFECT_TYPE::DUST) {
			nPoolSize = 2000;
			nParticleCount = 1;
		}

		for (int i = 0; i < nPoolSize; ++i)
		{
			CParticleSystem* pSys = new CParticleSystem(pd3dDevice, pd3dCommandList, nParticleCount);

			ActiveEffect* pEffect = new ActiveEffect{
				(EFFECT_TYPE)typeIndex,
				false,
				0.0f,
				pSys,
				nullptr
			};
			m_vEffectPool[typeIndex].push_back(pEffect);
		}
	}
}

void CEffectLibrary::BuildRootSignature(ID3D12Device* pd3dDevice)
{
	CD3DX12_ROOT_PARAMETER rootParameters[3];

	CD3DX12_DESCRIPTOR_RANGE ranges[1];

	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsConstants(32, 6, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstants(20, 7, 0, D3D12_SHADER_VISIBILITY_ALL);

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
		if ((EFFECT_TYPE)i == EFFECT_TYPE::WIND_EFFECT)
		{
			currentCpuHandle.ptr += nDescriptorSize;
			continue;
		}

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

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
	if (!pd3dCommandList) return;
	if (m_vActiveEffects.empty() || m_pd3dSrvHeap == nullptr) return;
	if (m_pRootSignature == nullptr) return;

	pd3dCommandList->SetGraphicsRootSignature(m_pRootSignature);

	XMFLOAT4X4 tMats[2];
	XMStoreFloat4x4(&tMats[0], XMMatrixTranspose(XMLoadFloat4x4(&view)));
	XMStoreFloat4x4(&tMats[1], XMMatrixTranspose(XMLoadFloat4x4(&proj)));
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 32, tMats, 0);

	ID3D12DescriptorHeap* pParticleHeap[] = { m_pd3dSrvHeap };

	// [상태 추적용 변수] 현재 바인딩된 PSO가 무엇인지
	// 0: 없음, 1: Particle, 2: Mesh
	int currentPsoType = 0;

	for (auto eff : m_vActiveEffects)
	{
		if (eff->pParticleSys)
		{
			if (m_pPipelineState == nullptr) continue;
			// 파티클 PSO로 변경이 필요한 경우
			if (currentPsoType != 1)
			{
				pd3dCommandList->SetPipelineState(m_pPipelineState); // 파티클 PSO
				pd3dCommandList->SetDescriptorHeaps(1, pParticleHeap);
				currentPsoType = 1;
			}
			else
			{
				pd3dCommandList->SetDescriptorHeaps(1, pParticleHeap);
			}

			D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = m_d3dSrvGpuHandleStart;
			textureHandle.ptr += (UINT64)eff->type * m_nSrvDescriptorIncrementSize;
			pd3dCommandList->SetGraphicsRootDescriptorTable(0, textureHandle);

			eff->pParticleSys->Render(pd3dCommandList);
		}
		else if (eff->pMeshEffect)
		{
			if (m_pMeshEffectPSO == nullptr) continue;
			if (currentPsoType != 2)
			{
				pd3dCommandList->SetPipelineState(m_pMeshEffectPSO); // 바람저항효과 PSO
				currentPsoType = 2;
			}

			eff->pMeshEffect->Render(pd3dCommandList);
		}
	}
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
}

ActiveEffect* CEffectLibrary::Play(EFFECT_TYPE type, XMFLOAT3 position, XMFLOAT2 size, XMFLOAT3 color)
{
	if (m_vEffectPool[(int)type].empty()) return nullptr;

	ActiveEffect* pEffectData = m_vEffectPool[(int)type].back();
	m_vEffectPool[(int)type].pop_back();

	pEffectData->bActive = true;
	pEffectData->fAge = 0.0f;
	pEffectData->type = type;

	if (pEffectData->pParticleSys)
	{
		// 파티클 시스템
		pEffectData->pParticleSys->SetPosition(position);

		float fSpread = 0.0f; // 기본값, 흙먼지 파티클 퍼짐정도, 0.f일때 일자로 나오게 수정

		if (type >= EFFECT_TYPE::ITEM1 && type <= EFFECT_TYPE::ITEM9)
		{
			fSpread = 50.0f; // 아이템 획득 파티클 퍼짐정도
		}
		else if (type == EFFECT_TYPE::COLLISION)
		{
			fSpread = 20.0f; // 충돌 파티클 퍼짐정도
		}

		if (IsZero(fSpread))
		{
			m_bSpreadZero = false;
		}
		else
		{
			m_bSpreadZero = true;
		}

		pEffectData->pParticleSys->ResetParticles(size, fSpread, m_bSpreadZero, color);

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

void CEffectLibrary::Update(float fTimeElapsed)
{
	auto it = m_vActiveEffects.begin();

	while (it != m_vActiveEffects.end())
	{
		ActiveEffect* eff = *it;
		bool bIsDead = false;

		if (eff->pParticleSys)
		{
			if (m_pPipelineState == nullptr) continue;
			if (eff->type == EFFECT_TYPE::BOOSTER)
			{
				if (eff->bActive) eff->pParticleSys->BoosterAnimate(fTimeElapsed);
			}
			else if (eff->type == EFFECT_TYPE::DUST)
			{
				eff->pParticleSys->DustAnimate(fTimeElapsed, m_bSpreadZero);
			}
			else if (eff->type >= EFFECT_TYPE::ITEM1 && eff->type <= EFFECT_TYPE::ITEM9)
			{
				eff->pParticleSys->ItemAnimate(fTimeElapsed);
			}
			else // COLLISION1, 2, 3 등
			{
				eff->pParticleSys->CollisionAnimate(fTimeElapsed);
			}
		}

		if (eff->pMeshEffect && eff->pMeshEffect->IsActive())
		{
			eff->pMeshEffect->Update(fTimeElapsed);
		}

		if (eff->type != EFFECT_TYPE::BOOSTER && eff->type != EFFECT_TYPE::WIND_EFFECT)
		{
			eff->fAge += fTimeElapsed;

			if (eff->fAge > 2.0f)
			{
				bIsDead = true;
			}
		}

		if (bIsDead)
		{
			eff->bActive = false;

			m_vEffectPool[(int)eff->type].push_back(eff);

			it = m_vActiveEffects.erase(it);
		}
		else
		{
			++it;
		}
	}

	m_fSpeedLineAccumTime += fTimeElapsed;

	if (m_fSpeedLineAccumTime > 0.05f)
	{
		m_fSpeedLineAccumTime = 0.0f;
		m_fSpeedLineAngle = ((rand() % 360) * 3.141592f) / 180.0f;
		m_fSpeedLineScale = 0.9f + ((rand() % 20) / 100.0f);       
	}

	if (m_fCurrentPlayerSpeedRatio > 0.7f) {
		m_fSpeedLineAlpha = (m_fCurrentPlayerSpeedRatio - 0.7f) * 3.33f;
		if (m_fSpeedLineAlpha > 1.0f) m_fSpeedLineAlpha = 1.0f;
	}
	else {
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

	m_pRootSignature = nullptr;
	m_pPipelineState = nullptr;
	m_pMeshEffectPSO = nullptr;

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
		XMVECTOR vFrontPos = XMLoadFloat3(&pos) - (vLook * 150.0f); // 캡슐의 앞이 객체 바로 앞에 오게끔 vLook* n 수정
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

	if (m_pBoosterEffect && m_pBoosterEffect->pParticleSys)
	{
		XMVECTOR vRearPos = XMLoadFloat3(&pos) - (vLook * 12.0f);
		XMFLOAT3 fRearPos;
		XMStoreFloat3(&fRearPos, vRearPos);

		m_pBoosterEffect->pParticleSys->SetPosition(fRearPos);
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

	// 속도 계산 로직
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