#include "MathHelper.h"

#include "Include/DirectX/d3dx12.h"

using namespace DirectX;

UINT MathHelper::Calculate32AlignedBufferSize(UINT size)
{
	return CalculateAlignedSize(size, 32);
}

UINT MathHelper::Calculate64AlignedBufferSize(UINT size)
{
	return CalculateAlignedSize(size, 64);
}

UINT MathHelper::CalculateAlignedSize(UINT uiSize, UINT uiAlignment)
{
	return (uiSize + (uiAlignment - 1)) & ~(uiAlignment - 1);
}

UINT MathHelper::CalculatePaddedConstantBufferSize(UINT size)
{
	return CalculateAlignedSize(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

XMFLOAT4X4 MathHelper::Identity()
{
	return XMFLOAT4X4
	(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
}
