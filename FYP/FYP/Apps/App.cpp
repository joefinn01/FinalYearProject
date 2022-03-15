#include "App.h"
#include "Commons/Timer.h"
#include "Commons/ShaderTable.h"
#include "Commons/DescriptorHeap.h"
#include "Commons/UAVDescriptor.h"
#include "Commons/SRVDescriptor.h"
#include "Commons/RTVDescriptor.h"
#include "Commons/DSVDescriptor.h"
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
#include "Commons/Texture.h"
#include "GameObjects/GameObject.h"
#include "Include/ImGui/imgui_impl_dx12.h"
#include "Include/ImGui/imgui_impl_win32.h"
#include "Include/ImGui/imgui.h"
#include "GIVolume.h"

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

	CreateInputDescs();

	if (CreatePSOs() == false)
	{
		return false;
	}

	CreateGeometry();

	if (CreateDescriptorHeaps() == false)
	{
		return false;
	}

	m_pRaytracingOutput = new Texture(nullptr, m_BackBufferFormat);

	Texture** GBuffer;

	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		GBuffer = GetGBuffer(i);

		for (int j = 0; j < (int)GBuffer::COUNT; ++j)
		{
			GBuffer[j] = new Texture(nullptr, m_GBufferFormats[j]);
		}

		SetDepthStencilBuffer(i, new Texture(nullptr, m_DepthStencilFormat));
	}

	if (CreateOutputBuffers() == false)
	{
		return false;
	}

	InitImGui();

	CreateScreenQuad();

	InitScene();

	CreateCBs();

	InitConstantBuffers();

	PopulateDescriptorHeaps();

	PopulatePrimitivePerInstanceCB();
	PopulateDeferredPerFrameCB();

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
	DebugHelper::ResetFrameTimes();

	InputManager::GetInstance()->Update(kTimer);

	ObjectManager::GetInstance()->GetActiveCamera()->Update(kTimer);

	GameObject* pGameObject = ObjectManager::GetInstance()->GetGameObject("Fish");

	//pGameObject->Rotate(0, 20.0f * kTimer.DeltaTime(), 0);

	//pGameObject = ObjectManager::GetInstance()->GetGameObject("WaterBottle");

	//pGameObject->Rotate(0, 10.0f * kTimer.DeltaTime(), 0);

	UpdatePerFrameCB(m_uiFrameIndex);

	ObjectManager::GetInstance()->Update(kTimer);

	m_pGIVolume->Update(kTimer);

	ImGuiIO io = ImGui::GetIO();
	io.DeltaTime = kTimer.DeltaTime();
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

	//Recreate all output buffers and corresponding descriptors

	CreateOutputBuffers();

	Descriptor** GBufferRTVs;
	Texture** GBuffer;

	//G Buffer RTVs haven't been created yet so create them
	if (m_pRTVHeap->GetNumDescsAllocated() != m_pRTVHeap->GetMaxNumDescs())
	{
		UINT uiDescIndex = 0;

		for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
		{
			GBuffer = GetGBuffer(i);
			GBufferRTVs = GetGBufferRTVDescs(i);

			for (int j = 0; j < (int)GBuffer::COUNT; ++j)
			{
				if (m_pRTVHeap->Allocate(uiDescIndex) == false)
				{
					LOG_ERROR(tag, L"Couldn't allocate G Buffer RTV as heap is full!");

					return;
				}

				GBufferRTVs[j] = new RTVDescriptor(uiDescIndex, GBuffer[j]->GetResource().Get(), m_pRTVHeap->GetCpuDescriptorHandle(uiDescIndex));
			}
		}
	}
	else 	//Recreate G buffer RTV
	{
		Descriptor* pDesc = nullptr;

		for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
		{
			GBuffer = GetGBuffer(i);
			GBufferRTVs = GetGBufferRTVDescs(i);

			for (int j = 0; j < (int)GBuffer::COUNT; ++j)
			{
				pDesc = new RTVDescriptor(GBufferRTVs[j]->GetDescriptorIndex(), GBuffer[j]->GetResource().Get(), m_pRTVHeap->GetCpuDescriptorHandle(GBufferRTVs[j]->GetDescriptorIndex()));
				delete GBufferRTVs[j];
				GBufferRTVs[j] = pDesc;
			}
		}
	}

	m_pRaytracingOutput->RecreateUAVDesc(m_pSRVHeap);

	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		GBuffer = GetGBuffer(i);

		for (int j = 0; j < (int)GBuffer::COUNT; ++j)
		{
			GBuffer[j]->RecreateSRVDesc(m_pSRVHeap);
		}

		GetDepthStencilBuffer(i)->RecreateSRVDesc(m_pSRVHeap, m_DepthStencilSRVFormat);
	}

	//Create depth stencil view
	if (m_pDSVHeap->GetNumDescsAllocated() != m_pDSVHeap->GetMaxNumDescs())
	{
		UINT uiDescIndex = 0;

		for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
		{
			if (m_pDSVHeap->Allocate(uiDescIndex) == false)
			{
				LOG_ERROR(tag, L"Couldn't allocate depth stencil buffer DSV as heap is full!");

				return;
			}

			SetDepthStencilBufferView(i, new DSVDescriptor(uiDescIndex, GetDepthStencilBuffer(i)->GetResource().Get(), m_pDSVHeap->GetCpuDescriptorHandle(uiDescIndex), m_DepthStencilDSVFormat));
		}
	}
	else 	//Recreate depth stencil DSV
	{
		Descriptor* pDesc = nullptr;

		for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
		{
			pDesc = new DSVDescriptor(GetDepthStencilBufferView(i)->GetDescriptorIndex(), GetDepthStencilBuffer(i)->GetResource().Get(), m_pDSVHeap->GetCpuDescriptorHandle(GetDepthStencilBufferView(i)->GetDescriptorIndex()), m_DepthStencilDSVFormat);
			SetDepthStencilBufferView(i, pDesc);
		}
	}

	ObjectManager::GetInstance()->GetActiveCamera()->Reshape(m_fCameraNearDepth, m_fCameraFarDepth);

	//Update the viewport transform to cover the client area.
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width = (float)WindowManager::GetInstance()->GetWindowWidth();
	m_Viewport.Height = (float)WindowManager::GetInstance()->GetWindowHeight();
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_ScissorRect = { 0, 0, static_cast<long>(WindowManager::GetInstance()->GetWindowWidth()), static_cast<long>(WindowManager::GetInstance()->GetWindowHeight()) };

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
	HRESULT hr;

	{
		PROFILE("Full Frame");

		hr = GetCommandAllocator()->Reset();

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

		PopulateRenderInfoQueue();

		PopulatePrimitiveIndexCB();

		std::vector<ID3D12DescriptorHeap*> heaps = { m_pSRVHeap->GetHeap().Get() };
		m_pGraphicsCommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

		m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		DrawDeferredPass();

		m_pGIVolume->Draw(m_pSRVHeap, m_pScenePerFrameCBUpload, m_pGraphicsCommandList.Get());
	}

	DrawImGui();

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

