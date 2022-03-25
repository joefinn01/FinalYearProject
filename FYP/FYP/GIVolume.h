#pragma once

#include "Include/DirectX/d3dx12.h"
#include "Commons/UploadBuffer.h"
#include "Shaders/ConstantBuffers.h"
#include "Commons/AccelerationBuffers.h"

#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include <random>

struct IDxcBlob;

class Timer;
class GameObject;
class Texture;
class DescriptorHeap;

struct GIVolumeDesc
{
	DirectX::XMFLOAT3 Position;
	
	DirectX::XMINT3 ProbeCounts;
	DirectX::XMFLOAT3 ProbeSpacing;
	float ProbeScale;

	bool ProbeRelocation;
	bool ProbeTracking;
};

namespace RaytracingPass
{
	namespace GlobalRootSignatureParams
	{
		enum Value
		{
			RAY_DATA = 0,
			ACCELERATION_STRUCTURE,
			STANDARD_DESCRIPTORS,
			PER_FRAME_SCENE_CB,
			PER_FRAME_RAYTRACE_CB,
			COUNT
		};
	}

	namespace LocalRootSignatureParams 
	{
		enum Value
		{
			INDEX = 0,
			COUNT
		};
	}

	namespace ProbeBlendingRootSignatureParams 
	{
		enum Value
		{
			RAY_DATA = 0,
			TEXTURE_ATLAS,
			STANDARD_DESCRIPTORS,
			PER_FRAME_SCENE_CB,
			PER_FRAME_RAYTRACE_CB,
			COUNT
		};
	}
}

enum class ClosestHitShaderVariants
{
	ALBEDO = 0,
	NONE,

	COUNT
};

class GIVolume
{
public:
	GIVolume(const GIVolumeDesc& kVolumeDesc, ID3D12GraphicsCommandList4* pCommandList, DescriptorHeap* pSRVHeap, DescriptorHeap* pRTVHeap);

	void ShowUI();

	void Update(const Timer& kTimer);

	void Draw(DescriptorHeap* pSRVHeap, UploadBuffer<ScenePerFrameCB>* pScenePerFrameUpload, ID3D12GraphicsCommandList4* pGraphicsCommandList);

	//Getters
	const DirectX::XMFLOAT3& GetPosition() const;
	const DirectX::XMFLOAT3& GetProbeTrackingTarget() const;
	const DirectX::XMFLOAT3& GetProbeSpacing() const;
	const float& GetProbeScale () const;

	const DirectX::XMINT3& GetProbeCounts() const;

	UploadBuffer<RaytracePerFrameCB>* GetRaytracePerFrameUpload();

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

	bool CreateStateObject();
	bool CreateRootSignatures();

	bool CreateShaderTables();
	bool CreateRayGenShaderTable();
	bool CreateMissShaderTable();
	bool CreateHitGroupShaderTable();

	bool CreateAccelerationStructures();
	bool CreateTLAS(bool bUpdate);

	bool CreatePSOs();

	bool CompileShaders();

	void CreateConstantBuffers();

	void UpdateConstantBuffers();

	void UpdateRandomRotation();

	DXGI_FORMAT GetRayDataFormat();
	DXGI_FORMAT GetIrradianceFormat();
	DXGI_FORMAT GetDistanceFormat();
	DXGI_FORMAT GetProbeDataFormat();

	void AssociateShader(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc);

	void CreateHitGroup(LPCWSTR shaderName, LPCWSTR shaderExport, CD3DX12_STATE_OBJECT_DESC& pipelineDesc, D3D12_HIT_GROUP_TYPE hitGroupType = D3D12_HIT_GROUP_TYPE_TRIANGLES);

	void PopulateRayData(DescriptorHeap* pSRVHeap, UploadBuffer<ScenePerFrameCB>* pScenePerFrameUpload, ID3D12GraphicsCommandList4* pGraphicsCommandList);
	void BlendProbeAtlases(DescriptorHeap* pSRVHeap, UploadBuffer<ScenePerFrameCB>* pScenePerFrameUpload, ID3D12GraphicsCommandList4* pGraphicsCommandList);
	void DrawIndirect(DescriptorHeap* pSRVHeap, UploadBuffer<ScenePerFrameCB>* pScenePerFrameUpload, ID3D12GraphicsCommandList4* pGraphicsCommandList);

