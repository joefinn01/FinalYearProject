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
class Texture;
class GIVolume;
class GameObject;

struct RayGenerationCB;
struct Vertex;
struct Primitive;

enum class PrimitiveAttributes : UINT8;

enum class GBuffer
{
	NORMAL = 0,
	ALBEDO,
	DIRECT_LIGHT,
	POSITION,

	COUNT
};

enum class TlasMask
{
	CONTRIBUTE_GI = 1,
	RAYTRACE = 2
};

DEFINE_ENUM_FLAG_OPERATORS(TlasMask);

struct FrameResources
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pRenderTarget = nullptr;

	UploadBuffer<GameObjectPerFrameCB>* m_pGameObjectPerFrameCBUpload = nullptr;
	UploadBuffer<LightCB>* m_pLightCBUpload = nullptr;
	UploadBuffer<DeferredPerFrameCB>* m_pDeferredPerFrameCBUpload = nullptr;

	Texture* m_GBuffer[(int)GBuffer::COUNT];

	Texture* m_pDepthStencilBuffer = nullptr;
	Descriptor* m_pDepthStencilBufferView;
};

namespace DeferredPass
{
	enum Value
	{
		STANDARD_DESCRIPTORS = 0,
		PER_FRAME_SCENE_CB,
		PRIMITIVE_INDEX_CB,
		COUNT
	};

	enum class ShaderVersions
	{
		NORMAL_OCCLUSION_EMISSION_ALBEDO_METALLIC_ROUGHNESS = 0,
		NORMAL_OCCLUSION_ALBEDO_METALLIC_ROUGHNESS,
		NORMAL_ALBEDO_METALLIC_ROUGHNESS,
		OCCLUSION_ALBEDO_METALLIC_ROUGHNESS,
		ALBEDO_METALLIC_ROUGHNESS,
		ALBEDO,
		NOTHING,

		COUNT
	};

	namespace LocalRootSignatureParams
	{
		enum Value
		{
			INDEX = 0,

			COUNT
		};
	}

	namespace GlobalRootSignatureParams
	{
		enum Value
		{
			STANDARD_DESCRIPTORS = 0,
			ACCELERATION_STRUCTURE,
			NORMAL,
			ALBEDO,
			DIRECT_LIGHT,
			POSITION,
			PER_FRAME_SCENE_CB,
			PER_FRAME_RAYTRACE_CB,

			COUNT
		};
	}

	namespace LightPass
	{
		namespace LightPassRootSignatureParams
		{
			enum Value
			{
				STANDARD_DESCRIPTORS = 0,
				PER_FRAME_SCENE_CB,
				PER_FRAME_DEFERRED_CB,
				PER_FRAME_RAYTRACE_CB,

				COUNT
			};
		}

		enum class ShaderVersions
		{
			SHOW_INDIRECT = 0,
			USE_GI,
			DIRECT,

			COUNT
		};
	}

}

class App
{
public:
	App(HINSTANCE hInstance);
	~App();

	virtual bool Init(const std::string& ksFilepath);

#if PIX
	static std::wstring GetPixGpuCapturePath();
#endif

	virtual void Update(const Timer& kTimer);
	virtual void OnResize();

	virtual void Draw();

	void DrawDeferredPass();
	void DrawGBufferPass();
	void DrawLightPass();

	int Run();

	virtual void Load();
	virtual void Save(const std::string& sFileName);

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool Get4xMSAAState() const;
	void Set4xMSAAState(bool bState);

	ID3D12GraphicsCommandList4* GetGraphicsCommandList();
	ID3D12Device5* GetDevice();

	static App* GetApp();

	const UINT GetNumStandardDescriptorRanges() const;
	const UINT GetNumUserDescriptorRanges() const;

	void ExecuteCommandList();
	void ResetCommandList();

	UINT GetFrameIndex() const;

protected:
	bool InitWindow();
	bool InitDirectX3D();

