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

	XMFLOAT4 AlbedoColor;

	UINT32 MetallicRoughnessIndex;
	UINT32 OcclusionIndex;
	XMFLOAT2 pad;
};

struct DeferredPerFrameCB
{
	UINT32 AlbedoIndex;
	UINT32 DirectLightIndex;
	UINT32 PositionIndex;
	UINT32 NormalIndex;
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

	XMFLOAT4 RayRotation;

	float ViewBias;
	float DistancePower;
	float Hysteresis;
	float IrradianceGammaEncoding;

	float IrradianceThreshold;
	float BrightnessThreshold;
	int NumDistanceTexels;
	int NumIrradianceTexels;

	int IrradianceIndex;
	int DistanceIndex;
	int IrradianceFormat;
	int RayDataIndex;

	XMINT3 ProbeOffsets;
	float pad;

	XMINT3 ClearPlane;
	float pad2;
};

#endif // CONSTANT_BUFFERS_H