#pragma once

#include <wrl.h>
#include <Include/DirectX/d3dx12.h>
#include <array>

struct IDxcBlob;
struct DxcDefine;

struct CompileRecord
{
	CompileRecord(LPCWSTR wsFilename, LPCWSTR wsShaderName, LPCWSTR wsShaderVersion, LPCWSTR wsEntrypoint = L"", LPCWSTR* pDefines = nullptr, int iDefineCount = 0)
	{
		Filepath = wsFilename;
		ShaderName = wsShaderName;
		Entrypoint = wsEntrypoint;
		ShaderVersion = wsShaderVersion;
		Defines = pDefines;
		NumDefines = iDefineCount;
	}

	LPCWSTR Filepath;
	LPCWSTR ShaderName;
	LPCWSTR Entrypoint;
	LPCWSTR ShaderVersion;
	LPCWSTR* Defines;
	int NumDefines;
};

class DXRHelper
{
public:
	static IDxcBlob* CompileShader(LPCWSTR wsFilename, LPCWSTR wsTargetLevel, LPCWSTR wsEntrypoint = L"", LPCWSTR* pDefines = nullptr, UINT32 uiDefineCount = 0);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pGraphicsCommandList, const void* pData, UINT64 uiByteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& pUploadBuffer);

	static bool CreateUAVBuffer(ID3D12Device* pDevice, UINT64 uiBufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);

	static bool CreateRootSignature(D3D12_ROOT_PARAMETER1* rootParams, int iNumRootParams, const CD3DX12_STATIC_SAMPLER_DESC* staticSamplers, int iNumStaticSamplers, ID3D12RootSignature** ppRootSignature, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

	static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	static D3D12_DESCRIPTOR_RANGE1* GetDescriptorRanges();

protected:

private:

};

