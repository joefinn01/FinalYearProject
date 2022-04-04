#pragma once

#include "Commons/UploadBuffer.h"
#include "Commons/AccelerationBuffers.h"
#include "Helpers/DXRHelper.h"
#include "Helpers/DebugHelper.h"
#include "Shaders/Vertices.h"

#include <vector>

class Texture;
class Descriptor;

struct Vertex;

enum class PrimitiveAttributes : UINT8
{
	NORMAL = 1,
	OCCLUSION = 2,
	METALLIC_ROUGHNESS = 4,
	EMISSIVE = 8,
	ALBEDO = 16
};

DEFINE_ENUM_FLAG_OPERATORS(PrimitiveAttributes);

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

	bool CreateBLAS(ID3D12GraphicsCommandList4*& pGraphicsCommandList, UploadBuffer<Vertex>*& pVertexBuffer, UploadBuffer<UINT>*& pIndexBuffer, ID3D12Device5*& pDevice)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc.Triangles.IndexBuffer = pIndexBuffer->GetBufferGPUAddress(m_uiFirstIndex);
		geomDesc.Triangles.IndexCount = m_uiNumIndices;
		geomDesc.Triangles.Transform3x4 = 0;
		geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geomDesc.Triangles.VertexCount = m_uiNumVertices;
		geomDesc.Triangles.VertexBuffer.StartAddress = pVertexBuffer->GetBufferGPUAddress(m_uiFirstVertex);
		geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = 1;
		inputs.pGeometryDescs = &geomDesc;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		if (DXRHelper::CreateUAVBuffer(pDevice, info.ScratchDataSizeInBytes, m_BottomLevel.m_pScratch.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == false)
		{
			LOG_ERROR(L"Primitive", L"Failed to create the bottom level acceleration structure scratch buffer!");

			return false;
		}

		if (DXRHelper::CreateUAVBuffer(pDevice, info.ResultDataMaxSizeInBytes, m_BottomLevel.m_pResult.GetAddressOf(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) == false)
		{
			LOG_ERROR(L"Primitive", L"Failed to create the bottom level acceleration structure result buffer!");

			return false;
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.Inputs = inputs;
		buildDesc.DestAccelerationStructureData = m_BottomLevel.m_pResult->GetGPUVirtualAddress();
		buildDesc.ScratchAccelerationStructureData = m_BottomLevel.m_pScratch->GetGPUVirtualAddress();

		pGraphicsCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_BottomLevel.m_pResult.Get()));

		return true;
	}

	Descriptor* m_pIndexDesc;
	Descriptor* m_pVertexDesc;

	UINT m_uiFirstIndex;
	UINT m_uiFirstVertex;
	UINT m_uiNumIndices;
	UINT m_uiNumVertices;

	int m_iIndex;
	int m_iAlbedoIndex;
	int m_iNormalIndex;
	int m_iMetallicRoughnessIndex;
	int m_iOcclusionIndex;

	DirectX::XMFLOAT4 m_BaseColour;

	PrimitiveAttributes m_Attributes = (PrimitiveAttributes)0;

	AccelerationBuffers m_BottomLevel;
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

	bool CreateBLAS(ID3D12GraphicsCommandList4*& pGraphicsCommandList, ID3D12Device5*& pDevice);

	std::vector<Texture*>* GetTextures();

	UploadBuffer<Vertex>* GetVertexUploadBuffer();
	UploadBuffer<UINT>* GetIndexUploadBuffer();

	std::vector<MeshNode*>* GetNodes();
	const MeshNode* GetNode(int iIndex) const;

	UINT GetNumVertices() const;
	UINT GetNumIndices() const;
	UINT GetNumPrimitives() const;

	std::string GetName() const;

protected:

private:
	std::vector<Texture*> m_Textures;

	UploadBuffer<Vertex>* m_pVertexBuffer;
	UploadBuffer<UINT>* m_pIndexBuffer;

	std::vector<MeshNode*> m_Nodes;

	UINT m_uiNumVertices;
	UINT m_uiNumIndices;
	UINT m_uiNumPrimitives;

	std::string m_sFilePath;
	std::string m_sName;

	friend class MeshManager;
};