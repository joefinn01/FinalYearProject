#pragma once

#include <Windows.h>

#include <DirectXMath.h>

class MathHelper
{
public:
	static UINT Calculate32AlignedBufferSize(UINT size);
	static UINT Calculate64AlignedBufferSize(UINT size);

	static UINT CalculateAlignedSize(UINT uiSize, UINT uiAlignment);

	static UINT CalculatePaddedConstantBufferSize(UINT size);

	static DirectX::XMFLOAT4X4 Identity();
protected:

private:

};