void App::DrawDeferredPass()
{
	PROFILE("Deferred Pass");

	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Deferred Render"));

	DrawGBufferPass();

	DrawLightPass();

	PIX_ONLY(PIXEndEvent());
}

void App::DrawGBufferPass()
{
	PROFILE("G Buffer Pass");

	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "G Buffer Pass"));

	m_pGraphicsCommandList->SetGraphicsRootSignature(m_pGBufferRootSignature.Get());

	m_pGraphicsCommandList->RSSetViewports(1, &m_Viewport);
	m_pGraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);

	Texture** GBuffer = GetGBuffer();

	CD3DX12_RESOURCE_BARRIER* pResourceBarriers = new CD3DX12_RESOURCE_BARRIER[(int)GBuffer::COUNT + 1];

	for (int i = 0; i < (int)GBuffer::COUNT; ++i)
	{
		pResourceBarriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(GBuffer[i]->GetResource().Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	//Last index is depth stencil buffer transition
	pResourceBarriers[(int)GBuffer::COUNT] = CD3DX12_RESOURCE_BARRIER::Transition(GetDepthStencilBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	m_pGraphicsCommandList->ResourceBarrier((int)GBuffer::COUNT + 1, pResourceBarriers);

	for (int i = 0; i < (int)GBuffer::COUNT; ++i)
	{
		m_pGraphicsCommandList->ClearRenderTargetView(m_pRTVHeap->GetCpuDescriptorHandle(GetGBufferRTVDescs()[i]->GetDescriptorIndex()), m_GBufferClearColors[i], 0, nullptr);
	}

	m_pGraphicsCommandList->ClearDepthStencilView(m_pDSVHeap->GetCpuDescriptorHandle(GetDepthStencilBufferView()->GetDescriptorIndex()), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescs = new D3D12_CPU_DESCRIPTOR_HANDLE[(int)GBuffer::COUNT];

	for (int i = 0; i < (int)GBuffer::COUNT; ++i)
	{
		pRenderTargetDescs[i] = m_pRTVHeap->GetCpuDescriptorHandle(GetGBufferRTVDescs()[i]->GetDescriptorIndex());
	}

	m_pGraphicsCommandList->OMSetRenderTargets((int)GBuffer::COUNT, pRenderTargetDescs, FALSE, &m_pDSVHeap->GetCpuDescriptorHandle(GetDepthStencilBufferView()->GetDescriptorIndex()));

	m_pGraphicsCommandList->SetGraphicsRootDescriptorTable(DeferredPass::GBufferPass::STANDARD_DESCRIPTORS, m_pSRVHeap->GetGpuDescriptorHandle(0));
	m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(DeferredPass::GBufferPass::PER_FRAME_SCENE_CB, m_pScenePerFrameCBUpload->GetBufferGPUAddress(m_uiFrameIndex));

	RenderInfo* pRenderInfo = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	for (std::unordered_map<PrimitiveAttributes, std::vector<RenderInfo>>::iterator it = m_RenderInfoQueue.begin(); it != m_RenderInfoQueue.end(); ++it)
	{
		for (int i = 0; i < it->second.size(); ++i)
		{
			pRenderInfo = &it->second[i];

			switch (pRenderInfo->m_pPrimitive->m_Attributes)
			{
			case PrimitiveAttributes::NORMAL | PrimitiveAttributes::OCCLUSION | PrimitiveAttributes::METALLIC_ROUGHNESS | PrimitiveAttributes::EMISSIVE:
				m_pGraphicsCommandList->SetPipelineState(m_pGBufferPSOs[(int)ShaderVersions::NORMAL_OCCLUSION_EMISSION].Get());
				break;

			case PrimitiveAttributes::NORMAL | PrimitiveAttributes::OCCLUSION | PrimitiveAttributes::METALLIC_ROUGHNESS:
				m_pGraphicsCommandList->SetPipelineState(m_pGBufferPSOs[(int)ShaderVersions::NORMAL_OCCLUSION].Get());
				break;

			case PrimitiveAttributes::NORMAL | PrimitiveAttributes::METALLIC_ROUGHNESS:
				m_pGraphicsCommandList->SetPipelineState(m_pGBufferPSOs[(int)ShaderVersions::NORMAL].Get());
				break;

			case PrimitiveAttributes::OCCLUSION | PrimitiveAttributes::METALLIC_ROUGHNESS:
				m_pGraphicsCommandList->SetPipelineState(m_pGBufferPSOs[(int)ShaderVersions::OCCLUSION].Get());
				break;

			case PrimitiveAttributes::METALLIC_ROUGHNESS:
				m_pGraphicsCommandList->SetPipelineState(m_pGBufferPSOs[(int)ShaderVersions::NOTHING].Get());
				break;

			default:
				m_pGraphicsCommandList->SetPipelineState(m_pGBufferPSOs[(int)ShaderVersions::NO_METALLIC_ROUGHNESS].Get());
				break;
			}

			m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(DeferredPass::GBufferPass::PRIMITIVE_INDEX_CB, GetPrimitiveIndexUploadBuffer()->GetBufferGPUAddress((UINT)pRenderInfo->m_uiInstanceIndex));

			vertexBufferView.BufferLocation = pRenderInfo->m_pVertexBuffer->GetBufferGPUAddress();
			vertexBufferView.StrideInBytes = sizeof(Vertex);
			vertexBufferView.SizeInBytes = pRenderInfo->m_pVertexBuffer->GetByteSize();

			indexBufferView.BufferLocation = pRenderInfo->m_pIndexBuffer->GetBufferGPUAddress();
			indexBufferView.Format = DXGI_FORMAT_R32_UINT;
			indexBufferView.SizeInBytes = pRenderInfo->m_pIndexBuffer->GetByteSize();

			m_pGraphicsCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			m_pGraphicsCommandList->IASetIndexBuffer(&indexBufferView);

			m_pGraphicsCommandList->DrawIndexedInstanced(pRenderInfo->m_pPrimitive->m_uiNumIndices, 1, pRenderInfo->m_pPrimitive->m_uiFirstIndex, pRenderInfo->m_pPrimitive->m_uiFirstVertex, 0);
		}
	}

	for (int i = 0; i < (int)GBuffer::COUNT; ++i)
	{
		pResourceBarriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(GBuffer[i]->GetResource().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	}

	pResourceBarriers[(int)GBuffer::COUNT] = CD3DX12_RESOURCE_BARRIER::Transition(GetDepthStencilBuffer()->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);

	m_pGraphicsCommandList->ResourceBarrier((int)GBuffer::COUNT + 1, pResourceBarriers);

	PIX_ONLY(PIXEndEvent());
}

void App::DrawLightPass()
{
	PROFILE("Light Pass");

	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Light Pass"));

	m_pGraphicsCommandList->SetPipelineState(m_pLightPSO.Get());

	m_pGraphicsCommandList->SetGraphicsRootSignature(m_pLightRootSignature.Get());

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	m_pGraphicsCommandList->ClearRenderTargetView(GetBackBufferView(), clearColor, 0, nullptr);

	m_pGraphicsCommandList->OMSetRenderTargets(1, &GetBackBufferView(), FALSE, nullptr);

	m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(DeferredPass::LightPass::PER_FRAME_DEFERRED_CB, GetDeferredPerFrameUploadBuffer()->GetBufferGPUAddress());

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	vertexBufferView.BufferLocation = m_pScreenQuadVertexBufferGPU->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(ScreenQuadVertex);
	vertexBufferView.SizeInBytes = (UINT)sizeof(ScreenQuadVertex) * 4;

	m_pGraphicsCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

	m_pGraphicsCommandList->DrawInstanced(4, 1, 0, 0);

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	PIX_ONLY(PIXEndEvent());
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

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) == true)
	{
		return true;
	}

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

ID3D12Device5* App::GetDevice()
{
	return m_pDevice.Get();
}

App* App::GetApp()
{
	return m_pApp;
}

const UINT App::GetNumStandardDescriptorRanges() const
{
	return s_kuiNumStandardDescriptorRanges;
}

const UINT App::GetNumUserDescriptorRanges() const
{
	return s_kuiNumUserDescriptorRanges;
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

bool App::CreatePSOs()
{
	if (CreateGBufferPSO() == false)
	{
		return false;
	}

	if (CreateLightPSO() == false)
	{
		return false;
	}

	return true;
}

bool App::CreateGBufferPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_DefaultInputDesc.data(), (UINT)m_DefaultInputDesc.size() };
	psoDesc.pRootSignature = m_pGBufferRootSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = (UINT)GBuffer::COUNT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = m_DepthStencilFormat;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE((void*)m_Shaders[m_wsGBufferVertexName].Get()->GetBufferPointer(), m_Shaders[m_wsGBufferVertexName].Get()->GetBufferSize());

	for (int i = 0; i < (int)GBuffer::COUNT; ++i)
	{
		psoDesc.RTVFormats[i] = m_GBufferFormats[i];
	}

	for (int i = 0; i < (int)ShaderVersions::COUNT; ++i)
	{
		psoDesc.PS.BytecodeLength = m_Shaders[m_GBufferPixelNames[i]]->GetBufferSize();
		psoDesc.PS.pShaderBytecode = m_Shaders[m_GBufferPixelNames[i]]->GetBufferPointer();

		HRESULT hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pGBufferPSOs[i].GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to create one of the G buffer pass pipeline state objects!");

			return false;
		}
	}

	return true;
}

bool App::CreateLightPSO()
{
	D3D12_DEPTH_STENCIL_DESC depthDesc;
	depthDesc.DepthEnable = false;
	depthDesc.StencilEnable = false;
	depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_ScreenQuadInputDesc.data(), (UINT)m_ScreenQuadInputDesc.size() };
	psoDesc.pRootSignature = m_pLightRootSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthDesc;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.SampleDesc.Count = m_b4xMSAAState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_b4xMSAAState ? (m_uiMSAAQuality - 1) : 0;
	psoDesc.RTVFormats[0] = m_BackBufferFormat;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	psoDesc.VS.BytecodeLength = m_Shaders[m_wsLightVertexName].Get()->GetBufferSize();
	psoDesc.VS.pShaderBytecode = m_Shaders[m_wsLightVertexName].Get()->GetBufferPointer();
	psoDesc.PS.BytecodeLength = m_Shaders[m_wsLightPixelName]->GetBufferSize();
	psoDesc.PS.pShaderBytecode = m_Shaders[m_wsLightPixelName]->GetBufferPointer();

	HRESULT hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pLightPSO.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create light pass pipeline state object!");

		return false;
	}

	return true;
}

