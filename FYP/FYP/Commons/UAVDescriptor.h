#pragma once
#include "Descriptor.h"

class UAVDescriptor : public Descriptor
{
public:
	UAVDescriptor(UINT uiDescriptorIndex, D3D12_CPU_DESCRIPTOR_HANDLE handle, ID3D12Resource* pResource, D3D12_UAV_DIMENSION viewDimension);

protected:

};

