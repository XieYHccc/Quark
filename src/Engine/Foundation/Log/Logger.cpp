#include "Foundation/Log/Logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


std::shared_ptr<spdlog::logger> Logger::m_CoreLogger;
std::shared_ptr<spdlog::logger> Logger::m_ClientLogger;

void Logger::Init()
{
    std::vector<spdlog::sink_ptr> logSinks;
    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks[0]->set_level(spdlog::level::trace);
    logSinks[0]->set_pattern("%^[%T] %n: %v%$");

    logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("bin/XEngine.log", true));
    logSinks[1]->set_level(spdlog::level::trace);
    logSinks[1]->set_pattern("[%T] [%l] %n: %v");

    m_CoreLogger = std::make_shared<spdlog::logger>("ENGINE", logSinks.begin(), logSinks.end());
    m_CoreLogger->set_level(spdlog::level::trace);

    m_ClientLogger = std::make_shared<spdlog::logger>("APP", logSinks.begin(), logSinks.end());
    m_ClientLogger->set_level(spdlog::level::trace);
}