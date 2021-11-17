#include "DXRHelper.h"
#include "Helpers/DebugHelper.h"

#include <dxcapi.h>
#include <fstream>
#include <sstream>

Tag tag = L"DXRHelper";

IDxcBlob* DXRHelper::CompileShader(LPCWSTR wsFilename, LPCWSTR wsEntrypoint, DxcDefine* pDefines, UINT32 uiDefineCount)
{
	//Create compiler, library and include handler
	IDxcCompiler* pCompiler = nullptr;
	IDxcLibrary* pLibrary = nullptr;
	IDxcIncludeHandler* pDxcIncludeHandler;

	HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)&pCompiler);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create dxc compiler!");

		return nullptr;
	}

	hr = DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void**)&pLibrary);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create dxc library!");

		return nullptr;
	}

	hr = pLibrary->CreateIncludeHandler(&pDxcIncludeHandler);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create dxc include handler!");

		return nullptr;
	}

	//Open shader file
	std::ifstream file = std::ifstream(wsFilename);

	if (file.good() == false)
	{
		LOG_ERROR(tag, L"Failed to open file with path %ls!", wsFilename);

		return nullptr;
	}

	//Read shader file
	std::stringstream stringStream;
	stringStream << file.rdbuf();

	std::string sShader = stringStream.str();

	//Create blob
	IDxcBlobEncoding* pBlobEncoding = nullptr;

	hr = pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)sShader.c_str(), (uint32_t)sShader.size(), 0, &pBlobEncoding);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create encoded blob from file %ls!", wsFilename);

		return nullptr;
	}

	//Compile shader
	IDxcOperationResult* pResult = nullptr;

	LPCWSTR* arguments = nullptr;
	UINT32 uiArgCount = 0;

	//If debug then set flags to make debugging possible
#if _DEBUG
	uiArgCount = 2;

	arguments = new LPCWSTR[uiArgCount];

	arguments[0] = L"-Zi";	//Attach debug information
	arguments[1] = L"-Od";	//Disable shader optimization
#endif

	hr = pCompiler->Compile(pBlobEncoding, wsFilename, wsEntrypoint, L"lib_6_3", arguments, uiArgCount, pDefines, uiDefineCount, pDxcIncludeHandler, &pResult);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to compile shader from file %ls!", wsFilename);

		return nullptr;
	}

	//Check compiled shader
	HRESULT resultCode;

	hr = pResult->GetStatus(&resultCode);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to get the status from the result code!");

		return nullptr;
	}

	if (FAILED(resultCode))
	{
		LOG_ERROR(tag, L"The result code returned when compiling shader failed!");

		IDxcBlobEncoding* pError;
		hr = pResult->GetErrorBuffer(&pError);

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to get shader error!");

			return nullptr;
		}

		OutputDebugStringA((char*)pError->GetBufferPointer());

		return nullptr;
	}

	IDxcBlob* pBlob;

	hr = pResult->GetResult(&pBlob);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to get shader blob from result!");

		return nullptr;
	}

	return pBlob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DXRHelper::CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pGraphicsCommandList, const void* pData, UINT64 uiByteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& pUploadBuffer)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pDefaultBuffer;

	HRESULT hr = pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uiByteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDefaultBuffer.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the default buffer!");

		return nullptr;
	}

	hr = pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uiByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pUploadBuffer.GetAddressOf()));


	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pData;
	subResourceData.RowPitch = static_cast<LONG_PTR>(uiByteSize);
	subResourceData.SlicePitch = subResourceData.RowPitch;

	pGraphicsCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources<1>(pGraphicsCommandList,
		pDefaultBuffer.Get(),
		pUploadBuffer.Get(),
		0,
		0,
		1,
		&subResourceData);

	pGraphicsCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	return pDefaultBuffer;
}

bool DXRHelper::CreateUAVBuffer(ID3D12Device* pDevice, UINT64 uiBufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialState)
{
	CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uiBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	HRESULT hr = pDevice->CreateCommittedResource(&uploadHeapProperties,
												D3D12_HEAP_FLAG_NONE,
												&bufferDesc,
												initialState,
												nullptr,
												IID_PPV_ARGS(ppResource));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create UAV buffer!");

		return false;
	}

	return true;
}
