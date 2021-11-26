#ifndef VERTICES_H
#define VERTICES_H

#ifdef HLSL
#include "HlslCompat.h"
#else
#include <DirectXMath.h>

using namespace DirectX;
#endif

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
};

#endif // VERTICES_H