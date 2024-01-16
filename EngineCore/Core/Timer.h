#pragma once
#include <chrono>


namespace kbs
{
	class Timer
	{
	public:
		Timer();

		float TotalTime();
		float DeltaTime();

		void  Tick();
	private:
		uint64_t m_StartTick = 0;
		uint64_t m_LastTick = 0;
		uint64_t m_CurrentTick = 0;
	};

}