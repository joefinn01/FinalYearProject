#pragma once

#include <Windows.h>

class ShaderRecord
{
public:
	ShaderRecord()
	{
		m_pLocalRootArguments = nullptr;
		m_uiRootArgumentsSize = 0;

		m_pShaderIdentifier = nullptr;
		m_uiIdentifierSize = 0;
	}

	ShaderRecord(void* pLocalRootArgs, UINT uiArgsSize, void* pShaderIdentifier, UINT uiIdentifierSize)
	{
		m_pLocalRootArguments = pLocalRootArgs;
		m_uiRootArgumentsSize = uiArgsSize;

		m_pShaderIdentifier = pShaderIdentifier;
		m_uiIdentifierSize = uiIdentifierSize;
	}

	void Copy(void* pDestination) const
	{
		char* pCharDestination = (char*)pDestination;

		memcpy(pCharDestination, m_pShaderIdentifier, m_uiIdentifierSize);

		if (m_pLocalRootArguments != nullptr)
		{
			//Offset to the end of the identifier
			memcpy(pCharDestination + m_uiIdentifierSize, m_pLocalRootArguments, m_uiRootArgumentsSize);
		}
	}

protected:

private:
	void* m_pShaderIdentifier;
	UINT m_uiIdentifierSize;

	void* m_pLocalRootArguments;
	UINT m_uiRootArgumentsSize;
};

