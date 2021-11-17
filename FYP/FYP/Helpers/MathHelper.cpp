#include "MathHelper.h"

int MathHelper::Calculate32AlignedBufferSize(size_t size)
{
	return (size + 32) & ~32;
}

int MathHelper::Calculate64AlignedBufferSize(size_t size)
{
	int iRemainder = size % 64;

	if (iRemainder != 0)
	{
		return size + (64 - iRemainder);
	}
	else
	{
		return size;
	}


}

int MathHelper::CalculatePaddedConstantBufferSize(size_t size)
{
	return (size + 255) & ~255;
}