void App::CreateInputDescs()
{
	m_DefaultInputDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	m_ScreenQuadInputDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void App::AssociateShader(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc)
{
	IDxcBlob* pBlob = m_Shaders[shaderName].Get();

	CD3DX12_DXIL_LIBRARY_SUBOBJECT* pLib = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	pLib->SetDXILLibrary(&CD3DX12_SHADER_BYTECODE((void*)pBlob->GetBufferPointer(), pBlob->GetBufferSize()));
	pLib->DefineExport(shaderExport);
}

void App::CreateHitGroup(LPCWSTR shaderName, LPCWSTR shaderExport,  CD3DX12_STATE_OBJECT_DESC& pipelineDesc, D3D12_HIT_GROUP_TYPE hitGroupType)
{
	CD3DX12_HIT_GROUP_SUBOBJECT* pHitGroup = pipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	pHitGroup->SetClosestHitShaderImport(shaderName);
	pHitGroup->SetHitGroupExport(shaderExport);
	pHitGroup->SetHitGroupType(hitGroupType);
}

bool App::CompileShaders()
{
	ComPtr<IDxcBlob> pBlob;

	//Create different preprocessor define configs.
	LPCWSTR normal[] =
	{
		L"NORMAL_MAPPING=1"
	};

	LPCWSTR occlusion[] =
	{
		L"OCCLUSION_MAPPING=1",
	};

	LPCWSTR noMetallicRoughness[] =
	{
		L"NO_METALLIC_ROUGHNESS=1",
	};

	LPCWSTR normalOcclusion[] =
	{
		L"NORMAL_MAPPING=1",
		L"OCCLUSION_MAPPING=1"
	};

	LPCWSTR normalOcclusionEmission[] =
	{
		L"NORMAL_MAPPING=1",
		L"OCCLUSION_MAPPING=1",
		L"EMISSION_MAPPING=1"
	};

	//Create array of shaders to compile
	CompileRecord records[] =
	{
		//G Buffer pass shaders
		CompileRecord(L"Shaders/GBufferVertex.hlsl", m_wsGBufferVertexName, L"vs_6_3", L"main"),	//Vertex shader
		CompileRecord(L"Shaders/GBufferPixel.hlsl", m_GBufferPixelNames[(int)ShaderVersions::NOTHING], L"ps_6_3"),	//Nothing
		CompileRecord(L"Shaders/GBufferPixel.hlsl", m_GBufferPixelNames[(int)ShaderVersions::NORMAL], L"ps_6_3", L"main", normal, (int)_countof(normal)),	//Normal
		CompileRecord(L"Shaders/GBufferPixel.hlsl", m_GBufferPixelNames[(int)ShaderVersions::OCCLUSION], L"ps_6_3", L"main", occlusion, (int)_countof(occlusion)),	//Occlusion
		CompileRecord(L"Shaders/GBufferPixel.hlsl", m_GBufferPixelNames[(int)ShaderVersions::NORMAL_OCCLUSION], L"ps_6_3", L"main", normalOcclusion, (int)_countof(normalOcclusion)),	//Normal, Occlusion
		CompileRecord(L"Shaders/GBufferPixel.hlsl", m_GBufferPixelNames[(int)ShaderVersions::NORMAL_OCCLUSION_EMISSION], L"ps_6_3", L"main", normalOcclusionEmission, (int)_countof(normalOcclusionEmission)),	//Normal, Occlusion, Emission
		CompileRecord(L"Shaders/GBufferPixel.hlsl", m_GBufferPixelNames[(int)ShaderVersions::NO_METALLIC_ROUGHNESS], L"ps_6_3", L"main", noMetallicRoughness, (int)_countof(noMetallicRoughness)),	//No metallic roughness or anything else


		//Light pass shaders
		CompileRecord(L"Shaders/LightVertex.hlsl", m_wsLightVertexName, L"vs_6_3", L"main"),	//Vertex shader
		CompileRecord(L"Shaders/LightPixel.hlsl", m_wsLightPixelName, L"ps_6_3", L"main"),	//Pixel shader
	};

	for (int i = 0; i < _countof(records); ++i)
	{
		pBlob = DXRHelper::CompileShader(records[i].Filepath, records[i].ShaderVersion, records[i].Entrypoint, records[i].Defines, records[i].NumDefines);

		if (pBlob == nullptr)
		{
			return false;
		}

		m_Shaders[records[i].ShaderName] = pBlob.Get();
	}

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

	MeshManager::GetInstance()->LoadMesh("Models/Sponza/gLTF/Sponza.gltf", "FlightHelmet", m_pGraphicsCommandList.Get());
	MeshManager::GetInstance()->LoadMesh("Models/WaterBottle/gLTF/WaterBottle.gltf", "WaterBottle", m_pGraphicsCommandList.Get());
	MeshManager::GetInstance()->LoadMesh("Models/BoomBox/gLTF/BoomBox.gltf", "BoomBox", m_pGraphicsCommandList.Get());

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ExecuteCommandList();
}

bool App::CreateOutputBuffers()
{
	//Create raytracing output texture
	m_pRaytracingOutput->GetResourcePtr()->Reset();

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = m_BackBufferFormat;
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

	HRESULT hr = m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(m_pRaytracingOutput->GetResourcePtr()->GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create output resource!");

		return false;
	}

	//Create G Buffer textures and depth stencil texture
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	Texture** GBuffer;
	D3D12_CLEAR_VALUE GBufferClear;

	D3D12_CLEAR_VALUE DepthStencilClear;
	DepthStencilClear.Format = m_DepthStencilDSVFormat;
	DepthStencilClear.DepthStencil.Depth = 1.0f;
	DepthStencilClear.DepthStencil.Stencil = 0;

	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		GBuffer = GetGBuffer(i);

		for (int j = 0; j < (int)GBuffer::COUNT; ++j)
		{
			GBuffer[j]->GetResourcePtr()->Reset();

			resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			resDesc.Format = m_GBufferFormats[j];

			GBufferClear.Format = m_GBufferFormats[j];
			GBufferClear.Color[0] = m_GBufferClearColors[j][0];
			GBufferClear.Color[1] = m_GBufferClearColors[j][1];
			GBufferClear.Color[2] = m_GBufferClearColors[j][2];
			GBufferClear.Color[3] = m_GBufferClearColors[j][3];

			hr = m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &GBufferClear, IID_PPV_ARGS(GBuffer[j]->GetResourcePtr()->GetAddressOf()));

			if (FAILED(hr))
			{
				LOG_ERROR(tag, L"Failed to create a G buffer resource!");

				return false;
			}
		}

		//Create depth stenil buffer
		resDesc.Format = m_DepthStencilFormat;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		hr = m_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_DEPTH_READ, &DepthStencilClear, IID_PPV_ARGS(GetDepthStencilBuffer(i)->GetResourcePtr()->GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to create a depth stencil buffer!");

			return false;
		}
	}

	return true;
}

