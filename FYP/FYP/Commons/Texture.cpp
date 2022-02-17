#include "Texture.h"
#include "Commons/DescriptorHeap.h"
#include "Commons/SRVDescriptor.h"
#include "Commons/UAVDescriptor.h"

Texture::Texture(ID3D12Resource* pTextureRes, DXGI_FORMAT textureFormat)
{
	m_pTexture = pTextureRes;
	m_Format = textureFormat;
}

bool Texture::CreateSRVDesc(DescriptorHeap* pHeap)
{
	UINT uiIndex;

	if (pHeap->Allocate(uiIndex) == false)
	{
		return false;
	}

	m_pSRVDesc = new SRVDescriptor(uiIndex, pHeap->GetCpuDescriptorHandle(uiIndex), m_pTexture.Get(), m_Format, 1);

	return true;
}

bool Texture::CreateSRVDesc(DescriptorHeap* pHeap, DXGI_FORMAT format)
{
	UINT uiIndex;

	if (pHeap->Allocate(uiIndex) == false)
	{
		return false;
	}

	m_pSRVDesc = new SRVDescriptor(uiIndex, pHeap->GetCpuDescriptorHandle(uiIndex), m_pTexture.Get(), format, 1);

	return true;
}

void Texture::RecreateSRVDesc(DescriptorHeap* pHeap)
{
	SRVDescriptor* pDesc = new SRVDescriptor(m_pSRVDesc->GetDescriptorIndex(), pHeap->GetCpuDescriptorHandle(m_pSRVDesc->GetDescriptorIndex()), m_pTexture.Get(), m_Format, 1);
	delete m_pSRVDesc;
	m_pSRVDesc = pDesc;
}

void Texture::RecreateSRVDesc(DescriptorHeap* pHeap, DXGI_FORMAT format)
{
	SRVDescriptor* pDesc = new SRVDescriptor(m_pSRVDesc->GetDescriptorIndex(), pHeap->GetCpuDescriptorHandle(m_pSRVDesc->GetDescriptorIndex()), m_pTexture.Get(), format, 1);
	delete m_pSRVDesc;
	m_pSRVDesc = pDesc;
}

bool Texture::CreateUAVDesc(DescriptorHeap* pHeap)
{
	UINT uiIndex;

	if (pHeap->Allocate(uiIndex) == false)
	{
		return false;
	}

	m_pUAVDesc = new UAVDescriptor(uiIndex, pHeap->GetCpuDescriptorHandle(uiIndex), m_pTexture.Get(), D3D12_UAV_DIMENSION_TEXTURE2D, m_Format);

	return true;
}

void Texture::RecreateUAVDesc(DescriptorHeap* pHeap)
{
	UAVDescriptor* pDesc = new UAVDescriptor(m_pUAVDesc->GetDescriptorIndex(), pHeap->GetCpuDescriptorHandle(m_pUAVDesc->GetDescriptorIndex()), m_pTexture.Get(), D3D12_UAV_DIMENSION_TEXTURE2D, m_Format);
	delete m_pUAVDesc;
	m_pUAVDesc = pDesc;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Texture::GetResource() const
{
	return m_pTexture;
}

Microsoft::WRL::ComPtr<ID3D12Resource>* Texture::GetResourcePtr()
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

Descriptor* Texture::GetSRVDesc() const
{
	return m_pSRVDesc;
}

Descriptor* Texture::GetUAVDesc() const
{
	return m_pUAVDesc;
}

DXGI_FORMAT Texture::GetFormat() const
{
	return m_Format;
}

void Texture::SetFormat(DXGI_FORMAT format)
{
	m_Format = format;
}
