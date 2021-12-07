#pragma once

#include "Helpers/DebugHelper.h"
#include "Helpers/MathHelper.h"

#include <Include/DirectX/d3dx12.h>

template<class T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* pDevice, UINT uiCount, bool bIsConstant)
	{
		m_bIsConstant = bIsConstant;
		m_uiByteSize = sizeof(T);

		//Ensuring always a multiple of 255 by adding 255 and masking bits
		if (bIsConstant)
		{
			m_uiByteSize = MathHelper::CalculatePaddedConstantBufferSize(m_uiByteSize);
		}

		HRESULT hr = pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
																			D3D12_HEAP_FLAG_NONE,
																			&CD3DX12_RESOURCE_DESC::Buffer(m_uiByteSize * uiCount),
																			D3D12_RESOURCE_STATE_GENERIC_READ,
																			nullptr,
																			IID_PPV_ARGS(m_pUploadBuffer.GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to create committed resource in upload buffer!");

			return;
		}

		hr = m_pUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedData));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to map the upload buffer!");

			return;
		}
	}

	UploadBuffer(ID3D12Device* pDevice, UINT uiCount, bool bIsConstant, UINT uiByteSize)
	{
		m_bIsConstant = bIsConstant;
		m_uiByteSize = uiByteSize;

		//Ensuring always a multiple of 255 by adding 255 and masking bits
		if (bIsConstant)
		{
			m_uiByteSize = MathHelper::CalculatePaddedConstantBufferSize(m_uiByteSize);
		}

		HRESULT hr = pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
																			D3D12_HEAP_FLAG_NONE,
																			&CD3DX12_RESOURCE_DESC::Buffer(m_uiByteSize * uiCount),
																			D3D12_RESOURCE_STATE_GENERIC_READ,
																			nullptr,
																			IID_PPV_ARGS(m_pUploadBuffer.GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to create committed resource in upload buffer!");

			return;
		}

		hr = m_pUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedData));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to map the upload buffer!");

			return;
		}
	}

	~UploadBuffer()
	{
		if (m_pUploadBuffer != nullptr)
		{
			m_pUploadBuffer->Unmap(0, nullptr);
		}

		m_pMappedData = nullptr;
	}

	UploadBuffer(const UploadBuffer& rhs) = default;
	UploadBuffer& operator=(const UploadBuffer& rhs) = default;

	ID3D12Resource* Get() const
	{
		return m_pUploadBuffer.Get();
	}

	void CopyData(int iIndex, const T& data)
	{
		memcpy(&m_pMappedData[iIndex * m_uiByteSize], &data, sizeof(T));
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUAddress(UINT uiCount)
	{
		return m_pUploadBuffer.Get()->GetGPUVirtualAddress() + (uiCount * m_uiByteSize);
	}

protected:

private:
	bool m_bIsConstant;

	UINT m_uiByteSize = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pUploadBuffer;

	BYTE* m_pMappedData = nullptr;

	Tag tag = L"UploadBuffer";
};