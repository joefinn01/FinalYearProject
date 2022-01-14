#pragma once

#include "Commons/UploadBuffer.h"
#include "Commons/AccelerationBuffers.h"

#include <vector>

class Texture;
class Descriptor;

struct Vertex;

enum class PrimitiveAttributes : UINT8
{
	NORMAL = 1,
	OCCLUSION = 2,
	EMISSIVE = 4
};

inline PrimitiveAttributes operator|(PrimitiveAttributes a, PrimitiveAttributes b)
{
	return static_cast<PrimitiveAttributes>(static_cast<UINT8>(a) | static_cast<UINT8>(b));
}

inline PrimitiveAttributes operator&(PrimitiveAttributes a, PrimitiveAttributes b)
{
	return static_cast<PrimitiveAttributes>(static_cast<UINT8>(a) & static_cast<UINT8>(b));
}

struct Primitive
{
	Primitive()
	{
		m_uiFirstIndex = 0;
		m_uiNumIndices = 0;
		m_uiNumVertices = 0;
		m_uiFirstVertex = 0;

		m_iAlbedoIndex = 0;
		m_iNormalIndex = 0;
		m_iMetallicRoughnessIndex = 0;
		m_iOcclusionIndex = 0;

		m_iIndex = -1;

		m_pIndexDesc = nullptr;
		m_pVertexDesc = nullptr;

		m_Attributes = (PrimitiveAttributes)0;
	}

	bool HasAttribute(PrimitiveAttributes primAttribute)
	{
		return (UINT8)m_Attributes & (UINT8)primAttribute;
	}

	Descriptor* m_pIndexDesc;
	Descriptor* m_pVertexDesc;

	UINT16 m_uiFirstIndex;
	UINT16 m_uiFirstVertex;
	UINT16 m_uiNumIndices;
	UINT16 m_uiNumVertices;

	int m_iIndex;
	int m_iAlbedoIndex;
	int m_iNormalIndex;
	int m_iMetallicRoughnessIndex;
	int m_iOcclusionIndex;

	PrimitiveAttributes m_Attributes = (PrimitiveAttributes)0;
};

struct MeshNode
{
	MeshNode()
	{
		m_pParent = nullptr;
		m_ChildNodes = std::vector<MeshNode*>();
		m_Primitives = std::vector<Primitive*>();
		m_uiIndex = 0;
		m_sName = "";
		m_Transform = MathHelper::Identity();
		m_Translation = DirectX::XMFLOAT3();
		m_Rotation = DirectX::XMFLOAT4();
		m_Scale = DirectX::XMFLOAT3(1, 1, 1);
	}

	MeshNode* m_pParent;
	std::vector<MeshNode*> m_ChildNodes;
	std::vector<Primitive*> m_Primitives;

	UINT16 m_uiIndex;

	std::string m_sName;
	DirectX::XMFLOAT4X4 m_Transform;
	DirectX::XMFLOAT3 m_Translation;
	DirectX::XMFLOAT4 m_Rotation;
	DirectX::XMFLOAT3 m_Scale;
};

class Mesh
{
public:
	Mesh();

	bool CreateBLAS(ID3D12GraphicsCommandList4* pGraphicsCommandList, std::vector<UploadBuffer<DirectX::XMFLOAT3X4>*>& uploadBuffers);
	AccelerationBuffers* GetBLAS();

	std::vector<Texture*>* GetTextures();

	UploadBuffer<Vertex>* GetVertexUploadBuffer();
	UploadBuffer<UINT16>* GetIndexUploadBuffer();

	std::vector<MeshNode*>* GetNodes();
	const MeshNode* GetNode(int iIndex) const;

	UINT16 GetNumVertices() const;
	UINT16 GetNumIndices() const;

protected:

private:
	std::vector<Texture*> m_Textures;

	UploadBuffer<Vertex>* m_pVertexBuffer;
	UploadBuffer<UINT16>* m_pIndexBuffer;

	std::vector<MeshNode*> m_Nodes;

	UINT16 m_uiNumVertices;
	UINT16 m_uiNumIndices;

	AccelerationBuffers m_BottomLevel;

	friend class MeshManager;
};