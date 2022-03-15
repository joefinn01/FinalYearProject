#include "DXRHelper.h"
#include "Helpers/DebugHelper.h"
#include "Apps/App.h"

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

bool DXRHelper::CreateRootSignature(D3D12_ROOT_PARAMETER1* rootParams, int iNumRootParams, const CD3DX12_STATIC_SAMPLER_DESC* staticSamplers, int iNumStaticSamplers, ID3D12RootSignature** ppRootSignature, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	D3D12_ROOT_SIGNATURE_DESC1 rootSignatureDesc = {};
	rootSignatureDesc.NumParameters = iNumRootParams;
	rootSignatureDesc.pParameters = rootParams;
	rootSignatureDesc.NumStaticSamplers = iNumStaticSamplers;
	rootSignatureDesc.pStaticSamplers = staticSamplers;
	rootSignatureDesc.Flags = flags;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc = { };
	versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	versionedDesc.Desc_1_1 = rootSignatureDesc;

	Microsoft::WRL::ComPtr<ID3DBlob> pSignature;
	Microsoft::WRL::ComPtr<ID3DBlob> pError;

	HRESULT hr = D3D12SerializeVersionedRootSignature(&versionedDesc, pSignature.GetAddressOf(), pError.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create serialize root signature!");

		OutputDebugStringA((char*)pError->GetBufferPointer());

		return false;
	}

	hr = App::GetApp()->GetDevice()->CreateRootSignature(1, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(ppRootSignature));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create root signature!");

		return false;
	}

	return true;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> DXRHelper::GetStaticSamplers()
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

D3D12_DESCRIPTOR_RANGE1* DXRHelper::GetDescriptorRanges()
{
	D3D12_DESCRIPTOR_RANGE1* ranges = new D3D12_DESCRIPTOR_RANGE1[App::GetApp()->GetNumStandardDescriptorRanges()];

	UINT32 userIndex = App::GetApp()->GetNumStandardDescriptorRanges() - App::GetApp()->GetNumUserDescriptorRanges();
	for (UINT32 i = 0; i < App::GetApp()->GetNumStandardDescriptorRanges(); ++i)
	{
		ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[i].NumDescriptors = UINT_MAX;
		ranges[i].BaseShaderRegister = 0;
		ranges[i].RegisterSpace = i;
		ranges[i].OffsetInDescriptorsFromTableStart = 0;
		ranges[i].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		if (i >= userIndex)
		{
			ranges[i].RegisterSpace = (i - userIndex) + 100;
		}
	}

	return ranges;
}
