#pragma once

#include <wrl.h>
#include <Include/DirectX/d3dx12.h>

struct IDxcBlob;
struct DxcDefine;

class DXRHelper
{
public:
	static IDxcBlob* CompileShader(LPCWSTR wsFilename, LPCWSTR wsEntrypoint = L"", DxcDefine* pDefines = nullptr, UINT32 uiDefineCount = 0);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pGraphicsCommandList, const void* pData, UINT64 uiByteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& pUploadBuffer);

	static bool CreateUAVBuffer(ID3D12Device* pDevice, UINT64 uiBufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON);

protected:

private:

};

