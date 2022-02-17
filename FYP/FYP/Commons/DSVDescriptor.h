#pragma once
#include "Descriptor.h"
class DSVDescriptor : public Descriptor
{
public:
	DSVDescriptor(UINT uiDescriptorIndex, ID3D12Resource* pRes, D3D12_CPU_DESCRIPTOR_HANDLE handle, DXGI_FORMAT format);
};

