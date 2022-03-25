#include "GIVolume.h"
#include "Commons/Mesh.h"
#include "Commons/Texture.h"
#include "GameObjects/GameObject.h"
#include "Managers/MeshManager.h"
#include "Managers/ObjectManager.h"
#include "Helpers/ImGuiHelper.h"
#include "Helpers/DebugHelper.h"
#include "Helpers/DXRHelper.h"
#include "Include/ImGui/imgui.h"
#include "Commons/ShaderTable.h"
#include "Apps/App.h"

#if PIX
#include "pix3.h"

#include <shlobj.h>
#include <strsafe.h>
#endif

#include <math.h>
#include <time.h>

Tag tag = L"GIVolume";

GIVolume::GIVolume(const GIVolumeDesc& kVolumeDesc, ID3D12GraphicsCommandList4* pCommandList, DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap)
{
	m_Position = kVolumeDesc.Position;
	
	m_ProbeCounts = kVolumeDesc.ProbeCounts;
	m_ProbeSpacing = kVolumeDesc.ProbeSpacing;

	m_bProbeRelocation = kVolumeDesc.ProbeRelocation;
	m_bProbeTracking = kVolumeDesc.ProbeTracking;

	m_rng.seed(time(0));

	CreateProbeGameObjects(pCommandList);

	CreateTextureAtlases(pSRVHeap, pRTVHeap);

	CompileShaders();

	CreateRootSignatures();

	CreatePSOs();

	CreateStateObject();

	CreateShaderTables();

	CreateConstantBuffers();

	UpdateConstantBuffers();

	CreateAccelerationStructures();
}

void GIVolume::ShowUI()
{
	if (ImGui::CollapsingHeader("GI Settings"))
	{
		if (ImGuiHelper::DragFloat3("Position", m_Position) == true)
		{
			UpdateProbePositions();
		}

		ImGui::Spacing();

		if (ImGuiHelper::DragFloat3("Probe Spacing", m_ProbeSpacing) == true)
		{
			UpdateProbePositions();
		}

		ImGui::Spacing();

		if (ImGuiHelper::DragFloat("Probe Scale", m_ProbeScale) == true)
		{
			UpdateProbeScales();
		}

		ImGui::Spacing();

		ImGuiHelper::DragFloat3("Miss Radiance", m_MissRadiance);

		ImGui::Spacing();

		ImGuiHelper::DragFloat("Max Ray Distance", m_fMaxRayDistance, 150.0f, 0.1f, 0, 10000);

		ImGui::Spacing();

		ImGuiHelper::DragFloat("Distance Exponent", m_fDistancePower, 150.0f, 0.1f, 0, 1000);

		ImGui::Spacing();

		ImGuiHelper::DragFloat("View Bias", m_fViewBias, 150.0f, 0.1f, 0, 10);

		ImGui::Spacing();

		ImGuiHelper::DragFloat("Normal Bias", m_fNormalBias, 150.0f, 0.1f, 0, 10);

		ImGui::Spacing();

		ImGuiHelper::DragFloat("Hysteresis", m_fHysteresis, 150.0f, 0.1f, 0, 1);

		ImGui::Spacing();

		ImGuiHelper::DragFloat("Irradiance Gamma", m_fIrradianceGammaEncoding, 150.0f, 0.1f, 0, 100);

		ImGui::Spacing();

		ImGuiHelper::DragFloat("Irradiance Threshold", m_fIrradianceThreshold, 150.0f, 0.1f, 0, 100);

		ImGui::Spacing();

		ImGuiHelper::DragFloat("Brightness Threshold", m_fBrightnessThreshold, 150.0f, 0.1f, 0, 100);

		ImGui::Spacing();

		if (ImGuiHelper::Checkbox("Show Probes", m_bShowProbes, 150.0f) == true)
		{
			ToggleProbeVisibility();
		}
	}
}

void GIVolume::Update(const Timer& kTimer)
{
	UpdateRandomRotation();

	UpdateConstantBuffers();
}

void GIVolume::Draw(DescriptorHeap* pSRVHeap, UploadBuffer<ScenePerFrameCB>* pScenePerFrameUpload, ID3D12GraphicsCommandList4* pGraphicsCommandList)
{
	if (CreateTLAS(true) == false)
	{
		return;
	}

	GPU_PROFILE_BEGIN(GpuStats::DRAW_VOLUME, pGraphicsCommandList)
	PIX_ONLY(PIXBeginEvent(pGraphicsCommandList, PIX_COLOR(50, 50, 50), "Draw GI Volume"));

	PopulateRayData(pSRVHeap, pScenePerFrameUpload, pGraphicsCommandList);
	BlendProbeAtlases(pSRVHeap, pScenePerFrameUpload, pGraphicsCommandList);
	DrawIndirect(pSRVHeap, pScenePerFrameUpload, pGraphicsCommandList);

	PIX_ONLY(PIXEndEvent());
	GPU_PROFILE_END(GpuStats::DRAW_VOLUME, pGraphicsCommandList)
}

const DirectX::XMFLOAT3& GIVolume::GetPosition() const
{
	return m_Position;
}

const DirectX::XMFLOAT3& GIVolume::GetProbeTrackingTarget() const
{
	return m_ProbeTrackingTarget;
}

const DirectX::XMFLOAT3& GIVolume::GetProbeSpacing() const
{
	return m_ProbeSpacing;
}

const float& GIVolume::GetProbeScale() const
{
	return m_ProbeScale;
}

const DirectX::XMINT3& GIVolume::GetProbeCounts() const
{
	return m_ProbeCounts;
}

UploadBuffer<RaytracePerFrameCB>* GIVolume::GetRaytracePerFrameUpload()
{
	return m_pRaytracedPerFrameUpload;
}

const bool& GIVolume::IsRelocating() const
{
	return m_bProbeRelocation;
}

const bool& GIVolume::IsTracking() const
{
	return m_bProbeTracking;
}

const bool& GIVolume::IsShowingProbes() const
{
	return m_bShowProbes;
}

void GIVolume::SetPosition(const DirectX::XMFLOAT3& kPosition)
{
	m_Position = kPosition;

	UpdateProbePositions();
}

void GIVolume::SetProbeTrackingTarget(const DirectX::XMFLOAT3& kTargetPos)
{
	m_ProbeTrackingTarget = kTargetPos;
}

void GIVolume::SetProbeSpacing(const DirectX::XMFLOAT3& kProbeSpacing)
{
	m_ProbeSpacing = kProbeSpacing;

	UpdateProbePositions();
}

void GIVolume::SetProbeScale(const float& kProbeScale)
{
	m_ProbeScale = kProbeScale;

	UpdateProbeScales();
}

void GIVolume::SetProbeCounts(DirectX::XMINT3& kProbeCounts)
{
	m_ProbeCounts = kProbeCounts;
}

