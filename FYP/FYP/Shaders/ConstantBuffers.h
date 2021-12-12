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
	XMMATRIX InvWorldProjection;

	XMFLOAT3 EyePosW;
	int NumLights;
};

struct PrimitivePerFrameCB
{
	XMMATRIX World;
	XMMATRIX InvWorld;
};

struct PrimitiveInstanceCB
{
	int InstanceIndex;
	XMFLOAT3 pad;
};

#endif // CONSTANT_BUFFERS_H