void App::CreateCBs()
{
	m_pScenePerFrameCBUpload = new UploadBuffer<ScenePerFrameCB>(m_pDevice.Get(), s_kuiSwapChainBufferCount, true);
	m_pPrimitiveInstanceCBUpload = new UploadBuffer<PrimitiveInstanceCB>(m_pDevice.Get(), MeshManager::GetInstance()->GetNumPrimitives(), false);

	//Reserve and populate CB vectors

	//Primitive per frame
	m_GameObjectPerFrameCBs.reserve(MeshManager::GetInstance()->GetNumActivePrimitives());

	for (int i = 0; i < MeshManager::GetInstance()->GetNumActivePrimitives(); ++i)
	{
		m_GameObjectPerFrameCBs.push_back(GameObjectPerFrameCB());
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
		m_FrameResources[i].m_pGameObjectPerFrameCBUpload = new UploadBuffer<GameObjectPerFrameCB>(m_pDevice.Get(), MeshManager::GetInstance()->GetNumActivePrimitives(), false);
		m_FrameResources[i].m_pPrimitiveIndexCBUpload = new UploadBuffer<PrimitiveIndexCB>(m_pDevice.Get(), MeshManager::GetInstance()->GetNumActivePrimitives(), true);
		m_FrameResources[i].m_pLightCBUpload = new UploadBuffer<LightCB>(m_pDevice.Get(), MAX_LIGHTS, false);
		m_FrameResources[i].m_pDeferredPerFrameCBUpload = new UploadBuffer<DeferredPerFrameCB>(m_pDevice.Get(), 1, true);

		if (m_FrameResources[i].m_pLightCBUpload->CreateSRV(m_pSRVHeap) == false)
		{
			return;
		}

		if (m_FrameResources[i].m_pGameObjectPerFrameCBUpload->CreateSRV(m_pSRVHeap) == false)
		{
			return;
		}
	}

	//Create descriptors to structured buffers
	if (m_pPrimitiveInstanceCBUpload->CreateSRV(m_pSRVHeap) == false)
	{
		LOG_ERROR(tag, L"Failed to create primitive per instance descriptor!");

		return;
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
	Camera* pCamera = new DebugCamera(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 0, 0), XMFLOAT3(0, 1, 0), m_fCameraNearDepth, m_fCameraFarDepth, "BasicCamera");

	ObjectManager::GetInstance()->AddCamera(pCamera);

	ObjectManager::GetInstance()->SetActiveCamera(pCamera->GetName());
}

