#ifndef __LOGGER__H__
#define __LOGGER__H__
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include <string>
using namespace std;
using namespace spdlog;
namespace raft {
class Logger {
private:
  std::shared_ptr<logger> logger_;

public:
  Logger();
  void Debug(const std::string &);
  void Info(const std::string &);
  void Error(const std::string &);
  void Trace(const std::string &);
};
}
#endif
