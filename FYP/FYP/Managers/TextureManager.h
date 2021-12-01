#pragma once

#include "Commons/Singleton.h"

#include <unordered_map>

class Texture;

class TextureManager : public Singleton<TextureManager>
{
public:
	bool LoadMesh(const std::string& ksFilename, Texture*& pTexture);

	bool GetTexture(const std::string& ksName, Texture*& pTexture);
	bool RemoveTexture(const std::string& ksName);

protected:

private:
	std::unordered_map<std::string, Texture*> m_Textures;
};

