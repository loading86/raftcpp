#include "log.h"
using namespace spdlog;

Logger::Logger()
{
	m_logger = spd::daily_logger_mt("daily_logger", "logs/daily", 0, 0);
}

void Logger::Debug(const std::string& str)
{
	m_logger.debug(str);
}

void Logger::Info(const std::string& str)
{
        m_logger.info(str);
}

void Logger::Error(const std::string& str)
{
        m_logger.error(str);
}

