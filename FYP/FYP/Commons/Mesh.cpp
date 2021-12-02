#include "Mesh.h"

Mesh::Mesh()
{
	m_Textures = std::vector<Texture*>();

	m_pVertexBuffer = nullptr;
	m_pIndexBuffer = nullptr;
	m_pRootNode = nullptr;
	m_pIndexDesc = nullptr;
	m_pVertexDesc = nullptr;

	m_uiNumIndices = 0;
	m_uiNumVertices = 0;
}

bool Mesh::CreateBLAS()
{
	return false;
}

std::vector<Texture*>* Mesh::GetTextures()
{
	return &m_Textures;
}

UploadBuffer<Vertex>* Mesh::GetVertexUploadBuffer()
{
	return m_pVertexBuffer;
}

UploadBuffer<UINT16>* Mesh::GetIndexUploadBuffer()
{
	return m_pIndexBuffer;
}

Descriptor* Mesh::GetVertexDesc()
{
	return m_pVertexDesc;
}

Descriptor* Mesh::GetIndexDesc()
{
	return m_pIndexDesc;
}

MeshNode* Mesh::GetRootNode()
{
	return m_pRootNode;
}

UINT16 Mesh::GetNumVertices() const
{
	return m_uiNumVertices;
}

UINT16 Mesh::GetNumIndices() const
{
	return m_uiNumIndices;
}
