#include "App.h"
#include "Commons/Timer.h"
#include "Shaders/ConstantBuffers.h"
#include "Commons/Vertices.h"
#include "Commons/ShaderTable.h"
#include "Helpers/DebugHelper.h"
#include "Helpers/MathHelper.h"
#include "Helpers/DXRHelper.h"
#include "Managers/WindowManager.h"

#if PIX
#include "pix3.h"

#include <shlobj.h>
#include <strsafe.h>
#endif

#include <vector>

using namespace Microsoft::WRL;
using namespace DirectX;

App* App::m_pApp = nullptr;

Tag tag = L"App";

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return App::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

App::App(HINSTANCE hInstance)
{
	if (m_pApp != nullptr)
	{
		LOG_ERROR(tag, L"Tried to create new app when one already exists!");

		return;
	}

	m_AppInstance = hInstance;

	m_pApp = this;
}

App::~App()
{
}

bool App::Init()
{
	UINT uiDXGIFactoryFlags = 0;
	HRESULT hr;

#if PIX
	// Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
	// This may happen if the application is launched through the PIX UI. 
	if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
	{
		LoadLibrary(GetPixGpuCapturePath().c_str());
	}
#endif

#if _DEBUG //&& !PIX
	//Create debug controller
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(m_pDebug.GetAddressOf()));

	ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> pDredSettings;

	hr = D3D12GetDebugInterface(IID_PPV_ARGS(pDredSettings.GetAddressOf()));

	pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	pDredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create Debug controller!");

		uiDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}

	m_pDebug->EnableDebugLayer();
#endif

	//Create the DXGI factory and output all information related to adapters.
	hr = CreateDXGIFactory2(uiDXGIFactoryFlags, IID_PPV_ARGS(m_pDXGIFactory.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create DXGIFactory!");

		return false;
	}

	//Log adapters before initialising anything else so we can get the correct window dimensions and refresh rate
	LogAdapters();

	if (InitWindow() == false)
	{
		return false;
	}

	if (InitDirectX3D() == false)
	{
		return false;
	}

	if (CheckRaytracingSupport() == false)
	{
		return false;
	}

	if (CreateDescriptorHeaps() == false)
	{
		return false;
	}

	if (CompileShaders() == false)
	{
		return false;
	}

	if (CreateGlobalSignature() == false)
	{
		return false;
	}

	if (CreateStateObject() == false)
	{
		return false;
	}

	CreateGeometry();

	if (CreateAccelerationStructures() == false)
	{
		return false;
	}

	if (CreateOutputBuffer() == false)
	{
		return false;
	}

	CreateCBUploadBuffers();

	if (CreateShaderTables() == false)
	{
		return false;
	}

	PopulateDescriptorHeaps();

	OnResize();

	Load();

	return true;
}

#if PIX
std::wstring App::GetPixGpuCapturePath()
{
	LPWSTR programFilesPath = nullptr;
	SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

	std::wstring pixSearchPath = programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

	WIN32_FIND_DATA findData;
	bool foundPixInstallation = false;
	wchar_t newestVersionFound[MAX_PATH];

	HANDLE hFind = FindFirstFile(pixSearchPath.c_str(), &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
				(findData.cFileName[0] != '.'))
			{
				if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
				{
					foundPixInstallation = true;
					StringCchCopy(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
				}
			}
		} while (FindNextFile(hFind, &findData) != 0);
	}

	FindClose(hFind);

	if (!foundPixInstallation)
	{
		// TODO: Error, no PIX installation found
	}

	wchar_t output[MAX_PATH];
	StringCchCopy(output, pixSearchPath.length(), pixSearchPath.data());
	StringCchCat(output, MAX_PATH, &newestVersionFound[0]);
	StringCchCat(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

	return &output[0];
}
#endif

void App::Update(const Timer& kTimer)
{

}

