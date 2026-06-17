//-----------------------------------------------------------------------------
// File: CScene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "WireframeBoxMesh.h"

std::random_device rd;
std::default_random_engine dre{ rd()};
std::uniform_int_distribution<int> uid(0, 180);
std::uniform_int_distribution<int> uid1(-2500, -1500);
std::uniform_int_distribution<int> uid2(1500, 2500);
std::uniform_int_distribution<int> uid3(-2500, 2500);
std::uniform_int_distribution<int> uid4(-1500, 1500);

CScene::CScene()
{
	m_d3dGpuTreeSrvHandle.ptr = 0;
	m_d3dGpuFlowerSrvHandle.ptr = 0;
	m_d3dGpuRockSrvHandle.ptr = 0;
}

CScene::~CScene()
{
	if (m_pd3dCbvSrvHeap) m_pd3dCbvSrvHeap->Release();
	if (m_pTreeTexture) m_pTreeTexture->Release();
	if (m_pFlowerTexture) m_pFlowerTexture->Release();
	if (m_pRockTexture) m_pRockTexture->Release();
	m_d3dDefaultSrvTableHandle.ptr = 0;
}

void CScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 4;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = POINT_LIGHT;
	m_pLights[0].m_fRange = 1000.0f;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.1f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(30.0f, 30.0f, 30.0f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pLights[0].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.001f, 0.0001f);
	
	m_pLights[1].m_bEnable = true;
	m_pLights[1].m_nType = SPOT_LIGHT;
	m_pLights[1].m_fRange = 500.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(-50.0f, 20.0f, -5.0f);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[1].m_fFalloff = 8.0f;
	m_pLights[1].m_fPhi = (float)cos(XMConvertToRadians(40.0f));
	m_pLights[1].m_fTheta = (float)cos(XMConvertToRadians(20.0f));
	
	m_pLights[2].m_bEnable = true;
	m_pLights[2].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f); // 0.3 -> 0.8 //  
	m_pLights[2].m_xmf4Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 0.0f);
	m_pLights[2].m_xmf3Direction = XMFLOAT3(1.0f, -1.0f, 0.5f); //   -> 7
	
	m_pLights[3].m_bEnable = true;
	m_pLights[3].m_nType = SPOT_LIGHT;
	m_pLights[3].m_fRange = 600.0f;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.3f, 0.7f, 0.0f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(50.0f, 30.0f, 30.0f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(0.0f, 1.0f, 1.0f);
	m_pLights[3].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[3].m_fFalloff = 8.0f;
	m_pLights[3].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	m_pLights[3].m_fTheta = (float)cos(XMConvertToRadians(30.0f));

}

void CScene::BuildObjectsGameStart(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	LoadTexture(pd3dDevice, pd3dCommandList);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}//

void CScene::BuildObjectsGameRoom(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
// 대기방
	
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();
	
	LoadTexture(pd3dDevice, pd3dCommandList);

	//m_nGameObjects = 1;
	//m_ppGameObjects = new CGameObject * [m_nGameObjects];
	//
	//for (int i = 0; i < 1; ++i) {
	//	CGameObject* pCarModel;
	//	CGameObject* pCarObject = new CGameObject();
	//	if (0 == i) {
	//		pCarModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/FINAL_MODEL_241.bin");
	//		//pCarModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/untitled.bin");
	//		pCarObject->m_bIsActive = true;
	//	}
	//	else if (1 == i) {
	//		//pCarModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/untitled.bin");
	//		//pCarObject->m_bIsActive = false;
	//	}
	//	else if (2 == i) {
	//		//pCarModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/untitled.bin");
	//		//pCarObject->m_bIsActive = false;
	//	}
	//	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCarModel);
	//	pCarObject->SetChild(pCarModel);
	//	pCarObject->SetPosition(120.0f, 0.0f, 100.0f);
	//	pCarObject->Rotate(0.0f, 180.0f, 0.0f);
	//	pCarObject->SetScale(100, 100, 100);
	//	m_ppGameObjects[i] = pCarObject;
	//}
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}//

void CScene::BuildObjectsGameEnd(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	LoadTexture(pd3dDevice, pd3dCommandList);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::ApplyMeshTextures(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pObject)
{
	if (!pObject) return;

	if (!m_pd3dCbvSrvHeap)
	{
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pObject->m_pChild);
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pObject->m_pSibling);
		return;
	}

	//   SRV (Heap Start)   :
	//  - t2(Albedo) : 1x1 White
	//  - t4(Shadow) : ( White, ShadowMap    SRV )
	//  - t5(Skybox) : Skybox   CubeMap SRV
	// =>    ""  (white texture )
	if (pObject->m_nMaterials > 0 && pObject->m_ppMaterials)
	{
		for (int i = 0; i < pObject->m_nMaterials; i++)
		{
			CMaterial* pMaterial = pObject->m_ppMaterials[i];
			if (!pMaterial) continue;

			// (illumination)   (t2) 
			if (pMaterial->m_pShader != CMaterial::m_pIlluminatedShader) continue;

			//  ( "null")    
			if (IsNullTextureName(pMaterial->m_pstrAlbedoTexture))
			{
				pMaterial->SetTexture(m_d3dDefaultSrvTableHandle);
				continue;
			}

			//     
			UINT tableStartIndex = 0;
			auto it = m_materialSrvTableStarts.find(pMaterial);
			if (it != m_materialSrvTableStarts.end())
			{
				tableStartIndex = it->second;
			}
			else
			{
				tableStartIndex = m_nNextSrvTableIndex;
				m_nNextSrvTableIndex += kSrvTableSize;

				//  (8 SRV)  default white 
				D3D12_CPU_DESCRIPTOR_HANDLE cpuHeapStart = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
				D3D12_CPU_DESCRIPTOR_HANDLE cpuDefaultWhite = cpuHeapStart;
				cpuDefaultWhite.ptr += (SIZE_T)m_nDescriptorIncrementSize * kDefaultWhiteSrvIndex;

				for (UINT s = 0; s < kSrvTableSize; ++s)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE dst = cpuHeapStart;
					dst.ptr += (SIZE_T)m_nDescriptorIncrementSize * (tableStartIndex + s);
					pd3dDevice->CopyDescriptorsSimple(1, dst, cpuDefaultWhite, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}

				// ShadowMap SRV(t4)    ,
				//   t4   .
				if (m_bShadowSrvReady)
				{
					D3D12_CPU_DESCRIPTOR_HANDLE cpuShadow = cpuHeapStart;
					cpuShadow.ptr += (SIZE_T)m_nDescriptorIncrementSize * kShadowMapSrvIndex;

					D3D12_CPU_DESCRIPTOR_HANDLE dstShadow = cpuHeapStart;
					dstShadow.ptr += (SIZE_T)m_nDescriptorIncrementSize * (tableStartIndex + kShadowMapSrvIndex);

					pd3dDevice->CopyDescriptorsSimple(1, dstShadow, cpuShadow, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
				else
				{
					m_srvTableStartsNeedingShadowUpdate.push_back(tableStartIndex);
				}

				m_materialSrvTableStarts.insert(std::make_pair(pMaterial, tableStartIndex));
			}

			//  " " GPU   (t0~t7 )
			D3D12_GPU_DESCRIPTOR_HANDLE gpuTableStart = m_pd3dCbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
			gpuTableStart.ptr += (UINT64)m_nDescriptorIncrementSize * tableStartIndex;
			pMaterial->SetTexture(gpuTableStart);

			//    ,  t2(slot +2) SRV 
			std::wstring wFile = L"Asset/DDS_File/";
			wFile += std::wstring(pMaterial->m_pstrAlbedoTexture,
				pMaterial->m_pstrAlbedoTexture + strlen(pMaterial->m_pstrAlbedoTexture));

			ID3D12Resource* pTex = nullptr;
			ID3D12Resource* pUpload = nullptr;
			std::unique_ptr<uint8_t[]> ddsData;
			std::vector<D3D12_SUBRESOURCE_DATA> subresources;

			HRESULT hr = DirectX::LoadDDSTextureFromFile(pd3dDevice, wFile.c_str(), &pTex, ddsData, subresources);
			if (FAILED(hr) || !pTex || subresources.empty())
			{
				//     => (white) 
				if (pTex) pTex->Release();
				pMaterial->SetTexture(m_d3dDefaultSrvTableHandle);
				continue;
			}

			UINT64 uploadSize = GetRequiredIntermediateSize(pTex, 0, (UINT)subresources.size());
			hr = pd3dDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pUpload));

			if (FAILED(hr) || !pUpload)
			{
				if (pTex) pTex->Release();
				pMaterial->SetTexture(m_d3dDefaultSrvTableHandle);
				continue;
			}

			UpdateSubresources(pd3dCommandList, pTex, pUpload, 0, 0, (UINT)subresources.size(), subresources.data());
			pd3dCommandList->ResourceBarrier(
				1,
				&CD3DX12_RESOURCE_BARRIER::Transition(
					pTex,
					D3D12_RESOURCE_STATE_COPY_DEST,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = pTex->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = pTex->GetDesc().MipLevels;

			D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
			cpuSrvHandle.ptr += (SIZE_T)m_nDescriptorIncrementSize * (tableStartIndex + kDefaultWhiteSrvIndex); // t2 

			pd3dDevice->CreateShaderResourceView(pTex, &srvDesc, cpuSrvHandle);

			//   
			m_vLoadedTextures.push_back(pTex);
			m_vLoadedTextureUploadBuffers.push_back(pUpload);
		}
	}

	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pObject->m_pChild);
	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pObject->m_pSibling);
}