void GIVolume::SetIsRelocating(bool bIsRelocating)
{
	m_bProbeRelocation = bIsRelocating;
}

void GIVolume::SetIsTracking(bool bIsTracking)
{
	m_bProbeTracking = bIsTracking;
}

void GIVolume::SetIsShowingProbes(bool bIsShowingProbes)
{
	m_bShowProbes = bIsShowingProbes;
}

void GIVolume::CreateProbeGameObjects(ID3D12GraphicsCommandList4* pCommandList)
{
	App::GetApp()->ResetCommandList();

	MeshManager::GetInstance()->LoadMesh("Models/Sphere/gLTF/Sphere.gltf", "Sphere", pCommandList);

	Mesh* pMesh = nullptr;
	MeshManager::GetInstance()->GetMesh("Sphere", pMesh);

	GameObject* pGameObject = nullptr;
	int iIndex = -1;

	XMFLOAT3 totalDimensions = XMFLOAT3(m_ProbeSpacing.x * (m_ProbeCounts.x - 1), m_ProbeSpacing.y * (m_ProbeCounts.y - 1), m_ProbeSpacing.z * (m_ProbeCounts.z - 1));
	XMFLOAT3 offset;

	m_ProbeGameObjects.reserve(m_ProbeCounts.x * m_ProbeCounts.y * m_ProbeCounts.z);

	for (int i = 0; i < m_ProbeCounts.x; ++i)
	{
		for (int j = 0; j < m_ProbeCounts.y; ++j)
		{
			for (int k = 0; k < m_ProbeCounts.z; ++k)
			{
				iIndex = i + m_ProbeCounts.x * (j + m_ProbeCounts.z * k);

				offset = XMFLOAT3((i * m_ProbeSpacing.x) - totalDimensions.x * 0.5f, (j * m_ProbeSpacing.y) - totalDimensions.y * 0.5f, (k * m_ProbeSpacing.z) - totalDimensions.z * 0.5f);

				pGameObject = new GameObject();
				pGameObject->Init("Probe" + std::to_string(iIndex), XMFLOAT3(m_Position.x + offset.x, m_Position.y + offset.y, m_Position.z + offset.z), XMFLOAT3(), XMFLOAT3(m_ProbeScale, m_ProbeScale, m_ProbeScale), pMesh, m_bShowProbes, false);

				m_ProbeGameObjects.push_back(pGameObject);
			}
		}
	}

	App::GetApp()->ExecuteCommandList();
}

void GIVolume::UpdateProbePositions()
{
	XMFLOAT3 totalDimensions = XMFLOAT3(m_ProbeSpacing.x * (m_ProbeCounts.x - 1), m_ProbeSpacing.y * (m_ProbeCounts.y - 1), m_ProbeSpacing.z * (m_ProbeCounts.z - 1));
	XMFLOAT3 offset;

	for (int i = 0; i < m_ProbeCounts.x; ++i)
	{
		for (int j = 0; j < m_ProbeCounts.y; ++j)
		{
			for (int k = 0; k < m_ProbeCounts.z; ++k)
			{
				offset = XMFLOAT3((i * m_ProbeSpacing.x) - totalDimensions.x * 0.5f, (j * m_ProbeSpacing.y) - totalDimensions.y * 0.5f, (k * m_ProbeSpacing.z) - totalDimensions.z * 0.5f);

				m_ProbeGameObjects[i + m_ProbeCounts.x * (j + m_ProbeCounts.z * k)]->SetPosition(XMFLOAT3(m_Position.x + offset.x, m_Position.y + offset.y, m_Position.z + offset.z));
			}
		}
	}
}

void GIVolume::UpdateProbeScales()
{
	for (int i = 0; i < m_ProbeCounts.x; ++i)
	{
		for (int j = 0; j < m_ProbeCounts.y; ++j)
		{
			for (int k = 0; k < m_ProbeCounts.z; ++k)
			{
				m_ProbeGameObjects[i + m_ProbeCounts.x * (j + m_ProbeCounts.z * k)]->SetScale(XMFLOAT3(m_ProbeScale, m_ProbeScale, m_ProbeScale));
			}
		}
	}
}

void GIVolume::ToggleProbeVisibility()
{
	for (int i = 0; i < m_ProbeCounts.x; ++i)
	{
		for (int j = 0; j < m_ProbeCounts.y; ++j)
		{
			for (int k = 0; k < m_ProbeCounts.z; ++k)
			{
				m_ProbeGameObjects[i + m_ProbeCounts.x * (j + m_ProbeCounts.z * k)]->SetIsRendering(m_bShowProbes);
			}
		}
	}
}

bool GIVolume::CreateTextureAtlases(DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap)
{
	if (CreateRayDataAtlas(pSRVHeap) == false)
	{
		return false;
	}

	if (CreateIrradianceAtlas(pSRVHeap, pRTVHeap) == false)
	{
		return false;
	}

	if (CreateDistanceAtlas(pSRVHeap, pRTVHeap) == false)
	{
		return false;
	}

	if (CreateProbeDataAtlas(pSRVHeap) == false)
	{
		return false;
	}

	return true;
}

