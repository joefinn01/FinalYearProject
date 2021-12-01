#include "UAVDescriptor.h"
#include "Apps/App.h"

UAVDescriptor::UAVDescriptor(UINT uiDescriptorIndex, D3D12_CPU_DESCRIPTOR_HANDLE handle, ID3D12Resource* pResource, D3D12_UAV_DIMENSION viewDimension) : Descriptor(uiDescriptorIndex)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	App::GetApp()->GetDevice()->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, handle);
}
