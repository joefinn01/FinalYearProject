#include "MeshManager.h"
#include "Include/tinygltf/tiny_gltf.h"
#include "Helpers/DebugHelper.h"
#include "Shaders/Vertices.h"
#include "Apps/App.h"
#include "Commons/DescriptorHeap.h"
#include "Commons/Texture.h"
#include "Commons/SRVDescriptor.h"
#include "Commons/Mesh.h"
#include "Managers/TextureManager.h"
#include "Commons/Mesh.h"

#include <queue>

Tag tag = L"MeshManager";

void MeshManager::CreateDescriptors(DescriptorHeap* pHeap)
{
	UINT uiIndex;

	const MeshNode* pNode;

	for (std::unordered_map<std::string, Mesh*>::iterator it = m_Meshes.begin(); it != m_Meshes.end(); ++it)
	{
		for (int i = 0; i < it->second->GetNodes()->size(); ++i)
		{
			pNode = it->second->GetNode(i);

			//create descriptors per primitive

			for (int i = 0; i < pNode->m_Primitives.size(); ++i)
			{
				if (pHeap->Allocate(uiIndex) == false)
				{
					return;
				}

				pNode->m_Primitives[i]->m_pIndexDesc = new SRVDescriptor(uiIndex, pHeap->GetCpuDescriptorHandle(uiIndex), it->second->m_pIndexBuffer->Get(), D3D12_SRV_DIMENSION_BUFFER, pNode->m_Primitives[i]->m_uiNumIndices, DXGI_FORMAT_UNKNOWN, D3D12_BUFFER_SRV_FLAG_NONE, sizeof(UINT), pNode->m_Primitives[i]->m_uiFirstIndex);

				if (pHeap->Allocate(uiIndex) == false)
				{
					return;
				}

				pNode->m_Primitives[i]->m_pVertexDesc = new SRVDescriptor(uiIndex, pHeap->GetCpuDescriptorHandle(uiIndex), it->second->m_pVertexBuffer->Get(), D3D12_SRV_DIMENSION_BUFFER, pNode->m_Primitives[i]->m_uiNumVertices, DXGI_FORMAT_UNKNOWN, D3D12_BUFFER_SRV_FLAG_NONE, it->second->m_pVertexBuffer->GetStride(), pNode->m_Primitives[i]->m_uiFirstVertex);
			}
		}

		for (int i = 0; i < it->second->m_Textures.size(); ++i)
		{
			it->second->m_Textures[i]->CreateSRVDesc(pHeap);
		}
	}
}

bool MeshManager::LoadMesh(const std::string& sFilename, const std::string& sName, ID3D12GraphicsCommandList* pGraphicsCommandList)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool bSuccess = loader.LoadASCIIFromFile(&model, &err, &warn, sFilename);

	Mesh* pMesh = new Mesh();

	pMesh->m_sFilePath = sFilename;
	pMesh->m_sName = sName;

	if (!warn.empty()) 
	{
		LOG_WARNING(tag, L"%s", warn);
	}

	if (!err.empty()) 
	{
		LOG_ERROR(tag, L"%s", err);
	}

	if (bSuccess == false) 
	{
		LOG_ERROR(tag, L"Failed to load mesh with name %s!", sFilename);

		return false;
	}

	if (LoadTextures(sName, pMesh, model, pGraphicsCommandList) == false)
	{
		return false;
	}

	tinygltf::Scene* pScene;

	if (model.defaultScene >= 0)
	{
		pScene = &model.scenes[model.defaultScene];
	}
	else
	{
		pScene = &model.scenes[0];
	}

	std::vector<Vertex> vertexBuffer = std::vector<Vertex>();
	std::vector<UINT> indexBuffer = std::vector<UINT>();

	for (UINT i = 0; i < pScene->nodes.size(); ++i)
	{
		if (ProcessNode(nullptr, model.nodes[pScene->nodes[i]], pScene->nodes[i], model, pMesh, &vertexBuffer, &indexBuffer) == false)
		{
			return false;
		}
	}

	pMesh->m_pVertexBuffer = new UploadBuffer<Vertex>(App::GetApp()->GetDevice(), vertexBuffer.size(), false);
	pMesh->m_pVertexBuffer->CopyData(0, vertexBuffer);

	pMesh->m_pIndexBuffer = new UploadBuffer<UINT>(App::GetApp()->GetDevice(), indexBuffer.size(), false);
	pMesh->m_pIndexBuffer->CopyData(0, indexBuffer);

	pMesh->m_uiNumVertices = vertexBuffer.size();
	pMesh->m_uiNumIndices = indexBuffer.size();

	if (m_Meshes.count(sName) != 0)
	{
		LOG_ERROR(tag, L"Tried to create a new mesh called %s but one with that name already exists!", sName);

		return false;
	}

	m_Meshes[sName] = pMesh;

	return true;
}