bool GIVolume::CreateRayDataAtlas(DescriptorHeap* pSRVHeap)
{
	if (m_pRayDataAtlas != nullptr)
	{
		delete m_pRayDataAtlas;
		m_pRayDataAtlas = nullptr;
	}

	m_pRayDataAtlas = new Texture(nullptr, GetRayDataFormat());

	if (m_pRayDataAtlas->CreateResource(m_iRaysPerProbe, m_ProbeCounts.x * m_ProbeCounts.y * m_ProbeCounts.z, 1, GetRayDataFormat(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == false)
	{
		return false;
	}

	if (m_pRayDataAtlas->CreateSRVDesc(pSRVHeap) == false)
	{
		return false;
	}

	if (m_pRayDataAtlas->CreateUAVDesc(pSRVHeap) == false)
	{
		return false;
	}

	return true;
}

bool GIVolume::CreateProbeDataAtlas(DescriptorHeap* pSRVHeap)
{
	if (m_pProbeDataAtlas != nullptr)
	{
		delete m_pProbeDataAtlas;
		m_pProbeDataAtlas = nullptr;
	}

	m_pProbeDataAtlas = new Texture(nullptr, GetRayDataFormat());

	if (m_pProbeDataAtlas->CreateResource(m_ProbeCounts.x * m_ProbeCounts.y, m_ProbeCounts.z, 1, GetRayDataFormat(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) == false)
	{
		return false;
	}

	if (m_pProbeDataAtlas->CreateSRVDesc(pSRVHeap) == false)
	{
		return false;
	}

	if (m_pProbeDataAtlas->CreateUAVDesc(pSRVHeap) == false)
	{
		return false;
	}

	return true;
}

bool GIVolume::CreateIrradianceAtlas(DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap)
{
	if (m_pIrradianceAtlas != nullptr)
	{
		delete m_pIrradianceAtlas;
		m_pIrradianceAtlas = nullptr;
	}

	m_pIrradianceAtlas = new Texture(nullptr, GetRayDataFormat());

	if (m_pIrradianceAtlas->CreateResource(m_ProbeCounts.x * m_ProbeCounts.y * (UINT)(m_iIrradianceTexelsPerProbe + 2), m_ProbeCounts.z * (UINT)(m_iIrradianceTexelsPerProbe + 2), 1, GetRayDataFormat(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) == false)
	{
		return false;
	}

	if (m_pIrradianceAtlas->CreateSRVDesc(pSRVHeap) == false)
	{
		return false;
	}

	if (m_pIrradianceAtlas->CreateUAVDesc(pSRVHeap) == false)
	{
		return false;
	}

	if (m_pIrradianceAtlas->CreateRTVDesc(pRTVHeap) == false)
	{
		return false;
	}

	return true;
}

bool GIVolume::CreateDistanceAtlas(DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap)
{
	if (m_pDistanceAtlas != nullptr)
	{
		delete m_pDistanceAtlas;
		m_pDistanceAtlas = nullptr;
	}

	m_pDistanceAtlas = new Texture(nullptr, GetRayDataFormat());

	if (m_pDistanceAtlas->CreateResource(m_ProbeCounts.x * m_ProbeCounts.y * (UINT)(m_iDistanceTexelsPerProbe + 2), m_ProbeCounts.z * (UINT)(m_iDistanceTexelsPerProbe + 2), 1, GetRayDataFormat(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) == false)
	{
		return false;
	}

	if (m_pDistanceAtlas->CreateSRVDesc(pSRVHeap) == false)
	{
		return false;
	}

	if (m_pDistanceAtlas->CreateUAVDesc(pSRVHeap) == false)
	{
		return false;
	}

	if (m_pDistanceAtlas->CreateRTVDesc(pRTVHeap) == false)
	{
		return false;
	}

	return true;
}

bool GIVolume::CreateStateObject()
{
	CD3DX12_STATE_OBJECT_DESC pipelineDesc = CD3DX12_STATE_OBJECT_DESC(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	//Associate shaders with pipeline and create hit groups
	AssociateShader(m_kwsRayGenName, m_kwsRayGenName, pipelineDesc);

	AssociateShader(m_kwsMissName, m_kwsMissName, pipelineDesc);

	for (int i = 0; i < (int)ClosestHitShaderVariants::COUNT; ++i)
	{
		AssociateShader(m_ClosestHitNames[i], m_ClosestHitNames[i], pipelineDesc);
		CreateHitGroup(m_ClosestHitNames[i], m_HitGroupNames[i], pipelineDesc);
	}

	//Do shader config stuff
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* pShaderConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	pShaderConfig->Config(sizeof(XMFLOAT4) * 3, sizeof(XMFLOAT2));

	//Set root signatures

	//Local
	CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT* pLocalRootSignature = pipelineDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
	pLocalRootSignature->SetRootSignature(m_pLocalRootSignature.Get());

	CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT* pAssociation = pipelineDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	pAssociation->SetSubobjectToAssociate(*pLocalRootSignature);

	for (int i = 0; i < (int)ClosestHitShaderVariants::COUNT; ++i)
	{
		pAssociation->AddExport(m_HitGroupNames[i]);
	}

	//Global
	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* pGlobalRootSignature = pipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	pGlobalRootSignature->SetRootSignature(m_pGlobalRootSignature.Get());

	//Do pipeline config
	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pPipelineConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	pPipelineConfig->Config(1);

	HRESULT hr = ((ID3D12Device5*)App::GetApp()->GetDevice())->CreateStateObject(pipelineDesc, IID_PPV_ARGS(&m_pStateObject));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the raytracing pipeline object!");

		return false;
	}

	hr = m_pStateObject.As(&m_pStateObjectProps);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to get raytracing pipeline properties from object!");

		return false;
	}

	return true;
}

bool GIVolume::CreateRootSignatures()
{
	//Local root signature
	{
		D3D12_ROOT_PARAMETER1 slotRootParameter[RaytracingPass::LocalRootSignatureParams::COUNT] = {};

		//Primitive instance index
		slotRootParameter[RaytracingPass::LocalRootSignatureParams::INDEX].Constants.Num32BitValues = ceil(sizeof(PrimitiveIndexCB) / sizeof(UINT32));
		slotRootParameter[RaytracingPass::LocalRootSignatureParams::INDEX].Constants.ShaderRegister = 2;
		slotRootParameter[RaytracingPass::LocalRootSignatureParams::INDEX].Constants.RegisterSpace = 0;
		slotRootParameter[RaytracingPass::LocalRootSignatureParams::INDEX].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		slotRootParameter[RaytracingPass::LocalRootSignatureParams::INDEX].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		if (DXRHelper::CreateRootSignature((D3D12_ROOT_PARAMETER1*)slotRootParameter, (int)_countof(slotRootParameter), nullptr, 0, m_pLocalRootSignature.GetAddressOf(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE) == false)
		{
			return false;
		}
	}

	//Global root signature
	{
		D3D12_DESCRIPTOR_RANGE1 outputRange[1] = {};
		outputRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		outputRange[0].NumDescriptors = 1;
		outputRange[0].BaseShaderRegister = 0;
		outputRange[0].RegisterSpace = 0;
		outputRange[0].OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER1 slotRootParameter[RaytracingPass::GlobalRootSignatureParams::COUNT] = {};

		//SRV descriptors
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::STANDARD_DESCRIPTORS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::STANDARD_DESCRIPTORS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::STANDARD_DESCRIPTORS].DescriptorTable.pDescriptorRanges = DXRHelper::GetDescriptorRanges();
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::STANDARD_DESCRIPTORS].DescriptorTable.NumDescriptorRanges = App::GetApp()->GetNumStandardDescriptorRanges();

		//Acceleration structure
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::ACCELERATION_STRUCTURE].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::ACCELERATION_STRUCTURE].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::ACCELERATION_STRUCTURE].Descriptor.ShaderRegister = 0;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::ACCELERATION_STRUCTURE].Descriptor.RegisterSpace = 200;

		//UAV descriptors
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::RAY_DATA].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::RAY_DATA].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::RAY_DATA].DescriptorTable.pDescriptorRanges = outputRange;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::RAY_DATA].DescriptorTable.NumDescriptorRanges = _countof(outputRange);

		//Scene per frame CB
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_SCENE_CB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_SCENE_CB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_SCENE_CB].Descriptor.RegisterSpace = 0;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_SCENE_CB].Descriptor.ShaderRegister = 0;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_SCENE_CB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

		//Raytrace per frame CB
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_RAYTRACE_CB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_RAYTRACE_CB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_RAYTRACE_CB].Descriptor.RegisterSpace = 0;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_RAYTRACE_CB].Descriptor.ShaderRegister = 1;
		slotRootParameter[RaytracingPass::GlobalRootSignatureParams::PER_FRAME_RAYTRACE_CB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

		std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = DXRHelper::GetStaticSamplers();

		if (DXRHelper::CreateRootSignature((D3D12_ROOT_PARAMETER1*)slotRootParameter, (int)_countof(slotRootParameter), staticSamplers.data(), (int)staticSamplers.size(), m_pGlobalRootSignature.GetAddressOf()) == false)
		{
			return false;
		}
	}

	{
		D3D12_DESCRIPTOR_RANGE1 rayDataRange[1] = {};
		rayDataRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		rayDataRange[0].NumDescriptors = 1;
		rayDataRange[0].BaseShaderRegister = 0;
		rayDataRange[0].RegisterSpace = 0;
		rayDataRange[0].OffsetInDescriptorsFromTableStart = 0;

		D3D12_DESCRIPTOR_RANGE1 textureAtlasRange[1] = {};
		textureAtlasRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		textureAtlasRange[0].NumDescriptors = 1;
		textureAtlasRange[0].BaseShaderRegister = 1;
		textureAtlasRange[0].RegisterSpace = 0;
		textureAtlasRange[0].OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER1 slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::COUNT] = {};

		//SRV descriptors
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::STANDARD_DESCRIPTORS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::STANDARD_DESCRIPTORS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::STANDARD_DESCRIPTORS].DescriptorTable.pDescriptorRanges = DXRHelper::GetDescriptorRanges();
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::STANDARD_DESCRIPTORS].DescriptorTable.NumDescriptorRanges = App::GetApp()->GetNumStandardDescriptorRanges();

		//Ray Data UAV
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::RAY_DATA].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::RAY_DATA].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::RAY_DATA].DescriptorTable.pDescriptorRanges = rayDataRange;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::RAY_DATA].DescriptorTable.NumDescriptorRanges = _countof(rayDataRange);

		//Texture atlas UAV
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::TEXTURE_ATLAS].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::TEXTURE_ATLAS].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::TEXTURE_ATLAS].DescriptorTable.pDescriptorRanges = textureAtlasRange;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::TEXTURE_ATLAS].DescriptorTable.NumDescriptorRanges = _countof(textureAtlasRange);

		//Scene per frame CB
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB].Descriptor.RegisterSpace = 0;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB].Descriptor.ShaderRegister = 0;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

		//Raytrace per frame CB
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB].Descriptor.RegisterSpace = 0;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB].Descriptor.ShaderRegister = 1;
		slotRootParameter[RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

		std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = DXRHelper::GetStaticSamplers();

		if (DXRHelper::CreateRootSignature((D3D12_ROOT_PARAMETER1*)slotRootParameter, (int)_countof(slotRootParameter), staticSamplers.data(), (int)staticSamplers.size(), m_pProbeBlendingRootSignature.GetAddressOf()) == false)
		{
			return false;
		}
	}

	return true;
}

