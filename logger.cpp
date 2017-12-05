#include "log.h"
using namespace spdlog;
namespace raft {
Logger::Logger() {
  logger_ = spdlog::daily_logger_mt("daily_logger", "logs/daily", 0, 0);
}

void Logger::Debug(const std::string &str) { logger_->debug(str); }

void Logger::Info(const std::string &str) { logger_->info(str); }

void Logger::Error(const std::string &str) { logger_->error(str); }

void Logger::Trace(const std::string &str) { SPDLOG_TRACE(logger_, str); }
}