bool MeshManager::ProcessNode(MeshNode* pParentNode, const tinygltf::Node& kNode, UINT16 uiNodeIndex, const tinygltf::Model& kModel, Mesh* pMesh, std::vector<Vertex>* pVertexBuffer, std::vector<UINT>* pIndexBuffer)
{
	MeshNode* pNode = new MeshNode();
	pNode->m_uiIndex = uiNodeIndex;
	pNode->m_pParent = pParentNode;
	pNode->m_sName = kNode.name;
	pNode->m_Transform = MathHelper::Identity();

	XMFLOAT3 translation = XMFLOAT3();
	if (kNode.translation.size() == 3)
	{
		translation.x = (float)kNode.translation[0];
		translation.y = (float)kNode.translation[1];
		translation.z = (float)kNode.translation[2];
	}

	XMFLOAT4 rotation = XMFLOAT4(0, 0, 0, 1);
	if (kNode.rotation.size() == 4)
	{
		rotation.x = kNode.rotation[0];
		rotation.y = kNode.rotation[1];
		rotation.z = kNode.rotation[2];
		rotation.w = kNode.rotation[3];
	}

	XMFLOAT3 scale = XMFLOAT3(1, 1, 1);
	if (kNode.scale.size() == 3)
	{
		scale.x = kNode.scale[0];
		scale.y = kNode.scale[1];
		scale.z = kNode.scale[2];
	}

	if (kNode.matrix.size() == 16)
	{
		pNode->m_Transform = XMFLOAT4X4
		(
			kNode.matrix[0], kNode.matrix[1], kNode.matrix[2], kNode.matrix[3],
			kNode.matrix[4], kNode.matrix[5], kNode.matrix[6], kNode.matrix[7],
			kNode.matrix[8], kNode.matrix[9], kNode.matrix[10], kNode.matrix[11],
			kNode.matrix[12], kNode.matrix[13], kNode.matrix[14], kNode.matrix[15]
		);

		if (pNode->m_pParent != nullptr)
		{
			XMStoreFloat4x4(&pNode->m_Transform, XMLoadFloat4x4(&pNode->m_Transform) * XMLoadFloat4x4(&pNode->m_pParent->m_Transform));
		}
	}
	else
	{
		if (pNode->m_pParent != nullptr)
		{
			XMStoreFloat4x4(&pNode->m_Transform, XMMatrixScalingFromVector(XMLoadFloat3(&scale)) * XMMatrixRotationQuaternion(XMQuaternionNormalize(XMLoadFloat4(&rotation))) * XMMatrixTranslationFromVector(XMLoadFloat3(&translation)) * XMLoadFloat4x4(&pNode->m_pParent->m_Transform));
		}
		else
		{
			XMStoreFloat4x4(&pNode->m_Transform, XMMatrixScalingFromVector(XMLoadFloat3(&scale)) * XMMatrixRotationQuaternion(XMQuaternionNormalize(XMLoadFloat4(&rotation))) * XMMatrixTranslationFromVector(XMLoadFloat3(&translation)));
		}
	}

	for (UINT i = 0; i < kNode.children.size(); ++i)
	{
		if (ProcessNode(pNode, kModel.nodes[kNode.children[i]], kNode.children[i], kModel, pMesh, pVertexBuffer, pIndexBuffer) == false)
		{
			return false;
		}
	}

	if (kNode.mesh >= 0)
	{
		const tinygltf::Mesh& kMesh = kModel.meshes[kNode.mesh];

		m_uiNumPrimitives += kMesh.primitives.size();
		pMesh->m_uiNumPrimitives += kMesh.primitives.size();

		for (UINT i = 0; i < kMesh.primitives.size(); ++i)
		{
			const tinygltf::Primitive& kPrimitive = kMesh.primitives[i];
			UINT uiIndexStart = (UINT)pIndexBuffer->size();
			UINT uiVertexStart = (UINT)pVertexBuffer->size();
			UINT uiIndexCount = 0;
			UINT uiVertexCount = 0;
			bool bHasIndices = kPrimitive.indices >= 0;

			//Vertex information buffers
			const float* kpfPositionBuffer = nullptr;
			const float* kpfNormalBuffer = nullptr;
			const float* kpfTangentBuffer = nullptr;
			const float* kpfTexCoordBuffer = nullptr;

			UINT uiPositionStride;
			UINT uiNormalStride;
			UINT uiTexCoordStride;
			UINT uiTangentStride;

			Primitive* pPrimitive = new Primitive();

			if (GetVertexData(kModel, kPrimitive, &kpfPositionBuffer, &uiPositionStride, &kpfNormalBuffer, &uiNormalStride, &kpfTexCoordBuffer, &uiTexCoordStride, &kpfTangentBuffer, &uiTangentStride, &uiVertexCount, pPrimitive) == false)
			{
				return false;
			}

			Vertex vertex = {};

			for (UINT j = 0; j < uiVertexCount; ++j)
			{
				vertex.Position = XMFLOAT3(kpfPositionBuffer[j * uiPositionStride], kpfPositionBuffer[(j * uiPositionStride) + 1], kpfPositionBuffer[(j * uiPositionStride) + 2]);

				if (kpfNormalBuffer != nullptr)
				{
					XMStoreFloat3(&vertex.Normal, XMVector3Normalize(XMVectorSet(kpfNormalBuffer[j * uiNormalStride], kpfNormalBuffer[(j * uiNormalStride) + 1], kpfNormalBuffer[(j * uiNormalStride) + 2], 0.0f)));
				}

				if (kpfTangentBuffer != nullptr)
				{
					XMStoreFloat4(&vertex.Tangent, XMVector4Normalize(XMVectorSet(kpfTangentBuffer[j * uiTangentStride], kpfTangentBuffer[(j * uiTangentStride) + 1], kpfTangentBuffer[(j * uiTangentStride) + 2], kpfTangentBuffer[(j * uiTangentStride) + 3])));
				}

				if (kpfTexCoordBuffer != nullptr)
				{
					vertex.TexCoords = XMFLOAT2(kpfTexCoordBuffer[j * uiTexCoordStride], kpfTexCoordBuffer[(j * uiTexCoordStride) + 1]);
				}

				pVertexBuffer->push_back(vertex);
			}

			if (bHasIndices == true)
			{
				if (GetIndexData(kModel, kPrimitive, pIndexBuffer, &uiIndexCount) == false)
				{
					return false;
				}
			}
			else //If no index buffer then create them
			{
				uiIndexCount = pVertexBuffer->size();

				for (int i = 0; i < uiIndexCount; ++i)
				{
					pIndexBuffer->push_back((UINT)(i + uiVertexStart));
				}
			}

			pPrimitive->m_uiFirstIndex = uiIndexStart;
			pPrimitive->m_uiFirstVertex = uiVertexStart;
			pPrimitive->m_uiNumIndices = uiIndexCount;
			pPrimitive->m_uiNumVertices = uiVertexCount;
			const std::vector<double>& baseColor = kModel.materials[kPrimitive.material].pbrMetallicRoughness.baseColorFactor;
			pPrimitive->m_BaseColour = DirectX::XMFLOAT4(baseColor[0], baseColor[1], baseColor[2], baseColor[3]);
			pPrimitive->m_iAlbedoIndex = kModel.materials[kPrimitive.material].pbrMetallicRoughness.baseColorTexture.index;
			pPrimitive->m_iNormalIndex = kModel.materials[kPrimitive.material].normalTexture.index;
			pPrimitive->m_iMetallicRoughnessIndex = kModel.materials[kPrimitive.material].pbrMetallicRoughness.metallicRoughnessTexture.index;
			pPrimitive->m_iOcclusionIndex = kModel.materials[kPrimitive.material].occlusionTexture.index;
			pPrimitive->m_iIndex = m_uiNumPrimitives - kMesh.primitives.size() + i;

			if (pPrimitive->m_iAlbedoIndex != -1)
			{
				pPrimitive->m_Attributes = pPrimitive->m_Attributes | PrimitiveAttributes::ALBEDO;
			}

			if (pPrimitive->m_iOcclusionIndex != -1)
			{
				pPrimitive->m_Attributes = pPrimitive->m_Attributes | PrimitiveAttributes::OCCLUSION;
			}

			if (pPrimitive->m_iMetallicRoughnessIndex != -1)
			{
				pPrimitive->m_Attributes = pPrimitive->m_Attributes | PrimitiveAttributes::METALLIC_ROUGHNESS;
			}

			pNode->m_Primitives.push_back(pPrimitive);
		}
	}

	if (pParentNode != nullptr)
	{
		pParentNode->m_ChildNodes.push_back(pNode);
	}

	pMesh->m_Nodes.push_back(pNode);

	return true;
}

