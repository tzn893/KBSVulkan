#pragma once
#include "Platform/Platform.h"
#include "spdlog/spdlog.h"
#include <memory>

namespace kbs
{
	class KBS_API Logger
	{
	public:

		Logger();

		template<typename ...Args>
		void info(const char* msg, Args... args)
		{
			m_stdLogger->info(msg, args...);
			m_fileLogger->info(msg, args...);
		}

		template<typename ...Args>
		void warn(const char* msg, Args... args)
		{
			m_stdLogger->warn(msg, args...);
			m_fileLogger->warn(msg, args...);
		}

		template<typename ...Args>
		void error(const char* msg, Args... args)
		{
			m_stdLogger->error(msg, args...);
			m_fileLogger->error(msg, args...);
		}

		void flush();


	private:
		std::shared_ptr<spdlog::logger> m_stdLogger;
		std::shared_ptr<spdlog::logger> m_fileLogger;
	};

    KBS_API std::shared_ptr<Logger> _GetGlobalLogger();
}

#define KBS_LOG(msg, ...) ::kbs::_GetGlobalLogger()->info(msg, __VA_ARGS__)
#define KBS_WARN(msg, ...) ::kbs::_GetGlobalLogger()->warn("warning from file {} line {}:"##msg,__FILE__,__LINE__ ,__VA_ARGS__)
#define KBS_ASSERT(expr, msg, ...) if(!(expr)) {::kbs::_GetGlobalLogger()->error("assertion failure from file {} line {}:"##msg##" application quiting",__FILE__,__LINE__, __VA_ARGS__); KBS_BREAK_POINT;_GetGlobalLogger()->flush(); exit(-1); }
#define KBS_FLUSH_LOG() ::kbs::_GetGlobalLogger()->flush();

