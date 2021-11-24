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
	Vertex(XMFLOAT3 pos)
	{
		Position = pos;
	}

	DirectX::XMFLOAT3 Position;
};

#endif // VERTICES_H