void CScene::BuildGameObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
// 스테이지1
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	LoadTexture(pd3dDevice, pd3dCommandList);

	BuildUIResources(pd3dDevice, pd3dCommandList);

	// 
	m_nGameObjects = 35 + 1 + 6;
	m_ppGameObjects = new CGameObject * [m_nGameObjects];

	// 맵 모델링
	CGameObject* pGroundModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/untitled.bin");
	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pGroundModel);
	CGameObject* pGroundObject = new CGameObject();
	pGroundObject->SetChild(pGroundModel);
	pGroundObject->SetPosition(0.0f, -500.0f, 0.0f);
	pGroundObject->Rotate(0.0f, 0.0f, 0.0f);
	pGroundObject->SetScale(1, 1, 1);
	//pGroundObject->ComputeNewLocalAABB();
	pGroundObject->m_bIsGround = true;
	m_ppGameObjects[0] = pGroundObject;

	// 바닥 모델링
	CGameObject* pGroundModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/MapGround.bin");
	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pGroundModel1);
	CGameObject* pGroundObject1 = new CGameObject();
	pGroundObject1->SetChild(pGroundModel1);
	pGroundObject1->SetPosition(0.0f, 0.0f, 0.0f);
	pGroundObject1->Rotate(0.0f, 0.0f, 0.0f);
	pGroundObject1->SetScale(1, 1, 1);
	pGroundObject1->ComputeNewLocalAABB();
	pGroundObject1->m_bIsGround = true;
	m_ppGameObjects[1] = pGroundObject1;
	
	// 사이드 벽 모델링
	CGameObject* pSideWallModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/MapSideWall.bin");
	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pSideWallModel);
	CGameObject* pSideWallObject = new CGameObject();
	pSideWallObject->SetChild(pSideWallModel);
	pSideWallObject->SetPosition(0.0f, -550.0f, 0.0f);
	pSideWallObject->SetScale(1, 1, 1);
	pSideWallObject->m_bIsInvisibleWall = true;
	m_ppGameObjects[2] = pSideWallObject;

	// 체크포인트 모델링
	{
		CGameObject* pCPModel0 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP0.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel0);
		CGameObject* pCPObject0 = new CGameObject();
		pCPObject0->SetChild(pCPModel0);
		pCPObject0->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject0->m_bIsCheckPoint = true;
		pCPObject0->m_nCheckPointIndex = 10;
		m_ppGameObjects[3] = pCPObject0;

		CGameObject* pCPModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel1);
		CGameObject* pCPObject1 = new CGameObject();
		pCPObject1->SetChild(pCPModel1);
		pCPObject1->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject1->m_bIsCheckPoint = true;
		pCPObject1->m_nCheckPointIndex = 1;
		m_ppGameObjects[4] = pCPObject1;

		CGameObject* pCPModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP2.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel2);
		CGameObject* pCPObject2 = new CGameObject();
		pCPObject2->SetChild(pCPModel2);
		pCPObject2->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject2->m_bIsCheckPoint = true;
		pCPObject2->m_nCheckPointIndex = 2;
		m_ppGameObjects[5] = pCPObject2;

		CGameObject* pCPModel3 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP3.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel3);
		CGameObject* pCPObject3 = new CGameObject();
		pCPObject3->SetChild(pCPModel3);
		pCPObject3->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject3->m_bIsCheckPoint = true;
		pCPObject3->m_nCheckPointIndex = 3;
		m_ppGameObjects[6] = pCPObject3;

		CGameObject* pCPModel4 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP4.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel4);
		CGameObject* pCPObject4 = new CGameObject();
		pCPObject4->SetChild(pCPModel4);
		pCPObject4->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject4->m_bIsCheckPoint = true;
		pCPObject4->m_nCheckPointIndex = 4;
		m_ppGameObjects[7] = pCPObject4;

		CGameObject* pCPModel5 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP5.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel5);
		CGameObject* pCPObject5 = new CGameObject();
		pCPObject5->SetChild(pCPModel5);
		pCPObject5->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject5->m_bIsCheckPoint = true;
		pCPObject5->m_nCheckPointIndex = 5;
		m_ppGameObjects[8] = pCPObject5;

		CGameObject* pCPModel6 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP6.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel6);
		CGameObject* pCPObject6 = new CGameObject();
		pCPObject6->SetChild(pCPModel6);
		pCPObject6->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject6->m_bIsCheckPoint = true;
		pCPObject6->m_nCheckPointIndex = 6;
		m_ppGameObjects[9] = pCPObject6;

		CGameObject* pCPModel7 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP7.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel7);
		CGameObject* pCPObject7 = new CGameObject();
		pCPObject7->SetChild(pCPModel7);
		pCPObject7->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject7->m_bIsCheckPoint = true;
		pCPObject7->m_nCheckPointIndex = 7;
		m_ppGameObjects[10] = pCPObject7;

		CGameObject* pCPModel8 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP8.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel8);
		CGameObject* pCPObject8 = new CGameObject();
		pCPObject8->SetChild(pCPModel8);
		pCPObject8->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject8->m_bIsCheckPoint = true;
		pCPObject8->m_nCheckPointIndex = 8;
		m_ppGameObjects[11] = pCPObject8;

		CGameObject* pCPModel9 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CP9.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel9);
		CGameObject* pCPObject9 = new CGameObject();
		pCPObject9->SetChild(pCPModel9);
		pCPObject9->SetPosition(0.0f, -500.0f, 0.0f);
		pCPObject9->m_bIsCheckPoint = true;
		pCPObject9->m_nCheckPointIndex = 9;
		m_ppGameObjects[12] = pCPObject9;
	}
	//////////////////////////////////////////////////
	{

		CGameObject* pItemModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject = new CGameObject();
		pItemObject->SetChild(pItemModel);
		pItemObject->SetPosition(-2201, -210, 1890);
		pItemObject->SetScale(10, 10, 10);
		pItemObject->ComputeNewLocalAABB();
		pItemObject->m_bIsItemBox = true;
		pItemObject->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[13] = pItemObject;

		CGameObject* pItemModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject1 = new CGameObject();
		pItemObject1->SetChild(pItemModel1);
		pItemObject1->SetPosition(-500, -260, 2724);
		pItemObject1->SetScale(10, 10, 10);
		pItemObject1->ComputeNewLocalAABB();
		pItemObject1->m_bIsItemBox = true;
		pItemObject1->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[14] = pItemObject1;

		CGameObject* pItemModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject2 = new CGameObject();
		pItemObject2->SetChild(pItemModel2);
		pItemObject2->SetPosition(570, -190, -600);
		pItemObject2->SetScale(10, 10, 10);
		pItemObject2->ComputeNewLocalAABB();
		pItemObject2->m_bIsItemBox = true;
		pItemObject2->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[15] = pItemObject2;

		CGameObject* pItemModel3 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject3 = new CGameObject();
		pItemObject3->SetChild(pItemModel3);
		pItemObject3->SetPosition(-2000, -190, -80);
		pItemObject3->SetScale(10, 10, 10);
		pItemObject3->ComputeNewLocalAABB();
		pItemObject3->m_bIsItemBox = true;
		pItemObject3->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[16] = pItemObject3;


		CGameObject* pItemModel4 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject4 = new CGameObject();
		pItemObject4->SetChild(pItemModel4);
		pItemObject4->SetPosition(-2263, -212, 1978);
		pItemObject4->SetScale(10, 10, 10);
		pItemObject4->ComputeNewLocalAABB();
		pItemObject4->m_bIsItemBox = true;
		pItemObject4->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[17] = pItemObject4;

		CGameObject* pItemModel5 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject5 = new CGameObject();
		pItemObject5->SetChild(pItemModel5);
		pItemObject5->SetPosition(-2317, -212, 1954);
		pItemObject5->SetScale(10, 10, 10);
		pItemObject5->ComputeNewLocalAABB();
		pItemObject5->m_bIsItemBox = true;
		pItemObject5->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[18] = pItemObject5;

		CGameObject* pItemModel6 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject6 = new CGameObject();
		pItemObject6->SetChild(pItemModel6);
		pItemObject6->SetPosition(-2360, -212, 1982);
		pItemObject6->SetScale(10, 10, 10);
		pItemObject6->ComputeNewLocalAABB();
		pItemObject6->m_bIsItemBox = true;
		pItemObject6->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[19] = pItemObject6;

		CGameObject* pItemModel7 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject7 = new CGameObject();
		pItemObject7->SetChild(pItemModel7);
		pItemObject7->SetPosition(-2392, -212, 2003);
		pItemObject7->SetScale(10, 10, 10);
		pItemObject7->ComputeNewLocalAABB();
		pItemObject7->m_bIsItemBox = true;
		pItemObject7->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[20] = pItemObject7;

		CGameObject* pItemModel8 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject8 = new CGameObject();
		pItemObject8->SetChild(pItemModel8);
		pItemObject8->SetPosition(-2436, -212, 2032);
		pItemObject8->SetScale(10, 10, 10);
		pItemObject8->ComputeNewLocalAABB();
		pItemObject8->m_bIsItemBox = true;
		pItemObject8->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[21] = pItemObject8;
	
		CGameObject* pItemModel9 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject9 = new CGameObject();
		pItemObject9->SetChild(pItemModel9);
		pItemObject9->SetPosition(-503, -265, 2677);
		pItemObject9->SetScale(10, 10, 10);
		pItemObject9->ComputeNewLocalAABB();
		pItemObject9->m_bIsItemBox = true;
		pItemObject9->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[22] = pItemObject9;

		CGameObject* pItemModel10 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject10 = new CGameObject();
		pItemObject10->SetChild(pItemModel10);
		pItemObject10->SetPosition(-568, -274,2619);
		pItemObject10->SetScale(10, 10, 10);
		pItemObject10->ComputeNewLocalAABB();
		pItemObject10->m_bIsItemBox = true;
		pItemObject10->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[23] = pItemObject10;

		CGameObject* pItemModel11 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject11 = new CGameObject();
		pItemObject11->SetChild(pItemModel11);
		pItemObject11->SetPosition(-604,-278,2577);
		pItemObject11->SetScale(10, 10, 10);
		pItemObject11->ComputeNewLocalAABB();
		pItemObject11->m_bIsItemBox = true;
		pItemObject11->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[24] = pItemObject11;

		CGameObject* pItemModel12 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject12 = new CGameObject();
		pItemObject12->SetChild(pItemModel12);
		pItemObject12->SetPosition(-440,-260,2786);
		pItemObject12->SetScale(10, 10, 10);
		pItemObject12->ComputeNewLocalAABB();
		pItemObject12->m_bIsItemBox = true;
		pItemObject12->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[25] = pItemObject12;

		CGameObject* pItemModel13 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject13 = new CGameObject();
		pItemObject13->SetChild(pItemModel13);
		pItemObject13->SetPosition(-396, -240,2826);
		pItemObject13->SetScale(10, 10, 10);
		pItemObject13->ComputeNewLocalAABB();
		pItemObject13->m_bIsItemBox = true;
		pItemObject13->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[26] = pItemObject13;
	
		CGameObject* pItemModel14 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject14 = new CGameObject();
		pItemObject14->SetChild(pItemModel14);
		pItemObject14->SetPosition(523,-193,-616);
		pItemObject14->SetScale(10, 10, 10);
		pItemObject14->ComputeNewLocalAABB();
		pItemObject14->m_bIsItemBox = true;
		pItemObject14->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[27] = pItemObject14;

		CGameObject* pItemModel15 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject15 = new CGameObject();
		pItemObject15->SetChild(pItemModel15);
		pItemObject15->SetPosition(450,-194,-600);
		pItemObject15->SetScale(10, 10, 10);
		pItemObject15->ComputeNewLocalAABB();
		pItemObject15->m_bIsItemBox = true;
		pItemObject15->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[28] = pItemObject15;

		CGameObject* pItemModel16 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject16 = new CGameObject();
		pItemObject16->SetChild(pItemModel16);
		pItemObject16->SetPosition(658,-192,-627);
		pItemObject16->SetScale(10, 10, 10);
		pItemObject16->ComputeNewLocalAABB();
		pItemObject16->m_bIsItemBox = true;
		pItemObject16->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[29] = pItemObject16;

		CGameObject* pItemModel17 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject17 = new CGameObject();
		pItemObject17->SetChild(pItemModel17);
		pItemObject17->SetPosition(754,-191,-640);
		pItemObject17->SetScale(10, 10, 10);
		pItemObject17->ComputeNewLocalAABB();
		pItemObject17->m_bIsItemBox = true;
		pItemObject17->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[30] = pItemObject17;

		CGameObject* pItemModel18 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject18 = new CGameObject();
		pItemObject18->SetChild(pItemModel18);
		pItemObject18->SetPosition(-1932,-192,-54);
		pItemObject18->SetScale(10, 10, 10);
		pItemObject18->ComputeNewLocalAABB();
		pItemObject18->m_bIsItemBox = true;
		pItemObject18->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[31] = pItemObject18;

		CGameObject* pItemModel19 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject19 = new CGameObject();
		pItemObject19->SetChild(pItemModel19);
		pItemObject19->SetPosition(-1844,-192,-97);
		pItemObject19->SetScale(10, 10, 10);
		pItemObject19->ComputeNewLocalAABB();
		pItemObject19->m_bIsItemBox = true;
		pItemObject19->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[32] = pItemObject19;

		CGameObject* pItemModel20 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject20 = new CGameObject();
		pItemObject20->SetChild(pItemModel20);
		pItemObject20->SetPosition(-2084,-192,-60);
		pItemObject20->SetScale(10, 10, 10);
		pItemObject20->ComputeNewLocalAABB();
		pItemObject20->m_bIsItemBox = true;
		pItemObject20->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[33] = pItemObject20;

		CGameObject* pItemModel21 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject21 = new CGameObject();
		pItemObject21->SetChild(pItemModel21);
		pItemObject21->SetPosition(-2155,-192,-80);
		pItemObject21->SetScale(10, 10, 10);
		pItemObject21->ComputeNewLocalAABB();
		pItemObject21->m_bIsItemBox = true;
		pItemObject21->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[34] = pItemObject21;
	}

	CFlagObject* pPlayerPlane = new CFlagObject();
	pPlayerPlane->SetFPS(9.0f);
	CGameObject* pPlaneRoot = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Flag_Sequence.bin");
	pPlayerPlane->SetChild(pPlaneRoot);
	pPlayerPlane->OnInitialize(); 
	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pPlaneRoot);
	pPlayerPlane->SetScale(1, 1, 1);
	pPlayerPlane->Rotate(0, -90, 0);
	pPlayerPlane->SetPosition(-1700.0f, -200.0f, 360.0f);
	m_ppGameObjects[35] = pPlayerPlane;

	{
		CGameObject* pRLModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map1RedLight1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel1);
		CGameObject* pRLObject1 = new CGameObject();
		pRLObject1->SetChild(pRLModel1);
		pRLObject1->SetPosition(-2038.0f, -150.0f, 247.0f);
		pRLObject1->SetScale(10, 10, 10);
		m_ppGameObjects[36] = pRLObject1;

		CGameObject* pRLModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map1RedLight1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel2);
		CGameObject* pRLObject2 = new CGameObject();
		pRLObject2->SetChild(pRLModel2);
		pRLObject2->SetPosition(-2022+1.5f, -150.0f, 247.0f);
		pRLObject2->SetScale(10, 10, 10);
		pRLObject2->m_bIsActive = false;
		pRLObject2->m_bCanRespawn = true;
		pRLObject2->m_fInactiveTime = 0.0f;
		pRLObject2->m_fRespawnDelay = 2.0f;
		m_ppGameObjects[37] = pRLObject2;

		CGameObject* pRLModel3 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map1RedLight1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel3);
		CGameObject* pRLObject3 = new CGameObject();
		pRLObject3->SetChild(pRLModel3);
		pRLObject3->SetPosition(-2006+3.0f, -150.0f, 247.0f);
		pRLObject3->SetScale(10, 10, 10);
		pRLObject3->m_bIsActive = false;
		pRLObject3->m_bCanRespawn = true;
		pRLObject3->m_fInactiveTime = 0.0f;
		pRLObject3->m_fRespawnDelay = 4.0f;
		m_ppGameObjects[38] = pRLObject3;

		CGameObject* pRLModel4 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map1RedLight1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel4);
		CGameObject* pRLObject4 = new CGameObject();
		pRLObject4->SetChild(pRLModel4);
		pRLObject4->SetPosition(-1990+4.5f, -150.0f, 247.0f);
		pRLObject4->SetScale(10, 10, 10);
		pRLObject4->m_bIsActive = false;
		pRLObject4->m_bCanRespawn = true;
		pRLObject4->m_fInactiveTime = 0.0f;
		pRLObject4->m_fRespawnDelay = 6.0f;
		m_ppGameObjects[39] = pRLObject4;

		CGameObject* pRLModel5 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map1RedLight1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel5);
		CGameObject* pRLObject5 = new CGameObject();
		pRLObject5->SetChild(pRLModel5);
		pRLObject5->SetPosition(-1974+6.0f, -150.0f, 247.0f);
		pRLObject5->SetScale(10, 10, 10);
		pRLObject5->m_bIsActive = false;
		pRLObject5->m_bCanRespawn = true;
		pRLObject5->m_fInactiveTime = 0.0f;
		pRLObject5->m_fRespawnDelay = 8.0f;
		m_ppGameObjects[40] = pRLObject5;

		CGameObject* pRLModel6 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map1RedLight1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel6);
		CGameObject* pRLObject6 = new CGameObject();
		pRLObject6->SetChild(pRLModel6);
		pRLObject6->SetPosition(-1958+7.5f, -150.0f, 247.0f);
		pRLObject6->SetScale(10, 10, 10);
		pRLObject6->m_bIsActive = false;
		pRLObject6->m_bCanRespawn = true;
		pRLObject6->m_fInactiveTime = 0.0f;
		pRLObject6->m_fRespawnDelay = 10.0f;
		m_ppGameObjects[41] = pRLObject6;
	}

	CreateWireFrameBox(pd3dDevice, pd3dCommandList);
	CreateAABBWireFrameBox(pd3dDevice, pd3dCommandList);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CreateSkybox(pd3dDevice, pd3dCommandList);

	m_pShadowShader = new CShadowShader();
	m_pShadowShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
}

