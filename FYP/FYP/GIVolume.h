#pragma once

#include "Include/DirectX/d3dx12.h"

#include <DirectXMath.h>
#include <vector>

class Timer;
class GameObject;

struct GIVolumeDesc
{
	DirectX::XMFLOAT3 Position;
	
	DirectX::XMINT3 ProbeCounts;
	DirectX::XMFLOAT3 ProbeSpacing;
	float ProbeScale;

	bool ProbeRelocation;
	bool ProbeTracking;
};

class GIVolume
{
public:
	GIVolume(const GIVolumeDesc& kVolumeDesc, ID3D12GraphicsCommandList4* pCommandList);

	void ShowUI();

	void Update(const Timer& kTimer);

	//Getters
	const DirectX::XMFLOAT3& GetPosition() const;
	const DirectX::XMFLOAT3& GetProbeTrackingTarget() const;
	const DirectX::XMFLOAT3& GetProbeSpacing() const;
	const float& GetProbeScale () const;

	const DirectX::XMINT3& GetProbeCounts() const;

	const bool& IsRelocating() const;
	const bool& IsTracking() const;
	const bool& IsShowingProbes() const;

	//Setters
	void SetPosition(const DirectX::XMFLOAT3& kPosition);
	void SetProbeTrackingTarget(const DirectX::XMFLOAT3& kTargetPos);
	void SetProbeSpacing(const DirectX::XMFLOAT3& kProbeSpacing);
	void SetProbeScale(const float& kProbeScale);

	void SetProbeCounts(DirectX::XMINT3& kProbeCounts);

	void SetIsRelocating(bool bIsRelocating);
	void SetIsTracking(bool bIsTracking);
	void SetIsShowingProbes(bool bIsShowingProbes);

protected:

private:
	void CreateProbeGameObjects(ID3D12GraphicsCommandList4* pCommandList);

	void UpdateProbePositions();
	void UpdateProbeScales();

	DirectX::XMFLOAT3 m_Position = DirectX::XMFLOAT3();
	DirectX::XMFLOAT3 m_ProbeTrackingTarget = DirectX::XMFLOAT3();
	DirectX::XMFLOAT3 m_ProbeSpacing = DirectX::XMFLOAT3();
	float m_ProbeScale = 1.0f;
	DirectX::XMINT3 m_ProbeCounts = DirectX::XMINT3(-1, -1, -1);

	std::vector<GameObject*> m_ProbeGameObjects = std::vector<GameObject*>();

	bool m_bProbeRelocation = false;
	bool m_bProbeTracking = false;
	bool m_bShowProbes = false;
};

