#pragma once

#include <DirectXMath.h>

struct Vertex 
{ 
	Vertex(DirectX::XMFLOAT3 pos)
	{
		Position = pos;
	}

	DirectX::XMFLOAT3 Position;
};