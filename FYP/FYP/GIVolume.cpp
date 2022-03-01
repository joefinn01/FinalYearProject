#include "GIVolume.h"
#include "Commons/Mesh.h"
#include "GameObjects/GameObject.h"
#include "Managers/MeshManager.h"
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
}

void GIVolume::SetProbeTrackingTarget(const DirectX::XMFLOAT3& kTargetPos)
{
	m_ProbeTrackingTarget = kTargetPos;
}

void GIVolume::SetProbeSpacing(const DirectX::XMFLOAT3& kProbeSpacing)
{
	m_ProbeSpacing = kProbeSpacing;
}

void GIVolume::SetProbeScale(const float& kProbeScale)
{
	m_ProbeScale = kProbeScale;
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

	for (int i = 0; i < m_ProbeCounts.x; ++i)
	{
		for (int j = 0; j < m_ProbeCounts.y; ++j)
		{
			for (int k = 0; k < m_ProbeCounts.z; ++k)
			{
				iIndex = i + m_ProbeSpacing.x * (j + m_ProbeSpacing.z * k);

				pGameObject = new GameObject();
				pGameObject->Init("Probe" + std::to_string(iIndex), XMFLOAT3((i * m_ProbeSpacing.x) - totalDimensions.x * 0.5f, (j * m_ProbeSpacing.y) - totalDimensions.y * 0.5f, (k * m_ProbeSpacing.z) - totalDimensions.z * 0.5f), XMFLOAT3(), XMFLOAT3(m_ProbeScale, m_ProbeScale, m_ProbeScale), pMesh);
			}
		}
	}
}
