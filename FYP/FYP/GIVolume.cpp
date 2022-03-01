#include "GIVolume.h"
#include "Commons/Mesh.h"
#include "GameObjects/GameObject.h"
#include "Managers/MeshManager.h"
#include "Helpers/ImGuiHelper.h"
#include "Include/ImGui/imgui.h"


GIVolume::GIVolume(const GIVolumeDesc& kVolumeDesc, ID3D12GraphicsCommandList4* pCommandList)
{
	m_Position = kVolumeDesc.Position;
	
	m_ProbeCounts = kVolumeDesc.ProbeCounts;
	m_ProbeSpacing = kVolumeDesc.ProbeSpacing;

	m_bProbeRelocation = kVolumeDesc.ProbeRelocation;
	m_bProbeTracking = kVolumeDesc.ProbeTracking;

	CreateProbeGameObjects(pCommandList);
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

const DirectX::XMINT3& GIVolume::GetProbeCounts() const
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
				pGameObject->Init("Probe" + std::to_string(iIndex), XMFLOAT3(m_Position.x + offset.x, m_Position.y + offset.y, m_Position.z + offset.z), XMFLOAT3(), XMFLOAT3(m_ProbeScale, m_ProbeScale, m_ProbeScale), pMesh);

				m_ProbeGameObjects.push_back(pGameObject);
			}
		}
	}
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
