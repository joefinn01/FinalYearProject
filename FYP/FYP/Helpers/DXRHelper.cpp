#include "DXRHelper.h"
#include "Helpers/DebugHelper.h"

#include <dxcapi.h>
#include <fstream>
#include <sstream>

Tag tag = L"DXRHelper";

IDxcBlob* DXRHelper::CompileShader(LPCWSTR wsFilename, LPCWSTR wsTargetLevel, LPCWSTR wsEntrypoint, LPCWSTR* pDefines, UINT32 uiDefineCount)
{
	//Create compiler, library and include handler
	Microsoft::WRL::ComPtr<IDxcCompiler3> pCompiler = nullptr;
	Microsoft::WRL::ComPtr<IDxcLibrary> pLibrary = nullptr;
	Microsoft::WRL::ComPtr<IDxcUtils> pUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> pIncludeHandler = nullptr;

	HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(pCompiler.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create dxc compiler!");

		return nullptr;
	}

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create dxc utils!");

		return nullptr;
	}

	hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(pLibrary.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create dxc library!");

		return nullptr;
	}

	hr = pUtils->CreateDefaultIncludeHandler(pIncludeHandler.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create dxc include handler!");

		return nullptr;
	}

	Microsoft::WRL::ComPtr<IDxcBlobEncoding> pSource = nullptr;
	hr = pUtils->LoadFile(wsFilename, nullptr, pSource.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to load shader file!");

		return nullptr;
	}

	DxcBuffer buffer;
	buffer.Ptr = pSource->GetBufferPointer();
	buffer.Size = pSource->GetBufferSize();
	buffer.Encoding = DXC_CP_ACP;

	std::vector<LPCWSTR> args;

	args.push_back(wsFilename);

	//Only add entry point if one has been provided
	if (wsEntrypoint != L"")
	{
		args.push_back(L"-E");
		args.push_back(wsEntrypoint);
	}

	args.push_back(L"-T");	//Target
	args.push_back(wsTargetLevel);

#if _DEBUG
	args.push_back(L"-Zi");	//Attach debug information
	args.push_back(L"-Od");	//Disable shader optimization
#endif

	for (int i = 0; i < uiDefineCount; ++i)
	{
		args.push_back(L"-D");
		args.push_back(pDefines[i]);
	}

	Microsoft::WRL::ComPtr<IDxcResult> pResult = nullptr;
	hr = pCompiler->Compile(&buffer, args.data(), args.size(), pIncludeHandler.Get(), IID_PPV_ARGS(pResult.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to compile the shader!");

		return nullptr;
	}

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> pError = nullptr;
	hr = pResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pError.GetAddressOf()), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to get the error output of the compiled shader!");

		return nullptr;
	}

	if (pError != nullptr && pError->GetStringLength() != 0)
	{
		OutputDebugStringA(pError->GetStringPointer());

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

	hr = pResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pBlob), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to get shader blob from result!");

		return nullptr;
	}

#if _DEBUG
	Microsoft::WRL::ComPtr<IDxcBlob> pDebugData;
	Microsoft::WRL::ComPtr<IDxcBlobUtf16> pDebugDataPath;
	pResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pDebugData.GetAddressOf()), pDebugDataPath.GetAddressOf());

	std::wstring tempWString = std::wstring(pDebugDataPath->GetStringPointer());
	std::string sPdBName = std::string(tempWString.begin(), tempWString.end());

	sPdBName = std::getenv("LOCALAPPDATA") + std::string("\\Temp\\") + sPdBName;

	FILE* pFile = nullptr;
	fopen_s(&pFile, sPdBName.c_str(), "wb");

	if (pFile != nullptr)
	{
		fwrite(pDebugData->GetBufferPointer(), 1, pDebugData->GetBufferSize(), pFile);

		fclose(pFile);
	}
	else
	{
		LOG_WARNING(tag, L"Failed to write PDB for shader!");
	}
#endif

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
