#include "App.h"
#include "Commons/Timer.h"
#include "Commons/ShaderTable.h"
#include "Commons/DescriptorHeap.h"
#include "Commons/UAVDescriptor.h"
#include "Commons/SRVDescriptor.h"
#include "Commons/Texture.h"
#include "Commons/Mesh.h"
#include "Shaders/ConstantBuffers.h"
#include "Shaders/Vertices.h"
#include "Helpers/DebugHelper.h"
#include "Helpers/MathHelper.h"
#include "Helpers/DXRHelper.h"
#include "Managers/WindowManager.h"
#include "Managers/InputManager.h"
#include "Managers/ObjectManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "GameObjects/GameObject.h"

#if PIX
#include "pix3.h"

#include <shlobj.h>
#include <strsafe.h>
#endif

#include <vector>
#include <queue>

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

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create Debug controller!");

		uiDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}

#if DRED
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> pDredSettings;

	hr = D3D12GetDebugInterface(IID_PPV_ARGS(pDredSettings.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create DRED interface!");
	}

	pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	pDredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif

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

	if (CompileShaders() == false)
	{
		return false;
	}

	if (CreateSignatures() == false)
	{
		return false;
	}

	if (CreateStateObject() == false)
	{
		return false;
	}

	CreateGeometry();

	if (CreateDescriptorHeaps() == false)
	{
		return false;
	}

	CreateCBs();

	InitScene();

	if (CreateAccelerationStructures() == false)
	{
		return false;
	}

	if (CreateOutputBuffer() == false)
	{
		return false;
	}

	PopulateDescriptorHeaps();

	if (CreateShaderTables() == false)
	{
		return false;
	}

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
	InputManager::GetInstance()->Update(kTimer);

	ObjectManager::GetInstance()->GetActiveCamera()->Update(kTimer);

	GameObject* pGameObject = ObjectManager::GetInstance()->GetGameObject("Fish");

	pGameObject->Rotate(0, 20.0f * kTimer.DeltaTime(), 0);

	pGameObject = ObjectManager::GetInstance()->GetGameObject("WaterBottle");

	pGameObject->Rotate(0, 10.0f * kTimer.DeltaTime(), 0);

	UpdatePerFrameCB(m_uiFrameIndex);

	ObjectManager::GetInstance()->Update(kTimer);
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_pRTVHeap->GetHeap()->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < s_kuiSwapChainBufferCount; i++)
	{
		hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(GetBackBufferComptr(i)->GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to get swap chain buffer at index %u!", i);

			return;
		}

		if (m_pRTVHeap->GetNumDescsAllocated() < s_kuiSwapChainBufferCount)
		{
			if (m_pRTVHeap->Allocate() == false)
			{
				return;
			}
		}

		m_pDevice->CreateRenderTargetView(GetBackBuffer(i), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_pRTVHeap->GetDescriptorSize());
	}

	//Recreate the raytracing output
	m_pRaytracingOutput.Reset();

	CreateOutputBuffer();

	Descriptor* pDescriptor = new UAVDescriptor(m_pOutputDesc->GetDescriptorIndex(), m_pSrvUavHeap->GetCpuDescriptorHandle(m_pOutputDesc->GetDescriptorIndex()), m_pRaytracingOutput.Get(), D3D12_UAV_DIMENSION_TEXTURE2D);

	delete m_pOutputDesc;

	m_pOutputDesc = pDescriptor;

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

	if (CreateTLAS(true) == false)
	{
		return;
	}

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRaytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	std::vector<ID3D12DescriptorHeap*> heaps = { m_pSrvUavHeap->GetHeap().Get() };
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

	m_pGraphicsCommandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OUTPUT, m_pSrvUavHeap->GetGpuDescriptorHandle(m_pOutputDesc->GetDescriptorIndex()));

	m_pGraphicsCommandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::ACCELERATION_STRUCTURE, m_TopLevelBuffer.m_pResult->GetGPUVirtualAddress());

	m_pGraphicsCommandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::PER_FRAME_PRIMITIVE_CB, GetPrimitiveUploadBuffer()->GetBufferGPUAddress());

	m_pGraphicsCommandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::LIGHT_CB, GetLightUploadBuffer()->GetBufferGPUAddress());

	m_pGraphicsCommandList->SetComputeRootConstantBufferView(GlobalRootSignatureParams::PER_FRAME_SCENE_CB, m_pScenePerFrameCBUpload->GetBufferGPUAddress(m_uiFrameIndex));

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