bool MeshManager::GetAttributeData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::string sAttribName, const float** kppfBuffer, UINT* puiStride, UINT* puiCount, uint32_t uiType)
{
	if (kPrimitive.attributes.find(sAttribName) != kPrimitive.attributes.end())
	{
		const tinygltf::Accessor kAccessor = kModel.accessors[kPrimitive.attributes.find(sAttribName)->second];
		const tinygltf::BufferView& kBufferView = kModel.bufferViews[kAccessor.bufferView];

		*kppfBuffer = (const float*)&kModel.buffers[kBufferView.buffer].data[kAccessor.byteOffset + kBufferView.byteOffset];

		if (kAccessor.ByteStride(kBufferView) != 0)
		{
			*puiStride = (UINT)(kAccessor.ByteStride(kBufferView) / sizeof(float));
		}
		else
		{
			*puiStride = (UINT)(tinygltf::GetNumComponentsInType(uiType) * sizeof(float));
		}

		if (puiCount != nullptr)
		{
			*puiCount = (UINT)kAccessor.count;
		}
	}
	else
	{
		LOG_ERROR(tag, L"Failed to find attribute data with name %s for primitive!", sAttribName);

		return false;
	}

	return true;
}

UINT MeshManager::GetNumMeshes() const
{
	return m_Meshes.size();
}

