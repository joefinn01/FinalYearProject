#include "Mesh.h"
#include "Shaders/Vertices.h"
#include "Helpers/DXRHelper.h"
#include "Helpers/DebugHelper.h"
#include "Apps/App.h"

Tag tag = L"Mesh";

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

bool Mesh::CreateBLAS(ID3D12GraphicsCommandList4* pGraphicsCommandList, std::vector<UploadBuffer<DirectX::XMFLOAT3X4>*>& uploadBuffers)
{
	ID3D12Device5* pDevice = (ID3D12Device5*)App::GetApp()->GetDevice();

	std::vector<MeshNode*> meshNodes;
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;

	meshNodes.push_back(m_pRootNode);

	MeshNode* pNode;
	Primitive* pPrimitive;

	while (meshNodes.size() != 0)
	{
		//get reference and then remove node from vector
		pNode = meshNodes[0];

		meshNodes.erase(meshNodes.begin());

		//Push child mesh nodes into vector
		for (int i = 0; i < pNode->m_ChildNodes.size(); ++i)
		{
			meshNodes.push_back(pNode->m_ChildNodes[i]);
		}

		if (pNode->m_Primitives.size() == 0)
		{
			continue;
		}

		UploadBuffer<XMFLOAT3X4>* uploadBuffer = new UploadBuffer<XMFLOAT3X4>(App::GetApp()->GetDevice(), 1, false);

		XMFLOAT3X4 world3X4;

		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				world3X4.m[i][j] = pNode->m_Transform.m[i][j];
				//world3X4.m[i][j] = MathHelper::Identity().m[i][j];
			}
		}

		uploadBuffer->CopyData(0, world3X4);

		uploadBuffers.push_back(uploadBuffer);

		//Create a geometry desc for each primitive
		for (int i = 0; i < pNode->m_Primitives.size(); ++i)
		{
			pPrimitive = pNode->m_Primitives[i];

			D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
			geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geomDesc.Triangles.IndexBuffer = m_pIndexBuffer->GetBufferGPUAddress(pPrimitive->m_uiFirstIndex);
			geomDesc.Triangles.IndexCount = pPrimitive->m_uiNumIndices;
			geomDesc.Triangles.Transform3x4 = uploadBuffer->GetBufferGPUAddress(0);
			geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
			geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			geomDesc.Triangles.VertexCount = pPrimitive->m_uiNumVertices;
			geomDesc.Triangles.VertexBuffer.StartAddress = m_pVertexBuffer->GetBufferGPUAddress(pPrimitive->m_uiFirstVertex);
			geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
			geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

			geometryDescs.push_back(geomDesc);
		}
	}

	//D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	//geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	//geomDesc.Triangles.IndexBuffer = m_pIndexBuffer->GetBufferGPUAddress(0);
	//geomDesc.Triangles.IndexCount = m_uiNumIndices;
	//geomDesc.Triangles.Transform3x4 = 0;
	//geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	//geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	//geomDesc.Triangles.VertexCount = m_uiNumVertices;
	//geomDesc.Triangles.VertexBuffer.StartAddress = m_pVertexBuffer->GetBufferGPUAddress(0);
	//geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	//geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	//D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	//inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	//inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	//inputs.NumDescs = 1;
	//inputs.pGeometryDescs = &geomDesc;
	//inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

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