void App::OnResize()
{
	FlushCommandQueue();

	HRESULT hr = m_pGraphicsCommandList->Reset(GetCommandAllocator(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the graphics command list!");

		return;
	}

	// Release the previous resources we will be recreating.
	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		GetBackBufferComptr(i)->Reset();
	}

	// Resize the swap chain.
	hr = m_pSwapChain->ResizeBuffers(s_kuiSwapChainBufferCount, WindowManager::GetInstance()->GetWindowWidth(), WindowManager::GetInstance()->GetWindowHeight(), m_BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	m_uiFrameIndex = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < s_kuiSwapChainBufferCount; i++)
	{
		hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(GetBackBufferComptr(i)->GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to get swap chain buffer at index %u!", i);

			return;
		}

		m_pDevice->CreateRenderTargetView(GetBackBuffer(i), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_uiRtvDescriptorSize);
	}

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to Create albedo committed resource when resizing screen!");

		return;
	}

	m_pRaytracingOutput.Reset();

	CreateOutputBuffer();

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	m_pDevice->CreateUnorderedAccessView(m_pRaytracingOutput.Get(), nullptr, &uavDesc, GetSrvUavDescriptorHandleCPU(GlobalRootSignatureParams::OUTPUT));

	// Execute the resize commands.
	hr = m_pGraphicsCommandList->Close();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to close the command list!");

		return;
	}

	ID3D12CommandList* cmdsLists[] = { m_pGraphicsCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();
}

void App::Draw()
{
	HRESULT hr = GetCommandAllocator()->Reset();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the command allocator!");

		return;
	}

	hr = m_pGraphicsCommandList->Reset(GetCommandAllocator(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the command list!");

		return;
	}

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRaytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	std::vector<ID3D12DescriptorHeap*> heaps = { m_pSrvUavHeap.Get() };
	m_pGraphicsCommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchDesc.RayGenerationShaderRecord.StartAddress = m_pRayGenTable->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_pRayGenTable->GetDesc().Width;
	dispatchDesc.MissShaderTable.StartAddress = m_pMissTable->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.StrideInBytes = m_uiMissRecordSize;
	dispatchDesc.MissShaderTable.SizeInBytes = m_pMissTable->GetDesc().Width;
	dispatchDesc.HitGroupTable.StartAddress = m_pHitGroupTable->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.StrideInBytes = m_uiHitGroupRecordSize;
	dispatchDesc.HitGroupTable.SizeInBytes = m_pHitGroupTable->GetDesc().Width;
	dispatchDesc.Width = WindowManager::GetInstance()->GetWindowWidth();
	dispatchDesc.Height = WindowManager::GetInstance()->GetWindowHeight();
	dispatchDesc.Depth = 1;
	
	m_pGraphicsCommandList->SetComputeRootSignature(m_pGlobalRootSignature.Get());

	m_pGraphicsCommandList->SetComputeRootDescriptorTable(0, GetSrvUavDescriptorHandleGPU(GlobalRootSignatureParams::OUTPUT));

	m_pGraphicsCommandList->SetComputeRootDescriptorTable(1, GetSrvUavDescriptorHandleGPU(GlobalRootSignatureParams::ACCELERATION_STRUCTURE));

	m_pGraphicsCommandList->SetComputeRootConstantBufferView(2, m_pRayGenCB->Get()->GetGPUVirtualAddress());

	m_pGraphicsCommandList->SetPipelineState1(m_pStateObject.Get());

	m_pGraphicsCommandList->DispatchRays(&dispatchDesc);

	//Copy to back buffer
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRaytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST));

	m_pGraphicsCommandList->CopyResource(GetBackBuffer(), m_pRaytracingOutput.Get());

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));

	hr = m_pGraphicsCommandList->Close();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to close the graphics command list!");

		return;
	}

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_pGraphicsCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	hr = m_pSwapChain->Present(0, 0);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to present the current frame!");

		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
			m_pDevice->QueryInterface(IID_PPV_ARGS(&pDred));

			D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput;
			D3D12_DRED_PAGE_FAULT_OUTPUT DredPageFaultOutput;
			pDred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput);
			pDred->GetPageFaultAllocationOutput(&DredPageFaultOutput);
		}

		return;
	}

	m_uiFrameIndex = (m_uiFrameIndex + 1) % s_kuiSwapChainBufferCount;

	FlushCommandQueue();
}

