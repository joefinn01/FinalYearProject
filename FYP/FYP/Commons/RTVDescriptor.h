#pragma once
#include "Descriptor.h"

class RTVDescriptor : public Descriptor
{
public:
	RTVDescriptor(UINT uiDescriptorIndex, ID3D12Resource* pRes, D3D12_CPU_DESCRIPTOR_HANDLE handle);
};

