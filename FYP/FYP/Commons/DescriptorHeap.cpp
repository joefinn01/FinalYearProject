#include "DescriptorHeap.h"
#include "Apps/App.h"
#include "Helpers/DebugHelper.h"

Tag tag = L"DescriptorHeap";

DescriptorHeap::DescriptorHeap()
{
    m_pDescriptorHeap = nullptr;

    m_uiDescriptorsAllocated = 0;
    m_uiNumDescriptors = 0;
	m_uiDescriptorSize = 0;
}

bool DescriptorHeap::Init(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT uiNumDescriptors)
{
	ID3D12Device* pDevice = App::GetApp()->GetDevice();

	m_uiNumDescriptors = uiNumDescriptors;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_uiNumDescriptors;
	heapDesc.Type = type;
	heapDesc.Flags = flags;
	heapDesc.NodeMask = 0;

	HRESULT hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pDescriptorHeap));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create descriptor heap!");

		return false;
	}

	m_uiDescriptorSize = App::GetApp()->GetDevice()->GetDescriptorHandleIncrementSize(type);

	return true;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeap::GetHeap() const
{
    return m_pDescriptorHeap;
}

bool DescriptorHeap::Allocate(UINT& uiDescriptorIndex)
{
	if (m_uiDescriptorsAllocated >= m_uiNumDescriptors)
	{
		LOG_ERROR(tag, L"Tried to allocate to a descriptor heap but they've all already been allocated");

		return false;
	}

	uiDescriptorIndex = m_uiDescriptorsAllocated;

	++m_uiDescriptorsAllocated;

    return true;
}

bool DescriptorHeap::Allocate()
{
	if (m_uiDescriptorsAllocated >= m_uiNumDescriptors)
	{
		LOG_ERROR(tag, L"Tried to allocate to a descriptor heap but they've all already been allocated");

		return false;
	}

	++m_uiDescriptorsAllocated;

	return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuDescriptorHandle(UINT uiIndex) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), uiIndex, m_uiDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuDescriptorHandle(UINT uiIndex) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), (UINT)uiIndex, m_uiDescriptorSize);
}

UINT DescriptorHeap::GetDescriptorSize() const
{
	return m_uiDescriptorSize;
}

UINT DescriptorHeap::GetNumDescsAllocated() const
{
	return m_uiDescriptorsAllocated;
}

UINT DescriptorHeap::GetMaxNumDescs() const
{
	return m_uiNumDescriptors;
}