void CScene::BuildGameStage2(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	LoadTexture(pd3dDevice, pd3dCommandList);

	BuildUIResources(pd3dDevice, pd3dCommandList);

	// 
	m_nGameObjects = 1 + 1 + 10 + 4 + 21 + 6;
	m_ppGameObjects = new CGameObject * [m_nGameObjects];

	CGameObject* pMapModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/FORTR.bin");
	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pMapModel);
	CGameObject* pMapObject = new CGameObject();
	pMapObject->SetChild(pMapModel);
	pMapObject->SetPosition(0.0f, -2500.0f, 0.0f);
	pMapObject->SetScale(8, 8, 8);
	pMapObject->m_bIsGround = true;
	m_ppGameObjects[0] = pMapObject;

	// 사이드 벽 모델링
	CGameObject* pSideWallModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/SideWall.bin");
	ApplyMeshTextures(pd3dDevice, pd3dCommandList, pSideWallModel);
	CGameObject* pSideWallObject = new CGameObject();
	pSideWallObject->SetChild(pSideWallModel);
	pSideWallObject->SetPosition(0.0f, -2500.0f, 0.0f);
	pSideWallObject->SetScale(8, 8, 8);
	pSideWallObject->m_bIsInvisibleWall = true;
	m_ppGameObjects[1] = pSideWallObject;

	{
		CGameObject* pCPModel0 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel0);
		CGameObject* pCPObject0 = new CGameObject();
		pCPObject0->SetChild(pCPModel0);
		pCPObject0->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject0->SetScale(8, 8, 8);
		pCPObject0->m_bIsCheckPoint = true;
		pCPObject0->m_nCheckPointIndex = 10;
		m_ppGameObjects[2] = pCPObject0;

		CGameObject* pCPModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP2.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel1);
		CGameObject* pCPObject1 = new CGameObject();
		pCPObject1->SetChild(pCPModel1);
		pCPObject1->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject1->SetScale(8, 8, 8);
		pCPObject1->m_bIsCheckPoint = true;
		pCPObject1->m_nCheckPointIndex = 1;
		m_ppGameObjects[3] = pCPObject1;

		CGameObject* pCPModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP3.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel2);
		CGameObject* pCPObject2 = new CGameObject();
		pCPObject2->SetChild(pCPModel2);
		pCPObject2->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject2->SetScale(8, 8, 8);
		pCPObject2->m_bIsCheckPoint = true;
		pCPObject2->m_nCheckPointIndex = 2;
		m_ppGameObjects[4] = pCPObject2;

		CGameObject* pCPModel3 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP4.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel3);
		CGameObject* pCPObject3 = new CGameObject();
		pCPObject3->SetChild(pCPModel3);
		pCPObject3->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject3->SetScale(8, 8, 8);
		pCPObject3->m_bIsCheckPoint = true;
		pCPObject3->m_nCheckPointIndex = 3;
		m_ppGameObjects[5] = pCPObject3;

		CGameObject* pCPModel4 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP5.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel4);
		CGameObject* pCPObject4 = new CGameObject();
		pCPObject4->SetChild(pCPModel4);
		pCPObject4->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject4->SetScale(8, 8, 8);
		pCPObject4->m_bIsCheckPoint = true;
		pCPObject4->m_nCheckPointIndex = 4;
		m_ppGameObjects[6] = pCPObject4;

		CGameObject* pCPModel5 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP6.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel5);
		CGameObject* pCPObject5 = new CGameObject();
		pCPObject5->SetChild(pCPModel5);
		pCPObject5->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject5->SetScale(8, 8, 8);
		pCPObject5->m_bIsCheckPoint = true;
		pCPObject5->m_nCheckPointIndex = 5;
		m_ppGameObjects[7] = pCPObject5;

		CGameObject* pCPModel6 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP7.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel6);
		CGameObject* pCPObject6 = new CGameObject();
		pCPObject6->SetChild(pCPModel6);
		pCPObject6->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject6->SetScale(8, 8, 8);
		pCPObject6->m_bIsCheckPoint = true;
		pCPObject6->m_nCheckPointIndex = 6;
		m_ppGameObjects[8] = pCPObject6;

		CGameObject* pCPModel7 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP8.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel7);
		CGameObject* pCPObject7 = new CGameObject();
		pCPObject7->SetChild(pCPModel7);
		pCPObject7->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject7->SetScale(8, 8, 8);
		pCPObject7->m_bIsCheckPoint = true;
		pCPObject7->m_nCheckPointIndex = 7;
		m_ppGameObjects[9] = pCPObject7;

		CGameObject* pCPModel8 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP9.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel8);
		CGameObject* pCPObject8 = new CGameObject();
		pCPObject8->SetChild(pCPModel8);
		pCPObject8->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject8->SetScale(8, 8, 8);
		pCPObject8->m_bIsCheckPoint = true;
		pCPObject8->m_nCheckPointIndex = 8;
		m_ppGameObjects[10] = pCPObject8;

		CGameObject* pCPModel9 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Map2CP10.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCPModel9);
		CGameObject* pCPObject9 = new CGameObject();
		pCPObject9->SetChild(pCPModel9);
		pCPObject9->SetPosition(0.0f, -2500.0f, 0.0f);
		pCPObject9->SetScale(8, 8, 8);
		pCPObject9->m_bIsCheckPoint = true;
		pCPObject9->m_nCheckPointIndex = 9;
		m_ppGameObjects[11] = pCPObject9;
	}

	{
		CGameObject* pRCPModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RCP1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRCPModel1);
		CGameObject* pRCPObject1 = new CGameObject();
		pRCPObject1->SetChild(pRCPModel1);
		pRCPObject1->SetPosition(0.0f, -2500.0f, 0.0f);
		pRCPObject1->SetScale(8, 8, 8);
		pRCPObject1->m_bIsRCP = true;
		pRCPObject1->m_nCheckPointIndex = 1;
		m_ppGameObjects[12] = pRCPObject1;

		CGameObject* pRCPModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RCP2.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRCPModel2);
		CGameObject* pRCPObject2 = new CGameObject();
		pRCPObject2->SetChild(pRCPModel2);
		pRCPObject2->SetPosition(0.0f, -2500.0f, 0.0f);
		pRCPObject2->SetScale(8, 8, 8);
		pRCPObject2->m_bIsRCP = true;
		pRCPObject2->m_nCheckPointIndex = 2;
		m_ppGameObjects[13] = pRCPObject2;

		CGameObject* pRCPModel3 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RCP3.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRCPModel3);
		CGameObject* pRCPObject3 = new CGameObject();
		pRCPObject3->SetChild(pRCPModel3);
		pRCPObject3->SetPosition(0.0f, -2500.0f, 0.0f);
		pRCPObject3->SetScale(8, 8, 8);
		pRCPObject3->m_bIsRCP = true;
		pRCPObject3->m_nCheckPointIndex = 3;
		m_ppGameObjects[14] = pRCPObject3;

		CGameObject* pRCPModel4 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RCP4.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRCPModel4);
		CGameObject* pRCPObject4 = new CGameObject();
		pRCPObject4->SetChild(pRCPModel4);
		pRCPObject4->SetPosition(0.0f, -2500.0f, 0.0f);
		pRCPObject4->SetScale(8, 8, 8);
		pRCPObject4->m_bIsRCP = true;
		pRCPObject4->m_nCheckPointIndex = 4;
		m_ppGameObjects[15] = pRCPObject4;
	}

	{
		int y = 20;
		CGameObject* pItemModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject1 = new CGameObject();
		pItemObject1->SetChild(pItemModel1);
		pItemObject1->SetPosition(-7749, -2259+y, 6891);
		pItemObject1->SetScale(10, 10, 10);
		pItemObject1->ComputeNewLocalAABB();
		pItemObject1->m_bIsItemBox = true;
		pItemObject1->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[16] = pItemObject1;

		CGameObject* pItemModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject2 = new CGameObject();
		pItemObject2->SetChild(pItemModel2);
		pItemObject2->SetPosition(-7843.63, -2259 + y, 6891);
		pItemObject2->SetScale(10, 10, 10);
		pItemObject2->ComputeNewLocalAABB();
		pItemObject2->m_bIsItemBox = true;
		pItemObject2->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[17] = pItemObject2;

		CGameObject* pItemModel3 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject3 = new CGameObject();
		pItemObject3->SetChild(pItemModel3);
		pItemObject3->SetPosition(-7906, -2259 + y, 6891);
		pItemObject3->SetScale(10, 10, 10);
		pItemObject3->ComputeNewLocalAABB();
		pItemObject3->m_bIsItemBox = true;
		pItemObject3->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[18] = pItemObject3;

		CGameObject* pItemModel4 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject4 = new CGameObject();
		pItemObject4->SetChild(pItemModel4);
		pItemObject4->SetPosition(-6018, -2272 + y, 6423);
		pItemObject4->SetScale(10, 10, 10);
		pItemObject4->ComputeNewLocalAABB();
		pItemObject4->m_bIsItemBox = true;
		pItemObject4->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[19] = pItemObject4;

		CGameObject* pItemModel5 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject5 = new CGameObject();
		pItemObject5->SetChild(pItemModel5);
		pItemObject5->SetPosition(-6018, -2275 + y, 6361);
		pItemObject5->SetScale(10, 10, 10);
		pItemObject5->ComputeNewLocalAABB();
		pItemObject5->m_bIsItemBox = true;
		pItemObject5->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[20] = pItemObject5;

		CGameObject* pItemModel6 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject6 = new CGameObject();
		pItemObject6->SetChild(pItemModel6);
		pItemObject6->SetPosition(-6018.00, -2272 + y, 6295);
		pItemObject6->SetScale(10, 10, 10);
		pItemObject6->ComputeNewLocalAABB();
		pItemObject6->m_bIsItemBox = true;
		pItemObject6->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[21] = pItemObject6;

		CGameObject* pItemModel7 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject7 = new CGameObject();
		pItemObject7->SetChild(pItemModel7);
		pItemObject7->SetPosition(-6018, -2274 + y, 6231);
		pItemObject7->SetScale(10, 10, 10);
		pItemObject7->ComputeNewLocalAABB();
		pItemObject7->m_bIsItemBox = true;
		pItemObject7->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[22] = pItemObject7;

		//////////////////////////////////////////////////////////////////
		CGameObject* pItemModel8 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject8 = new CGameObject();
		pItemObject8->SetChild(pItemModel8);
		pItemObject8->SetPosition(-5650.00, -2248 + y, 5141);
		pItemObject8->SetScale(10, 10, 10);
		pItemObject8->ComputeNewLocalAABB();
		pItemObject8->m_bIsItemBox = true;
		pItemObject8->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[23] = pItemObject8;

		CGameObject* pItemModel9 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject9 = new CGameObject();
		pItemObject9->SetChild(pItemModel9);
		pItemObject9->SetPosition(-5586, -2253 + y, 5180);
		pItemObject9->SetScale(10, 10, 10);
		pItemObject9->ComputeNewLocalAABB();
		pItemObject9->m_bIsItemBox = true;
		pItemObject9->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[24] = pItemObject9;

		CGameObject* pItemModel10 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject10 = new CGameObject();
		pItemObject10->SetChild(pItemModel10);
		pItemObject10->SetPosition(-5516, -2256 + y, 5215);
		pItemObject10->SetScale(10, 10, 10);
		pItemObject10->ComputeNewLocalAABB();
		pItemObject10->m_bIsItemBox = true;
		pItemObject10->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[25] = pItemObject10;

		CGameObject* pItemModel11 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject11 = new CGameObject();
		pItemObject11->SetChild(pItemModel11);
		pItemObject11->SetPosition(-5438.67, -2257 + y, 5215);
		pItemObject11->SetScale(10, 10, 10);
		pItemObject11->ComputeNewLocalAABB();
		pItemObject11->m_bIsItemBox = true;
		pItemObject11->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[26] = pItemObject11;
		//////////////////////////////////////////////////////////////
		CGameObject* pItemModel12 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject12 = new CGameObject();
		pItemObject12->SetChild(pItemModel12);
		pItemObject12->SetPosition(-6257.89, -2135 + y, 3550);
		pItemObject12->SetScale(10, 10, 10);
		pItemObject12->ComputeNewLocalAABB();
		pItemObject12->m_bIsItemBox = true;
		pItemObject12->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[27] = pItemObject12;

		CGameObject* pItemModel13 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject13 = new CGameObject();
		pItemObject13->SetChild(pItemModel13);
		pItemObject13->SetPosition(-6257.89, -2133 + y, 3460);
		pItemObject13->SetScale(10, 10, 10);
		pItemObject13->ComputeNewLocalAABB();
		pItemObject13->m_bIsItemBox = true;
		pItemObject13->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[28] = pItemObject13;

		CGameObject* pItemModel14 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject14 = new CGameObject();
		pItemObject14->SetChild(pItemModel14);
		pItemObject14->SetPosition(-6257, -2133 + y, 3393);
		pItemObject14->SetScale(10, 10, 10);
		pItemObject14->ComputeNewLocalAABB();
		pItemObject14->m_bIsItemBox = true;
		pItemObject14->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[29] = pItemObject14;

		CGameObject* pItemModel15 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject15 = new CGameObject();
		pItemObject15->SetChild(pItemModel15);
		pItemObject15->SetPosition(-6258.98, -2132 + y, 3351);
		pItemObject15->SetScale(10, 10, 10);
		pItemObject15->ComputeNewLocalAABB();
		pItemObject15->m_bIsItemBox = true;
		pItemObject15->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[30] = pItemObject15;

		CGameObject* pItemModel16 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject16 = new CGameObject();
		pItemObject16->SetChild(pItemModel16);
		pItemObject16->SetPosition(-7377, -2200 + y, 3425);
		pItemObject16->SetScale(10, 10, 10);
		pItemObject16->ComputeNewLocalAABB();
		pItemObject16->m_bIsItemBox = true;
		pItemObject16->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[31] = pItemObject16;

		CGameObject* pItemModel17 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject17 = new CGameObject();
		pItemObject17->SetChild(pItemModel17);
		pItemObject17->SetPosition(-7166.54, -2206 + y, 4043);
		pItemObject17->SetScale(10, 10, 10);
		pItemObject17->ComputeNewLocalAABB();
		pItemObject17->m_bIsItemBox = true;
		pItemObject17->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[32] = pItemObject17;

		CGameObject* pItemModel18 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject18 = new CGameObject();
		pItemObject18->SetChild(pItemModel18);
		pItemObject18->SetPosition(-7610.63, -2246 + y, 4944);
		pItemObject18->SetScale(10, 10, 10);
		pItemObject18->ComputeNewLocalAABB();
		pItemObject18->m_bIsItemBox = true;
		pItemObject18->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[33] = pItemObject18;

		CGameObject* pItemModel19 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject19 = new CGameObject();
		pItemObject19->SetChild(pItemModel19);
		pItemObject19->SetPosition(-7531.58, -2247 + y, 4944);
		pItemObject19->SetScale(10, 10, 10);
		pItemObject19->ComputeNewLocalAABB();
		pItemObject19->m_bIsItemBox = true;
		pItemObject19->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[34] = pItemObject19;

		CGameObject* pItemModel20 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject20 = new CGameObject();
		pItemObject20->SetChild(pItemModel20);
		pItemObject20->SetPosition(-7401.33, -2248 + y, 4945);
		pItemObject20->SetScale(10, 10, 10);
		pItemObject20->ComputeNewLocalAABB();
		pItemObject20->m_bIsItemBox = true;
		pItemObject20->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[35] = pItemObject20;

		CGameObject* pItemModel21 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Item_box.bin");
		CGameObject* pItemObject21 = new CGameObject();
		pItemObject21->SetChild(pItemModel21);
		pItemObject21->SetPosition(-7348.50, -2249 + y, 4945);
		pItemObject21->SetScale(10, 10, 10);
		pItemObject21->ComputeNewLocalAABB();
		pItemObject21->m_bIsItemBox = true;
		pItemObject21->m_fRespawnDelay = 3.0f;
		m_ppGameObjects[36] = pItemObject21;
	}

	//CCrabObject* pCrabObject = new CCrabObject();
	//pCrabObject->SetFPS(20.0f);
	//CGameObject* pCrabModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Crab.bin");
	//pCrabObject->SetChild(pCrabModel);
	//pCrabObject->OnInitialize();
	////ApplyMeshTextures(pd3dDevice, pd3dCommandList, pCrabModel);
	//pCrabObject->SetScale(100, 100, 100);
	//pCrabObject->Rotate(0, 0, 0);
	//pCrabObject->SetPosition(-7749, -2259+100 , 6891);
	//m_ppGameObjects[37] = pCrabObject;

	{
		CGameObject* pRLModel1 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RedLight1.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel1);
		CGameObject* pRLObject1 = new CGameObject();
		pRLObject1->SetChild(pRLModel1);
		pRLObject1->SetPosition(0.0f, -2500.0f, 0.0f);
		pRLObject1->SetScale(8, 8, 8);
		m_ppGameObjects[37] = pRLObject1;

		CGameObject* pRLModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RedLight2.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel2);
		CGameObject* pRLObject2 = new CGameObject();
		pRLObject2->SetChild(pRLModel2);
		pRLObject2->SetPosition(0.0f, -2500.0f, 0.0f);
		pRLObject2->SetScale(8, 8, 8);
		pRLObject2->m_bIsActive = false;
		pRLObject2->m_bCanRespawn = true;
		pRLObject2->m_fInactiveTime = 0.0f;
		pRLObject2->m_fRespawnDelay = 2.0f;
		m_ppGameObjects[38] = pRLObject2;

		CGameObject* pRLModel3 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RedLight3.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel3);
		CGameObject* pRLObject3 = new CGameObject();
		pRLObject3->SetChild(pRLModel3);
		pRLObject3->SetPosition(0.0f, -2500.0f, 0.0f);
		pRLObject3->SetScale(8, 8, 8);
		pRLObject3->m_bIsActive = false;
		pRLObject3->m_bCanRespawn = true;
		pRLObject3->m_fInactiveTime = 0.0f;
		pRLObject3->m_fRespawnDelay = 4.0f;
		m_ppGameObjects[39] = pRLObject3;

		CGameObject* pRLModel4 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RedLight4.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel4);
		CGameObject* pRLObject4 = new CGameObject();
		pRLObject4->SetChild(pRLModel4);
		pRLObject4->SetPosition(0.0f, -2500.0f, 0.0f);
		pRLObject4->SetScale(8, 8, 8);
		pRLObject4->m_bIsActive = false;
		pRLObject4->m_bCanRespawn = true;
		pRLObject4->m_fInactiveTime = 0.0f;
		pRLObject4->m_fRespawnDelay = 6.0f;
		m_ppGameObjects[40] = pRLObject4;

		CGameObject* pRLModel5 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RedLight5.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel5);
		CGameObject* pRLObject5 = new CGameObject();
		pRLObject5->SetChild(pRLModel5);
		pRLObject5->SetPosition(0.0f, -2500.0f, 0.0f);
		pRLObject5->SetScale(8, 8, 8);
		pRLObject5->m_bIsActive = false;
		pRLObject5->m_bCanRespawn = true;
		pRLObject5->m_fInactiveTime = 0.0f;
		pRLObject5->m_fRespawnDelay = 8.0f;
		m_ppGameObjects[41] = pRLObject5;

		CGameObject* pRLModel6 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/RedLight6.bin");
		ApplyMeshTextures(pd3dDevice, pd3dCommandList, pRLModel6);
		CGameObject* pRLObject6 = new CGameObject();
		pRLObject6->SetChild(pRLModel6);
		pRLObject6->SetPosition(0.0f, -2500.0f, 0.0f);
		pRLObject6->SetScale(8, 8, 8);
		pRLObject6->m_bIsActive = false;
		pRLObject6->m_bCanRespawn = true;
		pRLObject6->m_fInactiveTime = 0.0f;
		pRLObject6->m_fRespawnDelay = 10.0f;
		m_ppGameObjects[42] = pRLObject6;
	}

	CreateWireFrameBox(pd3dDevice, pd3dCommandList);
	CreateAABBWireFrameBox(pd3dDevice, pd3dCommandList);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CreateSkybox(pd3dDevice, pd3dCommandList);

	m_pShadowShader = new CShadowShader();
	m_pShadowShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

}

