#include "ShaderTable.h"

#include "Helpers/MathHelper.h"
#include "Helpers/DXRHelper.h"
#include "Helpers/DebugHelper.h"

Tag tag = L"ShaderTable";

ShaderTable::ShaderTable(ID3D12Device* pDevice, UINT uiNumRecords, UINT uiRecordSize)
{
	m_uiRecordsSize = MathHelper::CalculateAlignedSize(uiRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	m_Records.reserve(uiNumRecords);

	HRESULT hr = pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
													D3D12_HEAP_FLAG_NONE,
													&CD3DX12_RESOURCE_DESC::Buffer(m_uiRecordsSize * uiNumRecords),
													D3D12_RESOURCE_STATE_GENERIC_READ,
													nullptr,
													IID_PPV_ARGS(&m_pBuffer));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the buffer for the shader table!");

		return;
	}

	CD3DX12_RANGE readRange(0, 0);

	hr = m_pBuffer->Map(0, &readRange, (void**)&m_pMappedRecords);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to map the shader table buffer!");

		return;
	}
}

ShaderTable::~ShaderTable()
{
	m_pBuffer->Unmap(0, nullptr);
}

bool ShaderTable::AddRecord(const ShaderRecord& record)
{
	//Check if theres space for the record
	if (m_Records.size() >= m_Records.capacity())
	{
		LOG_ERROR(tag, L"Tried to add a shader record to the shader table but there isn't enough space!");

		return false;
	}

	//Copy data into buffer and increment pointer so ready for next record
	record.Copy((void*)m_pMappedRecords);

	m_pMappedRecords += m_uiRecordsSize;

	return true;
}

ID3D12Resource* ShaderTable::GetBuffer()
{
	return m_pBuffer.Get();
}

UINT ShaderTable::GetRecordSize()
{
	return m_uiRecordsSize;
}
