#pragma once

#include "Commons/UploadBuffer.h"

struct AccelerationBuffers
{
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScratch;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pResult;
	UploadBuffer<D3D12_RAYTRACING_INSTANCE_DESC>* m_pInstanceDesc;
};