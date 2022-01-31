#pragma once

#include <Include/DirectX/d3dx12.h>
#include <wrl.h>
#include <dxgi1_4.h>

#include "Commons/Singleton.h"
#include "Commons/Timer.h"
#include "Commons/UploadBuffer.h"
#include "Commons/AccelerationBuffers.h"
#include "Cameras/DebugCamera.h"
#include "Shaders/ConstantBuffers.h"

#include <unordered_map>
#include <array>
#include <dxcapi.h>

#define MAX_LIGHTS 5

class Timer;
class DescriptorHeap;
class Descriptor;

struct RayGenerationCB;
struct Vertex;

struct FrameResources
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pRenderTarget = nullptr;

	UploadBuffer<PrimitivePerFrameCB>* m_pPrimitivePerFrameCBUpload = nullptr;
	UploadBuffer<LightCB>* m_pLightCBUpload = nullptr;
};

namespace GlobalRootSignatureParams
{
	enum Value
	{
		OUTPUT = 0,
		ACCELERATION_STRUCTURE,
		PER_FRAME_SCENE_CB,
		PER_FRAME_PRIMITIVE_CB,
		LIGHT_CB,
		COUNT
	};
}

namespace LocalRootSignatureParams {
	enum Value 
	{
		ALBEDO_TEX,
		NORMAL_TEX,
		METALLIC_ROUGHNESS_TEX,
		OCCLUSION_TEX,
		PRIMITIVE_CB,
		VERTEX_INDEX,
		COUNT
	};
}

class App
{
public:
	App(HINSTANCE hInstance);
	~App();

	virtual bool Init();

#if PIX
	static std::wstring GetPixGpuCapturePath();
#endif

	virtual void Update(const Timer& kTimer);
	virtual void OnResize();

	virtual void Draw();

	int Run();

	virtual void Load();

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool Get4xMSAAState() const;
	void Set4xMSAAState(bool bState);

	ID3D12GraphicsCommandList4* GetGraphicsCommandList();
	ID3D12Device* GetDevice();

	static App* GetApp();

protected:
	bool InitWindow();
	bool InitDirectX3D();

	void FlushCommandQueue();
	
	bool CreateDescriptorHeaps();
	
	bool CreateStateObject();

	void AssociateShader(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc);

	void CreateHitGroup(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc, D3D12_HIT_GROUP_TYPE hitGroupType = D3D12_HIT_GROUP_TYPE_TRIANGLES);

	bool CompileShaders();

	void CreateGeometry();

	bool CreateAccelerationStructures();
	bool CreateTLAS(bool bUpdate);

	bool CreateShaderTables();
	bool CreateRayGenShaderTable();
	bool CreateMissShaderTable();
	bool CreateHitGroupShaderTable();

	bool CreateOutputBuffer();

	void CreateCBs();

	bool CreateSignatures();
	bool CreateLocalSignature();
	bool CreateGlobalSignature();

	void PopulateDescriptorHeaps();

	bool CheckRaytracingSupport();

	void CreateCameras();

	void InitScene();
	void InitConstantBuffers();

	void UpdatePerFrameCB(UINT uiFrameIndex);

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* pAdapter, bool bSaveSettings);
	void LogOutputInfo(IDXGIOutput* pOutput, DXGI_FORMAT format, bool bSaveSettings);

	void ExecuteCommandList();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	ID3D12Resource* GetBackBuffer() const;
	ID3D12Resource* GetBackBuffer(int iIndex) const;
	Microsoft::WRL::ComPtr<ID3D12Resource>* GetBackBufferComptr();
	Microsoft::WRL::ComPtr<ID3D12Resource>* GetBackBufferComptr(int iIndex);

	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView(UINT uiIndex) const;

	ID3D12CommandAllocator* GetCommandAllocator() const;
	ID3D12CommandAllocator* GetCommandAllocator(int iIndex) const;
	Microsoft::WRL::ComPtr <ID3D12CommandAllocator>* GetCommandAllocatorComptr();
	Microsoft::WRL::ComPtr <ID3D12CommandAllocator>* GetCommandAllocatorComptr(int iIndex);

	UploadBuffer<PrimitivePerFrameCB>* GetPrimitiveUploadBuffer();
	UploadBuffer<PrimitivePerFrameCB>* GetPrimitiveUploadBuffer(int iIndex);

	UploadBuffer<LightCB>* GetLightUploadBuffer();
	UploadBuffer<LightCB>* GetLightUploadBuffer(int iIndex);

	static App* m_pApp;

	Timer m_Timer;

	bool m_bPaused = false;
	bool m_bMinimized = false;
	bool m_bMaximized = false;
	bool m_bResizing = false;

	HINSTANCE m_AppInstance = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_pGraphicsCommandList = nullptr;

	AccelerationBuffers m_TopLevelBuffer;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pGlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pLocalRootSignature;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pRaytracingOutput;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_pDXGIFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;
	static const UINT s_kuiSwapChainBufferCount = 2;

	Descriptor* m_pOutputDesc;

	UINT m_uiFrameIndex = 0;

	UINT64 m_uiFenceValue = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue = nullptr;

	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pStateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_pStateObjectProps;

	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>> m_Shaders;

	DescriptorHeap* m_pSrvUavHeap = nullptr;
	DescriptorHeap* m_pRTVHeap = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pMissTable;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pHitGroupTable;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pRayGenTable;

	UINT m_uiMissRecordSize;
	UINT m_uiHitGroupRecordSize;
	UINT m_uiRayGenRecordSize;

	UploadBuffer<ScenePerFrameCB>* m_pScenePerFrameCBUpload = nullptr;

	std::vector<PrimitivePerFrameCB> m_PrimitivePerFrameCBs;
	std::vector<LightCB> m_LightCBs;

	ScenePerFrameCB m_PerFrameCBs[s_kuiSwapChainBufferCount];

	UINT m_uiNumLights = 0;

	//shader stuff
	LPCWSTR m_kwsRayGenName = L"RayGen";

	LPCWSTR m_kwsMissName = L"Miss";

	LPCWSTR m_kwsClosestHitEntrypoint = L"ClosestHit";

	LPCWSTR m_kwsClosestHitName = L"ClosestHit";
	LPCWSTR m_kwsClosestHitNameNormal = L"ClosestHitNormal";
	LPCWSTR m_kwsClosestHitNameOcclusion = L"ClosestHitOcclusion";
	LPCWSTR m_kwsClosestHitNameNormalOcclusion = L"ClosestHitNormalOcclusion";
	LPCWSTR m_kwsClosestHitNameNormalOcclusionEmission = L"ClosestHitNormalOcclusionEmission";

	LPCWSTR m_kwsHitGroupName = L"HitGroup";
	LPCWSTR m_kwsHitGroupNameNormal = L"HitGroupNormal";
	LPCWSTR m_kwsHitGroupNameOcclusion = L"HitGroupOcclusion";
	LPCWSTR m_kwsHitGroupNameNormalOcclusion = L"HitGroupNormalOcclusion";
	LPCWSTR m_kwsHitGroupNameNormalOcclusionEmission = L"HitGroupNormalOcclusionEmission";

	bool m_b4xMSAAState = false;
	UINT m_uiMSAAQuality = 0;

	DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> m_pDebug = nullptr;
#endif

private:
	FrameResources m_FrameResources[s_kuiSwapChainBufferCount];
};

