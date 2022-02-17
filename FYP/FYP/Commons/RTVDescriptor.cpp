#include "RTVDescriptor.h"
#include "Apps/App.h"

RTVDescriptor::RTVDescriptor(UINT uiDescriptorIndex, ID3D12Resource* pRes, D3D12_CPU_DESCRIPTOR_HANDLE handle) : Descriptor(uiDescriptorIndex)
{
	App::GetApp()->GetDevice()->CreateRenderTargetView(pRes, nullptr, handle);
}
