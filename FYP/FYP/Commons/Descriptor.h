#pragma once

#include "Include/DirectX/d3dx12.h"

#include <Windows.h>

class Descriptor
{
public:
	Descriptor(UINT uiDescriptorIndex);

	UINT GetDescriptorIndex() const;

protected:
	UINT m_uiDescriptorIndex;
};

