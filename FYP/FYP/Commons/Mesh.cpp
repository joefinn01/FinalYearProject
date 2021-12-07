#include "Mesh.h"
#include "Shaders/Vertices.h"
#include "Helpers/DXRHelper.h"
#include "Helpers/DebugHelper.h"
#include "Apps/App.h"

#include <queue>

Tag tag = L"Mesh";

Mesh::Mesh()
{
	m_Textures = std::vector<Texture*>();

	m_pVertexBuffer = nullptr;
	m_pIndexBuffer = nullptr;
	m_RootNodes = std::vector<MeshNode*>();

	m_uiNumIndices = 0;
	m_uiNumVertices = 0;
}

bool Mesh::CreateBLAS(ID3D12GraphicsCommandList4* pGraphicsCommandList, std::vector<UploadBuffer<DirectX::XMFLOAT3X4>*>& uploadBuffers)
{
	ID3D12Device5* pDevice = (ID3D12Device5*)App::GetApp()->GetDevice();

	std::queue<MeshNode*> meshNodes;

	for (int i = 0; i < m_RootNodes.size(); ++i)
	{
		meshNodes.push(m_RootNodes[i]);
	}

	MeshNode* pNode;
	Primitive* pPrimitive;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;

	while (meshNodes.size() != 0)
	{
		//get reference and then remove node from vector
		pNode = meshNodes.front();

		meshNodes.pop();

		//Push child mesh nodes into vector
		for (int i = 0; i < pNode->m_ChildNodes.size(); ++i)
		{
			meshNodes.push(pNode->m_ChildNodes[i]);
		}

		if (pNode->m_Primitives.size() == 0)
		{
			continue;
		}

		UploadBuffer<XMFLOAT3X4>* pTransformUploadBuffer = new UploadBuffer<XMFLOAT3X4>(App::GetApp()->GetDevice(), 1, false);

		XMFLOAT3X4 world3X4;

		if (pNode->m_pParent != nullptr)
		{
			XMStoreFloat3x4(&world3X4, XMLoadFloat4x4(&pNode->m_Transform));
		}
		else
		{
			XMStoreFloat3x4(&world3X4, XMLoadFloat4x4(&pNode->m_Transform));
		}


		pTransformUploadBuffer->CopyData(0, world3X4);

		uploadBuffers.push_back(pTransformUploadBuffer);

		//Create a geometry desc for each primitive
		for (int i = 0; i < pNode->m_Primitives.size(); ++i)
		{
			pPrimitive = pNode->m_Primitives[i];

			D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
			geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geomDesc.Triangles.IndexBuffer = m_pIndexBuffer->GetBufferGPUAddress(pPrimitive->m_uiFirstIndex);
			geomDesc.Triangles.IndexCount = pPrimitive->m_uiNumIndices;
			geomDesc.Triangles.Transform3x4 = pTransformUploadBuffer->GetBufferGPUAddress(0);
			geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
			geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			geomDesc.Triangles.VertexCount = pPrimitive->m_uiNumVertices;
			geomDesc.Triangles.VertexBuffer.StartAddress = m_pVertexBuffer->GetBufferGPUAddress(pPrimitive->m_uiFirstVertex);
			geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
			geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

			geometryDescs.push_back(geomDesc);
		}
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = geometryDescs.size();
	inputs.pGeometryDescs = geometryDescs.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (DXRHelper::CreateUAVBuffer(pDevice, info.ScratchDataSizeInBytes, m_BottomLevel.m_pScratch.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == false)
	{
		LOG_ERROR(tag, L"Failed to create the bottom level acceleration structure scratch buffer!");

		return false;
	}

	if (DXRHelper::CreateUAVBuffer(pDevice, info.ResultDataMaxSizeInBytes, m_BottomLevel.m_pResult.GetAddressOf(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) == false)
	{
		LOG_ERROR(tag, L"Failed to create the bottom level acceleration structure result buffer!");

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

AccelerationBuffers* Mesh::GetBLAS()
{
	return &m_BottomLevel;
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

std::vector<MeshNode*>* Mesh::GetRootNodes()
{
	return &m_RootNodes;
}

UINT16 Mesh::GetNumVertices() const
{
	return m_uiNumVertices;
}

UINT16 Mesh::GetNumIndices() const
{
	return m_uiNumIndices;
}