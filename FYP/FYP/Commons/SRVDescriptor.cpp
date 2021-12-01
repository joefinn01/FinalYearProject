#include "SRVDescriptor.h"
#include "Apps/App.h"

SRVDescriptor::SRVDescriptor(UINT uiDescriptorIndex, D3D12_CPU_DESCRIPTOR_HANDLE handle, ID3D12Resource* pResource, D3D12_SRV_DIMENSION viewDimension, UINT uiNumElements, DXGI_FORMAT format, D3D12_BUFFER_SRV_FLAGS bufferFlags, UINT bufferByteStride) : Descriptor(uiDescriptorIndex)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = viewDimension;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = uiNumElements;
	srvDesc.Format = format;
	srvDesc.Buffer.Flags = bufferFlags;
	srvDesc.Buffer.StructureByteStride = bufferByteStride;

	App::GetApp()->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, handle);
}
