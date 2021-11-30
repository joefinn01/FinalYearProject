#include "MeshManager.h"
#include "Include/tinygltf/tiny_gltf.h"
#include "Helpers/DebugHelper.h"
#include "Shaders/Vertices.h"

Tag tag = L"MeshManager";

bool MeshManager::LoadMesh(const std::string& sFilename)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool bSuccess = loader.LoadASCIIFromFile(&model, &err, &warn, sFilename);

	Mesh mesh;

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
	std::vector<UINT16> indexBuffer = std::vector<UINT16>();

	for (UINT i = 0; i < pScene->nodes.size(); ++i)
	{
		ProcessNode(nullptr, model.nodes[pScene->nodes[i]], pScene->nodes[i], model, &mesh, &vertexBuffer, &indexBuffer);
	}

	return true;
}

bool MeshManager::ProcessNode(MeshNode* pParentNode, const tinygltf::Node& kNode, UINT16 uiNodeIndex, const tinygltf::Model& kModel, Mesh* pMesh, std::vector<Vertex>* pVertexBuffer, std::vector<UINT16>* pIndexBuffer)
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
	}

	for (UINT i = 0; i < kNode.children.size(); ++i)
	{
		ProcessNode(pNode, kModel.nodes[kNode.children[i]], kNode.children[i], kModel, pMesh, pVertexBuffer, pIndexBuffer);
	}

	if (kNode.mesh >= 0)
	{
		const tinygltf::Mesh& kMesh = kModel.meshes[kNode.mesh];

		for (UINT i = 0; i < kMesh.primitives.size(); ++i)
		{
			const tinygltf::Primitive& kPrimitive = kMesh.primitives[i];
			UINT16 uiIndexStart = (UINT16)pIndexBuffer->size();
			UINT16 uiVertexStart = (UINT16)pVertexBuffer->size();
			UINT16 uiIndexCount = 0;
			UINT16 uiVertexCount = 0;
			bool bHasIndices = kPrimitive.indices >= 0;

			//Vertex information buffers
			const float* kpfPositionBuffer = nullptr;
			const float* kpfNormalBuffer = nullptr;
			const float* kpfTexCoordBuffer0 = nullptr;
			const float* kpfTexCoordBuffer1 = nullptr;

			UINT uiPositionStride;
			UINT uiNormalStride;
			UINT uiTexCoordStride0;
			UINT uiTexCoordStride1;

			if (GetVertexData(kModel, kPrimitive, &kpfPositionBuffer, &uiPositionStride, &kpfNormalBuffer, &uiNormalStride, &kpfTexCoordBuffer0, &uiTexCoordStride0, &kpfTexCoordBuffer1, &uiTexCoordStride1, &uiVertexCount) == false)
			{
				return false;
			}

			Vertex vertex = {};

			for (UINT i = 0; i < uiVertexCount; ++i)
			{
				vertex.Position = XMFLOAT3(kpfPositionBuffer[i * uiPositionStride], kpfPositionBuffer[(i * uiPositionStride) + 1], kpfPositionBuffer[(i * uiPositionStride) + 2]);
				//vertex.TexCoords0 = XMFLOAT2(kpfTexCoordBuffer0[i * uiTexCoordStride0], kpfTexCoordBuffer0[(i * 2 + 1) * uiTexCoordStride0]);
				//vertex.TexCoords1 = XMFLOAT2(kpfTexCoordBuffer1[i * 2 * uiTexCoordStride1], kpfTexCoordBuffer1[(i * 2 + 1) * uiTexCoordStride1]);

				XMStoreFloat3(&vertex.Normal, XMVector3Normalize(XMVectorSet(kpfNormalBuffer[i * uiNormalStride], kpfNormalBuffer[(i * uiNormalStride) + 1], kpfNormalBuffer[(i * uiNormalStride) + 2], 0.0f)));

				pVertexBuffer->push_back(vertex);
			}

			if (bHasIndices == true)
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
			}
		}
	}

	return true;
}

bool MeshManager::GetAttributeData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, std::string sAttribName, const float** kppfBuffer, UINT* puiStride, UINT16* puiCount, uint32_t uiType)
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
			*puiCount = (UINT16)kAccessor.count;
		}
	}
	else
	{
		LOG_ERROR(tag, L"Failed to find attribute data with name %s for primitive!", sAttribName);

		return false;
	}

	return true;
}

bool MeshManager::GetVertexData(const tinygltf::Model& kModel, const tinygltf::Primitive& kPrimitive, const float** kppfPositionBuffer, UINT* puiPositionStride, const float** kppfNormalBuffer, UINT* puiNormalStride, const float** kppfTexCoordBuffer0, UINT* puiTexCoordStride0, const float** kppfTexCoordBuffer1, UINT* puiTexCoordStride1, UINT16* puiVertexCount)
{
	if (GetAttributeData(kModel, kPrimitive, "POSITION", kppfPositionBuffer, puiPositionStride, puiVertexCount, TINYGLTF_TYPE_VEC3) == false)
	{
		return false;
	}

	if (GetAttributeData(kModel, kPrimitive, "NORMAL", kppfNormalBuffer, puiNormalStride, nullptr, TINYGLTF_TYPE_VEC3) == false)
	{
		return false;
	}

	//if (GetAttributeData(kModel, kPrimitive, "TEXCOORD_0", kppfTexCoordBuffer0, puiTexCoordStride0, nullptr, TINYGLTF_TYPE_VEC2) == false)
	//{
	//	return false;
	//}

	//if (GetAttributeData(kModel, kPrimitive, "TEXCOORD_1", kppfTexCoordBuffer1, puiTexCoordStride1, nullptr, TINYGLTF_TYPE_VEC2) == false)
	//{
	//	return false;
	//}

	return true;
}
