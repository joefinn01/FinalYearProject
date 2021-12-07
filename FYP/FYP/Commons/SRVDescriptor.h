#pragma once
#include "Descriptor.h"
class SRVDescriptor : public Descriptor
{
public:
	SRVDescriptor(UINT uiDescriptorIndex, D3D12_CPU_DESCRIPTOR_HANDLE handle, ID3D12Resource* pResource, D3D12_SRV_DIMENSION viewDimension, UINT uiNumElements, DXGI_FORMAT format, D3D12_BUFFER_SRV_FLAGS bufferFlags, UINT uiBufferByteStride, UINT64 uiFirstElement);
	SRVDescriptor(UINT uiDescriptorIndex, D3D12_CPU_DESCRIPTOR_HANDLE handle, ID3D12Resource* pResource, DXGI_FORMAT format, UINT uiMipMapLevels);

protected:

};

