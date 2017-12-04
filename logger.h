#include <string>
#include "spdlog/spdlog.h"
#include "spdlog/logger.h"
using namespace std;
using namespace spdlog;
class Logger
{
    private:
        std::shared_ptr<logger> m_logger;
    public:
        void Debug(const std::string&);
        void Info(const std::string&);
	void Error(const std::string&);
	void Trace(const std::string&)
};