void CScene::CreateShadowMapSRV(ID3D12Device* pd3dDevice, ID3D12Resource* pShadowMapResource)
{
	if (!m_pd3dCbvSrvHeap) return;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dSrvCpuHandle = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();

	UINT nDescriptorSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	d3dSrvCpuHandle.ptr += (nDescriptorSize * kShadowMapSrvIndex); // t4

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	pd3dDevice->CreateShaderResourceView(pShadowMapResource, &srvDesc, d3dSrvCpuHandle);

	//  Shadow SRV :    t4   ShadowMap 
	m_bShadowSrvReady = true;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHeapStart = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE cpuShadowSrc = cpuHeapStart;
	cpuShadowSrc.ptr += (SIZE_T)m_nDescriptorIncrementSize * kShadowMapSrvIndex;

	for (UINT tableStartIndex : m_srvTableStartsNeedingShadowUpdate)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuShadowDst = cpuHeapStart;
		cpuShadowDst.ptr += (SIZE_T)m_nDescriptorIncrementSize * (tableStartIndex + kShadowMapSrvIndex);

		pd3dDevice->CopyDescriptorsSimple(1, cpuShadowDst, cpuShadowSrc, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	m_srvTableStartsNeedingShadowUpdate.clear();
}

void CScene::CreateWireFrameBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CWireframeBoxMesh* pDebugBoxMesh = new CWireframeBoxMesh(pd3dDevice, pd3dCommandList);

	CMaterial* pDebugMaterial = new CMaterial();

	CMaterialColors* pDebugColors = new CMaterialColors();
	pDebugColors->m_xmf4Diffuse = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	pDebugColors->m_xmf4Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	pDebugColors->m_xmf4Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	pDebugMaterial->SetMaterialColors(pDebugColors);

	pDebugMaterial->SetShader(CMaterial::m_pDiffusedShader);

	m_pWireframeBoxObject = new CGameObject();
	m_pWireframeBoxObject->SetMesh(pDebugBoxMesh);
	m_pWireframeBoxObject->m_nMaterials = 1;
	m_pWireframeBoxObject->m_ppMaterials = new CMaterial * [m_pWireframeBoxObject->m_nMaterials];
	m_pWireframeBoxObject->m_ppMaterials[0] = NULL;
	m_pWireframeBoxObject->SetMaterial(0, pDebugMaterial);
	m_pWireframeBoxObject->SetScale(0.0f, 0.0f, 0.0f);
	m_pWireframeBoxObject->UpdateTransform(NULL);
}

