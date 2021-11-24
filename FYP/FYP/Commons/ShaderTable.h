#pragma once

#include "Commons/ShaderRecord.h"

#include <vector>
#include <Include/DirectX/d3dx12.h>
#include <wrl.h>

class ShaderTable
{
public:
	ShaderTable(ID3D12Device* pDevice, UINT uiNumRecords, UINT uiRecordSize);
	~ShaderTable();

	bool AddRecord(const ShaderRecord& record);

	ID3D12Resource* GetBuffer();

	UINT GetRecordSize();

protected:

private:
	char* m_pMappedRecords;
	UINT m_uiRecordsSize;

	std::vector<ShaderRecord> m_Records;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pBuffer;
};

