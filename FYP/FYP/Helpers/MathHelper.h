#pragma once

#include <Windows.h>

class MathHelper
{
public:
	static UINT Calculate32AlignedBufferSize(UINT size);
	static UINT Calculate64AlignedBufferSize(UINT size);

	static UINT CalculateAlignedSize(UINT uiSize, UINT uiAlignment);

	static UINT CalculatePaddedConstantBufferSize(UINT size);
protected:

private:

};

