#pragma once
class Timer
{
public:
	Timer();

	float GameTime() const; //In seconds
	float DeltaTime() const; //In seconds

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double m_dSecondsPerCount;
	double m_dDetlaTime;

	__int64 m_iBaseTime;
	__int64 m_iPausedTime;
	__int64 m_iStopTime;
	__int64 m_iPreviousTime;
	__int64 m_iCurrentTime;

	bool m_bPaused;
};