#if DRED
		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			ComPtr<ID3D12DeviceRemovedExtendedData1> pDred;
			m_pDevice->QueryInterface(IID_PPV_ARGS(&pDred));

			D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput;
			D3D12_DRED_PAGE_FAULT_OUTPUT DredPageFaultOutput;
			pDred->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput);
			pDred->GetPageFaultAllocationOutput(&DredPageFaultOutput);
		}
#endif

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

		InputManager::GetInstance()->KeyUp((int)wParam);

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
		InputManager::GetInstance()->KeyDown((int)wParam);
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

	m_pRTVHeap = new DescriptorHeap();

	if (m_pRTVHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, s_kuiSwapChainBufferCount) == false)
	{
		return false;
	}



	return true;
}

void App::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point
	++m_uiFenceValue;

	// Add an instruction to the command queue to set a new fence point
	HRESULT hr = m_pCommandQueue->Signal(m_pFence.Get(), m_uiFenceValue);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create a new fence point!");

		return;
	}

	// Wait until the GPU has completed commands up to this fence point.
	if (m_pFence->GetCompletedValue() < m_uiFenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

		if (eventHandle == 0)
		{
			LOG_ERROR(tag, L"Failed to create event when fence point it hit!");

			return;
		}

		// Fire event when GPU hits current fence.  
		hr = m_pFence->SetEventOnCompletion(m_uiFenceValue, eventHandle);

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
	CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT* pLocalRootSignature = pipelineDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
	pLocalRootSignature->SetRootSignature(m_pLocalRootSignature.Get());

	CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT* pAssociation = pipelineDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	pAssociation->SetSubobjectToAssociate(*pLocalRootSignature);
	pAssociation->AddExport(m_kwsHitGroupName);

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
	HRESULT hr = m_pGraphicsCommandList->Reset(GetCommandAllocator(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the graphics command list!");

		return;
	}

	MeshManager::GetInstance()->LoadMesh("Models/BarramundiFish/gLTF/BarramundiFish.gltf", "Barramundi", m_pGraphicsCommandList.Get());
	MeshManager::GetInstance()->LoadMesh("Models/WaterBottle/gLTF/WaterBottle.gltf", "WaterBottle", m_pGraphicsCommandList.Get());
	MeshManager::GetInstance()->LoadMesh("Models/BoomBox/gLTF/BoomBox.gltf", "BoomBox", m_pGraphicsCommandList.Get());

	ExecuteCommandList();
}

bool App::CreateAccelerationStructures()
{
	HRESULT hr = m_pGraphicsCommandList->Reset(GetCommandAllocator(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the graphics command list!");

		return false;
	}

	std::vector<UploadBuffer<XMFLOAT3X4>*> uploadBuffers;

	if (MeshManager::GetInstance()->CreateBLAS(m_pGraphicsCommandList.Get(), uploadBuffers) == false)
	{
		return false;
	}

	if (CreateTLAS(false) == false)
	{
		return false;
	}

	ExecuteCommandList();

	for (int i = 0; i < uploadBuffers.size(); ++i)
	{
		delete uploadBuffers[i];
	}

	return true;
}

bool App::CreateTLAS(bool bUpdate)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.NumDescs = ObjectManager::GetInstance()->GetNumGameObjects();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (bUpdate == true)
	{
		m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_TopLevelBuffer.m_pResult.Get()));
	}
	else
	{
		if (DXRHelper::CreateUAVBuffer(m_pDevice.Get(), info.ScratchDataSizeInBytes, m_TopLevelBuffer.m_pScratch.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == false)
		{
			LOG_ERROR(tag, L"Failed to create the top level acceleration structure scratch buffer!");

			return false;
		}

		if (DXRHelper::CreateUAVBuffer(m_pDevice.Get(), info.ResultDataMaxSizeInBytes, m_TopLevelBuffer.m_pResult.GetAddressOf(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) == false)
		{
			LOG_ERROR(tag, L"Failed to create the top level acceleration structure result buffer!");

			return false;
		}
		 
		m_TopLevelBuffer.m_pInstanceDesc = new UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC>(m_pDevice.Get(), ObjectManager::GetInstance()->GetNumGameObjects(), false);
	}

	inputs.InstanceDescs = m_TopLevelBuffer.m_pInstanceDesc->GetBufferGPUAddress(0);

	int iCount = 0;

	for (std::unordered_map<std::string, GameObject*>::iterator it = ObjectManager::GetInstance()->GetGameObjects()->begin(); it != ObjectManager::GetInstance()->GetGameObjects()->end(); ++it)
	{
		XMFLOAT3X4 world = it->second->Get3X4WorldMatrix();

		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};

		for (int j = 0; j < 3; ++j)
		{
			for (int k = 0; k < 4; ++k)
			{
				instanceDesc.Transform[j][k] = world.m[j][k];
			}
		}

		instanceDesc.InstanceMask = 1;
		instanceDesc.AccelerationStructure = it->second->GetMesh()->GetBLAS()->m_pResult->GetGPUVirtualAddress();
		instanceDesc.Flags = 0;
		instanceDesc.InstanceID = iCount;
		instanceDesc.InstanceContributionToHitGroupIndex = iCount;
		instanceDesc.InstanceMask = 0xFF;

		m_TopLevelBuffer.m_pInstanceDesc->CopyData(iCount, instanceDesc);

		++iCount;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_TopLevelBuffer.m_pResult->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = m_TopLevelBuffer.m_pScratch->GetGPUVirtualAddress();

	if (bUpdate == true)
	{
		buildDesc.SourceAccelerationStructureData = m_TopLevelBuffer.m_pResult->GetGPUVirtualAddress();

		buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	}

	m_pGraphicsCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_TopLevelBuffer.m_pResult.Get()));

	return true;
}

