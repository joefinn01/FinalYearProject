#include "Descriptor.h"

Descriptor::Descriptor(UINT uiDescriptorIndex)
{
	m_uiDescriptorIndex = uiDescriptorIndex;
}

UINT Descriptor::GetDescriptorIndex() const
{
	return m_uiDescriptorIndex;
}
