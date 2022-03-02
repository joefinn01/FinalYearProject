#include "ScopedTimer.h"
#include "Helpers/DebugHelper.h"

ScopedTimer::ScopedTimer(std::string sName) : Timer()
{
	m_sName = sName;

	Start();

	Tick();
}

ScopedTimer::~ScopedTimer()
{
	Tick();

	DebugHelper::AddTime(this);
}

std::string ScopedTimer::GetName()
{
	return m_sName;
}