bool App::CreateShaderTables()
{
	if (CreateRayGenShaderTable() == false)
	{
		return false;
	}

	if (CreateMissShaderTable() == false)
	{
		return false;
	}

	if (CreateHitGroupShaderTable() == false)
	{
		return false;
	}

	return true;
}

bool App::CreateRayGenShaderTable()
{
	void* pRayGenIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_kwsRayGenName);

	ShaderTable shaderTable = ShaderTable(m_pDevice.Get(), 1, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	if (shaderTable.AddRecord(ShaderRecord(nullptr, 0, pRayGenIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
	{
		LOG_ERROR(tag, L"Failed to add a ray gen shader record!");

		return false;
	}

	m_pRayGenTable = shaderTable.GetBuffer();
	m_uiRayGenRecordSize = shaderTable.GetRecordSize();

	return true;
}

bool App::CreateMissShaderTable()
{
	void* pMissIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_kwsMissName);

	ShaderTable shaderTable = ShaderTable(m_pDevice.Get(), 1, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	if (shaderTable.AddRecord(ShaderRecord(nullptr, 0, pMissIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
	{
		LOG_ERROR(tag, L"Failed to add a miss shader record!");

		return false;
	}

	m_pMissTable = shaderTable.GetBuffer();
	m_uiMissRecordSize = shaderTable.GetRecordSize();

	return true;
}

bool App::CreateHitGroupShaderTable()
{
	void* pHitGroupIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_kwsHitGroupName);

	struct HitGroupRootArgs
	{
		D3D12_GPU_DESCRIPTOR_HANDLE AlbedoHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE NormalHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE MetallicRoughnessHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE OcclusionHandle;
		PrimitiveInstanceCB Cb;
		D3D12_GPU_DESCRIPTOR_HANDLE IndexDesc;
	};

	HitGroupRootArgs hitGroupRootArgs;

	ShaderTable hitGroupTable = ShaderTable(m_pDevice.Get(), MeshManager::GetInstance()->GetNumPrimitives(), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(hitGroupRootArgs));

	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	const MeshNode* pNode;
	Mesh* pMesh;

	for (std::unordered_map<std::string, GameObject*>::iterator it = pGameObjects->begin(); it != pGameObjects->end(); ++it)
	{
		pMesh = it->second->GetMesh();

		for (int i = 0; i < pMesh->GetNodes()->size(); ++i)
		{
			pNode = pMesh->GetNode(i);

			//Assign per primitive information
			for (int i = 0; i < pNode->m_Primitives.size(); ++i)
			{
				hitGroupRootArgs.IndexDesc = m_pSrvUavHeap->GetGpuDescriptorHandle(pNode->m_Primitives[i]->m_pIndexDesc->GetDescriptorIndex());

				if (pNode->m_Primitives[i]->m_iAlbedoIndex != -1)
				{
					hitGroupRootArgs.AlbedoHandle = m_pSrvUavHeap->GetGpuDescriptorHandle(it->second->GetMesh()->GetTextures()->at(pNode->m_Primitives[i]->m_iAlbedoIndex)->GetDescriptor()->GetDescriptorIndex());
				}

				if (pNode->m_Primitives[i]->m_iNormalIndex != -1)
				{
					hitGroupRootArgs.NormalHandle = m_pSrvUavHeap->GetGpuDescriptorHandle(it->second->GetMesh()->GetTextures()->at(pNode->m_Primitives[i]->m_iNormalIndex)->GetDescriptor()->GetDescriptorIndex());
				}

				if (pNode->m_Primitives[i]->m_iMetallicRoughnessIndex != -1)
				{
					hitGroupRootArgs.MetallicRoughnessHandle = m_pSrvUavHeap->GetGpuDescriptorHandle(it->second->GetMesh()->GetTextures()->at(pNode->m_Primitives[i]->m_iMetallicRoughnessIndex)->GetDescriptor()->GetDescriptorIndex());
				}

				if (pNode->m_Primitives[i]->m_iOcclusionIndex != -1)
				{
					hitGroupRootArgs.OcclusionHandle = m_pSrvUavHeap->GetGpuDescriptorHandle(it->second->GetMesh()->GetTextures()->at(pNode->m_Primitives[i]->m_iOcclusionIndex)->GetDescriptor()->GetDescriptorIndex());
				}

				hitGroupRootArgs.Cb.InstanceIndex = pNode->m_Primitives[i]->m_iIndex;

				if (hitGroupTable.AddRecord(ShaderRecord(&hitGroupRootArgs, sizeof(hitGroupRootArgs), pHitGroupIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
				{
					LOG_ERROR(tag, L"Failed to add a hit group shader record!");

					return false;
				}
			}
		}
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

void App::CreateCBs()
{
	m_pScenePerFrameCBUpload = new UploadBuffer<ScenePerFrameCB>(m_pDevice.Get(), s_kuiSwapChainBufferCount, true);

	//Reserve and populate CB vectors

	//Primitive per frame
	m_PrimitivePerFrameCBs.reserve(MeshManager::GetInstance()->GetNumPrimitives());

	for (int i = 0; i < MeshManager::GetInstance()->GetNumPrimitives(); ++i)
	{
		m_PrimitivePerFrameCBs.push_back(PrimitivePerFrameCB());
	}

	//LightCB
	m_LightCBs.reserve(MAX_LIGHTS);

	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		m_LightCBs.push_back(LightCB());
	}

	//Create upload buffers
	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		m_FrameResources[i].m_pPrimitivePerFrameCBUpload = new UploadBuffer<PrimitivePerFrameCB>(m_pDevice.Get(), MeshManager::GetInstance()->GetNumPrimitives(), false);
		m_FrameResources[i].m_pLightCBUpload = new UploadBuffer<LightCB>(m_pDevice.Get(), MAX_LIGHTS, false);
	}
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

void App::CreateCameras()
{
	Camera* pCamera = new DebugCamera(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 0, 0), XMFLOAT3(0, 1, 0), 0.1f, 1000.0f, "BasicCamera");

	ObjectManager::GetInstance()->AddCamera(pCamera);

	ObjectManager::GetInstance()->SetActiveCamera(pCamera->GetName());
}

void App::InitScene()
{
	Mesh* pMesh = nullptr;
	MeshManager::GetInstance()->GetMesh("Barramundi", pMesh);

	GameObject* pGameObject = new GameObject();
	pGameObject->Init("Fish", XMFLOAT3(5, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(3, 3, 3), pMesh);

	MeshManager::GetInstance()->GetMesh("WaterBottle", pMesh);

	pGameObject = new GameObject();
	pGameObject->Init("WaterBottle", XMFLOAT3(5, 0, 5), XMFLOAT3(0, 0, 0), XMFLOAT3(4.0f, 4.0f, 4.0f), pMesh);

	MeshManager::GetInstance()->GetMesh("BoomBox", pMesh);

	pGameObject = new GameObject();
	pGameObject->Init("BoomBox", XMFLOAT3(5, 0, -5), XMFLOAT3(0, 0, 0), XMFLOAT3(25, 25, 25), pMesh);

	CreateCameras();

	InitConstantBuffers();
}

void App::InitConstantBuffers()
{
	m_uiNumLights = 2;

	for (UINT i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		UpdatePerFrameCB(i);
	}

	m_LightCBs[0].Color = XMFLOAT4(1, 0, 0, 1);
	m_LightCBs[0].Position = XMFLOAT3(0, 0, 0);
	m_LightCBs[0].Attenuation = XMFLOAT3(0.2f, 0.09f, 0.0f);

	m_LightCBs[1].Color = XMFLOAT4(0, 1, 0, 1);
	m_LightCBs[1].Position = XMFLOAT3(10, 0, 0);
	m_LightCBs[1].Attenuation = XMFLOAT3(0.2f, 0.09f, 0.0f);

	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		GetLightUploadBuffer(i)->CopyData(0, m_LightCBs);
	}
}

void App::UpdatePerFrameCB(UINT uiFrameIndex)
{
	Camera* pCamera = ObjectManager::GetInstance()->GetActiveCamera();

	m_PerFrameCBs[uiFrameIndex].EyePosW = pCamera->GetPosition();
	m_PerFrameCBs[uiFrameIndex].InvWorldProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&pCamera->GetViewProjectionMatrix())));
	m_PerFrameCBs[uiFrameIndex].NumLights = (int)m_uiNumLights;

	m_pScenePerFrameCBUpload->CopyData(uiFrameIndex, m_PerFrameCBs[uiFrameIndex]);

	//Update primitive per frame constant buffers
	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	const MeshNode* pNode;
	Mesh* pMesh;

	XMMATRIX invWorld;

	for (std::unordered_map<std::string, GameObject*>::iterator it = pGameObjects->begin(); it != pGameObjects->end(); ++it)
	{
		pMesh = it->second->GetMesh();

		for (int i = 0; i < pMesh->GetNodes()->size(); ++i)
		{
			pNode = pMesh->GetNode(i);

			//Assign per primitive information
			for (int i = 0; i < pNode->m_Primitives.size(); ++i)
			{
				invWorld = XMLoadFloat4x4(&pNode->m_Transform) * XMLoadFloat4x4(&it->second->GetWorldMatrix());

				m_PrimitivePerFrameCBs[pNode->m_Primitives[i]->m_iIndex].World = XMMatrixTranspose(invWorld);

				invWorld.r[3] = XMVectorSet(0, 0, 0, 1);

				m_PrimitivePerFrameCBs[pNode->m_Primitives[i]->m_iIndex].InvWorld = XMMatrixInverse(nullptr, invWorld);
			}
		}
	}

	GetPrimitiveUploadBuffer()->CopyData(0, m_PrimitivePerFrameCBs);
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> App::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
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
	return m_pRTVHeap->GetCpuDescriptorHandle(m_uiFrameIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetBackBufferView(UINT uiIndex) const
{
	return m_pRTVHeap->GetCpuDescriptorHandle(uiIndex);
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

UploadBuffer<PrimitivePerFrameCB>* App::GetPrimitiveUploadBuffer()
{
	return m_FrameResources[m_uiFrameIndex].m_pPrimitivePerFrameCBUpload;
}

UploadBuffer<PrimitivePerFrameCB>* App::GetPrimitiveUploadBuffer(int iIndex)
{
	return m_FrameResources[iIndex].m_pPrimitivePerFrameCBUpload;
}

UploadBuffer<LightCB>* App::GetLightUploadBuffer()
{
	return m_FrameResources[m_uiFrameIndex].m_pLightCBUpload;
}

UploadBuffer<LightCB>* App::GetLightUploadBuffer(int iIndex)
{
	return m_FrameResources[iIndex].m_pLightCBUpload;
}

bool App::CreateSignatures()
{
	if (CreateGlobalSignature() == false)
	{
		return false;
	}

	if (CreateLocalSignature() == false)
	{
		return false;
	}

	return true;
}

bool App::CreateLocalSignature()
{
	CD3DX12_DESCRIPTOR_RANGE indexVertexRange;
	indexVertexRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)2, 1);

	CD3DX12_DESCRIPTOR_RANGE albedoRange;
	albedoRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 5);

	CD3DX12_DESCRIPTOR_RANGE normalRange;
	normalRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 6);

	CD3DX12_DESCRIPTOR_RANGE metallicRoughnessRange;
	metallicRoughnessRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 7);

	CD3DX12_DESCRIPTOR_RANGE occlusionRange;
	occlusionRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 8);

	CD3DX12_ROOT_PARAMETER slotRootParameter[LocalRootSignatureParams::COUNT] = {};
	slotRootParameter[LocalRootSignatureParams::ALBEDO_TEX].InitAsDescriptorTable(1, &albedoRange);
	slotRootParameter[LocalRootSignatureParams::NORMAL_TEX].InitAsDescriptorTable(1, &normalRange);
	slotRootParameter[LocalRootSignatureParams::METALLIC_ROUGHNESS_TEX].InitAsDescriptorTable(1, &metallicRoughnessRange);
	slotRootParameter[LocalRootSignatureParams::OCCLUSION_TEX].InitAsDescriptorTable(1, &occlusionRange);
	slotRootParameter[LocalRootSignatureParams::PRIMITIVE_CB].InitAsConstants((sizeof(PrimitiveInstanceCB) - 1) / (sizeof(UINT32) + 1), 1);
	slotRootParameter[LocalRootSignatureParams::VERTEX_INDEX].InitAsDescriptorTable(1, &indexVertexRange);	//Reference to vertex and index buffer

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init((UINT)_countof(slotRootParameter), slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	ComPtr<ID3DBlob> pSignature;
	ComPtr<ID3DBlob> pError;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create local serialize root signature!");

		OutputDebugStringA((char*)pError->GetBufferPointer());

		return false;
	}

	hr = m_pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(m_pLocalRootSignature.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create local root signature!");

		return false;
	}

	return true;
}

bool App::CreateGlobalSignature()
{
	CD3DX12_DESCRIPTOR_RANGE outputRange;
	outputRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[GlobalRootSignatureParams::COUNT] = {};
	slotRootParameter[GlobalRootSignatureParams::OUTPUT].InitAsDescriptorTable(1, &outputRange);
	slotRootParameter[GlobalRootSignatureParams::ACCELERATION_STRUCTURE].InitAsShaderResourceView(0);
	slotRootParameter[GlobalRootSignatureParams::PER_FRAME_PRIMITIVE_CB].InitAsShaderResourceView(3);
	slotRootParameter[GlobalRootSignatureParams::LIGHT_CB].InitAsShaderResourceView(4);
	slotRootParameter[GlobalRootSignatureParams::PER_FRAME_SCENE_CB].InitAsConstantBufferView(0);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init((UINT)ARRAYSIZE(slotRootParameter), slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data());

	ComPtr<ID3DBlob> pSignature;
	ComPtr<ID3DBlob> pError;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create global serialize root signature!");

		OutputDebugStringA((char*)pError->GetBufferPointer());

		return false;
	}

	hr = m_pDevice->CreateRootSignature(1, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_pGlobalRootSignature));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create global root signature!");

		return false;
	}

	return true;
}

void App::PopulateDescriptorHeaps()
{
	//Create output descriptor
	UINT uiDescIndex;
	
	if (m_pSrvUavHeap->Allocate(uiDescIndex) == false)
	{
		return;
	}

	m_pOutputDesc = new UAVDescriptor(uiDescIndex, m_pSrvUavHeap->GetCpuDescriptorHandle(uiDescIndex), m_pRaytracingOutput.Get(), D3D12_UAV_DIMENSION_TEXTURE2D);

	MeshManager::GetInstance()->CreateDescriptors(m_pSrvUavHeap);
}

bool App::CreateDescriptorHeaps()
{
	m_pSrvUavHeap = new DescriptorHeap();

	//2 descriptors per primitive (index and vertex buffers), 1 descriptor per texture, 1 extra descriptor for output texture
	if (m_pSrvUavHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, (MeshManager::GetInstance()->GetNumPrimitives() * 2) + TextureManager::GetInstance()->GetNumTextures() + 1) == false)
	{
		return false;
	}

	return true;
}
