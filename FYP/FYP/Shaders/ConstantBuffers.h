#ifndef CONSTANT_BUFFERS_H
#define CONSTANT_BUFFERS_H

#ifdef HLSL
#include "HlslCompat.h"
#else
#include <DirectXMath.h>

using namespace DirectX;
#endif

#define NUM_LIGHTS 5

struct LightCB
{
	XMFLOAT3 Position;
	int Type;

	XMFLOAT3 Color;
	int Enabled;

	XMFLOAT3 Attenuation;
	float pad;
};

struct ScenePerFrameCB
{
	XMMATRIX InvWorldProjection;

	XMVECTOR EyePosW;

	LightCB Lights[NUM_LIGHTS];
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