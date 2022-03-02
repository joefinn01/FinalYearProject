#pragma once
#include "Timer.h"

#include <string>

class ScopedTimer : public Timer
{
public:
	ScopedTimer(std::string sName);

	~ScopedTimer();

	std::string GetName();

protected:

private:
	std::string m_sName = "";
};

