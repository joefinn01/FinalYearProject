#include "TextureManager.h"
#include "Include/tinygltf/tiny_gltf.h"
#include "Commons/Texture.h"
#include "Helpers/DebugHelper.h"
#include "Apps/App.h"

Tag tag = L"TextureManager";

bool TextureManager::LoadTexture(std::string sName, const tinygltf::Image& kImage, Texture*& pTexture, ID3D12GraphicsCommandList* pGraphicsCommandList)
{
	Texture* pTempTexture = new Texture();

	switch (kImage.component)
	{
	case 1:
		switch (kImage.bits)
		{
		case 8:
			pTempTexture->SetFormat(DXGI_FORMAT_R8_UNORM);
			break;

		case 16:
			pTempTexture->SetFormat(DXGI_FORMAT_R16_UNORM);
			break;

		case 32:
			pTempTexture->SetFormat(DXGI_FORMAT_R32_FLOAT);
			break;
		}
		break;

	case 2:
		switch (kImage.bits)
		{
		case 8:
			pTempTexture->SetFormat(DXGI_FORMAT_R8G8_UNORM);
			break;

		case 16:
			pTempTexture->SetFormat(DXGI_FORMAT_R16G16_UNORM);
			break;

		case 32:
			pTempTexture->SetFormat(DXGI_FORMAT_R32G32_FLOAT);
			break;
		}
		break;

	case 3:
		switch (kImage.bits)
		{
		//case 8:
		//	pTempTexture->SetFormat(DXGI_FORMAT_R8G8B8_UNORM);
		//	break;

		//case 16:
		//	pTempTexture->SetFormat(DXGI_FORMAT_R16G16B16_UNORM);
		//	break;

		case 32:
			pTempTexture->SetFormat(DXGI_FORMAT_R32G32B32_FLOAT);
			break;
		}
		break;

	case 4:
		switch (kImage.bits)
		{
		case 8:
			pTempTexture->SetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
			break;

		case 16:
			pTempTexture->SetFormat(DXGI_FORMAT_R16G16B16A16_UNORM);
			break;

		case 32:
			pTempTexture->SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT);
			break;
		}
		break;

	default:
		LOG_ERROR(tag, L"Tried to load texture but the number of components isn't supported!");

		delete pTempTexture;

		return false;
	}

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = (UINT64)kImage.width;
	texDesc.Height = (UINT)kImage.height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = pTempTexture->GetFormat();
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = App::GetApp()->GetDevice()->CreateCommittedResource
	(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pTempTexture->GetTexturePtr()->GetAddressOf())
	);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create a texture from the tinygltf image!");

		delete pTempTexture;

		return false;
	}

	hr = App::GetApp()->GetDevice()->CreateCommittedResource
	(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(GetRequiredIntermediateSize(pTempTexture->GetTexture().Get(), 0, 1)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pTempTexture->GetUploadPtr()->GetAddressOf())
	);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create a texture upload buffer from the tinygltf image!");

		delete pTempTexture;

		return false;
	}

	//Copy image to texture
	D3D12_SUBRESOURCE_DATA data = {};
	data.pData = (void*)kImage.image.data();
	data.RowPitch = (LONG_PTR)kImage.width * (kImage.bits * kImage.component * 0.125f);
	data.SlicePitch = (LONG_PTR)kImage.height;

	pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pTempTexture->GetTexture().Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	UINT64 test = UpdateSubresources(pGraphicsCommandList, pTempTexture->GetTexture().Get(), pTempTexture->GetUpload().Get(), 0, 0, 1, &data);

	pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pTempTexture->GetTexture().Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	if (pTexture != nullptr)
	{
		pTexture = pTempTexture;
	}

	m_Textures["test"] = pTempTexture;

	return true;
}

bool TextureManager::GetTexture(const std::string& ksName, Texture*& pTexture)
{
	return false;
}

bool TextureManager::RemoveTexture(const std::string& ksName)
{
	return false;
}

UINT TextureManager::GetNumTextures() const
{
	return m_Textures.size();
}
