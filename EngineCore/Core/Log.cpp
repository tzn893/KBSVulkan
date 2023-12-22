#include "Log.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

KBS_API kbs::Logger::Logger()
{
	m_stdLogger = spdlog::stdout_color_mt("kbs"); 
	m_stdLogger->set_level(spdlog::level::trace); 
	m_stdLogger->set_pattern("%^[%T] %n: %v%$");

    m_fileLogger = spdlog::basic_logger_mt("kbs-file", "logs/kbs.txt"); 
	m_fileLogger->set_level(spdlog::level::trace);
    m_fileLogger->set_pattern("%^[%T] %n: %v%$");
}

KBS_API void kbs::Logger::flush()
{
	m_fileLogger->flush();
}

std::shared_ptr<kbs::Logger> g_logger;

KBS_API std::shared_ptr<kbs::Logger> kbs::_GetGlobalLogger()
{
	if (g_logger == nullptr)
	{
		g_logger = std::make_shared<kbs::Logger>();
	}
	return g_logger;
}