	void FlushCommandQueue();
	
	bool CreateDescriptorHeaps();

	bool CreatePSOs();

	void CreateInputDescs();

	void AssociateShader(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc);

	void CreateHitGroup(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc, D3D12_HIT_GROUP_TYPE hitGroupType = D3D12_HIT_GROUP_TYPE_TRIANGLES);

	bool CompileShaders();

	void CreateGeometry(const std::string& ksFilepath);

	bool CreateOutputBuffers();

	void CreateCBs();

	bool CreateSignatures();
	bool CreateGlobalRootSignature();
	bool CreateLocalRootSignature();
	bool CreateLightPassRootSignature();

	bool CreateStateObject();

	bool CreateShaderTables();
	bool CreateRayGenShaderTable();
	bool CreateMissShaderTable();
	bool CreateHitGroupShaderTable();

	bool CreateAccelerationStructures();
	bool CreateTLAS(bool bUpdate);

	void PopulateDescriptorHeaps();
	void PopulatePrimitivePerInstanceCB();
	void PopulateDeferredPerFrameCB();

	bool CheckRaytracingSupport();

	void CreateCameras();

	void CreateScreenQuad();

	void InitScene(const std::string& ksFilepath);
	void InitConstantBuffers(const std::string& ksFilepath);

	void InitImGui();
	void UpdateImGui(const Timer& kTimer);
	void DrawImGui();
	void ShutdownImGui();

