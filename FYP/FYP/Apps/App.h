#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_4.h>

#include "Commons/Singleton.h"
#include "Commons/Timer.h"
#include "Commons/UploadBuffer.h"
#include "Include/nv_helpers_dx12/ShaderBindingTableGenerator.h"

#include <unordered_map>
#include <dxcapi.h>


class Timer;

struct RayGenerationCB;

struct ShaderInfo
{
	Microsoft::WRL::ComPtr<IDxcBlob> m_pBlob;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
};

struct Vertex;

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
	void CreateStateObject();

	void CreateGeometry();

	bool CreateAccelerationStructures();

	bool CreateShaderTables();

	bool CreateOutputBuffer();

	void CreateCBUploadBuffers();

	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRayGenSignature();
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateHitSignature();
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateMissSignature();

	void PopulateDescriptorHeaps();

	bool CheckRaytracingSupport();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* pAdapter, bool bSaveSettings);
	void LogOutputInfo(IDXGIOutput* pOutput, DXGI_FORMAT format, bool bSaveSettings);

	void ExecuteCommandList();

	ID3D12Resource* GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;

	static App* m_pApp;

	Timer m_Timer;

	bool m_bPaused = false;
	bool m_bMinimized = false;
	bool m_bMaximized = false;
	bool m_bResizing = false;

	HINSTANCE m_AppInstance = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device5> m_pDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_pGraphicsCommandList = nullptr;

	// Acceleration structure
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pAccelerationStructure;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pBottomLevelAccelerationStructure;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pTopLevelAccelerationStructure;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pRaytracingOutput;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_pDXGIFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;
	static const UINT s_kuiSwapChainBufferCount = 2;

	int m_iBackBufferIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pSwapChainBuffer[s_kuiSwapChainBufferCount];

	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pStateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_pStateObjectProps;

	std::unordered_map<std::wstring, ShaderInfo> m_Shaders;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pSrvUavHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRTVHeap = nullptr;

	UINT m_uiRtvDescriptorSize;
	UINT m_uiSrvUavDescriptorSize;

	nv_helpers_dx12::ShaderBindingTableGenerator m_SBTHelper;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pSBTBuffer;

	UploadBuffer<Vertex>* m_pTriVertices = nullptr;
	UploadBuffer<UINT16>* m_pTriIndices = nullptr;

	UploadBuffer<RayGenerationCB>* m_pRayGenCB = nullptr;

	const std::wstring m_kwsRayGenName = L"RayGen";
	const std::wstring m_kwsClosestHitName = L"ClosestHit";
	const std::wstring m_kwsMissGenName = L"Miss";

	bool m_b4xMSAAState = false;
	UINT m_uiMSAAQuality = 0;

	UINT64 m_uiCurrentFence = 0;

	DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> m_pDebug = nullptr;
#endif

private:

};