int App::Run()
{
	MSG msg = { 0 };

	m_Timer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			m_Timer.Tick();

			if (!m_bPaused)
			{
				Update(m_Timer);
				Draw();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

void App::Load()
{
}

LRESULT App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			m_bPaused = true;
			m_Timer.Stop();
		}
		else
		{
			m_bPaused = false;
			m_Timer.Start();
		}

		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
	{
		// Save the new client area dimensions.
		WindowManager::GetInstance()->UpdateWindowDimensions();
		if (m_pDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				m_bPaused = true;
				m_bMinimized = true;
				m_bMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_bPaused = false;
				m_bMinimized = false;
				m_bMaximized = true;

				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (m_bMinimized)
				{
					m_bPaused = false;
					m_bMinimized = false;

					OnResize();
				}

				// Restoring from maximized state?
				else if (m_bMaximized)
				{
					m_bPaused = false;
					m_bMaximized = false;

					OnResize();
				}
				else if (m_bResizing)
				{
					//don't do anything while resizing
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}

		return 0;
	}

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		m_bPaused = true;
		m_bResizing = true;
		m_Timer.Stop();

		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		m_bPaused = false;
		m_bResizing = false;
		m_Timer.Start();

		OnResize();

		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;

		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		return 0;

	case WM_MOUSEMOVE:
		return 0;

	case WM_MOUSEWHEEL:
		return 0;

	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
		{
			Set4xMSAAState(!m_b4xMSAAState);
		}
		return 0;

	case WM_KEYDOWN:
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool App::Get4xMSAAState() const
{
	return m_b4xMSAAState;
}

void App::Set4xMSAAState(bool bState)
{
	m_b4xMSAAState = bState;
}

ID3D12GraphicsCommandList4* App::GetGraphicsCommandList()
{
	return m_pGraphicsCommandList.Get();
}

ID3D12Device* App::GetDevice()
{
	return m_pDevice.Get();
}

App* App::GetApp()
{
	return m_pApp;
}

bool App::InitWindow()
{
	WNDCLASS windowClass;
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = MainWndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = m_AppInstance;
	windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = L"Main Window";

	if (RegisterClass(&windowClass) == false)
	{
		MessageBox(0, L"RegisterClass Failed!", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, static_cast<long>(WindowManager::GetInstance()->GetWindowWidth()), static_cast<long>(WindowManager::GetInstance()->GetWindowHeight()) };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);

	int iWidth = R.right - R.left;
	int iHeight = R.bottom - R.top;

	UINT dpi = GetDpiForWindow(GetDesktopWindow());
	float fDPIScale = dpi / 96.0f;

	HWND window = CreateWindow(L"Main Window",
		L"DirectX App",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(int)(iWidth / fDPIScale),
		(int)(iHeight / fDPIScale),
		0,
		0,
		m_AppInstance,
		0);

	if (window == 0)
	{
		MessageBox(0, L"CreateWindow Failed!", 0, 0);
		return false;
	}

	ShowWindow(window, SW_SHOW);
	UpdateWindow(window);

	WindowManager::GetInstance()->SetHWND(window);

	WindowManager::GetInstance()->UpdateWindowDimensions();

	return true;
}

bool App::InitDirectX3D()
{
	//Create device
	 HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.GetAddressOf()));

	if (FAILED(hr))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
		hr = m_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(m_pDXGIFactory.GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to enum warp adapters!");

			return false;
		}

		hr = D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pDevice));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to create device using hardware and WARP adaptor!");

			return false;
		}
	}

	//Create fence for synchronizing CPU and GPU
	hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create fence!");

		return false;
	}

	// Check 4X MSAA quality support for our back buffer format.
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = m_BackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to get multisample quality level!");

		return false;
	}

	m_uiMSAAQuality = msQualityLevels.NumQualityLevels;

	//Create command objects

	//Create command list and queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	hr = m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the command queue!");

		return false;
	}

	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(GetCommandAllocatorComptr(i)->GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to create the command allocator!");

			return false;
		}
	}

	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, GetCommandAllocator(), nullptr, IID_PPV_ARGS(m_pGraphicsCommandList.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the graphics command list!");

		return false;
	}

	//Close the command list so it can be reset
	m_pGraphicsCommandList->Close();

	//Create the swap chain
	m_pSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC1 chainDesc = {};
	chainDesc.BufferCount = s_kuiSwapChainBufferCount;
	chainDesc.Width = WindowManager::GetInstance()->GetWindowWidth();
	chainDesc.Height = WindowManager::GetInstance()->GetWindowHeight();
	chainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	chainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	chainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	chainDesc.SampleDesc.Count = 1;

	hr = m_pDXGIFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), WindowManager::GetInstance()->GetHWND(), &chainDesc, nullptr, nullptr, m_pSwapChain.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the swap chain!");

		return false;
	}

	//Create RTV heap
	D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
	RTVHeapDesc.NumDescriptors = s_kuiSwapChainBufferCount;
	RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RTVHeapDesc.NodeMask = 0;

	hr = m_pDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the RTV heap!");

		return false;
	}

	m_uiRtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	return true;
}

