#include "Timer.h"
#include <Windows.h>

Timer::Timer()
{
	//Init variables
	m_dSecondsPerCount = 0.0;
	m_dDetlaTime = -1.0;
	m_iBaseTime = 0;
	m_iCurrentTime = 0;
	m_iPreviousTime = 0;
	m_iPausedTime = 0;
	m_iStopTime = 0;
	m_bPaused = false;

	//Cache the number of seconds per count
	__int64 iCountsPerSecond = 0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&iCountsPerSecond);
	m_dSecondsPerCount = 1.0 / (double)iCountsPerSecond;
}

float Timer::GameTime() const
{
	if (m_bPaused == true)
	{
		return (float)(((m_iStopTime - m_iPausedTime) - m_iBaseTime) * m_dSecondsPerCount);
	}
	else
	{
		return (float)(((m_iCurrentTime - m_iPausedTime) - m_iBaseTime) * m_dSecondsPerCount);
	}

	return 0.0f;
}

float Timer::DeltaTime() const
{
	return (float)m_dDetlaTime;
}

void Timer::Reset()
{
	__int64 iCurrentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&iCurrentTime);

	m_iBaseTime = iCurrentTime;
	m_iPreviousTime = iCurrentTime;
	m_iStopTime = 0;
	m_bPaused = false;
}

void Timer::Start()
{
	if (m_bPaused == true)
	{
		__int64 iStartTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&iStartTime);

		m_iPausedTime += (iStartTime - m_iStopTime);
		m_iPreviousTime = iStartTime;
		m_iStopTime = 0;
		m_bPaused = false;
	}
}

void Timer::Stop()
{
	if (m_bPaused == false)
	{
		__int64 iCurrentTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&iCurrentTime);

		m_iStopTime = iCurrentTime;
		m_bPaused = true;
	}
}

void Timer::Tick()
{
	//If paused then delta time is zero
	if (m_bPaused)
	{
		m_dDetlaTime = 0.0;

		return;
	}

	//Get current time
	QueryPerformanceCounter((LARGE_INTEGER*)&m_iCurrentTime);

	//Calculate delta time
	m_dDetlaTime = (m_iCurrentTime - m_iPreviousTime) * m_dSecondsPerCount;

	//Update previous time
	m_iPreviousTime = m_iCurrentTime;

	//Ensure delta time can'tbe negative
	if (m_dDetlaTime < 0.0)
	{
		m_dDetlaTime = 0.0;
	}
}