void App::CreateScreenQuad()
{
	HRESULT hr = m_pGraphicsCommandList->Reset(GetCommandAllocator(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the graphics command list!");

		return;
	}

	std::array<ScreenQuadVertex, 4> vertices =
	{
		ScreenQuadVertex(XMFLOAT3(-1, 1, 0), XMFLOAT2(0, 0)),	//Top left
		ScreenQuadVertex(XMFLOAT3(1, 1, 0), XMFLOAT2(1, 0)),	//Top right
		ScreenQuadVertex(XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1)),	//Bottom left
		ScreenQuadVertex(XMFLOAT3(1, -1, 0), XMFLOAT2(1, 1))	//Bottom right
	};

	m_pScreenQuadVertexBufferGPU = DXRHelper::CreateDefaultBuffer(m_pDevice.Get(), m_pGraphicsCommandList.Get(), vertices.data(), (UINT)sizeof(Vertex) * 4, m_pScreenQuadVertexBufferUploader);

	ExecuteCommandList();
}

void App::InitScene()
{
	Mesh* pMesh = nullptr;
	MeshManager::GetInstance()->GetMesh("FlightHelmet", pMesh);

	GameObject* pGameObject = new GameObject();
	pGameObject->Init("Fish", XMFLOAT3(5, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(3, 3, 3), pMesh);

	MeshManager::GetInstance()->GetMesh("WaterBottle", pMesh);

	pGameObject = new GameObject();
	pGameObject->Init("WaterBottle", XMFLOAT3(5, 0, 5), XMFLOAT3(0, 0, 0), XMFLOAT3(4.0f, 4.0f, 4.0f), pMesh);

	MeshManager::GetInstance()->GetMesh("BoomBox", pMesh);

	pGameObject = new GameObject();
	pGameObject->Init("BoomBox", XMFLOAT3(5, 0, -5), XMFLOAT3(0, 0, 0), XMFLOAT3(25, 25, 25), pMesh);

	GIVolumeDesc volumeDesc;
	volumeDesc.Position = XMFLOAT3();
	volumeDesc.ProbeCounts = XMINT3(5, 5, 5);
	volumeDesc.ProbeRelocation = false;
	volumeDesc.ProbeScale = 1.0f;
	volumeDesc.ProbeSpacing = XMFLOAT3(5, 5, 5);
	volumeDesc.ProbeTracking = false;

	m_pGIVolume = new GIVolume(volumeDesc, m_pGraphicsCommandList.Get(), m_pSRVHeap, m_pRTVHeap);

	CreateCameras();
}

void App::InitConstantBuffers()
{
	m_uiNumLights = 1;

	for (UINT i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		UpdatePerFrameCB(i);
	}

	//m_LightCBs[0].Color = XMFLOAT4(1, 0, 0, 1);
	m_LightCBs[0].Color = XMFLOAT4(0.93f, 0.19f, 0.14f, 1);
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

void App::InitImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(WindowManager::GetInstance()->GetHWND());
	ImGui_ImplDX12_Init(m_pDevice.Get(), s_kuiSwapChainBufferCount, m_BackBufferFormat, m_pImGuiSRVHeap->GetHeap().Get(), m_pImGuiSRVHeap->GetCpuDescriptorHandle(), m_pImGuiSRVHeap->GetGpuDescriptorHandle());
}

void App::UpdateImGui(const Timer& kTimer)
{
	ImGuiIO& io = ImGui::GetIO();

	io.DeltaTime = kTimer.DeltaTime();
}

void App::DrawImGui()
{
	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Render ImGui"));

	std::vector<ID3D12DescriptorHeap*> heaps = { m_pImGuiSRVHeap->GetHeap().Get() };
	m_pGraphicsCommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	ImGui::Begin("Options");

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	m_pGIVolume->ShowUI();

	DebugHelper::ShowUI();

	ImGui::End();

	ImGui::Render();

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pGraphicsCommandList.Get());

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	PIX_ONLY(PIXEndEvent());
}