bool GIVolume::CreateShaderTables()
{
	if (CreateRayGenShaderTable() == false)
	{
		return false;
	}

	if (CreateMissShaderTable() == false)
	{
		return false;
	}

	if (CreateHitGroupShaderTable() == false)
	{
		return false;
	}

	return true;
}

bool GIVolume::CreateRayGenShaderTable()
{
	void* pRayGenIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_kwsRayGenName);

	ShaderTable shaderTable = ShaderTable(App::GetApp()->GetDevice(), 1, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	if (shaderTable.AddRecord(ShaderRecord(nullptr, 0, pRayGenIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
	{
		LOG_ERROR(tag, L"Failed to add a ray gen shader record!");

		return false;
	}

	m_pRayGenTable = shaderTable.GetBuffer();
	m_uiRayGenRecordSize = shaderTable.GetRecordSize();

	return true;
}

bool GIVolume::CreateMissShaderTable()
{
	void* pMissIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_kwsMissName);

	ShaderTable shaderTable = ShaderTable(App::GetApp()->GetDevice(), 1, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	if (shaderTable.AddRecord(ShaderRecord(nullptr, 0, pMissIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
	{
		LOG_ERROR(tag, L"Failed to add a miss shader record!");

		return false;
	}

	m_pMissTable = shaderTable.GetBuffer();
	m_uiMissRecordSize = shaderTable.GetRecordSize();

	return true;
}

bool GIVolume::CreateHitGroupShaderTable()
{
	void* pHitGroupIdentifier = nullptr;

	struct HitGroupRootArgs
	{
		PrimitiveIndexCB IndexCB;
	};

	HitGroupRootArgs hitGroupRootArgs;

	ShaderTable hitGroupTable = ShaderTable(App::GetApp()->GetDevice(), MeshManager::GetInstance()->GetNumActiveRaytracedPrimitives(), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + sizeof(HitGroupRootArgs));

	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	const MeshNode* pNode;
	Mesh* pMesh;

	UINT uiNumPrimitives = 0;

	for (std::unordered_map<std::string, GameObject*>::iterator it = pGameObjects->begin(); it != pGameObjects->end(); ++it)
	{
		if (it->second->IsRaytraced() == false)
		{
			continue;
		}

		pMesh = it->second->GetMesh();

		uiNumPrimitives = 0;

		for (int i = 0; i < pMesh->GetNodes()->size(); ++i)
		{
			pNode = pMesh->GetNode(i);

			//Assign per primitive information
			for (int i = 0; i < pNode->m_Primitives.size(); ++i)
			{
				hitGroupRootArgs.IndexCB.InstanceIndex = it->second->GetIndex() + uiNumPrimitives;
				hitGroupRootArgs.IndexCB.PrimitiveIndex = pNode->m_Primitives[i]->m_iIndex;

				++uiNumPrimitives;

				switch (pNode->m_Primitives[i]->m_Attributes)
				{
				case PrimitiveAttributes::ALBEDO:
					pHitGroupIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_HitGroupNames[(int)ClosestHitShaderVariants::ALBEDO]);
					break;

				default:
					pHitGroupIdentifier = m_pStateObjectProps->GetShaderIdentifier(m_HitGroupNames[(int)ClosestHitShaderVariants::NONE]);
					break;
				}

				if (hitGroupTable.AddRecord(ShaderRecord(&hitGroupRootArgs, sizeof(HitGroupRootArgs), pHitGroupIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)) == false)
				{
					LOG_ERROR(tag, L"Failed to add a hit group shader record!");

					return false;
				}
			}
		}
	}

	m_pHitGroupTable = hitGroupTable.GetBuffer();
	m_uiHitGroupRecordSize = hitGroupTable.GetRecordSize();

	return true;
}

bool GIVolume::CreateAccelerationStructures()
{
	App::GetApp()->ResetCommandList();

	if (MeshManager::GetInstance()->CreateBLAS(App::GetApp()->GetGraphicsCommandList(), App::GetApp()->GetDevice()) == false)
	{
		return false;
	}

	if (CreateTLAS(false) == false)
	{
		return false;
	}

	App::GetApp()->ExecuteCommandList();

	return true;
}

bool GIVolume::CreateTLAS(bool bUpdate)
{
	ID3D12Device5* pDevice = App::GetApp()->GetDevice();
	ID3D12GraphicsCommandList4* pGraphicsCommandList = App::GetApp()->GetGraphicsCommandList();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.NumDescs = MeshManager::GetInstance()->GetNumActiveRaytracedPrimitives();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (bUpdate == true)
	{
		pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_TopLevelBuffer.m_pResult.Get()));
	}
	else
	{
		if (DXRHelper::CreateUAVBuffer(pDevice, info.ScratchDataSizeInBytes, m_TopLevelBuffer.m_pScratch.GetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == false)
		{
			LOG_ERROR(tag, L"Failed to create the top level acceleration structure scratch buffer!");

			return false;
		}

		if (DXRHelper::CreateUAVBuffer(pDevice, info.ResultDataMaxSizeInBytes, m_TopLevelBuffer.m_pResult.GetAddressOf(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) == false)
		{
			LOG_ERROR(tag, L"Failed to create the top level acceleration structure result buffer!");

			return false;
		}

		m_TopLevelBuffer.m_pInstanceDesc = new UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC>(pDevice, MeshManager::GetInstance()->GetNumActiveRaytracedPrimitives(), false);
	}

	inputs.InstanceDescs = m_TopLevelBuffer.m_pInstanceDesc->GetBufferGPUAddress();

	int iCount = 0;

	std::vector<MeshNode*>* pMeshNodes;

	XMFLOAT3X4 world;

	for (std::unordered_map<std::string, GameObject*>::iterator it = ObjectManager::GetInstance()->GetGameObjects()->begin(); it != ObjectManager::GetInstance()->GetGameObjects()->end(); ++it)
	{
		if (it->second->IsRaytraced() == false)
		{
			continue;
		}

		pMeshNodes = it->second->GetMesh()->GetNodes();

		for (int i = 0; i < pMeshNodes->size(); ++i)
		{
			for (int j = 0; j < pMeshNodes->at(i)->m_Primitives.size(); ++j)
			{
				D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};

				XMStoreFloat3x4(&world, XMMatrixMultiply(XMLoadFloat4x4(&pMeshNodes->at(i)->m_Transform), XMLoadFloat4x4(&it->second->GetWorldMatrix())));

				for (int j = 0; j < 3; ++j)
				{
					for (int k = 0; k < 4; ++k)
					{
						instanceDesc.Transform[j][k] = world.m[j][k];
					}
				}

				instanceDesc.AccelerationStructure = pMeshNodes->at(i)->m_Primitives[j]->m_BottomLevel.m_pResult->GetGPUVirtualAddress();
				instanceDesc.Flags = 0;
				instanceDesc.InstanceID = iCount;
				instanceDesc.InstanceContributionToHitGroupIndex = iCount;
				instanceDesc.InstanceMask = 0xFF;

				m_TopLevelBuffer.m_pInstanceDesc->CopyData(iCount, instanceDesc);

				++iCount;
			}
		}
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.DestAccelerationStructureData = m_TopLevelBuffer.m_pResult->GetGPUVirtualAddress();
	buildDesc.ScratchAccelerationStructureData = m_TopLevelBuffer.m_pScratch->GetGPUVirtualAddress();

	if (bUpdate == true)
	{
		buildDesc.SourceAccelerationStructureData = m_TopLevelBuffer.m_pResult->GetGPUVirtualAddress();

		buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	}

	pGraphicsCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_TopLevelBuffer.m_pResult.Get()));

	return true;
}

bool GIVolume::CreatePSOs()
{
	ID3D12Device* pDevice = App::GetApp()->GetDevice();
	//====================================================
	//Irradiance atlas blending
	//====================================================

	//General blend
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc;
	computePsoDesc.pRootSignature = m_pProbeBlendingRootSignature.Get();
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE((void*)m_Shaders[m_IrradianceProbeBlendingName]->GetBufferPointer(), m_Shaders[m_IrradianceProbeBlendingName]->GetBufferSize());
	computePsoDesc.CachedPSO = { nullptr, 0 };
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computePsoDesc.NodeMask = 0;

	HRESULT hr = pDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(m_pIrradianceBlendPSO.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the irradiance blend pipeline state objects!");

		return false;
	}

	//Row blend
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE((void*)m_Shaders[m_IrradianceRowProbeBlendingName]->GetBufferPointer(), m_Shaders[m_IrradianceRowProbeBlendingName]->GetBufferSize());

	hr = pDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(m_pIrradianceRowBlendPSO.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the irradiance row blend pipeline state objects!");

		return false;
	}

	//Column blend
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE((void*)m_Shaders[m_IrradianceColumnProbeBlendingName]->GetBufferPointer(), m_Shaders[m_IrradianceColumnProbeBlendingName]->GetBufferSize());

	hr = pDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(m_pIrradianceColumnBlendPSO.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the irradiance column blend pipeline state objects!");

		return false;
	}

	//====================================================
	//Distance atlas blending
	//====================================================

	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE((void*)m_Shaders[m_DistanceProbeBlendingName]->GetBufferPointer(), m_Shaders[m_DistanceProbeBlendingName]->GetBufferSize());

	hr = pDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(m_pDistanceBlendPSO.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the Distance blend pipeline state objects!");

		return false;
	}

	//Row blend
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE((void*)m_Shaders[m_DistanceRowProbeBlendingName]->GetBufferPointer(), m_Shaders[m_DistanceRowProbeBlendingName]->GetBufferSize());

	hr = pDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(m_pDistanceRowBlendPSO.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the Distance row blend pipeline state objects!");

		return false;
	}

	//Column blend
	computePsoDesc.CS = CD3DX12_SHADER_BYTECODE((void*)m_Shaders[m_DistanceColumnProbeBlendingName]->GetBufferPointer(), m_Shaders[m_DistanceColumnProbeBlendingName]->GetBufferSize());

	hr = pDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(m_pDistanceColumnBlendPSO.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the Distance column blend pipeline state objects!");

		return false;
	}

	return true;
}

bool GIVolume::CompileShaders()
{
	Microsoft::WRL::ComPtr<IDxcBlob> pBlob;

	std::wstring wsRaysPerProbe = std::wstring(L"BLEND_RAYS_PER_PROBE=") + std::to_wstring(m_iRaysPerProbe);
	std::wstring wsIrradianceTexelsPerProbe = std::wstring(L"NUM_TEXELS_PER_PROBE=") + std::to_wstring(m_iIrradianceTexelsPerProbe);
	std::wstring wsDistanceTexelsPerProbe = std::wstring(L"NUM_TEXELS_PER_PROBE=") + std::to_wstring(m_iDistanceTexelsPerProbe);

	LPCWSTR irradianceProbeBlendingDefines[] =
	{
		L"BLEND_RADIANCE=1",
		wsRaysPerProbe.c_str(),
		wsIrradianceTexelsPerProbe.c_str()
	};

	LPCWSTR distanceProbeBlendingDefines[] =
	{
		L"BLEND_RADIANCE=0",
		wsRaysPerProbe.c_str(),
		wsDistanceTexelsPerProbe.c_str()
	};

	LPCWSTR irradianceBorderProbeBlendingDefines[] =
	{
		L"BLEND_RADIANCE=1",
		wsIrradianceTexelsPerProbe.c_str()
	};

	LPCWSTR distanceBorderProbeBlendingDefines[] =
	{
		L"BLEND_RADIANCE=0",
		wsDistanceTexelsPerProbe.c_str()
	};

	LPCWSTR albedo[] =
	{
		L"ALBEDO=1",
	};

	//Create array of shaders to compile
	CompileRecord records[] =
	{
		//Raytracing pass shaders
		CompileRecord(L"Shaders/RayGen.hlsl", m_kwsRayGenName, L"lib_6_3"),
		CompileRecord(L"Shaders/Miss.hlsl", m_kwsMissName, L"lib_6_3"),
		CompileRecord(L"Shaders/Hit.hlsl", m_ClosestHitNames[(int)ClosestHitShaderVariants::NONE], L"lib_6_3"),
		CompileRecord(L"Shaders/Hit.hlsl", m_ClosestHitNames[(int)ClosestHitShaderVariants::ALBEDO], L"lib_6_3", L"", albedo, _countof(albedo)),

		CompileRecord(L"Shaders/ProbeBlendingCompute.hlsl", m_IrradianceProbeBlendingName, L"cs_6_3", L"", irradianceProbeBlendingDefines, _countof(irradianceProbeBlendingDefines)),
		CompileRecord(L"Shaders/ProbeBorderBlendingCompute.hlsl", m_IrradianceRowProbeBlendingName, L"cs_6_3", L"RowBlend", irradianceBorderProbeBlendingDefines, _countof(irradianceBorderProbeBlendingDefines)),
		CompileRecord(L"Shaders/ProbeBorderBlendingCompute.hlsl", m_IrradianceColumnProbeBlendingName, L"cs_6_3", L"ColumnBlend", irradianceBorderProbeBlendingDefines, _countof(irradianceBorderProbeBlendingDefines)),

		CompileRecord(L"Shaders/ProbeBlendingCompute.hlsl", m_DistanceProbeBlendingName, L"cs_6_3", L"", distanceProbeBlendingDefines, _countof(distanceProbeBlendingDefines)),
		CompileRecord(L"Shaders/ProbeBorderBlendingCompute.hlsl", m_DistanceRowProbeBlendingName, L"cs_6_3", L"RowBlend", distanceBorderProbeBlendingDefines, _countof(distanceBorderProbeBlendingDefines)),
		CompileRecord(L"Shaders/ProbeBorderBlendingCompute.hlsl", m_DistanceColumnProbeBlendingName, L"cs_6_3", L"ColumnBlend", distanceBorderProbeBlendingDefines, _countof(distanceBorderProbeBlendingDefines)),
	};

	for (int i = 0; i < _countof(records); ++i)
	{
		pBlob = DXRHelper::CompileShader(records[i].Filepath, records[i].ShaderVersion, records[i].Entrypoint, records[i].Defines, records[i].NumDefines);

		if (pBlob == nullptr)
		{
			return false;
		}

		m_Shaders[records[i].ShaderName] = pBlob.Get();
	}

	return true;
}

void GIVolume::CreateConstantBuffers()
{
	m_pRaytracedPerFrameUpload = new UploadBuffer<RaytracePerFrameCB>(App::GetApp()->GetDevice(), 1, true);
}

void GIVolume::UpdateConstantBuffers()
{
	RaytracePerFrameCB raytracePerFrame;
	raytracePerFrame.BrightnessThreshold = m_fBrightnessThreshold;
	raytracePerFrame.DistanceIndex = m_pDistanceAtlas->GetSRVDesc()->GetDescriptorIndex();
	raytracePerFrame.DistancePower = m_fDistancePower;
	raytracePerFrame.Hysteresis = m_fHysteresis;
	raytracePerFrame.IrradianceFormat = m_iIrradianceFormat;
	raytracePerFrame.IrradianceGammaEncoding = m_fIrradianceGammaEncoding;
	raytracePerFrame.IrradianceIndex = m_pIrradianceAtlas->GetSRVDesc()->GetDescriptorIndex();
	raytracePerFrame.IrradianceThreshold = m_fIrradianceThreshold;
	raytracePerFrame.MaxRayDistance = m_fMaxRayDistance;
	raytracePerFrame.MissRadiance = m_MissRadiance;
	raytracePerFrame.NormalBias = m_fNormalBias;
	raytracePerFrame.NumDistanceTexels = m_iDistanceTexelsPerProbe;
	raytracePerFrame.NumIrradianceTexels = m_iIrradianceTexelsPerProbe;
	raytracePerFrame.ProbeCounts = m_ProbeCounts;
	raytracePerFrame.ProbeSpacing = m_ProbeSpacing;
	raytracePerFrame.RayDataFormat = (int)m_AtlasSize;
	raytracePerFrame.RayDataIndex = m_pRayDataAtlas->GetSRVDesc()->GetDescriptorIndex();
	raytracePerFrame.RayRotation = m_RayRotation;
	raytracePerFrame.ViewBias = m_fViewBias;
	raytracePerFrame.RaysPerProbe = m_iRaysPerProbe;
	raytracePerFrame.VolumePosition = m_Position;
	raytracePerFrame.ProbeOffsets = m_ProbeScrolling;
	raytracePerFrame.ClearPlane = m_ClearPlanes;
	
	m_pRaytracedPerFrameUpload->CopyData(0, raytracePerFrame);
}

static std::uniform_real_distribution<float> s_distribution(0.f, 1.f);

void GIVolume::UpdateRandomRotation()
{
	// This approach is based on James Arvo's implementation from Graphics Gems 3 (pg 117-120).
// Also available at: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.53.1357&rep=rep1&type=pdf

// Setup a random rotation matrix using 3 uniform RVs
	float u1 = XM_PI * s_distribution(m_rng) * 2.0f;
	float cos1 = cosf(u1);
	float sin1 = sinf(u1);

	float u2 = XM_PI * s_distribution(m_rng) * 2.0f;
	float cos2 = cosf(u2);
	float sin2 = sinf(u2);

	float u3 = s_distribution(m_rng);
	float sq3 = 2.f * sqrtf(u3 * (1.f - u3));

	float s2 = 2.f * u3 * sin2 * sin2 - 1.f;
	float c2 = 2.f * u3 * cos2 * cos2 - 1.f;
	float sc = 2.f * u3 * sin2 * cos2;

	// Create the random rotation matrix
	float _11 = cos1 * c2 - sin1 * sc;
	float _12 = sin1 * c2 + cos1 * sc;
	float _13 = sq3 * cos2;

	float _21 = cos1 * sc - sin1 * s2;
	float _22 = sin1 * sc + cos1 * s2;
	float _23 = sq3 * sin2;

	float _31 = cos1 * (sq3 * cos2) - sin1 * (sq3 * sin2);
	float _32 = sin1 * (sq3 * cos2) + cos1 * (sq3 * sin2);
	float _33 = 1.f - 2.f * u3;

	// HLSL is column-major
	DirectX::XMFLOAT3X3 transform(_11, _12, _13, _21, _22, _23, _31, _32, _33);

	XMStoreFloat4(&m_RayRotation, XMQuaternionNormalize(XMQuaternionRotationMatrix(XMLoadFloat3x3(&transform))));
}

DXGI_FORMAT GIVolume::GetRayDataFormat()
{
	switch(m_AtlasSize)
	{
	case AtlasSize::SMALLER:
		return DXGI_FORMAT_R32G32_FLOAT;

	case AtlasSize::BIGGER:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT GIVolume::GetIrradianceFormat()
{
	switch (m_AtlasSize)
	{
	case AtlasSize::SMALLER:
		return DXGI_FORMAT_R10G10B10A2_UNORM;

	case AtlasSize::BIGGER:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT GIVolume::GetDistanceFormat()
{
	switch (m_AtlasSize)
	{
	case AtlasSize::SMALLER:
		return DXGI_FORMAT_R16G16_FLOAT;

	case AtlasSize::BIGGER:
		return DXGI_FORMAT_R32G32_FLOAT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT GIVolume::GetProbeDataFormat()
{
	switch (m_AtlasSize)
	{
	case AtlasSize::SMALLER:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;

	case AtlasSize::BIGGER:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

void GIVolume::AssociateShader(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc)
{
	IDxcBlob* pBlob = m_Shaders[shaderName].Get();

	CD3DX12_DXIL_LIBRARY_SUBOBJECT* pLib = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	pLib->SetDXILLibrary(&CD3DX12_SHADER_BYTECODE((void*)pBlob->GetBufferPointer(), pBlob->GetBufferSize()));
	pLib->DefineExport(shaderExport);
}

void GIVolume::CreateHitGroup(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc, D3D12_HIT_GROUP_TYPE hitGroupType)
{
	CD3DX12_HIT_GROUP_SUBOBJECT* pHitGroup = pipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	pHitGroup->SetClosestHitShaderImport(shaderName);
	pHitGroup->SetHitGroupExport(shaderExport);
	pHitGroup->SetHitGroupType(hitGroupType);
}

void GIVolume::PopulateRayData(DescriptorHeap* pSRVHeap, UploadBuffer<ScenePerFrameCB>* pScenePerFrameUpload, ID3D12GraphicsCommandList4* pGraphicsCommandList)
{
	GPU_PROFILE_BEGIN(GpuStats::TRACE_RAYS, pGraphicsCommandList)
	PIX_ONLY(PIXBeginEvent(pGraphicsCommandList, PIX_COLOR(50, 50, 50), "Populate Ray Data"));

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchDesc.RayGenerationShaderRecord.StartAddress = m_pRayGenTable->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_pRayGenTable->GetDesc().Width;
	dispatchDesc.MissShaderTable.StartAddress = m_pMissTable->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.StrideInBytes = m_uiMissRecordSize;
	dispatchDesc.MissShaderTable.SizeInBytes = m_pMissTable->GetDesc().Width;
	dispatchDesc.HitGroupTable.StartAddress = m_pHitGroupTable->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.StrideInBytes = m_uiHitGroupRecordSize;
	dispatchDesc.HitGroupTable.SizeInBytes = m_pHitGroupTable->GetDesc().Width;
	dispatchDesc.Width = m_iRaysPerProbe;
	dispatchDesc.Height = m_ProbeCounts.x * m_ProbeCounts.y * m_ProbeCounts.z;
	dispatchDesc.Depth = 1;

	pGraphicsCommandList->SetComputeRootSignature(m_pGlobalRootSignature.Get());

	pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::GlobalRootSignatureParams::RAY_DATA, pSRVHeap->GetGpuDescriptorHandle(m_pRayDataAtlas->GetUAVDesc()->GetDescriptorIndex()));
	pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::GlobalRootSignatureParams::STANDARD_DESCRIPTORS, pSRVHeap->GetGpuDescriptorHandle());

	pGraphicsCommandList->SetComputeRootShaderResourceView(RaytracingPass::GlobalRootSignatureParams::ACCELERATION_STRUCTURE, m_TopLevelBuffer.m_pResult->GetGPUVirtualAddress());

	pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::GlobalRootSignatureParams::PER_FRAME_SCENE_CB, pScenePerFrameUpload->GetBufferGPUAddress(App::GetApp()->GetFrameIndex()));
	pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::GlobalRootSignatureParams::PER_FRAME_RAYTRACE_CB, m_pRaytracedPerFrameUpload->GetBufferGPUAddress());

	pGraphicsCommandList->SetPipelineState1(m_pStateObject.Get());

	pGraphicsCommandList->DispatchRays(&dispatchDesc);

	PIX_ONLY(PIXEndEvent());
	GPU_PROFILE_END(GpuStats::TRACE_RAYS, pGraphicsCommandList)
}

void GIVolume::BlendProbeAtlases(DescriptorHeap* pSRVHeap, UploadBuffer<ScenePerFrameCB>* pScenePerFrameUpload, ID3D12GraphicsCommandList4* pGraphicsCommandList)
{
	GPU_PROFILE_BEGIN(GpuStats::BLEND_PROBES, pGraphicsCommandList)
	PIX_ONLY(PIXBeginEvent(pGraphicsCommandList, PIX_COLOR(50, 50, 50), "Blend Probe Atlases"));

	DirectX::XMINT2 probeCounts = DirectX::XMINT2(m_ProbeCounts.x * m_ProbeCounts.y, m_ProbeCounts.z);
	int threadGroupSize = 8;

	CD3DX12_RESOURCE_BARRIER* pResourceBarriers = new CD3DX12_RESOURCE_BARRIER[2];
	pResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_pIrradianceAtlas->GetResource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_pDistanceAtlas->GetResource().Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	pGraphicsCommandList->ResourceBarrier(2, pResourceBarriers);

	GPU_PROFILE_BEGIN(GpuStats::ATLAS_BLEND_PROBES, pGraphicsCommandList)

	//Irradiance main blend
	{
		pGraphicsCommandList->SetPipelineState(m_pIrradianceBlendPSO.Get());

		pGraphicsCommandList->SetComputeRootSignature(m_pProbeBlendingRootSignature.Get());

		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::RAY_DATA, pSRVHeap->GetGpuDescriptorHandle(m_pRayDataAtlas->GetUAVDesc()->GetDescriptorIndex()));
		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::TEXTURE_ATLAS, pSRVHeap->GetGpuDescriptorHandle(m_pIrradianceAtlas->GetUAVDesc()->GetDescriptorIndex()));
		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::STANDARD_DESCRIPTORS, pSRVHeap->GetGpuDescriptorHandle());
		pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB, pScenePerFrameUpload->GetBufferGPUAddress(App::GetApp()->GetFrameIndex()));
		pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB, m_pRaytracedPerFrameUpload->GetBufferGPUAddress());

		pGraphicsCommandList->Dispatch(probeCounts.x, probeCounts.y, 1);
	}

	//Distance main blend
	{
		pGraphicsCommandList->SetPipelineState(m_pDistanceBlendPSO.Get());

		pGraphicsCommandList->SetComputeRootSignature(m_pProbeBlendingRootSignature.Get());

		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::RAY_DATA, pSRVHeap->GetGpuDescriptorHandle(m_pRayDataAtlas->GetUAVDesc()->GetDescriptorIndex()));
		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::TEXTURE_ATLAS, pSRVHeap->GetGpuDescriptorHandle(m_pDistanceAtlas->GetUAVDesc()->GetDescriptorIndex()));
		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::STANDARD_DESCRIPTORS, pSRVHeap->GetGpuDescriptorHandle());
		pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB, pScenePerFrameUpload->GetBufferGPUAddress(App::GetApp()->GetFrameIndex()));
		pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB, m_pRaytracedPerFrameUpload->GetBufferGPUAddress());

		pGraphicsCommandList->Dispatch(probeCounts.x, probeCounts.y, 1);
	}

	GPU_PROFILE_END(GpuStats::ATLAS_BLEND_PROBES, pGraphicsCommandList)

	GPU_PROFILE_BEGIN(GpuStats::BORDER_BLEND_PROBES, pGraphicsCommandList)

	//Irradiance row and column
	{
		DirectX::XMINT2 numGroups = DirectX::XMINT2(ceil(probeCounts.x * (m_iIrradianceTexelsPerProbe + 2) / (float)threadGroupSize), ceil(probeCounts.y / (float)threadGroupSize));

		pGraphicsCommandList->SetPipelineState(m_pIrradianceRowBlendPSO.Get());

		pGraphicsCommandList->SetComputeRootSignature(m_pProbeBlendingRootSignature.Get());

		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::RAY_DATA, pSRVHeap->GetGpuDescriptorHandle(m_pRayDataAtlas->GetUAVDesc()->GetDescriptorIndex()));
		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::TEXTURE_ATLAS, pSRVHeap->GetGpuDescriptorHandle(m_pIrradianceAtlas->GetUAVDesc()->GetDescriptorIndex()));
		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::STANDARD_DESCRIPTORS, pSRVHeap->GetGpuDescriptorHandle());
		pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB, pScenePerFrameUpload->GetBufferGPUAddress(App::GetApp()->GetFrameIndex()));
		pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB, m_pRaytracedPerFrameUpload->GetBufferGPUAddress());

		pGraphicsCommandList->Dispatch(numGroups.x, numGroups.y, 1);

		numGroups = DirectX::XMINT2(ceil(probeCounts.x / (float)threadGroupSize), ceil(probeCounts.y * (m_iIrradianceTexelsPerProbe + 2) / (float)threadGroupSize));

		pGraphicsCommandList->SetPipelineState(m_pIrradianceColumnBlendPSO.Get());

		pGraphicsCommandList->Dispatch(numGroups.x, numGroups.y, 1);
	}

	//Distance row and column
	{
		DirectX::XMINT2 numGroups = DirectX::XMINT2(ceil(probeCounts.x * (m_iDistanceTexelsPerProbe + 2) / (float)threadGroupSize), ceil(probeCounts.y / (float)threadGroupSize));

		pGraphicsCommandList->SetPipelineState(m_pDistanceRowBlendPSO.Get());

		pGraphicsCommandList->SetComputeRootSignature(m_pProbeBlendingRootSignature.Get());

		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::RAY_DATA, pSRVHeap->GetGpuDescriptorHandle(m_pRayDataAtlas->GetUAVDesc()->GetDescriptorIndex()));
		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::TEXTURE_ATLAS, pSRVHeap->GetGpuDescriptorHandle(m_pDistanceAtlas->GetUAVDesc()->GetDescriptorIndex()));
		pGraphicsCommandList->SetComputeRootDescriptorTable(RaytracingPass::ProbeBlendingRootSignatureParams::STANDARD_DESCRIPTORS, pSRVHeap->GetGpuDescriptorHandle());
		pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_SCENE_CB, pScenePerFrameUpload->GetBufferGPUAddress(App::GetApp()->GetFrameIndex()));
		pGraphicsCommandList->SetComputeRootConstantBufferView(RaytracingPass::ProbeBlendingRootSignatureParams::PER_FRAME_RAYTRACE_CB, m_pRaytracedPerFrameUpload->GetBufferGPUAddress());

		pGraphicsCommandList->Dispatch(numGroups.x, numGroups.y, 1);

		numGroups = DirectX::XMINT2(ceil(probeCounts.x / (float)threadGroupSize), ceil(probeCounts.y * (m_iDistanceTexelsPerProbe + 2) / (float)threadGroupSize));

		pGraphicsCommandList->SetPipelineState(m_pDistanceColumnBlendPSO.Get());

		pGraphicsCommandList->Dispatch(numGroups.x, numGroups.y, 1);
	}

	GPU_PROFILE_END(GpuStats::BORDER_BLEND_PROBES, pGraphicsCommandList)

	pResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_pIrradianceAtlas->GetResource().Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_pDistanceAtlas->GetResource().Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	pGraphicsCommandList->ResourceBarrier(2, pResourceBarriers);

	PIX_ONLY(PIXEndEvent());
	GPU_PROFILE_END(GpuStats::BLEND_PROBES, pGraphicsCommandList)
}

void GIVolume::DrawIndirect(DescriptorHeap* pSRVHeap, UploadBuffer<ScenePerFrameCB>* pScenePerFrameUpload, ID3D12GraphicsCommandList4* pGraphicsCommandList)
{
	PIX_ONLY(PIXBeginEvent(pGraphicsCommandList, PIX_COLOR(50, 50, 50), "Draw indirect light"));

	PIX_ONLY(PIXEndEvent());
}