	UINT m_uiMissRecordSize;
	UINT m_uiHitGroupRecordSize;
	UINT m_uiRayGenRecordSize;

	Texture* m_pRayDataAtlas = nullptr;
	Texture* m_pIrradianceAtlas = nullptr;
	Texture* m_pDistanceAtlas = nullptr;
	Texture* m_pProbeDataAtlas = nullptr;

	DirectX::XMFLOAT3 m_Position = DirectX::XMFLOAT3();
	DirectX::XMFLOAT3 m_ProbeTrackingTarget = DirectX::XMFLOAT3();
	DirectX::XMFLOAT3 m_ProbeSpacing = DirectX::XMFLOAT3(0.3f, 0.3f, 0.3f);
	float m_ProbeScale = 0.05f;
	DirectX::XMINT3 m_ProbeCounts = DirectX::XMINT3(9, 9, 9);
	int m_iIrradianceTexelsPerProbe = 6;
	int m_iDistanceTexelsPerProbe = 14;
	
	AtlasSize m_AtlasSize = AtlasSize::BIGGER;

	std::vector<GameObject*> m_ProbeGameObjects = std::vector<GameObject*>();

	bool m_bProbeRelocation = false;
	bool m_bProbeTracking = false;
	bool m_bShowProbes = false;

	std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<IDxcBlob>> m_Shaders;

	LPCWSTR m_kwsClosestHitEntrypoint = L"ClosestHit";

	LPCWSTR m_kwsRayGenName = L"RayGen";
	LPCWSTR m_kwsMissName = L"Miss";

	LPCWSTR m_ClosestHitNames[(int)ClosestHitShaderVariants::COUNT] = 
	{
		L"ClosestHitAlbedo",
		L"ClosestHit",
	};

	LPCWSTR m_HitGroupNames[(int)ClosestHitShaderVariants::COUNT] = 
	{
		L"HitGroupAlbedo",
		L"HitGroup",
	};

	LPCWSTR m_IrradianceProbeBlendingName = L"IrradianceProbeBlendingCompute";
	LPCWSTR m_IrradianceRowProbeBlendingName = L"IrradianceRowProbeBlendingCompute";
	LPCWSTR m_IrradianceColumnProbeBlendingName = L"IrradianceColumnProbeBlendingCompute";
	LPCWSTR m_DistanceProbeBlendingName = L"DistanceProbeBlendingCompute";
	LPCWSTR m_DistanceRowProbeBlendingName = L"DistanceRowProbeBlendingCompute";
	LPCWSTR m_DistanceColumnProbeBlendingName = L"DistanceColumnProbeBlendingCompute";

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pGlobalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pLocalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pProbeBlendingRootSignature;

	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pStateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_pStateObjectProps;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pIrradianceBlendPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pIrradianceRowBlendPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pIrradianceColumnBlendPSO;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pDistanceBlendPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pDistanceRowBlendPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pDistanceColumnBlendPSO;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pMissTable;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pHitGroupTable;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pRayGenTable;

	UploadBuffer<RaytracePerFrameCB>* m_pRaytracedPerFrameUpload = nullptr;

	AccelerationBuffers m_TopLevelBuffer;

	float m_fMaxRayDistance = 10.0f;
	float m_fViewBias = 0.1f;
	float m_fNormalBias = 0.02f;
	int m_iRaysPerProbe = 288;
	float m_fBrightnessThreshold = 1.0f;
	float m_fDistancePower = 50.0f;
	float m_fHysteresis = 0.97f;
	int m_iIrradianceFormat = 1;
	float m_fIrradianceGammaEncoding = 5.0f;
	float m_fIrradianceThreshold = 0.2f;

	DirectX::XMFLOAT4 m_RayRotation = DirectX::XMFLOAT4(0, 0, 0, 1);

	std::mt19937 m_rng;
};