void CScene::CreateAABBWireFrameBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CWireframeBoxMesh* pRedBoxMesh = new CWireframeBoxMesh(pd3dDevice, pd3dCommandList);

	CMaterial* pRedMaterial = new CMaterial();
	CMaterialColors* pRedColors = new CMaterialColors();
	pRedColors->m_xmf4Diffuse = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	pRedColors->m_xmf4Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	pRedColors->m_xmf4Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	pRedMaterial->SetMaterialColors(pRedColors);
	pRedMaterial->SetShader(CMaterial::m_pDiffusedShader); //

	m_pCombinedAABBBoxObject = new CGameObject();
	m_pCombinedAABBBoxObject->SetMesh(pRedBoxMesh);
	m_pCombinedAABBBoxObject->m_nMaterials = 1;
	m_pCombinedAABBBoxObject->m_ppMaterials = new CMaterial * [1];
	m_pCombinedAABBBoxObject->m_ppMaterials[0] = NULL;
	m_pCombinedAABBBoxObject->SetMaterial(0, pRedMaterial);

	m_pCombinedAABBBoxObject->SetScale(0.0f, 0.0f, 0.0f);
	m_pCombinedAABBBoxObject->UpdateTransform(NULL);
}

void CScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	if (m_ppGameObjects)
	{
		for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Release();
		delete[] m_ppGameObjects;
	}

	if (m_pWireframeBoxObject) m_pWireframeBoxObject->Release();
	if (m_pCombinedAABBBoxObject) m_pCombinedAABBBoxObject->Release();

	if (m_pTreeTexture) m_pTreeTexture->Release();
	if (m_pTreeTextureUploadBuffer) m_pTreeTextureUploadBuffer->Release();

	if (m_pFlowerTexture) m_pFlowerTexture->Release();
	if (m_pFlowerTextureUploadBuffer) m_pFlowerTextureUploadBuffer->Release();

	if (m_pRockTexture) m_pRockTexture->Release();
	if (m_pRockTextureUploadBuffer) m_pRockTextureUploadBuffer->Release();

	// ApplyMeshTextures  /  
	for (auto* pUp : m_vLoadedTextureUploadBuffers) if (pUp) pUp->Release();
	m_vLoadedTextureUploadBuffers.clear();
	for (auto* pTex : m_vLoadedTextures) if (pTex) pTex->Release();
	m_vLoadedTextures.clear();

	if (m_pDefaultWhiteTexture) m_pDefaultWhiteTexture->Release();
	m_pDefaultWhiteTexture = NULL;
	if (m_pDefaultWhiteUploadBuffer) m_pDefaultWhiteUploadBuffer->Release();
	m_pDefaultWhiteUploadBuffer = NULL;

	m_materialSrvTableStarts.clear();
	m_srvTableStartsNeedingShadowUpdate.clear();
	m_bShadowSrvReady = false;
	m_nNextSrvTableIndex = kReservedSrvCount;
	m_d3dDefaultSrvTableHandle.ptr = 0;

	if (m_pd3dCbvSrvHeap) m_pd3dCbvSrvHeap->Release();

	if (m_pShadowShader) { m_pShadowShader->Release(); m_pShadowShader = NULL; }

	m_pTreeTexture = NULL;
	m_pTreeTextureUploadBuffer = NULL;

	m_pFlowerTexture = NULL;
	m_pFlowerTextureUploadBuffer = NULL;

	m_pRockTexture = NULL;
	m_pRockTextureUploadBuffer = NULL;

	m_pd3dCbvSrvHeap = NULL;

	ReleaseShaderVariables();

	if (m_pLights) delete[] m_pLights;
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	ID3D12RootSignature* pd3dGraphicsRootSignature = NULL;

	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[1];
	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 8; // 1  8 
	pd3dDescriptorRanges[0].BaseShaderRegister = 0; // t0 
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER pd3dRootParameters[5]; // 6

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 1; // b1 (Camera)
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 32;
	pd3dRootParameters[1].Constants.ShaderRegister = 2; // b2 (GameObject)
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[2].Descriptor.ShaderRegister = 4; // b4 (Lights)
	pd3dRootParameters[2].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[3].DescriptorTable.pDescriptorRanges = pd3dDescriptorRanges;
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[4].Constants.Num32BitValues = 16; // 4x4  (16 floats)
	pd3dRootParameters[4].Constants.ShaderRegister = 5;  // b5
	pd3dRootParameters[4].Constants.RegisterSpace = 0;
	pd3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	//D3D12_DESCRIPTOR_RANGE shadowRange[1];
	//shadowRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//shadowRange[0].NumDescriptors = 1;
	//shadowRange[0].BaseShaderRegister = 4; // t4 
	//shadowRange[0].RegisterSpace = 0;
	//shadowRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//pd3dRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//pd3dRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	//pd3dRootParameters[5].DescriptorTable.pDescriptorRanges = shadowRange;
	//pd3dRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC d3dStaticSamplerDescs[2];
	::ZeroMemory(&d3dStaticSamplerDescs, sizeof(D3D12_STATIC_SAMPLER_DESC) * 2);
	d3dStaticSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	d3dStaticSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dStaticSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dStaticSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	d3dStaticSamplerDescs[0].MipLODBias = 0;
	d3dStaticSamplerDescs[0].MaxAnisotropy = 16;
	d3dStaticSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dStaticSamplerDescs[0].MinLOD = 0;
	d3dStaticSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	d3dStaticSamplerDescs[0].ShaderRegister = 0; // s0
	d3dStaticSamplerDescs[0].RegisterSpace = 0;
	d3dStaticSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	///////////////////////////////////////////
	d3dStaticSamplerDescs[1] = d3dStaticSamplerDescs[0];
	d3dStaticSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dStaticSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dStaticSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	d3dStaticSamplerDescs[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	d3dStaticSamplerDescs[1].ShaderRegister = 1; // s1 

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;//


	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));

	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = 2;
	d3dRootSignatureDesc.pStaticSamplers = d3dStaticSamplerDescs;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255); //256 
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbLights->Map(0, NULL, (void**)&m_pcbMappedLights);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	::memcpy(m_pcbMappedLights->m_pLights, m_pLights, sizeof(LIGHT) * m_nLights);
	::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &m_xmf4GlobalAmbient, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedLights->m_nLights, &m_nLights, sizeof(int));
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbLights)
	{
		m_pd3dcbLights->Unmap(0, NULL);
		m_pd3dcbLights->Release();
	}
}