	void UpdatePerFrameCB(UINT uiFrameIndex);

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* pAdapter, bool bSaveSettings);
	void LogOutputInfo(IDXGIOutput* pOutput, DXGI_FORMAT format, bool bSaveSettings);

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

	UploadBuffer<GameObjectPerFrameCB>* GetPrimitiveUploadBuffer();
	UploadBuffer<GameObjectPerFrameCB>* GetPrimitiveUploadBuffer(int iIndex);

	UploadBuffer<LightCB>* GetLightUploadBuffer();
	UploadBuffer<LightCB>* GetLightUploadBuffer(int iIndex);

	UploadBuffer<PrimitiveIndexCB>* GetPrimitiveIndexUploadBuffer();
	UploadBuffer<PrimitiveIndexCB>* GetPrimitiveIndexUploadBuffer(int iIndex);

	UploadBuffer<DeferredPerFrameCB>* GetDeferredPerFrameUploadBuffer();
	UploadBuffer<DeferredPerFrameCB>* GetDeferredPerFrameUploadBuffer(int iIndex);

	Texture** GetGBuffer();
	Texture** GetGBuffer(int iIndex);

	Texture* GetDepthStencilBuffer();
	Texture* GetDepthStencilBuffer(int iIndex);
	void SetDepthStencilBuffer(int iIndex, Texture* pTexture);

	Descriptor* GetDepthStencilBufferView();
	Descriptor* GetDepthStencilBufferView(int iIndex);
	void SetDepthStencilBufferView(int iIndex, Descriptor* pDesc);

	static App* m_pApp;

	Timer m_Timer;

	bool m_bPaused = false;
	bool m_bMinimized = false;
	bool m_bMaximized = false;
	bool m_bResizing = false;

	HINSTANCE m_AppInstance = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_pGraphicsCommandList = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pGlobalRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pLocalRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pLightPassSignature = nullptr;

	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pStateObject = nullptr;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_pStateObjectProps = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pMissTable = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pHitGroupTable = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pRayGenTable = nullptr;

	AccelerationBuffers m_TopLevelBuffer;

	UINT m_uiMissRecordSize;
	UINT m_uiHitGroupRecordSize;
	UINT m_uiRayGenRecordSize;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pLightPassPSOs[(int)DeferredPass::LightPass::ShaderVersions::COUNT] = { nullptr };

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_pDXGIFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;
	static const UINT s_kuiSwapChainBufferCount = 2;

	UINT m_uiFrameIndex = 0;

	UINT64 m_uiFenceValue = 0;

	static const UINT32 s_kuiNumUserDescriptorRanges = 16;
	static const UINT32 s_kuiNumStandardDescriptorRanges = 4 + s_kuiNumUserDescriptorRanges;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue = nullptr;

	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>> m_Shaders;

	DescriptorHeap* m_pSRVHeap = nullptr;
	DescriptorHeap* m_pImGuiSRVHeap = nullptr;
	DescriptorHeap* m_pRTVHeap = nullptr;
	DescriptorHeap* m_pDSVHeap = nullptr;

	UploadBuffer<ScenePerFrameCB>* m_pScenePerFrameCBUpload = nullptr;
	UploadBuffer<PrimitiveInstanceCB>* m_pPrimitiveInstanceCBUpload = nullptr;

	std::vector<GameObjectPerFrameCB> m_GameObjectPerFrameCBs;
	std::vector<LightCB> m_LightCBs;

	ScenePerFrameCB m_PerFrameCBs[s_kuiSwapChainBufferCount];

	UINT m_uiNumLights = 2;

	//G Buffer pass shader names
	LPCWSTR m_wsGBufferRayGenName = L"GBufferRayGen";
	LPCWSTR m_wsGBufferMissName = L"Miss";

	LPCWSTR m_GBufferHitNames[(int)DeferredPass::ShaderVersions::COUNT] =
	{
		L"ClosestHitNormalOcclusionEmissionAlbedoMetallicRoughness",
		L"ClosestHitNormalOcclusionAlbedoMetallicRoughness",
		L"ClosestHitNormalAlbedoMetallicRoughness",
		L"ClosestHitOcclusionAlbedoMetallicRoughness",
		L"ClosestHitAlbedoMetallicRoughness",
		L"ClosestHitAlbedo",
		L"ClosestHit",
	};

	LPCWSTR m_GBufferHitGroupNames[(int)DeferredPass::ShaderVersions::COUNT] =
	{
		L"HitGroupNormalOcclusionEmissionAlbedoMetallicRoughness",
		L"HitGroupNormalOcclusionAlbedoMetallicRoughness",
		L"HitGroupNormalAlbedoMetallicRoughness",
		L"HitGroupOcclusionAlbedoMetallicRoughness",
		L"HitGroupAlbedoMetallicRoughness",
		L"HitGroupAlbedo",
		L"HitGroup",
	};

	LPCWSTR m_wsLightPassVertexName = L"LightPassVertex";
	LPCWSTR m_wsLightPassPixelNames[(int)DeferredPass::LightPass::ShaderVersions::COUNT] = 
	{
		L"LightPassPixelIndirect",
		L"LightPassGI",
		L"LightPassDirect",
	};

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_ScreenQuadInputDesc;
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_DefaultInputDesc;

	bool m_b4xMSAAState = false;
	UINT m_uiMSAAQuality = 0;

	DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_R24G8_TYPELESS;
	DXGI_FORMAT m_DepthStencilSRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	DXGI_FORMAT m_DepthStencilDSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	DXGI_FORMAT m_GBufferFormats[(int)GBuffer::COUNT] =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
	};

	float m_GBufferClearColors[(int)GBuffer::COUNT][4] =
	{
		{0, 0, 0, 1},
		{0, 0, 0, 1},
		{0.5f, 0.5f, 1, 0},
		{0.0f, 0.0f, 0.0f, 0.0f},
	};

	float m_fCameraNearDepth = 0.1f;
	float m_fCameraFarDepth = 1000.0f;

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenQuadVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenQuadVertexBufferUploader = nullptr;

	GameObject* m_pLight = nullptr;

	GIVolume* m_pGIVolume = nullptr;

	bool m_bShowIndirect = false;
	bool m_bUseGI = true;

#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> m_pDebug = nullptr;
#endif

private:
	FrameResources m_FrameResources[s_kuiSwapChainBufferCount];
};