std::unordered_map<std::string, Mesh*>* MeshManager::GetMeshes()
{
	return &m_Meshes;
}

bool MeshManager::CreateBLAS(ID3D12GraphicsCommandList4* pGraphicsCommandList, ID3D12Device5* pDevice)
{
	for (std::unordered_map<std::string, Mesh*>::iterator it = m_Meshes.begin(); it != m_Meshes.end(); ++it)
	{
		if (it->second->CreateBLAS(pGraphicsCommandList, pDevice) == false)
		{
			return false;
		}
	}

	return true;
}

UINT MeshManager::GetNumPrimitives() const
{
	return m_uiNumPrimitives;
}

UINT MeshManager::GetNumActivePrimitives() const
{
	return m_uiNumActivePrimitives;
}

UINT MeshManager::GetNumActiveRaytracedPrimitives() const
{
	return m_uiNumActiveRaytracedPrimitives;
}

void MeshManager::AddNumActivePrimitives(UINT uiNumActivePrimitives)
{
	m_uiNumActivePrimitives += uiNumActivePrimitives;
}

void MeshManager::AddNumActiveRaytracedPrimitives(UINT uiNumActiveRaytracedPrimitives)
{
	m_uiNumActiveRaytracedPrimitives += uiNumActiveRaytracedPrimitives;
}

void MeshManager::Save(nlohmann::json& data)
{
	for (std::unordered_map<std::string, Mesh*>::iterator it = m_Meshes.begin(); it != m_Meshes.end(); ++it)
	{
		data["Meshes"]["Name"].push_back(it->first);
		data["Meshes"]["Filepath"].push_back(it->second->m_sFilePath);
	}
}

void MeshManager::LoadScene(const std::string& ksFilepath, ID3D12GraphicsCommandList* pGraphicsCommandList)
{
	//Load in JSON data
	std::ifstream inFile(ksFilepath);

	if (inFile.is_open() == false)
	{
		LOG_ERROR(tag, L"Failed to open scene json file when loading mesh data!");

		return;
	}

	nlohmann::json data;
	inFile >> data;

	inFile.close();

	for (int i = 0; i < data["Meshes"]["Name"].size(); ++i)
	{
		LoadMesh(data["Meshes"]["Filepath"][i], data["Meshes"]["Name"][i], pGraphicsCommandList);
	}
}