void CScene::ReleaseUploadBuffers()
{
	if (m_ppGameObjects)
	{
		for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->ReleaseUploadBuffers();
	}

	if (m_pWireframeBoxObject) m_pWireframeBoxObject->ReleaseUploadBuffers();
	if (m_pCombinedAABBBoxObject) m_pCombinedAABBBoxObject->ReleaseUploadBuffers();

	if (m_pTreeTextureUploadBuffer) m_pTreeTextureUploadBuffer->Release();
	m_pTreeTextureUploadBuffer = NULL;

	if (m_pFlowerTextureUploadBuffer) m_pFlowerTextureUploadBuffer->Release();
	m_pFlowerTextureUploadBuffer = NULL;

	if (m_pRockTextureUploadBuffer) m_pRockTextureUploadBuffer->Release();
	m_pRockTextureUploadBuffer = NULL;

	if (m_pDefaultWhiteUploadBuffer) m_pDefaultWhiteUploadBuffer->Release();
	m_pDefaultWhiteUploadBuffer = NULL;

	for (auto* pUpload : m_vLoadedTextureUploadBuffers)
	{
		if (pUpload) pUpload->Release();
	}
	m_vLoadedTextureUploadBuffers.clear();
}

void CScene::PickObject(XMFLOAT3& fWorldRayOrigin, XMFLOAT3& fWorldRayDirection)
{
	XMVECTOR vWorldRayOrigin = XMLoadFloat3(&fWorldRayOrigin);
	XMVECTOR vWorldRayDirection = XMLoadFloat3(&fWorldRayDirection);
	XMVECTOR vWorldRayTarget = vWorldRayOrigin + vWorldRayDirection * 10000.0f;

	CGameObject* pSelectedObject = NULL;
	float fMinDistance = FLT_MAX;

	for (int i = 0; i < m_nGameObjects; ++i)
	{
		if (!m_ppGameObjects[i]) continue;

		CGameObject* pClosestHit = m_ppGameObjects[i]->PickObject(vWorldRayOrigin, vWorldRayTarget, fMinDistance);

		if (pClosestHit)
		{
			pSelectedObject = pClosestHit;
		}
	}

	m_pSelectedObject = pSelectedObject;
}

//bool CScene::CheckCollision()
//{
//	if (!m_pPlayer) return false;
//
//	BoundingBox localPlayerAABB = m_pPlayer->GetCombinedAABB();
//
//	BoundingBox worldPlayerAABB;
//	localPlayerAABB.Transform(worldPlayerAABB, XMLoadFloat4x4(&m_pPlayer->GetWorldMatrix()));
//
//	m_pCollidedObject = NULL;
//
//	for (int i = 1; i < m_nGameObjects; i++)
//	{
//		CGameObject* pObject = m_ppGameObjects[i];
//		if (!pObject || !pObject->m_bIsActive) continue;
//		if (pObject->m_bIsGround) continue;
//
//		BoundingBox localObjectAABB = pObject->GetCombinedAABB();
//
//		if (localObjectAABB.Extents.x == 0 && localObjectAABB.Extents.y == 0 && localObjectAABB.Extents.z == 0) continue;
//
//		BoundingBox worldObjectAABB;
//		localObjectAABB.Transform(worldObjectAABB, XMLoadFloat4x4(&pObject->GetWorldMatrix()));
//
//		if (worldPlayerAABB.Intersects(worldObjectAABB))
//		{
//			m_pCollidedObject = pObject;
//
//			return true;
//		}
//	}
//	return false;
//}

bool CheckRecursiveCollision(CGameObject* pObject, BoundingOrientedBox& worldPlayerOBB, XMFLOAT3 playerPos, CGameObject** ppCollidedObject)
{
	if (!pObject) return false;

	if (pObject->m_pMesh)
	{
		BoundingBox pureMeshAABB = pObject->m_pMesh->GetBoundingBox();

		BoundingOrientedBox meshOBB;
		BoundingOrientedBox::CreateFromBoundingBox(meshOBB, pureMeshAABB);

		meshOBB.Transform(meshOBB, XMLoadFloat4x4(&pObject->GetWorldMatrix()));

		if (worldPlayerOBB.Intersects(meshOBB))
		{
			*ppCollidedObject = pObject;
			
			return true;
		}
	}

	if (pObject->m_pChild)
	{
		if (CheckRecursiveCollision(pObject->m_pChild, worldPlayerOBB, playerPos, ppCollidedObject))
			return true;
	}

	if (pObject->m_pSibling)
	{
		if (CheckRecursiveCollision(pObject->m_pSibling, worldPlayerOBB, playerPos, ppCollidedObject))
			return true;
	}

	return false;
}

bool CScene::CheckCollision()
{
	if (!m_pPlayer) return false;

	BoundingOrientedBox worldPlayerOBB = m_pPlayer->GetWorldOBB();
	XMFLOAT3 playerPos = m_pPlayer->GetPosition(); 

	m_pCollidedObject = NULL;

	for (int i = 1; i < m_nGameObjects; i++)
	{
		CGameObject* pObject = m_ppGameObjects[i];

		if (!pObject || !pObject->m_bIsActive) continue;
		if (pObject->m_bIsGround) continue;

		if (pObject->m_bIsInvisibleWall || pObject->m_bIsCheckPoint || pObject->m_bIsRCP)
		{
			if (CheckRecursiveCollision(pObject, worldPlayerOBB, playerPos, &m_pCollidedObject))
			{
				m_pCollidedObject->m_bIsInvisibleWall = pObject->m_bIsInvisibleWall;
				m_pCollidedObject->m_bIsCheckPoint = pObject->m_bIsCheckPoint;
				m_pCollidedObject->m_nCheckPointIndex = pObject->m_nCheckPointIndex;
				m_pCollidedObject->m_bIsRCP = pObject->m_bIsRCP;
				return true;
			}
		}
		else
		{
			BoundingBox localObjectAABB = pObject->GetCombinedAABB();

			if (localObjectAABB.Extents.x == 0 && localObjectAABB.Extents.y == 0 && localObjectAABB.Extents.z == 0) continue;

			BoundingOrientedBox worldObjectOBB = pObject->GetWorldOBB();

			if (worldPlayerOBB.Intersects(worldObjectOBB))
			{
				m_pCollidedObject = pObject;
				m_nCollidedObjectIndex = i;
				return true;
			}
		}
	}

	return false;
}

bool CScene::CheckGroundCollision()
{
	if (!m_pPlayer) return false;

	BoundingBox localPlayerAABB = m_pPlayer->GetCombinedAABB();
	BoundingBox worldPlayerAABB;
	localPlayerAABB.Transform(worldPlayerAABB, XMLoadFloat4x4(&m_pPlayer->GetWorldMatrix()));

	float playerBottomY = worldPlayerAABB.Center.y - worldPlayerAABB.Extents.y;
	XMFLOAT3 playerPos = m_pPlayer->GetPosition();
	float rayX = worldPlayerAABB.Center.x;
	float rayZ = worldPlayerAABB.Center.z;


	const float rayMaxDistance = 100000.0f;

	float bestT = FLT_MAX;
	float bestY = -FLT_MAX;

	//{
	//	XMVECTOR vRayOrigin = XMVectorSet(rayX, playerBottomY + 10.0f, rayZ, 1.0f);
	//	XMVECTOR vRayDir = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
	//	XMVECTOR vRayTarget = vRayOrigin + vRayDir * rayMaxDistance;
	//
	//	for (int i = 0; i < m_nGameObjects; ++i)
	//	{
	//		CGameObject* pObject = m_ppGameObjects[i];
	//		if (!pObject) continue;
	//		if (!pObject->m_bIsGround) continue;
	//		RaycastDownRecursive(pObject, vRayOrigin, vRayTarget, vRayDir, rayMaxDistance, bestT, bestY);
	//	}
	//}

	if (bestT == FLT_MAX)
	{
		XMVECTOR vRayOrigin = XMVectorSet(rayX, playerBottomY + 30.0f, rayZ, 1.0f);
		XMVECTOR vRayDir = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
		XMVECTOR vRayTarget = vRayOrigin + vRayDir * rayMaxDistance;

		for (int i = 0; i < m_nGameObjects; ++i)
		{
			CGameObject* pObject = m_ppGameObjects[i];
			if (!pObject) continue;
			if (!pObject->m_bIsGround) continue;
			RaycastDownRecursive(pObject, vRayOrigin, vRayTarget, vRayDir, rayMaxDistance, bestT, bestY);
		}
	}

	if (bestT == FLT_MAX) return false;
	
	//
	XMFLOAT3 vel = m_pPlayer->GetVelocity();
	if (vel.y > 0.0f)
		return false;

	float distToGround = playerBottomY - bestY;
	const float epsilon = 3.0f;

	if (distToGround <= epsilon)
	{
		XMFLOAT3 newPos = playerPos;
		newPos.y -= distToGround;
		m_pPlayer->SetPosition(newPos);
		return true;
	}

	return false;
}

void CScene::RaycastDownRecursive(CGameObject* pObject, const XMVECTOR& vWorldRayOrigin, const XMVECTOR& vWorldRayTarget, const XMVECTOR& vWorldRayDir, float maxDistance, float& bestT, float& bestY)
{
	if (!pObject) return;

	if (pObject->m_pMesh)
	{
		XMMATRIX matWorld = XMLoadFloat4x4(&pObject->GetWorldMatrix());
		XMMATRIX matInvWorld = XMMatrixInverse(NULL, matWorld);

		XMVECTOR vLocalRayOrigin = XMVector3TransformCoord(vWorldRayOrigin, matInvWorld);
		XMVECTOR vLocalRayTarget = XMVector3TransformCoord(vWorldRayTarget, matInvWorld);
		XMVECTOR vLocalRayDirection = XMVector3Normalize(vLocalRayTarget - vLocalRayOrigin);

		XMFLOAT3 fLocalOrigin, fLocalDir;
		XMStoreFloat3(&fLocalOrigin, vLocalRayOrigin);
		XMStoreFloat3(&fLocalDir, vLocalRayDirection);

		float fHitDistance = 0.0f;
		if (pObject->m_pMesh->CheckRayIntersection(fLocalOrigin, fLocalDir, &fHitDistance))
		{
			XMVECTOR vLocalHit = vLocalRayOrigin + vLocalRayDirection * fHitDistance;
			XMVECTOR vWorldHit = XMVector3TransformCoord(vLocalHit, matWorld);

			float t = XMVectorGetX(XMVector3Dot(vWorldHit - vWorldRayOrigin, vWorldRayDir));
			if (t >= 0.0f && t <= maxDistance && t < bestT)
			{
				bestT = t;
				bestY = XMVectorGetY(vWorldHit);
			}
		}
	}

	if (pObject->m_pChild) RaycastDownRecursive(pObject->m_pChild, vWorldRayOrigin, vWorldRayTarget, vWorldRayDir, maxDistance, bestT, bestY);
	if (pObject->m_pSibling) RaycastDownRecursive(pObject->m_pSibling, vWorldRayOrigin, vWorldRayTarget, vWorldRayDir, maxDistance, bestT, bestY);
}

bool CScene::IsNullTextureName(const char* pstr) 
{
	if (!pstr) {
		return true;
	}

	if (pstr[0] == '\0') {
		return true;
	}

	return (_stricmp(pstr, "null") == 0);
}