void App::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point
	IncrementFenceValue();

	// Add an instruction to the command queue to set a new fence point
	HRESULT hr = m_pCommandQueue->Signal(m_pFence.Get(), GetFenceValue());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create a new fence point!");

		return;
	}

	// Wait until the GPU has completed commands up to this fence point.
	if (m_pFence->GetCompletedValue() < GetFenceValue())
	{
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

		if (eventHandle == 0)
		{
			LOG_ERROR(tag, L"Failed to create event when fence point it hit!");

			return;
		}

		// Fire event when GPU hits current fence.  
		hr = m_pFence->SetEventOnCompletion(GetFenceValue(), eventHandle);

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to set event for when fence point it hit!");

			return;
		}

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

bool App::CreateStateObject()
{
	CD3DX12_STATE_OBJECT_DESC pipelineDesc = CD3DX12_STATE_OBJECT_DESC(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	//Associate shaders with pipeline

	IDxcBlob* pBlob = m_Shaders[m_kwsRayGenName].Get();

	CD3DX12_DXIL_LIBRARY_SUBOBJECT* pRayGenLib = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	pRayGenLib->SetDXILLibrary(&CD3DX12_SHADER_BYTECODE((void*)pBlob->GetBufferPointer(), pBlob->GetBufferSize()));
	pRayGenLib->DefineExport(m_kwsRayGenName);

	pBlob = m_Shaders[m_kwsMissName].Get();
	CD3DX12_DXIL_LIBRARY_SUBOBJECT* pMissLib = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	pMissLib->SetDXILLibrary(&CD3DX12_SHADER_BYTECODE((void*)pBlob->GetBufferPointer(), pBlob->GetBufferSize()));
	pMissLib->DefineExport(m_kwsMissName);

	pBlob = m_Shaders[m_kwsClosestHitName].Get();
	CD3DX12_DXIL_LIBRARY_SUBOBJECT* pClosestHitLib = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	pClosestHitLib->SetDXILLibrary(&CD3DX12_SHADER_BYTECODE((void*)pBlob->GetBufferPointer(), pBlob->GetBufferSize()));
	pClosestHitLib->DefineExport(m_kwsClosestHitName);

	//Add a hit group
	CD3DX12_HIT_GROUP_SUBOBJECT* pHitGroup = pipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	pHitGroup->SetClosestHitShaderImport(m_kwsClosestHitName);
	pHitGroup->SetHitGroupExport(m_kwsHitGroupName);
	pHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	//Do shader config stuff
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* pShaderConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	pShaderConfig->Config(sizeof(XMFLOAT4), sizeof(XMFLOAT2));

	//Set root signatures

	//Local

	//Global
	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* pGlobalRootSignature = pipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	pGlobalRootSignature->SetRootSignature(m_pGlobalRootSignature.Get());

	//Do pipeline config
	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pPipelineConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	pPipelineConfig->Config(1);

	HRESULT hr = m_pDevice->CreateStateObject(pipelineDesc, IID_PPV_ARGS(&m_pStateObject));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the raytracing pipeline object!");

		return false;
	}

	hr = m_pStateObject.As(&m_pStateObjectProps);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to get raytracing pipeline properties from object!");

		return false;
	}

	return true;
}

