#pragma once

#include "Commons/Singleton.h"
#include "Include/DirectX/d3dx12.h"

#include <unordered_map>

class Texture;
class DescriptorHeap;

namespace tinygltf
{
	struct Image;
}

class TextureManager : public Singleton<TextureManager>
{
public:
	bool LoadTexture(const tinygltf::Image& kImage, Texture*& pTexture, ID3D12GraphicsCommandList* pGraphicsCommandList);

	bool GetTexture(const std::string& ksName, Texture*& pTexture);
	bool RemoveTexture(const std::string& ksName);

	UINT GetNumTextures() const;

protected:

private:
	std::unordered_map<std::string, Texture*> m_Textures;
};