void CScene::LoadTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	//  SRV (Shader Visible) :  SRV (8)  
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc = {};
	d3dDescriptorHeapDesc.NumDescriptors = 16384;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dCbvSrvHeap);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandleStart = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuSrvHandleStart = m_pd3dCbvSrvHeap->GetGPUDescriptorHandleForHeapStart();

	m_nDescriptorIncrementSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//   (Heap Start) CMaterial fallback    
	m_d3dDefaultSrvTableHandle = d3dGpuSrvHandleStart;

	CMaterial::m_d3dDefaultSrvTableHandle = d3dGpuSrvHandleStart;

	//    
	m_bShadowSrvReady = false;
	m_nNextSrvTableIndex = kReservedSrvCount;
	m_materialSrvTableStarts.clear();
	m_srvTableStartsNeedingShadowUpdate.clear();

	//   (ApplyMeshTextures) (   )
	for (auto* pUp : m_vLoadedTextureUploadBuffers) if (pUp) pUp->Release();
	m_vLoadedTextureUploadBuffers.clear();
	for (auto* pTex : m_vLoadedTextures) if (pTex) pTex->Release();
	m_vLoadedTextures.clear();

	if (m_pDefaultWhiteTexture) { m_pDefaultWhiteTexture->Release(); m_pDefaultWhiteTexture = NULL; }
	if (m_pDefaultWhiteUploadBuffer) { m_pDefaultWhiteUploadBuffer->Release(); m_pDefaultWhiteUploadBuffer = NULL; }

	// ------------------------------------------------------------
	// [slot2 = t2] 1x1 White   (   fallback)
	// ------------------------------------------------------------
	{
		D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
		pd3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_pDefaultWhiteTexture));

		UINT64 uploadSize = GetRequiredIntermediateSize(m_pDefaultWhiteTexture, 0, 1);
		pd3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_pDefaultWhiteUploadBuffer));

		const UINT32 whitePixel = 0xFFFFFFFF;
		D3D12_SUBRESOURCE_DATA subData = {};
		subData.pData = &whitePixel;
		subData.RowPitch = sizeof(UINT32);
		subData.SlicePitch = sizeof(UINT32);

		UpdateSubresources(pd3dCommandList, m_pDefaultWhiteTexture, m_pDefaultWhiteUploadBuffer, 0, 0, 1, &subData);
		pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_pDefaultWhiteTexture,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		D3D12_CPU_DESCRIPTOR_HANDLE cpuSlot2 = d3dCpuSrvHandleStart;
		cpuSlot2.ptr += (SIZE_T)m_nDescriptorIncrementSize * kDefaultWhiteSrvIndex;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		pd3dDevice->CreateShaderResourceView(m_pDefaultWhiteTexture, &srvDesc, cpuSlot2);

		// 0~7( )   SRV ( /)
		for (UINT i = 0; i < kReservedSrvCount; ++i)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cpuDst = d3dCpuSrvHandleStart;
			cpuDst.ptr += (SIZE_T)m_nDescriptorIncrementSize * i;
			pd3dDevice->CopyDescriptorsSimple(1, cpuDst, cpuSlot2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// [slot4=t4] ShadowMap SRV      white ()
		D3D12_CPU_DESCRIPTOR_HANDLE cpuSlot4 = d3dCpuSrvHandleStart;
		cpuSlot4.ptr += (SIZE_T)m_nDescriptorIncrementSize * kShadowMapSrvIndex;
		pd3dDevice->CopyDescriptorsSimple(1, cpuSlot4, cpuSlot2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// ------------------------------------------------------------
	// [slot0=t0] Tree.dds
	// ------------------------------------------------------------
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandle_Slot0 = d3dCpuSrvHandleStart;
		m_d3dGpuTreeSrvHandle = d3dGpuSrvHandleStart;

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		HRESULT hr = DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Asset/DDS_File/Tree.dds", &m_pTreeTexture, ddsData, subresources);
		if (SUCCEEDED(hr) && m_pTreeTexture)
		{
			UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pTreeTexture, 0, (UINT)subresources.size());
			pd3dDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				__uuidof(ID3D12Resource), (void**)&m_pTreeTextureUploadBuffer);

			UpdateSubresources(pd3dCommandList, m_pTreeTexture, m_pTreeTextureUploadBuffer, 0, 0, (UINT)subresources.size(), subresources.data());
			pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pTreeTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = m_pTreeTexture->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = m_pTreeTexture->GetDesc().MipLevels;
			pd3dDevice->CreateShaderResourceView(m_pTreeTexture, &srvDesc, d3dCpuSrvHandle_Slot0);
		}
	}

	// ------------------------------------------------------------
	// [slot1=t0] Flower.dds (billboard: base handle slot1  t0 )
	// ------------------------------------------------------------
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandle_Slot1 = d3dCpuSrvHandleStart;
		d3dCpuSrvHandle_Slot1.ptr += (SIZE_T)m_nDescriptorIncrementSize * 1;

		m_d3dGpuFlowerSrvHandle = d3dGpuSrvHandleStart;
		m_d3dGpuFlowerSrvHandle.ptr += (UINT64)m_nDescriptorIncrementSize * 1;

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		HRESULT hr = DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Asset/DDS_File/Flower.dds", &m_pFlowerTexture, ddsData, subresources);
		if (SUCCEEDED(hr) && m_pFlowerTexture)
		{
			UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pFlowerTexture, 0, (UINT)subresources.size());
			pd3dDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				__uuidof(ID3D12Resource), (void**)&m_pFlowerTextureUploadBuffer);

			UpdateSubresources(pd3dCommandList, m_pFlowerTexture, m_pFlowerTextureUploadBuffer, 0, 0, (UINT)subresources.size(), subresources.data());
			pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pFlowerTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = m_pFlowerTexture->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = m_pFlowerTexture->GetDesc().MipLevels;
			pd3dDevice->CreateShaderResourceView(m_pFlowerTexture, &srvDesc, d3dCpuSrvHandle_Slot1);
		}
	}

	// ------------------------------------------------------------
	// [slot3=t0] Rock.dds (billboard)
	// ------------------------------------------------------------
	{
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandle_Slot3 = d3dCpuSrvHandleStart;
		d3dCpuSrvHandle_Slot3.ptr += (SIZE_T)m_nDescriptorIncrementSize * 3;

		m_d3dGpuRockSrvHandle = d3dGpuSrvHandleStart;
		m_d3dGpuRockSrvHandle.ptr += (UINT64)m_nDescriptorIncrementSize * 3;

		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		HRESULT hr = DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Asset/DDS_File/Rock.dds", &m_pRockTexture, ddsData, subresources);
		if (SUCCEEDED(hr) && m_pRockTexture)
		{
			UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pRockTexture, 0, (UINT)subresources.size());
			pd3dDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				__uuidof(ID3D12Resource), (void**)&m_pRockTextureUploadBuffer);

			UpdateSubresources(pd3dCommandList, m_pRockTexture, m_pRockTextureUploadBuffer, 0, 0, (UINT)subresources.size(), subresources.data());
			pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRockTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = m_pRockTexture->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = m_pRockTexture->GetDesc().MipLevels;
			pd3dDevice->CreateShaderResourceView(m_pRockTexture, &srvDesc, d3dCpuSrvHandle_Slot3);
		}
	}
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return(false);
}


bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'Q':
			m_ppGameObjects[3]->MoveUp(10);
			break;
		case 'R':
			m_ppGameObjects[3]->MoveUp(-10);
			break;
		case 'W':
			m_ppGameObjects[0]->MoveForward(10);
			break;
		case 'S':
			m_ppGameObjects[0]->MoveForward(-10);
			break;
		case 'A':
			m_ppGameObjects[0]->MoveStrafe(-10);
			break;
		case 'D':
			m_ppGameObjects[0]->MoveStrafe(10);
			break;
		case 'Z':
			//m_ppGameObjects[114]->Rotate(0, 10, 0);
			break;
		case VK_F7:
			m_bShowWireframeBox = !m_bShowWireframeBox;
			break;
		case VK_F8:
			m_bShowCombinedAABB = !m_bShowCombinedAABB;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return(false);
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
	return(false);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;

	for (int i = 0; i < m_nGameObjects; i++) m_ppGameObjects[i]->Animate(fTimeElapsed, NULL);

	if (m_pLights)
	{
		m_pLights[1].m_xmf3Position = m_pPlayer->GetPosition();
		m_pLights[1].m_xmf3Direction = m_pPlayer->GetLookVector();
	}

}

void CScene::RenderShadowMap(ID3D12GraphicsCommandList* pd3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
	if (m_pd3dGraphicsRootSignature)
		pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);

	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, 16384.f, 16384.f, 0.0f, 1.0f };
	D3D12_RECT scissorRect = { 0, 0, 16384, 16384 };
	pd3dCommandList->RSSetViewports(1, &viewport);
	pd3dCommandList->RSSetScissorRects(1, &scissorRect);

	pd3dCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
	pd3dCommandList->OMSetRenderTargets(0, NULL, FALSE, &dsvHandle);

	if (m_pShadowShader) m_pShadowShader->Render(pd3dCommandList, NULL);

	XMMATRIX mLightViewProj = GetShadowLightViewProj();
	XMFLOAT4X4 xmf4x4LightViewProj;
	XMStoreFloat4x4(&xmf4x4LightViewProj, XMMatrixTranspose(mLightViewProj));
	pd3dCommandList->SetGraphicsRoot32BitConstants(4, 16, &xmf4x4LightViewProj, 0);

	for (int i = 0; i < m_nGameObjects; ++i)
	{
		if (i >= 1 && i <= 12) continue; // 사이드 벽 그림자 렌더링 안되게

		CGameObject* pObj = m_ppGameObjects[i];
		if (!pObj || !pObj->m_bIsActive) continue;

		bool bIsBillboard = false;
		if (pObj->m_nMaterials > 0 && pObj->m_ppMaterials && pObj->m_ppMaterials[0])
		{
			if (pObj->m_ppMaterials[0]->m_pShader == CMaterial::m_pBillboardShader)
				bIsBillboard = true;
		}
		if (bIsBillboard) continue;

		pObj->Render(pd3dCommandList, NULL, NULL);
	}

	if (m_pPlayer) m_pPlayer->Render(pd3dCommandList, NULL, NULL);
}

XMMATRIX CScene::GetShadowLightViewProj()
{
	XMFLOAT3 lightPos = m_pLights[2].m_xmf3Position;
	XMFLOAT3 lightDir = m_pLights[2].m_xmf3Direction;

	if (m_pPlayer)
	{
		XMFLOAT3 playerPos = m_pPlayer->GetPosition();

		XMVECTOR vLightDir = XMLoadFloat3(&lightDir);
		XMVECTOR vPlayerPos = XMLoadFloat3(&playerPos);

		XMVECTOR vLightPos = vPlayerPos - (vLightDir * 3000.0f);
		XMStoreFloat3(&lightPos, vLightPos);
	}

	XMVECTOR vEye = XMLoadFloat3(&lightPos);
	XMVECTOR vAt = XMLoadFloat3(&lightPos) + XMLoadFloat3(&lightDir);
	XMVECTOR vUp = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX mLightView = XMMatrixLookAtLH(vEye, vAt, vUp);

	XMMATRIX mLightProj = XMMatrixOrthographicLH(2500.0f, 2500.0f, 1.0f, 6000.f);

	return mLightView * mLightProj;
}

