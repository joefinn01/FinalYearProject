#pragma once
#include "Commons/Singleton.h"
#include "Commons/UploadBuffer.h"
#include "Include/DirectX/d3dx12.h"

#include <string>
#include <vector>
#include <unordered_map>


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.

class Texture;
class DescriptorHeap;
class Descriptor;

struct Vertex;

namespace tinygltf
{
	class Model;
	class Node;

	struct Primitive;
}

struct Primitive
{
	Primitive()
	{
		m_uiFirstIndex = 0;
		m_uiNumIndices = 0;
		m_uiNumVertices = 0;
	}

	UINT16 m_uiFirstIndex;
	UINT16 m_uiNumIndices;
	UINT16 m_uiNumVertices;
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
		m_Transform = DirectX::XMFLOAT4X4();
		m_Translation = DirectX::XMFLOAT3();
		m_Rotation = DirectX::XMFLOAT4();
		m_Scale = DirectX::XMFLOAT3();
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

struct Mesh
{
	Mesh()
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

	std::vector<Texture*> m_Textures;

	UploadBuffer<Vertex>* m_pVertexBuffer;
	UploadBuffer<UINT16>* m_pIndexBuffer;

	Descriptor* m_pIndexDesc;
	Descriptor* m_pVertexDesc;

	MeshNode* m_pRootNode;

	UINT16 m_uiNumVertices;
	UINT16 m_uiNumIndices;
};

class MeshManager : public Singleton<MeshManager>
{
public:
	void CreateDescriptors(DescriptorHeap* pHeap);

	bool LoadMesh(const std::string& sFilename, const std::string& sName, ID3D12GraphicsCommandList* pGraphicsCommandList);

	bool GetMesh(std::string sName, Mesh*& pMesh);
	bool RemoveMesh(std::string sName);

	UINT GetNumMeshes() const;

private:
	bool ProcessNode(MeshNode* pParentNode,const tinygltf::Node& kNode, UINT16 uiNodeIndex, const tinygltf::Model& kModel, Mesh* pMesh, std::vector<Vertex>* pVertexBuffer, std::vector<UINT16>* pIndexBuffer);

	bool GetVertexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, const float** kppfPositionBuffer, UINT* puiPositionStride, const float** kppfNormalBuffer, UINT* puiNormalStride, const float** kppfTexCoordBuffer0, UINT* puiTexCoordStride0, const float** kppfTexCoordBuffer1, UINT* puiTexCoordStride1, UINT16* puiVertexCount);
	bool GetIndexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::vector<UINT16>* pIndexBuffer, UINT16* puiIndexCount, const UINT16& kuiVertexStart);

	bool GetAttributeData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::string sAttribName, const float** kppfBuffer, UINT* puiStride, UINT16* puiCount, uint32_t uiType);

	bool LoadTextures(Mesh* pMesh, const tinygltf::Model kModel, ID3D12GraphicsCommandList* pGraphicsCommandList);

	std::unordered_map<std::string, Mesh*> m_Meshes;
};

