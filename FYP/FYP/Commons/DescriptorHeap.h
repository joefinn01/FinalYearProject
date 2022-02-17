#pragma once

#include "Include/DirectX/d3dx12.h"

#include <wrl.h>

class DescriptorHeap
{
public:
	DescriptorHeap();

	bool Init(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT uiNumDescriptors);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetHeap() const;

	bool Allocate(UINT& uiDescriptorIndex);
	bool Allocate();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(UINT uiIndex) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT uiIndex) const;

	UINT GetDescriptorSize() const;
	UINT GetNumDescsAllocated() const;
	UINT GetMaxNumDescs() const;

protected:

private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap = nullptr;

	UINT m_uiNumDescriptors = 0;
	UINT m_uiDescriptorsAllocated = 0;
	UINT m_uiDescriptorSize = 0;
};

