#ifndef CONSTANT_BUFFERS_H
#define CONSTANT_BUFFERS_H

#ifdef HLSL
#include "HlslCompat.h"
#else
#include <DirectXMath.h>

using namespace DirectX;
#endif

struct LightCB
{
	XMFLOAT3 Position;
	int Type;

	XMFLOAT4 Color;

	XMFLOAT3 Attenuation;
	float pad;
};

struct ScenePerFrameCB
{
	XMMATRIX ViewProjection;
	XMMATRIX InvViewProjection;

	XMFLOAT3 EyePosW;
	UINT32 NumLights;

	UINT32 LightIndex;
	UINT32 PrimitivePerFrameIndex;
	UINT32 PrimitivePerInstanceIndex;
	UINT32 ScreenWidth;

	XMFLOAT3 EyeDirection;
	UINT32 ScreenHeight;
};

struct GameObjectPerFrameCB
{
	XMMATRIX World;
	XMMATRIX InvTransposeWorld;
};

struct PrimitiveIndexCB
{
	UINT32 InstanceIndex;
	UINT32 PrimitiveIndex;
	XMFLOAT2 pad;
};

struct PrimitiveInstanceCB
{
	UINT32 IndicesIndex;
	UINT32 VerticesIndex;
	UINT32 AlbedoIndex;
	UINT32 NormalIndex;

	UINT32 MetallicRoughnessIndex;
	UINT32 OcclusionIndex;
	XMFLOAT2 pad;
};

struct DeferredPerFrameCB
{
	UINT32 NormalIndex;
	UINT32 AlbedoIndex;
	UINT32 MetallicRoughnessOcclusion;
	UINT32 DepthIndex;
};

struct RaytracePerFrameCB
{
	XMINT3 ProbeCounts;
	UINT32 RaysPerProbe;

	XMFLOAT3 ProbeSpacing;
	float MaxRayDistance;

	XMFLOAT3 VolumePosition;
	int RayDataFormat;

	XMFLOAT3 MissRadiance;
	float NormalBias;

	float ViewBias;
	XMFLOAT3 pad;
};

#endif // CONSTANT_BUFFERS_H