bool MeshManager::LoadTextures(std::string sName, Mesh* pMesh, const tinygltf::Model kModel, ID3D12GraphicsCommandList* pGraphicsCommandList)
{
	tinygltf::Image image;

	for (UINT i = 0; i < kModel.textures.size(); ++i)
	{
		image = kModel.images[kModel.textures[i].source];

		Texture* pTexture;

		std::string sTexName = sName + "Tex" + std::to_string(i);

		if (TextureManager::GetInstance()->LoadTexture(sTexName, image, pTexture, pGraphicsCommandList) == false)
		{
			return false;
		}

		pMesh->m_Textures.push_back(pTexture);
	}

	return true;
}

bool MeshManager::GetMesh(std::string sName, Mesh*& pMesh)
{
	if (m_Meshes.count(sName) == 0)
	{
		LOG_ERROR(tag, L"Tried to get a mesh called %s but one with that name doesn't exist!", sName);

		return false;
	}

	pMesh = m_Meshes[sName];

	return true;
}

bool MeshManager::RemoveMesh(std::string sName)
{
	if (m_Meshes.count(sName) == 0)
	{
		LOG_ERROR(tag, L"Tried to remove a mesh called %s but one with that name doesn't exist!", sName);

		return false;
	}

	m_Meshes.erase(sName);

	return true;
}

bool MeshManager::GetVertexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, const float** kppfPositionBuffer, UINT* puiPositionStride, const float** kppfNormalBuffer, UINT* puiNormalStride, const float** kppfTexCoordBuffer, UINT* puiTexCoordStride, const float** kppfTangentBuffer, UINT* puiTangentStride, UINT* puiVertexCount, Primitive* pPrimitive)
{
	if (GetAttributeData(kModel, kPrimitive, "POSITION", kppfPositionBuffer, puiPositionStride, puiVertexCount, TINYGLTF_TYPE_VEC3) == false)
	{
		return false;
	}

	if (GetAttributeData(kModel, kPrimitive, "TEXCOORD_0", kppfTexCoordBuffer, puiTexCoordStride, nullptr, TINYGLTF_TYPE_VEC2) == false)
	{
		return false;
	}

	if (GetAttributeData(kModel, kPrimitive, "NORMAL", kppfNormalBuffer, puiNormalStride, nullptr, TINYGLTF_TYPE_VEC3) == true)
	{

	}

	if (GetAttributeData(kModel, kPrimitive, "TANGENT", kppfTangentBuffer, puiTangentStride, nullptr, TINYGLTF_TYPE_VEC4) == true)
	{
		//has normal map so enable bit
		pPrimitive->m_Attributes = pPrimitive->m_Attributes | PrimitiveAttributes::NORMAL;
	}

	return true;
}

bool MeshManager::GetIndexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::vector<UINT>* pIndexBuffer, UINT* puiIndexCount)
{
	const tinygltf::Accessor* kpAccessor;

	if (kPrimitive.indices >= 0)
	{
		kpAccessor = &kModel.accessors[kPrimitive.indices];
	}
	else
	{
		kpAccessor = &kModel.accessors[0];
	}

	const tinygltf::BufferView& kBufferView = kModel.bufferViews[kpAccessor->bufferView];
	const tinygltf::Buffer& kBuffer = kModel.buffers[kBufferView.buffer];

	*puiIndexCount = (UINT)kpAccessor->count;

	const void* pData = &kBuffer.data[kpAccessor->byteOffset + kBufferView.byteOffset];

	switch (kpAccessor->componentType)
	{
	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
	{
		const UINT* pUINTData = (const UINT*)pData;

		for (UINT i = 0; i < *puiIndexCount; ++i)
		{
			pIndexBuffer->push_back((UINT)(pUINTData[i]));
		}
	}
	break;

	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
	{
		const UINT16* pUINTData = (const UINT16*)pData;

		for (UINT i = 0; i < *puiIndexCount; ++i)
		{
			pIndexBuffer->push_back((UINT)pUINTData[i]);
		}
	}
	break;

	case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
	{
		const uint8_t* pUINTData = (const uint8_t*)pData;

		for (UINT i = 0; i < *puiIndexCount; ++i)
		{
			pIndexBuffer->push_back((UINT)pUINTData[i]);
		}
	}
	break;

	default:
		LOG_ERROR(tag, L"The tinygltf index data type %i isn't supported!");

		return false;
	}

	return true;
}
