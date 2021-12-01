#pragma once

#include "Include/DirectX/d3dx12.h"

#include <wrl/client.h>

class Descriptor;
class DescriptorHeap;

class Texture
{
public:
	bool CreateDescriptor(DescriptorHeap* pHeap);

	Microsoft::WRL::ComPtr<ID3D12Resource> GetTexture() const;
	Microsoft::WRL::ComPtr<ID3D12Resource>* GetTexturePtr();

	Microsoft::WRL::ComPtr<ID3D12Resource> GetUpload() const;
	Microsoft::WRL::ComPtr<ID3D12Resource>* GetUploadPtr();

	Descriptor* GetDescriptor() const;

	DXGI_FORMAT GetFormat() const;

	void SetFormat(DXGI_FORMAT format);

protected:

private:
	Descriptor* m_pDescriptor = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pTexture = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pUploadHeap = nullptr;

	DXGI_FORMAT m_Format;
};

