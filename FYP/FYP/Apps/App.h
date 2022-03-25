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

struct RayGenerationCB;
struct Vertex;
struct Primitive;

enum class PrimitiveAttributes : UINT8;

enum class GBuffer
{
	NORMAL = 0,
	ALBEDO,
	METALLIC_ROUGHNESS_OCCLUSION,
	COUNT
};

struct FrameResources
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pRenderTarget = nullptr;

	UploadBuffer<GameObjectPerFrameCB>* m_pGameObjectPerFrameCBUpload = nullptr;
	UploadBuffer<LightCB>* m_pLightCBUpload = nullptr;
	UploadBuffer<PrimitiveIndexCB>* m_pPrimitiveIndexCBUpload = nullptr;
	UploadBuffer<DeferredPerFrameCB>* m_pDeferredPerFrameCBUpload = nullptr;

	Texture* m_GBuffer[(int)GBuffer::COUNT];
	Descriptor* m_pGBufferRTVDescs[(int)GBuffer::COUNT];

	Texture* m_pDepthStencilBuffer = nullptr;
	Descriptor* m_pDepthStencilBufferView;
};

namespace DeferredPass
{
	namespace GBufferPass
	{
		enum Value
		{
			STANDARD_DESCRIPTORS = 0,
			PER_FRAME_SCENE_CB,
			PRIMITIVE_INDEX_CB,
			COUNT
		};
	}

	namespace LightPass
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
}

enum class ShaderVersions
{
	NOTHING = 0,
	ALBEDO,
	METALLIC_ROUGHNESS,
	NORMAL,
	OCCLUSION,
	NORMAL_OCCLUSION,
	NORMAL_OCCLUSION_EMISSION,
	COUNT
};

struct RenderInfo
{
	Primitive* m_pPrimitive;
	UploadBuffer<Vertex>* m_pVertexBuffer;
	UploadBuffer<UINT>* m_pIndexBuffer;
	UINT m_uiInstanceIndex;
	UINT m_uiPrimitiveIndex;
};

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

	void DrawDeferredPass();
	void DrawGBufferPass();
	void DrawLightPass();

	int Run();

	virtual void Load();

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
	bool CreateGBufferPSO();
	bool CreateLightPSO();

	void CreateInputDescs();

	void AssociateShader(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc);

	void CreateHitGroup(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc, D3D12_HIT_GROUP_TYPE hitGroupType = D3D12_HIT_GROUP_TYPE_TRIANGLES);

	bool CompileShaders();

	void CreateGeometry();

	bool CreateOutputBuffers();

	void CreateCBs();

	bool CreateSignatures();

	bool CreateGBufferSignature();
	bool CreateLightSignature();

	void PopulateDescriptorHeaps();
	void PopulatePrimitivePerInstanceCB();
	void PopulatePrimitiveIndexCB();
	void PopulateDeferredPerFrameCB();

	void PopulateRenderInfoQueue();

	bool CheckRaytracingSupport();

	void CreateCameras();

	void CreateScreenQuad();

	void InitScene();
	void InitConstantBuffers();

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

	Descriptor** GetGBufferRTVDescs();
	Descriptor** GetGBufferRTVDescs(int iIndex);

	static App* m_pApp;

	Timer m_Timer;

	bool m_bPaused = false;
	bool m_bMinimized = false;
	bool m_bMaximized = false;
	bool m_bResizing = false;

	HINSTANCE m_AppInstance = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_pGraphicsCommandList = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pGlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pLocalRootSignature;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pGBufferRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pLightRootSignature;

	Texture* m_pRaytracingOutput;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_pDXGIFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;
	static const UINT s_kuiSwapChainBufferCount = 2;

	UINT m_uiFrameIndex = 0;

	UINT64 m_uiFenceValue = 0;

	static const UINT32 s_kuiNumUserDescriptorRanges = 16;
	static const UINT32 s_kuiNumStandardDescriptorRanges = 4 + s_kuiNumUserDescriptorRanges;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue = nullptr;

	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pStateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_pStateObjectProps;

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

	UINT m_uiNumLights = 0;

	//Light pass shader names
	LPCWSTR m_wsLightVertexName = L"LightVertex";
	LPCWSTR m_wsLightPixelName = L"LightPixel";

	//G Buffer pass shader names
	LPCWSTR m_wsGBufferVertexName = L"GBufferVertex";
	LPCWSTR m_GBufferPixelNames[(int)ShaderVersions::COUNT] =
	{
		L"GBufferPixel",
		L"GBufferPixelAlbedo",
		L"GBufferPixelNoMetallicRoughness",
		L"GBufferPixelNormal",
		L"GBufferPixelOcclusion",
		L"GBufferPixelNormalOcclusion",
		L"GBufferPixelNormalOcclusionEmission",
	};


	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pGBufferPSOs[(int)ShaderVersions::COUNT] = { };
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pLightPSO = nullptr;

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
	};

	float m_GBufferClearColors[(int)GBuffer::COUNT][4] =
	{
		{0, 0, 0, 1},
		{0, 0, 0, 1},
		{0.5f, 0.5f, 1, 0},
	};

	float m_fCameraNearDepth = 0.1f;
	float m_fCameraFarDepth = 1000.0f;

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	std::unordered_map<PrimitiveAttributes, std::vector<RenderInfo>> m_RenderInfoQueue;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenQuadVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenQuadVertexBufferUploader = nullptr;

	GIVolume* m_pGIVolume = nullptr;

#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> m_pDebug = nullptr;
#endif

private:
	FrameResources m_FrameResources[s_kuiSwapChainBufferCount];
};

