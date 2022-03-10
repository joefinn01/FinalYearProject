#include "GIVolume.h"
#include "Commons/Mesh.h"
#include "Commons/Texture.h"
#include "GameObjects/GameObject.h"
#include "Managers/MeshManager.h"
#include "Helpers/ImGuiHelper.h"
#include "Helpers/DebugHelper.h"
#include "Include/ImGui/imgui.h"
#include "Apps/App.h"

Tag tag = L"GIVolume";

GIVolume::GIVolume(const GIVolumeDesc& kVolumeDesc, ID3D12GraphicsCommandList4* pCommandList, DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap)
{
	m_Position = kVolumeDesc.Position;
	
	m_ProbeCounts = kVolumeDesc.ProbeCounts;
	m_ProbeSpacing = kVolumeDesc.ProbeSpacing;

	m_bProbeRelocation = kVolumeDesc.ProbeRelocation;
	m_bProbeTracking = kVolumeDesc.ProbeTracking;

	CreateProbeGameObjects(pCommandList);

	CreateTextureAtlases(pSRVHeap, pRTVHeap);
}

void GIVolume::ShowUI()
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

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

	if (ImGuiHelper::Checkbox("Show Probes", m_bShowProbes) == true)
	{
		ToggleProbeVisibility();
	}
}

void GIVolume::Update(const Timer& kTimer)
{
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

const DirectX::XMUINT3& GIVolume::GetProbeCounts() const
{
	return m_ProbeCounts;
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

void GIVolume::SetProbeCounts(DirectX::XMUINT3& kProbeCounts)
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
