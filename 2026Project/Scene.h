//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include <random>
#include <vector>
#include <unordered_map>

#include "Shader.h"
#include "Player.h"

#define MAX_LIGHTS			16 
#define MAX_BANANA_OBJECTS 32

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

struct LIGHT
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction;
	float 					m_fTheta; //cos(m_fTheta)
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct LIGHTS
{
	LIGHT					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};

class CScene
{
public:
    CScene();
    ~CScene();

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	void BuildDefaultLightsAndMaterials();
	void BuildObjectsGameStart(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void BuildObjectsGameRoom(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	bool ProcessInput(UCHAR *pKeysBuffer);
    void AnimateObjects(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL);

	void ReleaseUploadBuffers();

	CPlayer* m_pPlayer = NULL;

	//
	void PickObject(XMFLOAT3& fWorldRayOrigin, XMFLOAT3& fWorldRayDirection);
	void BuildGameObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);//스테이지1
	void BuildGameStage2(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	// 스테이지2
	void BuildGameStage3(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void BuildGameStage4(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	void BuildObjectsGameEnd(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	//void CreateParticle(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateWireFrameBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateAABBWireFrameBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	void CreateShadowMapSRV(ID3D12Device* pd3dDevice, ID3D12Resource* pShadowMapResource);
	void RenderShadowMap(ID3D12GraphicsCommandList* pd3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle); 
	XMMATRIX GetShadowLightViewProj();

	void CreateSkybox(ID3D12Device* pd3dDevice,ID3D12GraphicsCommandList* pd3dCommandList,const wchar_t* pTexturePath);
	void RenderSkybox(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

	void CreateUIRootSignature(ID3D12Device* pd3dDevice);
	void BuildUIResources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void RenderItemUI(ID3D12GraphicsCommandList* pd3dCommandList, int nItemIndex, int screenW, int screenH);

	CGameObject* m_pSelectedObject = NULL; // 클릭된 객체 저장용 
	CGameObject* m_pCollidedObject = NULL;
	
	CGameObject* m_pWireframeBoxObject = NULL;

	bool m_bShowWireframeBox{ false };

	CGameObject* m_pCombinedAABBBoxObject = NULL;
	bool m_bShowCombinedAABB{ false };

	bool m_bIsCollision{ false };

	bool CheckCollision();

	bool CheckGroundCollision();



	CGameObject* InstallBanana(
		int bananaId,
		int ownerPlayerId,
		const XMFLOAT3& position,
		float yaw
	);

	void RemoveBanana(int bananaId);

	CGameObject* FindBanana(int bananaId);

	void ClearBananas();

	CGameObject* m_ppBananaObjects[MAX_BANANA_OBJECTS]{};
	int m_nBananaObjectStartIndex = -1;




	void RaycastDownRecursive(CGameObject* pObject, const XMVECTOR& vWorldRayOrigin, const XMVECTOR& vWorldRayTarget, const XMVECTOR& vWorldRayDir, float maxDistance, float& bestT, float& bestY, XMVECTOR& bestNormal);

	bool IsNullTextureName(const char* pstr);

	void LoadTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	ID3D12Resource* m_pTreeTexture = NULL;
	ID3D12Resource* m_pTreeTextureUploadBuffer = NULL;

	ID3D12Resource* m_pFlowerTexture = NULL;
	ID3D12Resource* m_pFlowerTextureUploadBuffer = NULL;

	ID3D12Resource* m_pRockTexture = NULL;
	ID3D12Resource* m_pRockTextureUploadBuffer = NULL;

	ID3D12DescriptorHeap* m_pd3dCbvSrvHeap = NULL;

	D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dGpuTreeSrvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dGpuFlowerSrvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dGpuRockSrvHandle;

	CShadowShader* m_pShadowShader = NULL; // 쉐도우 맵 생성용 셰이더

	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dDefaultSrvTableHandle = {};

	UINT kSrvTableSize = 8;
	UINT kReservedSrvCount = 8;          // 0~7 ()  
	UINT kDefaultWhiteSrvIndex = 2;      // t2
	UINT kShadowMapSrvIndex = 4;         // t4
	UINT kSkyboxSrvIndex = 5;            // t5

	bool m_bShadowSrvReady = false;
	UINT m_nNextSrvTableIndex = kReservedSrvCount;

	std::unordered_map<CMaterial*, UINT> m_materialSrvTableStarts;
	std::vector<UINT> m_srvTableStartsNeedingShadowUpdate;

	std::vector<ID3D12Resource*> m_vLoadedTextures;
	std::vector<ID3D12Resource*> m_vLoadedTextureUploadBuffers;

	ID3D12Resource* m_pDefaultWhiteTexture = NULL;
	ID3D12Resource* m_pDefaultWhiteUploadBuffer = NULL;

	UINT m_nDescriptorIncrementSize;

	CGameObject* m_pSkyboxObject = NULL;
	ID3D12Resource* m_pSkyboxTexture = NULL;
	ID3D12Resource* m_pSkyboxTextureUploadBuffer = NULL;

	D3D12_GPU_DESCRIPTOR_HANDLE m_d3dGpuSkyboxSrvHandle;

	CSkyboxShader* m_pSkyboxShader = NULL;

	ID3D12RootSignature* m_pd3dUIRootSignature = nullptr;
	CUIShader* m_pUIShader = nullptr;
	CUIMesh* m_pUIMesh = nullptr;

	// D3D12_GPU_DESCRIPTOR_HANDLE m_pd3dUIItemSrvHandles[4];
	D3D12_GPU_DESCRIPTOR_HANDLE m_pd3dUIItemSrvHandles[5];

	//CGameObject* m_pRoomCars[1]; // 모델링 파일 추가하면 더 늘려야 함

	CMap1Shader* m_pMap1Shader = NULL;
	void ChangeObjectShader(CGameObject* pObject, CShader* pShader);
	//
public:
	ID3D12RootSignature			*m_pd3dGraphicsRootSignature = NULL;

	CGameObject					**m_ppGameObjects = NULL;
	int							m_nGameObjects = 0;

	LIGHT						*m_pLights = NULL;
	int							m_nLights = 0;

	XMFLOAT4					m_xmf4GlobalAmbient;

	ID3D12Resource				*m_pd3dcbLights = NULL;
	LIGHTS						*m_pcbMappedLights = NULL;

	float						m_fElapsedTime = 0.0f;

	int m_nGFStage{};

	int m_nCollidedObjectIndex{ -1 };

	int m_nCurrentMapStage{ 0 };

	void ApplyMeshTextures(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CGameObject* pObject);

};