bool App::CompileShaders()
{
	ComPtr<IDxcBlob> pBlob;

	//Compile ray gen shaders
	pBlob = DXRHelper::CompileShader(L"Shaders/RayGen.hlsl");

	if (pBlob == nullptr)
	{
		return false;
	}

	m_Shaders[m_kwsRayGenName] = pBlob.Get();

	//Compile miss shaders
	pBlob = DXRHelper::CompileShader(L"Shaders/Miss.hlsl");

	if (pBlob == nullptr)
	{
		return false;
	}

	m_Shaders[m_kwsMissName] = pBlob.Get();

	//Compile closest hit shaders
	pBlob = DXRHelper::CompileShader(L"Shaders/Hit.hlsl");

	if (pBlob == nullptr)
	{
		return false;
	}

	m_Shaders[m_kwsClosestHitName] = pBlob.Get();

	return true;
}

void App::CreateGeometry()
{
	//Create index buffer
	UINT16 indices[] =
	{
		0, 1, 2
	};

	m_pTriIndices = new UploadBuffer<UINT16>(m_pDevice.Get(), sizeof(indices) / sizeof(UINT16), false);
	
	for (int i = 0; i < sizeof(indices) / sizeof(UINT16); ++i)
	{
		m_pTriIndices->CopyData(i, indices[i]);
	}

	//Create vertex buffer
	float fDepthValue = 1.0;
	float fOffset = 0.7f;

	Vertex vertices[]
	{
		Vertex(XMFLOAT3(0, -fOffset, fDepthValue)),
		Vertex(XMFLOAT3(-fOffset, fOffset, fDepthValue)),
		Vertex(XMFLOAT3(fOffset, fOffset, fDepthValue))
	};

	m_pTriVertices = new UploadBuffer<Vertex>(m_pDevice.Get(), sizeof(vertices) / sizeof(Vertex), false);

	for (int i = 0; i < sizeof(vertices) / sizeof(Vertex); ++i)
	{
		m_pTriVertices->CopyData(i, vertices[i]);
	}
}

