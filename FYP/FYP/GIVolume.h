#pragma once

#include "Include/DirectX/d3dx12.h"

#include <DirectXMath.h>
#include <vector>

class Timer;
class GameObject;
class Texture;
class DescriptorHeap;

struct GIVolumeDesc
{
	DirectX::XMFLOAT3 Position;
	
	DirectX::XMUINT3 ProbeCounts;
	DirectX::XMFLOAT3 ProbeSpacing;
	float ProbeScale;

	bool ProbeRelocation;
	bool ProbeTracking;
};

class GIVolume
{
public:
	GIVolume(const GIVolumeDesc& kVolumeDesc, ID3D12GraphicsCommandList4* pCommandList, DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap);

	void ShowUI();

	void Update(const Timer& kTimer);

	//Getters
	const DirectX::XMFLOAT3& GetPosition() const;
	const DirectX::XMFLOAT3& GetProbeTrackingTarget() const;
	const DirectX::XMFLOAT3& GetProbeSpacing() const;
	const float& GetProbeScale () const;

	const DirectX::XMUINT3& GetProbeCounts() const;

	const bool& IsRelocating() const;
	const bool& IsTracking() const;
	const bool& IsShowingProbes() const;

	//Setters
	void SetPosition(const DirectX::XMFLOAT3& kPosition);
	void SetProbeTrackingTarget(const DirectX::XMFLOAT3& kTargetPos);
	void SetProbeSpacing(const DirectX::XMFLOAT3& kProbeSpacing);
	void SetProbeScale(const float& kProbeScale);

	void SetProbeCounts(DirectX::XMUINT3& kProbeCounts);

	void SetIsRelocating(bool bIsRelocating);
	void SetIsTracking(bool bIsTracking);
	void SetIsShowingProbes(bool bIsShowingProbes);

protected:

private:
	enum class AtlasSize
	{
		SMALLER = 0,
		BIGGER,
		COUNT
	};

	void CreateProbeGameObjects(ID3D12GraphicsCommandList4* pCommandList);

	void UpdateProbePositions();
	void UpdateProbeScales();

	void ToggleProbeVisibility();

	bool CreateTextureAtlases(DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap);
	bool CreateRayDataAtlas(DescriptorHeap* pSRVHeap);
	bool CreateProbeDataAtlas(DescriptorHeap* pSRVHeap);
	bool CreateIrradianceAtlas(DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap);
	bool CreateDistanceAtlas(DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap);

	DXGI_FORMAT GetRayDataFormat();
	DXGI_FORMAT GetIrradianceFormat();
	DXGI_FORMAT GetDistanceFormat();
	DXGI_FORMAT GetProbeDataFormat();

	Texture* m_pRayDataAtlas = nullptr;
	Texture* m_pIrradianceAtlas = nullptr;
	Texture* m_pDistanceAtlas = nullptr;
	Texture* m_pProbeDataAtlas = nullptr;

	DirectX::XMFLOAT3 m_Position = DirectX::XMFLOAT3();
	DirectX::XMFLOAT3 m_ProbeTrackingTarget = DirectX::XMFLOAT3();
	DirectX::XMFLOAT3 m_ProbeSpacing = DirectX::XMFLOAT3();
	float m_ProbeScale = 1.0f;
	DirectX::XMUINT3 m_ProbeCounts = DirectX::XMUINT3(5, 5, 5);
	int m_iRaysPerProbe = 10;
	int m_iIrradianceTexelsPerProbe = 10;
	int m_iDistanceTexelsPerProbe = 10;
	
	AtlasSize m_AtlasSize = AtlasSize::SMALLER;

	std::vector<GameObject*> m_ProbeGameObjects = std::vector<GameObject*>();

	bool m_bProbeRelocation = false;
	bool m_bProbeTracking = false;
	bool m_bShowProbes = false;
};

