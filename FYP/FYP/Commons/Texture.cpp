#include "Texture.h"
#include "Commons/DescriptorHeap.h"
#include "Commons/SRVDescriptor.h"

bool Texture::CreateDescriptor(DescriptorHeap* pHeap)
{
	UINT uiIndex;

	if (pHeap->Allocate(uiIndex) == false)
	{
		return false;
	}

	m_pDescriptor = new SRVDescriptor(uiIndex, pHeap->GetCpuDescriptorHandle(uiIndex), m_pTexture.Get(), m_Format, 1);

	return true;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Texture::GetTexture() const
{
	return m_pTexture;
}

Microsoft::WRL::ComPtr<ID3D12Resource>* Texture::GetTexturePtr()
{
	return &m_pTexture;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Texture::GetUpload() const
{
	return m_pUploadHeap;
}

Microsoft::WRL::ComPtr<ID3D12Resource>* Texture::GetUploadPtr()
{
	return &m_pUploadHeap;
}

Descriptor* Texture::GetDescriptor() const
{
	return m_pDescriptor;
}

DXGI_FORMAT Texture::GetFormat() const
{
	return m_Format;
}

void Texture::SetFormat(DXGI_FORMAT format)
{
	m_Format = format;
}
