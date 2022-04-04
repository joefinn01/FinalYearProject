#include "Mesh.h"
#include "Shaders/Vertices.h"
#include "Helpers/DXRHelper.h"
#include "Helpers/DebugHelper.h"
#include "Apps/App.h"

#include <queue>

Mesh::Mesh()
{
	m_Textures = std::vector<Texture*>();

	m_pVertexBuffer = nullptr;
	m_pIndexBuffer = nullptr;
	m_Nodes = std::vector<MeshNode*>();

	m_uiNumIndices = 0;
	m_uiNumVertices = 0;

	m_sFilePath = "";
	m_sName = "";
}

bool Mesh::CreateBLAS(ID3D12GraphicsCommandList4*& pGraphicsCommandList, ID3D12Device5*& pDevice)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;

	MeshNode* pNode;

	for (int i = 0;i < m_Nodes.size(); ++i)
	{
		pNode = m_Nodes[i];

		if (pNode->m_Primitives.size() == 0)
		{
			continue;
		}

		//Create a geometry desc for each primitive
		for (int i = 0; i < pNode->m_Primitives.size(); ++i)
		{
			pNode->m_Primitives[i]->CreateBLAS(pGraphicsCommandList, m_pVertexBuffer, m_pIndexBuffer, pDevice);
		}
	}

	return true;
}

std::vector<Texture*>* Mesh::GetTextures()
{
	return &m_Textures;
}

UploadBuffer<Vertex>* Mesh::GetVertexUploadBuffer()
{
	return m_pVertexBuffer;
}

UploadBuffer<UINT>* Mesh::GetIndexUploadBuffer()
{
	return m_pIndexBuffer;
}

std::vector<MeshNode*>* Mesh::GetNodes()
{
	return &m_Nodes;
}

const MeshNode* Mesh::GetNode(int iIndex) const
{
	return m_Nodes[iIndex];
}

UINT Mesh::GetNumVertices() const
{
	return m_uiNumVertices;
}

UINT Mesh::GetNumIndices() const
{
	return m_uiNumIndices;
}

UINT Mesh::GetNumPrimitives() const
{
	return m_uiNumPrimitives;
}

std::string Mesh::GetName() const
{
	return m_sName;
}
