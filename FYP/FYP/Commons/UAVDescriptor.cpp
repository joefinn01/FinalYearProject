#include "UAVDescriptor.h"
#include "Apps/App.h"

UAVDescriptor::UAVDescriptor(UINT uiDescriptorIndex, D3D12_CPU_DESCRIPTOR_HANDLE handle, ID3D12Resource* pResource, D3D12_UAV_DIMENSION viewDimension, DXGI_FORMAT format) : Descriptor(uiDescriptorIndex)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = viewDimension;
	uavDesc.Format = format;

	App::GetApp()->GetDevice()->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, handle);
}
