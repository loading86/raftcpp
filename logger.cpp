#include "log.h"
using namespace spdlog;
namespace raft
{
Logger::Logger()
{
	m_logger = spdlog::daily_logger_mt("daily_logger", "logs/daily", 0, 0);
}

void Logger::Debug(const std::string& str)
{
	m_logger->debug(str);
}

void Logger::Info(const std::string& str)
{
        m_logger->info(str);
}

void Logger::Error(const std::string& str)
{
        m_logger->error(str);
}

void Logger::Trace(const std::string& str)
{
	SPDLOG_TRACE(m_logger, str);
}
}