bool App::CreateAccelerationStructures()
{
	HRESULT hr = m_pGraphicsCommandList->Reset(GetCommandAllocator(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the graphics command list!");

		return false;
	}

	//Create geometry description
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.IndexBuffer = m_pTriIndices->Get()->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = (UINT)m_pTriIndices->Get()->GetDesc().Width / sizeof(UINT16);
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.VertexCount = (UINT)m_pTriVertices->Get()->GetDesc().Width / sizeof(Vertex);
	geometryDesc.Triangles.VertexBuffer.StartAddress = m_pTriVertices->Get()->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

	//Set as opaque to allow for more optimization
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	//Get prebuild info so can estimate size of TLAS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topInputs = {};
	topInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topInputs.Flags = buildFlags;
	topInputs.NumDescs = 1;
	topInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topPrebuildInfo = {};
	m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topInputs, &topPrebuildInfo);

	if (topPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
	{
		LOG_ERROR(tag, L"Prebuilt info data max size for TLAS is less than zero!");

		return false;
	}

	//Get prebuild info so can estimate size of BLAS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomPrebuildInfo = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomInputs = topInputs;
	bottomInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomInputs.pGeometryDescs = &geometryDesc;
	m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomInputs, &bottomPrebuildInfo);
	
	if (bottomPrebuildInfo.ResultDataMaxSizeInBytes <= 0)
	{
		LOG_ERROR(tag, L"Prebuilt info data max size for BLAS is less than zero!");

		return false;
	}

	//Create upload buffer for acceleration structure
	ComPtr<ID3D12Resource> pScratchBuffer;

	DXRHelper::CreateUAVBuffer(m_pDevice.Get(), max(topPrebuildInfo.ScratchDataSizeInBytes, bottomPrebuildInfo.ScratchDataSizeInBytes), pScratchBuffer.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	//Create BLAS and TLAS UAV buffers
	DXRHelper::CreateUAVBuffer(m_pDevice.Get(), topPrebuildInfo.ResultDataMaxSizeInBytes, m_pTopLevelAccelerationStructure.GetAddressOf(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	DXRHelper::CreateUAVBuffer(m_pDevice.Get(), bottomPrebuildInfo.ScratchDataSizeInBytes, m_pBottomLevelAccelerationStructure.GetAddressOf(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	//Create instance description
	UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC>* pInstanceDescsUploadBuffer = new UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC>(m_pDevice.Get(), 1, false);

	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = 1;
	instanceDesc.Transform[1][1] = 1;
	instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	instanceDesc.AccelerationStructure = m_pBottomLevelAccelerationStructure->GetGPUVirtualAddress();

	pInstanceDescsUploadBuffer->CopyData(0, instanceDesc);

	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topBuildDesc = {};
	{
		topInputs.InstanceDescs = pInstanceDescsUploadBuffer->Get()->GetGPUVirtualAddress();
		topBuildDesc.Inputs = topInputs;
		topBuildDesc.DestAccelerationStructureData = m_pTopLevelAccelerationStructure->GetGPUVirtualAddress();
		topBuildDesc.ScratchAccelerationStructureData = pScratchBuffer->GetGPUVirtualAddress();
	}

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomBuildDesc = {};
	{
		bottomBuildDesc.Inputs = bottomInputs;
		bottomBuildDesc.ScratchAccelerationStructureData = pScratchBuffer->GetGPUVirtualAddress();
		bottomBuildDesc.DestAccelerationStructureData = m_pBottomLevelAccelerationStructure->GetGPUVirtualAddress();
	}

	//Create acceleration structures
	m_pGraphicsCommandList->BuildRaytracingAccelerationStructure(&bottomBuildDesc, 0, nullptr);
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_pBottomLevelAccelerationStructure.Get()));
	m_pGraphicsCommandList->BuildRaytracingAccelerationStructure(&topBuildDesc, 0, nullptr);

	ExecuteCommandList();

	return true;
}

bool App::CreateShaderTables()
{
	void* pRayGenIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_kwsRayGenName);
	void* pMissIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_kwsMissName);
	void* pHitGroupIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_kwsHitGroupName);

	//Create ray gen shader table
	ShaderTable rayGenTable = ShaderTable(m_pDevice.Get(), 1, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	if (rayGenTable.AddRecord(ShaderRecord(nullptr, 0, pRayGenIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
	{
		return false;
	}

	m_pRayGenTable = rayGenTable.GetBuffer();
	m_uiRayGenRecordSize = rayGenTable.GetRecordSize();

	//Create miss shader table
	ShaderTable missTable = ShaderTable(m_pDevice.Get(), 1, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	if (missTable.AddRecord(ShaderRecord(nullptr, 0, pMissIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
	{
		return false;
	}

	m_pMissTable = missTable.GetBuffer();
	m_uiMissRecordSize = missTable.GetRecordSize();

	//Create hit group shader table
	ShaderTable hitGroupTable = ShaderTable(m_pDevice.Get(), 1, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	if (hitGroupTable.AddRecord(ShaderRecord(nullptr, 0, pHitGroupIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
	{
		return false;
	}

	m_pHitGroupTable = hitGroupTable.GetBuffer();
	m_uiHitGroupRecordSize = hitGroupTable.GetRecordSize();

	return true;
}

bool App::CreateOutputBuffer()
{
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = WindowManager::GetInstance()->GetWindowWidth();
	resDesc.Height = WindowManager::GetInstance()->GetWindowHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0;
	heapProperties.VisibleNodeMask = 0;

	HRESULT hr = m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(m_pRaytracingOutput.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create output resource!");

		return false;
	}

	return true;
}

void App::CreateCBUploadBuffers()
{
	m_pRayGenCB = new UploadBuffer<RayGenerationCB>(m_pDevice.Get(), 1, true);

	float fBorder = 0.1f;

	RayGenerationCB cb;
	cb.Stencil =
	{
		-1 + fBorder, -1 + fBorder * WindowManager::GetInstance()->GetAspectRatio(),
		1 - fBorder, 1 - fBorder * WindowManager::GetInstance()->GetAspectRatio()
	};

	cb.View =
	{
		-1, -1, 1, 1
	};

	m_pRayGenCB->CopyData(0, cb);
}

bool App::CheckRaytracingSupport()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};

	HRESULT hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to check if the graphics card supports raytracing!");

		return false;
	}

	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
	{
		LOG_ERROR(tag, L"Raytracing not supported on this graphics adapter!");

		return false;
	}

	return true;
}

void App::LogAdapters()
{
	if (m_pDXGIFactory != nullptr)
	{
		UINT i = 0;
		IDXGIAdapter* pAdapter = nullptr;
		std::vector<IDXGIAdapter*> adapters;
		DXGI_ADAPTER_DESC desc;

		while (m_pDXGIFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			pAdapter->GetDesc(&desc);
			adapters.push_back(pAdapter);

			++i;
		}

		for (i = 0; i < adapters.size(); ++i)
		{
			LogAdapterOutputs(adapters[i], i == 0);
			adapters[i]->Release();
		}
	}
}

void App::LogAdapterOutputs(IDXGIAdapter* pAdapter, bool bSaveSettings)
{
	UINT i = 0;
	IDXGIOutput* pOutput = nullptr;

	while (pAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		pOutput->GetDesc(&desc);
		LogOutputInfo(pOutput, m_BackBufferFormat, bSaveSettings && i == 0);

		pOutput->Release();

		++i;
	}
}

void App::LogOutputInfo(IDXGIOutput* pOutput, DXGI_FORMAT format, bool bSaveSettings)
{
	UINT count = 0;
	UINT flags = 0;
	UINT i = 0;

	pOutput->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modes(count);
	pOutput->GetDisplayModeList(format, flags, &count, &modes[0]);

	if (bSaveSettings)
	{
		WindowManager::GetInstance()->SetWindowSettings(modes[modes.size() - 1]);
	}
}

void App::ExecuteCommandList()
{
	//Execute Init command
	HRESULT hr = m_pGraphicsCommandList->Close();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to close the graphics command list!");
	}

	ID3D12CommandList* commandLists[] = { m_pGraphicsCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	FlushCommandQueue();
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetSrvUavDescriptorHandleCPU(UINT index)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle;
	handle.InitOffsetted(m_pSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), (UINT)index * m_uiSrvUavDescriptorSize);

	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE App::GetSrvUavDescriptorHandleGPU(UINT index)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;
	handle.InitOffsetted(m_pSrvUavHeap->GetGPUDescriptorHandleForHeapStart(), (UINT)index * m_uiSrvUavDescriptorSize);

	return handle;
}

ID3D12Resource* App::GetBackBuffer() const
{
	return m_FrameResources[m_uiFrameIndex].m_pRenderTarget.Get();
}

ID3D12Resource* App::GetBackBuffer(int iIndex) const
{
	return m_FrameResources[iIndex].m_pRenderTarget.Get();
}

Microsoft::WRL::ComPtr<ID3D12Resource>* App::GetBackBufferComptr()
{
	return &m_FrameResources[m_uiFrameIndex].m_pRenderTarget;
}

Microsoft::WRL::ComPtr<ID3D12Resource>* App::GetBackBufferComptr(int iIndex)
{
	return &m_FrameResources[iIndex].m_pRenderTarget;
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiFrameIndex, m_uiRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetBackBufferView(int iIndex) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), iIndex, m_uiRtvDescriptorSize);
}

ID3D12CommandAllocator* App::GetCommandAllocator() const
{
	return m_FrameResources[m_uiFrameIndex].m_pCommandAllocator.Get();
}

ID3D12CommandAllocator* App::GetCommandAllocator(int iIndex) const
{
	return m_FrameResources[iIndex].m_pCommandAllocator.Get();
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator>* App::GetCommandAllocatorComptr()
{
	return &m_FrameResources[m_uiFrameIndex].m_pCommandAllocator;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator>* App::GetCommandAllocatorComptr(int iIndex)
{
	return &m_FrameResources[iIndex].m_pCommandAllocator;
}

UINT64 App::GetFenceValue()
{
	return m_FrameResources[m_uiFrameIndex].m_uiFenceValue;
}

UINT64 App::GetFenceValue(int iIndex)
{
	return m_FrameResources[iIndex].m_uiFenceValue;
}

void App::IncrementFenceValue()
{
	++m_FrameResources[m_uiFrameIndex].m_uiFenceValue;
}

bool App::CreateGlobalSignature()
{
	CD3DX12_DESCRIPTOR_RANGE outputTable;
	outputTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (UINT)1, 0);

	CD3DX12_DESCRIPTOR_RANGE accelTable;
	accelTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[3] = {};
	slotRootParameter[0].InitAsDescriptorTable(1, &outputTable);	//Render target
	slotRootParameter[1].InitAsDescriptorTable(1, &accelTable);	//Reference to TLAS
	slotRootParameter[2].InitAsConstantBufferView(0);	//Constant buffer

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init((UINT)_countof(slotRootParameter), slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> pSignature;
	ComPtr<ID3DBlob> pError;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create ray gen serialize root signature!");

		OutputDebugStringA((char*)pError->GetBufferPointer());

		return false;
	}

	hr = m_pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(m_pGlobalRootSignature.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create ray gen root signature!");

		return false;
	}

	return true;
}

void App::PopulateDescriptorHeaps()
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	m_pDevice->CreateUnorderedAccessView(m_pRaytracingOutput.Get(), nullptr, &uavDesc, GetSrvUavDescriptorHandleCPU(GlobalRootSignatureParams::OUTPUT));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = m_pTopLevelAccelerationStructure.Get()->GetGPUVirtualAddress();

	// Write the acceleration structure view in the heap 
	m_pDevice->CreateShaderResourceView(nullptr, &srvDesc, GetSrvUavDescriptorHandleCPU(GlobalRootSignatureParams::ACCELERATION_STRUCTURE));

	//Create constant buffer uav
	D3D12_CONSTANT_BUFFER_VIEW_DESC rayGenDesc = {};
	rayGenDesc.BufferLocation = m_pRayGenCB->Get()->GetGPUVirtualAddress();
	rayGenDesc.SizeInBytes = MathHelper::CalculatePaddedConstantBufferSize(sizeof(RayGenerationCB));

	m_pDevice->CreateConstantBufferView(&rayGenDesc, GetSrvUavDescriptorHandleCPU(GlobalRootSignatureParams::SCENE_CB));
}

bool App::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = (UINT)SrvUavHeapIndex::COUNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pSrvUavHeap));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create render target descriptor heap!");

		return false;
	}

	m_uiSrvUavDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return true;
}