void CScene::CreateSkybox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandle = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuSrvHandle = m_pd3dCbvSrvHeap->GetGPUDescriptorHandleForHeapStart();

	int nSkyboxIndex = 5;

	d3dCpuSrvHandle.ptr += (m_nDescriptorIncrementSize * nSkyboxIndex);
	m_d3dGpuSkyboxSrvHandle = d3dGpuSrvHandle;
	m_d3dGpuSkyboxSrvHandle.ptr += (m_nDescriptorIncrementSize * nSkyboxIndex);

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	DirectX::LoadDDSTextureFromFile(pd3dDevice, L"Asset/DDS_File/SkyBox_1210.dds", &m_pSkyboxTexture, ddsData, subresources);

	UINT64 nUploadBufferSize = GetRequiredIntermediateSize(m_pSkyboxTexture, 0, (UINT)subresources.size());
	pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nUploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		__uuidof(ID3D12Resource), (void**)&m_pSkyboxTextureUploadBuffer);

	UpdateSubresources(pd3dCommandList, m_pSkyboxTexture, m_pSkyboxTextureUploadBuffer, 0, 0, (UINT)subresources.size(), subresources.data());
	pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pSkyboxTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_pSkyboxTexture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE; // *: Texture2D *
	srvDesc.TextureCube.MipLevels = m_pSkyboxTexture->GetDesc().MipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	pd3dDevice->CreateShaderResourceView(m_pSkyboxTexture, &srvDesc, d3dCpuSrvHandle);

	CGameObject* pSkyboxModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Cube.bin");

	CMaterial* pSkyboxMaterial = new CMaterial();
	CSkyboxShader* pSkyboxShader = new CSkyboxShader();
	pSkyboxShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	pSkyboxMaterial->SetShader(pSkyboxShader);
	pSkyboxMaterial->SetTexture(m_d3dDefaultSrvTableHandle);

	if (pSkyboxModel->m_pMesh)
	{
		if (pSkyboxModel->m_ppMaterials) delete[] pSkyboxModel->m_ppMaterials;
		pSkyboxModel->m_nMaterials = 1;
		pSkyboxModel->m_ppMaterials = new CMaterial * [1];
		pSkyboxModel->m_ppMaterials[0] = NULL;
		pSkyboxModel->SetMaterial(0, pSkyboxMaterial);
	}
	else if (pSkyboxModel->m_pChild)
	{
		CGameObject* pChild = pSkyboxModel->m_pChild;
		if (pChild->m_ppMaterials) delete[] pChild->m_ppMaterials;
		pChild->m_nMaterials = 1;
		pChild->m_ppMaterials = new CMaterial * [1];
		pChild->m_ppMaterials[0] = NULL;
		pChild->SetMaterial(0, pSkyboxMaterial);
	}

	m_pSkyboxObject = new CGameObject();
	m_pSkyboxObject->SetChild(pSkyboxModel);
	m_pSkyboxObject->SetScale(5000.0f, 5000.0f, 5000.0f);
}

void CScene::RenderSkybox(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (!m_pSkyboxObject) return;

	m_pSkyboxObject->Render(pd3dCommandList, NULL, pCamera);
}

void CScene::CreateUIRootSignature(ID3D12Device* pd3dDevice)
{
	CD3DX12_DESCRIPTOR_RANGE d3dDescriptorRange;
	d3dDescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7);

	CD3DX12_ROOT_PARAMETER d3dRootParameter[1];
	d3dRootParameter[0].InitAsDescriptorTable(1, &d3dDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC d3dSamplerDesc(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	CD3DX12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc(1, d3dRootParameter, 1, &d3dSamplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_pd3dUIRootSignature));
}

void CScene::BuildUIResources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CreateUIRootSignature(pd3dDevice);
	m_pUIMesh = new CUIMesh(pd3dDevice, pd3dCommandList);
	m_pUIShader = new CUIShader();
	m_pUIShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dUIRootSignature);

	const wchar_t* texPaths[4] = {
		L"Asset/DDS_File/Item_Dash_02.dds",
		L"Asset/DDS_File/Item_Gauge_02.dds",
		L"Asset/DDS_File/Item_Speed_02.dds",
		L"Asset/DDS_File/Item_Lock_02.dds" 
	};

	UINT uiSrvStartIndex = m_nNextSrvTableIndex;
	m_nNextSrvTableIndex += 4;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dCpuSrvHandleStart = m_pd3dCbvSrvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGpuSrvHandleStart = m_pd3dCbvSrvHeap->GetGPUDescriptorHandleForHeapStart();

	for (int i = 0; i < 4; ++i)
	{
		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;

		ID3D12Resource* pTex = nullptr;
		ID3D12Resource* pUpload = nullptr;

		HRESULT hr = DirectX::LoadDDSTextureFromFile(pd3dDevice, texPaths[i], &pTex, ddsData, subresources);

		if (SUCCEEDED(hr) && pTex)
		{
			UINT64 uploadSize = GetRequiredIntermediateSize(pTex, 0, (UINT)subresources.size());
			pd3dDevice->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadSize), D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&pUpload));

			UpdateSubresources(pd3dCommandList, pTex, pUpload, 0, 0, (UINT)subresources.size(), subresources.data());

			pd3dCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				pTex, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

			UINT currentSrvIndex = uiSrvStartIndex + i;

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = d3dCpuSrvHandleStart;
			cpuHandle.ptr += (SIZE_T)m_nDescriptorIncrementSize * currentSrvIndex;

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = pTex->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = pTex->GetDesc().MipLevels;

			pd3dDevice->CreateShaderResourceView(pTex, &srvDesc, cpuHandle);

			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = d3dGpuSrvHandleStart;
			gpuHandle.ptr += (UINT64)m_nDescriptorIncrementSize * currentSrvIndex;
			m_pd3dUIItemSrvHandles[i] = gpuHandle;

			m_vLoadedTextures.push_back(pTex);
			m_vLoadedTextureUploadBuffers.push_back(pUpload);
		}
	}
}


void CScene::RenderItemUI(ID3D12GraphicsCommandList* pd3dCommandList, int nItemIndex, int screenW, int screenH)
{
	if (!m_pd3dUIRootSignature || !m_pUIShader || !m_pUIMesh) return;
	if (nItemIndex < 0 || nItemIndex >= 4) return;

	float baseW = 1280.0f;
	float baseH = 720.0f;

	float scale = (float)screenH / baseH;

	float iconSize = 90.0f * scale; // 아이템 아이콘 사이즈 조절 

	float iconW = (iconSize / (float)screenW) * 2.0f;
	float iconH = (iconSize / (float)screenH) * 2.0f;

	float left = -0.95f;
	float top = 0.85f;
	float right = left + iconW;
	float bottom = top - iconH;

	m_pUIMesh->SetRect(left, top, right, bottom);

	pd3dCommandList->SetGraphicsRootSignature(m_pd3dUIRootSignature);
	m_pUIShader->Render(pd3dCommandList, 0);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D12DescriptorHeap* ppHeaps[] = { m_pd3dCbvSrvHeap };
	pd3dCommandList->SetDescriptorHeaps(1, ppHeaps);

	pd3dCommandList->SetGraphicsRootDescriptorTable(0, m_pd3dUIItemSrvHandles[nItemIndex]);

	m_pUIMesh->Render(pd3dCommandList);
}


void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvHeap) {
		ID3D12DescriptorHeap* ppHeaps[] = { m_pd3dCbvSrvHeap };
		pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	}
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);
	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress);

	XMMATRIX mLightViewProj = GetShadowLightViewProj();

	XMFLOAT4X4 xmf4x4LightViewProj;
	XMStoreFloat4x4(&xmf4x4LightViewProj, XMMatrixTranspose(mLightViewProj));

	pd3dCommandList->SetGraphicsRoot32BitConstants(4, 16, &xmf4x4LightViewProj, 0);

	CGameObject* pDebugBoxToRender = NULL;
	if (m_bShowWireframeBox && m_pWireframeBoxObject) pDebugBoxToRender = m_pWireframeBoxObject;

	for (int i = 0; i < m_nGameObjects; ++i)
	{
		if (m_ppGameObjects[i])
		{
			if (i >= 1 && i <= 12) continue; // 스테이지 1,2 공통 index 1~12번은 체크포인트&벽
			//if (i == 1) continue; // 스테이지 1 index 1~12번은 체크포인트&벽
			if (m_nCurrentMapStage == 1) { // 스테이지 2 회전 충돌체
				if (i >= 13 && i <= 15) continue;
			}
			m_ppGameObjects[i]->Animate(m_fElapsedTime, NULL);
			m_ppGameObjects[i]->UpdateTransform(NULL);
			m_ppGameObjects[i]->Render(pd3dCommandList, pDebugBoxToRender, pCamera);
		}
	} // 인게임 내부 루프

	RenderSkybox(pd3dCommandList, pCamera);

	if (m_bShowCombinedAABB && m_pCombinedAABBBoxObject)
	{
		if (m_pCombinedAABBBoxObject->m_ppMaterials[0])
		{
			if (m_pCombinedAABBBoxObject->m_ppMaterials[0]->m_pShader)
				m_pCombinedAABBBoxObject->m_ppMaterials[0]->m_pShader->Render(pd3dCommandList, pCamera);

			m_pCombinedAABBBoxObject->m_ppMaterials[0]->UpdateShaderVariable(pd3dCommandList);
		}

		if (m_pPlayer)
		{
			BoundingBox localAABB = m_pPlayer->GetCombinedAABB();
			XMMATRIX matWorld = XMLoadFloat4x4(&m_pPlayer->GetWorldMatrix());

			XMMATRIX matScale = XMMatrixScaling(localAABB.Extents.x * 2.0f, localAABB.Extents.y * 2.0f, localAABB.Extents.z * 2.0f);
			XMMATRIX matTranslate = XMMatrixTranslation(localAABB.Center.x, localAABB.Center.y, localAABB.Center.z);
			XMMATRIX matFinalWorld = matScale * matTranslate * matWorld;

			XMStoreFloat4x4(&m_pCombinedAABBBoxObject->m_xmf4x4World, matFinalWorld);
			m_pCombinedAABBBoxObject->UpdateShaderVariable(pd3dCommandList, &m_pCombinedAABBBoxObject->m_xmf4x4World);
			if (m_pCombinedAABBBoxObject->m_pMesh) m_pCombinedAABBBoxObject->m_pMesh->Render(pd3dCommandList);
		}

		for (int i = 0; i < m_nGameObjects; ++i)
		{
			if (m_ppGameObjects[i])
			{
				BoundingBox localAABB = m_ppGameObjects[i]->GetCombinedAABB();
				XMMATRIX matWorld = XMLoadFloat4x4(&m_ppGameObjects[i]->GetWorldMatrix());

				XMMATRIX matScale = XMMatrixScaling(localAABB.Extents.x * 2.0f, localAABB.Extents.y * 2.0f, localAABB.Extents.z * 2.0f);
				XMMATRIX matTranslate = XMMatrixTranslation(localAABB.Center.x, localAABB.Center.y, localAABB.Center.z);
				XMMATRIX matFinalWorld = matScale * matTranslate * matWorld;

				XMStoreFloat4x4(&m_pCombinedAABBBoxObject->m_xmf4x4World, matFinalWorld);
				m_pCombinedAABBBoxObject->UpdateShaderVariable(pd3dCommandList, &m_pCombinedAABBBoxObject->m_xmf4x4World);
				if (m_pCombinedAABBBoxObject->m_pMesh) m_pCombinedAABBBoxObject->m_pMesh->Render(pd3dCommandList);
			}
		}
	}
}