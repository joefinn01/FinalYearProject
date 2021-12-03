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

	bool CreateBLAS(ID3D12GraphicsCommandList4* pGraphicsCommandList, std::vector<UploadBuffer<DirectX::XMFLOAT3X4>>& uploadBuffers);

private:
	bool ProcessNode(MeshNode* pParentNode,const tinygltf::Node& kNode, UINT16 uiNodeIndex, const tinygltf::Model& kModel, Mesh* pMesh, std::vector<Vertex>* pVertexBuffer, std::vector<UINT16>* pIndexBuffer);

	bool GetVertexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, const float** kppfPositionBuffer, UINT* puiPositionStride, const float** kppfNormalBuffer, UINT* puiNormalStride, const float** kppfTexCoordBuffer0, UINT* puiTexCoordStride0, const float** kppfTexCoordBuffer1, UINT* puiTexCoordStride1, UINT16* puiVertexCount);
	bool GetIndexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::vector<UINT16>* pIndexBuffer, UINT16* puiIndexCount, const UINT16& kuiVertexStart);

	bool GetAttributeData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::string sAttribName, const float** kppfBuffer, UINT* puiStride, UINT16* puiCount, uint32_t uiType);

	bool LoadTextures(std::string sName, Mesh* pMesh, const tinygltf::Model kModel, ID3D12GraphicsCommandList* pGraphicsCommandList);

	std::unordered_map<std::string, Mesh*> m_Meshes;
};

