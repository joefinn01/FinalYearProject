#pragma once

#include "Include/DirectX/d3dx12.h"

#include <wrl/client.h>

class Descriptor;
class DescriptorHeap;

class Texture
{
public:
	Texture(ID3D12Resource* pTextureRes, DXGI_FORMAT textureFormat);

	bool CreateSRVDesc(DescriptorHeap* pHeap);
	bool CreateSRVDesc(DescriptorHeap* pHeap, DXGI_FORMAT format);
	void RecreateSRVDesc(DescriptorHeap* pHeap);
	void RecreateSRVDesc(DescriptorHeap* pHeap, DXGI_FORMAT format);

	bool CreateUAVDesc(DescriptorHeap* pHeap);
	void RecreateUAVDesc(DescriptorHeap* pHeap);

	bool CreateRTVDesc(DescriptorHeap* pHeap);

	bool CreateResource(UINT64 uiWidth, UINT uiHeight, UINT16 uiMipLevels, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state);

	Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() const;
	Microsoft::WRL::ComPtr<ID3D12Resource>* GetResourcePtr();

	Microsoft::WRL::ComPtr<ID3D12Resource> GetUpload() const;
	Microsoft::WRL::ComPtr<ID3D12Resource>* GetUploadPtr();

	Descriptor* GetSRVDesc() const;
	Descriptor* GetUAVDesc() const;

	DXGI_FORMAT GetFormat() const;

	void SetFormat(DXGI_FORMAT format);

protected:

private:
	Descriptor* m_pSRVDesc = nullptr;
	Descriptor* m_pUAVDesc = nullptr;
	Descriptor* m_pRTVDesc = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pTexture = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pUploadHeap = nullptr;

	DXGI_FORMAT m_Format;
};

