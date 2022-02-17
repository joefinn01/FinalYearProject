#include "DSVDescriptor.h"
#include "Apps/App.h"

DSVDescriptor::DSVDescriptor(UINT uiDescriptorIndex, ID3D12Resource* pRes, D3D12_CPU_DESCRIPTOR_HANDLE handle, DXGI_FORMAT format) : Descriptor(uiDescriptorIndex)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = format;
	dsvDesc.Texture2D.MipSlice = 0;

	App::GetApp()->GetDevice()->CreateDepthStencilView(pRes, &dsvDesc, handle);
}
