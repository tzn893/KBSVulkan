#include "Timer.h"
using namespace std::chrono;

namespace kbs
{
	Timer::Timer()
	{
		m_StartTick = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		m_LastTick = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		m_CurrentTick = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}

	float kbs::Timer::TotalTime()
	{
		return (float)(m_CurrentTick - m_StartTick) / 1000.f;
	}

	float Timer::DeltaTime()
	{
		return (float)(m_CurrentTick - m_LastTick) / 1000.f;
	}

	void Timer::Tick()
	{
		m_LastTick = m_CurrentTick;
		m_CurrentTick = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}
}
