#pragma once
#include "Commons/Singleton.h"
#include "Commons/UploadBuffer.h"
#include "Include/DirectX/d3dx12.h"
#include "Commons/Mesh.h"

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
class Mesh;

struct MeshNode;
struct Vertex;

namespace tinygltf
{
	class Model;
	class Node;

	struct Primitive;
}

class MeshManager : public Singleton<MeshManager>
{
public:
	void CreateDescriptors(DescriptorHeap* pHeap);

	bool LoadMesh(const std::string& sFilename, const std::string& sName, ID3D12GraphicsCommandList* pGraphicsCommandList);

	bool GetMesh(std::string sName, Mesh*& pMesh);
	bool RemoveMesh(std::string sName);

	UINT GetNumMeshes() const;

	std::unordered_map<std::string, Mesh*>* GetMeshes();

	bool CreateBLAS(ID3D12GraphicsCommandList4* pGraphicsCommandList, ID3D12Device5* pDevice);

	UINT GetNumPrimitives() const;
	UINT GetNumActivePrimitives() const;
	UINT GetNumActiveRaytracedPrimitives() const;

	void AddNumActivePrimitives(UINT uiNumActivePrimitives);
	void AddNumActiveRaytracedPrimitives(UINT uiNumActiveRaytracedPrimitives);

private:
	bool ProcessNode(MeshNode* pParentNode,const tinygltf::Node& kNode, UINT16 uiNodeIndex, const tinygltf::Model& kModel, Mesh* pMesh, std::vector<Vertex>* pVertexBuffer, std::vector<UINT>* pIndexBuffer);

	bool GetVertexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, const float** kppfPositionBuffer, UINT* puiPositionStride, const float** kppfNormalBuffer, UINT* puiNormalStride, const float** kppfTexCoordBuffer, UINT* puiTexCoordStride, const float** kppfTangentBuffer, UINT* puiTangentStride, UINT* puiVertexCount, Primitive* pPrimitive);
	bool GetIndexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::vector<UINT>* pIndexBuffer, UINT* puiIndexCount);

	bool GetAttributeData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::string sAttribName, const float** kppfBuffer, UINT* puiStride, UINT* puiCount, uint32_t uiType);

	bool LoadTextures(std::string sName, Mesh* pMesh, const tinygltf::Model kModel, ID3D12GraphicsCommandList* pGraphicsCommandList);

	std::unordered_map<std::string, Mesh*> m_Meshes;

	UINT m_uiNumPrimitives = 0;
	UINT m_uiNumActivePrimitives = 0;
	UINT m_uiNumActiveRaytracedPrimitives = 0;
};