void App::ShutdownImGui()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void App::UpdatePerFrameCB(UINT uiFrameIndex)
{
	Camera* pCamera = ObjectManager::GetInstance()->GetActiveCamera();

	XMMATRIX world = XMLoadFloat4x4(&pCamera->GetViewProjectionMatrix());

	m_PerFrameCBs[uiFrameIndex].EyePosW = pCamera->GetPosition();
	m_PerFrameCBs[uiFrameIndex].EyeDirection = pCamera->GetForwardVector();
	m_PerFrameCBs[uiFrameIndex].InvViewProjection = XMMatrixTranspose(XMMatrixInverse(nullptr, world));
	m_PerFrameCBs[uiFrameIndex].ViewProjection = XMMatrixTranspose(world);
	m_PerFrameCBs[uiFrameIndex].NumLights = (int)m_uiNumLights;
	m_PerFrameCBs[uiFrameIndex].LightIndex = (int)GetLightUploadBuffer(uiFrameIndex)->GetDesc()->GetDescriptorIndex();
	m_PerFrameCBs[uiFrameIndex].PrimitivePerFrameIndex = (int)GetPrimitiveUploadBuffer(uiFrameIndex)->GetDesc()->GetDescriptorIndex();
	m_PerFrameCBs[uiFrameIndex].PrimitivePerInstanceIndex = (int)m_pPrimitiveInstanceCBUpload->GetDesc()->GetDescriptorIndex();
	m_PerFrameCBs[uiFrameIndex].ScreenWidth = WindowManager::GetInstance()->GetWindowWidth();
	m_PerFrameCBs[uiFrameIndex].ScreenHeight = WindowManager::GetInstance()->GetWindowHeight();

	m_pScenePerFrameCBUpload->CopyData(uiFrameIndex, m_PerFrameCBs[uiFrameIndex]);

	//Update primitive per frame constant buffers
	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	const MeshNode* pNode;
	Mesh* pMesh;

	XMMATRIX invWorld;

	UINT uiNumPrimitives = 0;

	for (std::unordered_map<std::string, GameObject*>::iterator it = pGameObjects->begin(); it != pGameObjects->end(); ++it)
	{
		pMesh = it->second->GetMesh();

		uiNumPrimitives = 0;

		for (int i = 0; i < pMesh->GetNodes()->size(); ++i)
		{
			pNode = pMesh->GetNode(i);

			//Assign per primitive information
			for (int i = 0; i < pNode->m_Primitives.size(); ++i)
			{
				invWorld = XMLoadFloat4x4(&pNode->m_Transform) * XMLoadFloat4x4(&it->second->GetWorldMatrix());

				m_GameObjectPerFrameCBs[it->second->GetIndex() + uiNumPrimitives].World = XMMatrixTranspose(invWorld);

				invWorld.r[3] = XMVectorSet(0, 0, 0, 1);

				m_GameObjectPerFrameCBs[it->second->GetIndex() + uiNumPrimitives].InvTransposeWorld = XMMatrixInverse(nullptr, invWorld);

				++uiNumPrimitives;
			}
		}
	}

	GetPrimitiveUploadBuffer(uiFrameIndex)->CopyData(0, m_GameObjectPerFrameCBs);
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

void App::ResetCommandList()
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

	std::vector<ID3D12DescriptorHeap*> heaps = { m_pSRVHeap->GetHeap().Get() };
	m_pGraphicsCommandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

UINT App::GetFrameIndex() const
{
	return m_uiFrameIndex;
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

UploadBuffer<GameObjectPerFrameCB>* App::GetPrimitiveUploadBuffer()
{
	return m_FrameResources[m_uiFrameIndex].m_pGameObjectPerFrameCBUpload;
}

UploadBuffer<GameObjectPerFrameCB>* App::GetPrimitiveUploadBuffer(int iIndex)
{
	return m_FrameResources[iIndex].m_pGameObjectPerFrameCBUpload;
}

UploadBuffer<LightCB>* App::GetLightUploadBuffer()
{
	return m_FrameResources[m_uiFrameIndex].m_pLightCBUpload;
}

UploadBuffer<LightCB>* App::GetLightUploadBuffer(int iIndex)
{
	return m_FrameResources[iIndex].m_pLightCBUpload;
}

UploadBuffer<PrimitiveIndexCB>* App::GetPrimitiveIndexUploadBuffer()
{
	return m_FrameResources[m_uiFrameIndex].m_pPrimitiveIndexCBUpload;
}

UploadBuffer<PrimitiveIndexCB>* App::GetPrimitiveIndexUploadBuffer(int iIndex)
{
	return m_FrameResources[iIndex].m_pPrimitiveIndexCBUpload;
}

UploadBuffer<DeferredPerFrameCB>* App::GetDeferredPerFrameUploadBuffer()
{
	return m_FrameResources[m_uiFrameIndex].m_pDeferredPerFrameCBUpload;
}

UploadBuffer<DeferredPerFrameCB>* App::GetDeferredPerFrameUploadBuffer(int iIndex)
{
	return m_FrameResources[iIndex].m_pDeferredPerFrameCBUpload;
}

Texture** App::GetGBuffer()
{
	return m_FrameResources[m_uiFrameIndex].m_GBuffer;
}

Texture** App::GetGBuffer(int iIndex)
{
	return m_FrameResources[iIndex].m_GBuffer;
}

Texture* App::GetDepthStencilBuffer()
{
	return m_FrameResources[m_uiFrameIndex].m_pDepthStencilBuffer;
}

Texture* App::GetDepthStencilBuffer(int iIndex)
{
	return m_FrameResources[iIndex].m_pDepthStencilBuffer;
}

void App::SetDepthStencilBuffer(int iIndex, Texture* pTexture)
{
	m_FrameResources[iIndex].m_pDepthStencilBuffer = pTexture;
}

Descriptor* App::GetDepthStencilBufferView()
{
	return m_FrameResources[m_uiFrameIndex].m_pDepthStencilBufferView;
}

Descriptor* App::GetDepthStencilBufferView(int iIndex)
{
	return m_FrameResources[iIndex].m_pDepthStencilBufferView;
}

Descriptor** App::GetGBufferRTVDescs()
{
	return m_FrameResources[m_uiFrameIndex].m_pGBufferRTVDescs;
}

Descriptor** App::GetGBufferRTVDescs(int iIndex)
{
	return m_FrameResources[iIndex].m_pGBufferRTVDescs;
}

void App::SetDepthStencilBufferView(int iIndex, Descriptor* pDesc)
{
	if (m_FrameResources[iIndex].m_pDepthStencilBufferView != nullptr)
	{
		delete m_FrameResources[iIndex].m_pDepthStencilBufferView;
	}

	m_FrameResources[iIndex].m_pDepthStencilBufferView = pDesc;
}

bool App::CreateSignatures()
{
	//Deferred pass signatures
	if (CreateGBufferSignature() == false)
	{
		return false;
	}

	if (CreateLightSignature() == false)
	{
		return false;
	}

	return true;
}

bool App::CreateGBufferSignature()
{
	D3D12_ROOT_PARAMETER1 slotRootParameter[DeferredPass::GBufferPass::COUNT] = {};

	//SRV descriptors
	slotRootParameter[DeferredPass::GBufferPass::STANDARD_DESCRIPTORS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	slotRootParameter[DeferredPass::GBufferPass::STANDARD_DESCRIPTORS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	slotRootParameter[DeferredPass::GBufferPass::STANDARD_DESCRIPTORS].DescriptorTable.pDescriptorRanges = DXRHelper::GetDescriptorRanges();
	slotRootParameter[DeferredPass::GBufferPass::STANDARD_DESCRIPTORS].DescriptorTable.NumDescriptorRanges = s_kuiNumStandardDescriptorRanges;

	//Scene per frame CB
	slotRootParameter[DeferredPass::GBufferPass::PER_FRAME_SCENE_CB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	slotRootParameter[DeferredPass::GBufferPass::PER_FRAME_SCENE_CB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	slotRootParameter[DeferredPass::GBufferPass::PER_FRAME_SCENE_CB].Descriptor.RegisterSpace = 0;
	slotRootParameter[DeferredPass::GBufferPass::PER_FRAME_SCENE_CB].Descriptor.ShaderRegister = 0;
	slotRootParameter[DeferredPass::GBufferPass::PER_FRAME_SCENE_CB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	//Primitive index CB
	slotRootParameter[DeferredPass::GBufferPass::PRIMITIVE_INDEX_CB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	slotRootParameter[DeferredPass::GBufferPass::PRIMITIVE_INDEX_CB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	slotRootParameter[DeferredPass::GBufferPass::PRIMITIVE_INDEX_CB].Descriptor.RegisterSpace = 0;
	slotRootParameter[DeferredPass::GBufferPass::PRIMITIVE_INDEX_CB].Descriptor.ShaderRegister = 1;
	slotRootParameter[DeferredPass::GBufferPass::PRIMITIVE_INDEX_CB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = DXRHelper::GetStaticSamplers();

	return DXRHelper::CreateRootSignature((D3D12_ROOT_PARAMETER1*)slotRootParameter, (int)_countof(slotRootParameter), staticSamplers.data(), (int)staticSamplers.size(), m_pGBufferRootSignature.GetAddressOf(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

bool App::CreateLightSignature()
{
	D3D12_ROOT_PARAMETER1 slotRootParameter[DeferredPass::GBufferPass::COUNT] = {};

	//SRV descriptors
	slotRootParameter[DeferredPass::LightPass::STANDARD_DESCRIPTORS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	slotRootParameter[DeferredPass::LightPass::STANDARD_DESCRIPTORS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	slotRootParameter[DeferredPass::LightPass::STANDARD_DESCRIPTORS].DescriptorTable.pDescriptorRanges = DXRHelper::GetDescriptorRanges();
	slotRootParameter[DeferredPass::LightPass::STANDARD_DESCRIPTORS].DescriptorTable.NumDescriptorRanges = s_kuiNumStandardDescriptorRanges;

	//Scene per frame CB
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_SCENE_CB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_SCENE_CB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_SCENE_CB].Descriptor.RegisterSpace = 0;
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_SCENE_CB].Descriptor.ShaderRegister = 0;
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_SCENE_CB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	//Deferred per frame CB
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_DEFERRED_CB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_DEFERRED_CB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_DEFERRED_CB].Descriptor.RegisterSpace = 0;
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_DEFERRED_CB].Descriptor.ShaderRegister = 1;
	slotRootParameter[DeferredPass::LightPass::PER_FRAME_DEFERRED_CB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = DXRHelper::GetStaticSamplers();

	return DXRHelper::CreateRootSignature((D3D12_ROOT_PARAMETER1*)slotRootParameter, (int)_countof(slotRootParameter), staticSamplers.data(), (int)staticSamplers.size(), m_pLightRootSignature.GetAddressOf(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void App::PopulateDescriptorHeaps()
{
	//Create output descriptor
	UINT uiDescIndex;
	
	if (m_pSRVHeap->Allocate(uiDescIndex) == false)
	{
		return;
	}

	m_pRaytracingOutput->CreateUAVDesc(m_pSRVHeap);

	Texture** GBuffer;

	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		GBuffer = GetGBuffer(i);

		for (int j = 0; j < (int)GBuffer::COUNT; ++j)
		{
			GBuffer[j]->CreateSRVDesc(m_pSRVHeap);
		}

		GetDepthStencilBuffer(i)->CreateSRVDesc(m_pSRVHeap, m_DepthStencilSRVFormat);
	}

	MeshManager::GetInstance()->CreateDescriptors(m_pSRVHeap);
}

void App::PopulatePrimitivePerInstanceCB()
{
	PrimitiveInstanceCB primitiveInstanceCB;
	std::unordered_map<std::string, Mesh*>* pMeshes = MeshManager::GetInstance()->GetMeshes();
	std::vector<MeshNode*>* pNodes = nullptr;

	for (std::unordered_map<std::string, Mesh*>::iterator it = pMeshes->begin(); it != pMeshes->end(); ++it)
	{
		pNodes = it->second->GetNodes();

		for (int i = 0; i < pNodes->size(); ++i)
		{
			for (int j = 0; j < pNodes->at(i)->m_Primitives.size(); ++j)
			{
				primitiveInstanceCB.AlbedoIndex = it->second->GetTextures()->at(pNodes->at(i)->m_Primitives[j]->m_iAlbedoIndex)->GetSRVDesc()->GetDescriptorIndex();

				if (pNodes->at(i)->m_Primitives[j]->HasAttribute(PrimitiveAttributes::METALLIC_ROUGHNESS) == true)
				{
					primitiveInstanceCB.MetallicRoughnessIndex = it->second->GetTextures()->at(pNodes->at(i)->m_Primitives[j]->m_iMetallicRoughnessIndex)->GetSRVDesc()->GetDescriptorIndex();
				}

				if (pNodes->at(i)->m_Primitives[j]->HasAttribute(PrimitiveAttributes::NORMAL) == true)
				{
					primitiveInstanceCB.NormalIndex = it->second->GetTextures()->at(pNodes->at(i)->m_Primitives[j]->m_iNormalIndex)->GetSRVDesc()->GetDescriptorIndex();
				}

				if (pNodes->at(i)->m_Primitives[j]->HasAttribute(PrimitiveAttributes::OCCLUSION) == true)
				{
					primitiveInstanceCB.OcclusionIndex = it->second->GetTextures()->at(pNodes->at(i)->m_Primitives[j]->m_iOcclusionIndex)->GetSRVDesc()->GetDescriptorIndex();
				}

				primitiveInstanceCB.IndicesIndex = pNodes->at(i)->m_Primitives[j]->m_pIndexDesc->GetDescriptorIndex();
				primitiveInstanceCB.VerticesIndex = pNodes->at(i)->m_Primitives[j]->m_pVertexDesc->GetDescriptorIndex();

				m_pPrimitiveInstanceCBUpload->CopyData(pNodes->at(i)->m_Primitives[j]->m_iIndex, primitiveInstanceCB);
			}
		}
	}
}

void App::PopulatePrimitiveIndexCB()
{
	PROFILE("Populate Primitive Index");

	PrimitiveIndexCB primIndexCB;

	for (std::unordered_map<PrimitiveAttributes, std::vector<RenderInfo>>::iterator it = m_RenderInfoQueue.begin(); it != m_RenderInfoQueue.end(); ++it)
	{
		for (int i = 0; i < it->second.size(); ++i)
		{
			primIndexCB.InstanceIndex = (UINT32)it->second[i].m_uiInstanceIndex;
			primIndexCB.PrimitiveIndex = (UINT32)it->second[i].m_uiPrimitiveIndex;

			GetPrimitiveIndexUploadBuffer()->CopyData(primIndexCB.InstanceIndex, primIndexCB);
		}
	}
}

void App::PopulateDeferredPerFrameCB()
{
	DeferredPerFrameCB deferredPerFrameCB;

	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		deferredPerFrameCB.NormalIndex = m_FrameResources[i].m_GBuffer[(int)GBuffer::NORMAL]->GetSRVDesc()->GetDescriptorIndex();
		deferredPerFrameCB.AlbedoIndex = m_FrameResources[i].m_GBuffer[(int)GBuffer::ALBEDO]->GetSRVDesc()->GetDescriptorIndex();
		deferredPerFrameCB.MetallicRoughnessOcclusion = m_FrameResources[i].m_GBuffer[(int)GBuffer::METALLIC_ROUGHNESS_OCCLUSION]->GetSRVDesc()->GetDescriptorIndex();
		deferredPerFrameCB.DepthIndex = m_FrameResources[i].m_pDepthStencilBuffer->GetSRVDesc()->GetDescriptorIndex();

		GetDeferredPerFrameUploadBuffer(i)->CopyData(0, deferredPerFrameCB);
	}
}

void App::PopulateRenderInfoQueue()
{
	PROFILE("Populate Render Info");

	for (std::unordered_map<PrimitiveAttributes, std::vector<RenderInfo>>::iterator it = m_RenderInfoQueue.begin(); it != m_RenderInfoQueue.end(); ++it)
	{
		it->second.clear();
	}

	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	for (std::unordered_map<std::string, GameObject*>::iterator it = pGameObjects->begin(); it != pGameObjects->end(); ++it)
	{
		if (it->second->IsRendering() == true)
		{
			it->second->CreateRenderInfo(m_RenderInfoQueue);
		}
	}
}

bool App::CreateDescriptorHeaps()
{
	m_pSRVHeap = new DescriptorHeap();

	//2 descriptors per primitive (index and vertex buffers), 1 descriptor per texture, 1 extra descriptor for output texture, 2 extra per in flight frame for structured buffers and 1 extra for primitive instance structured buffer, 4 extra per in flight frame for G Buffer, 1 extra per in flight frame for depth buffer, 8 for Global illumination
	if (m_pSRVHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, (MeshManager::GetInstance()->GetNumPrimitives() * 2) + TextureManager::GetInstance()->GetNumTextures() + 30) == false)
	{
		return false;
	}

	m_pImGuiSRVHeap = new DescriptorHeap();

	if (m_pImGuiSRVHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10) == false)
	{
		return false;
	}

	m_pRTVHeap = new DescriptorHeap();

	if (m_pRTVHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, s_kuiSwapChainBufferCount + ((int)GBuffer::COUNT) * s_kuiSwapChainBufferCount + 2) == false)
	{
		return false;
	}

	m_pDSVHeap = new DescriptorHeap();

	//1 desc per in flight frame for depth stencil buffer
	if (m_pDSVHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, s_kuiSwapChainBufferCount) == false)
	{
		return false;
	}

	return true;
}
