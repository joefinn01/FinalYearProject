#pragma once

class MathHelper
{
public:
	static int Calculate32AlignedBufferSize(size_t size);
	static int Calculate64AlignedBufferSize(size_t size);

	static int CalculatePaddedConstantBufferSize(size_t size);
protected:

private:

};

