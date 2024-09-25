#include "Quark/Core/Logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace quark {

std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> Logger::s_ClientLogger;

std::map<std::string, Logger::TagDetails> Logger::s_DefaultTagDetails = {
	{ "AssetManager",      TagDetails{  true, Level::WARN  } },
	{ "Core",              TagDetails{  true, Level::TRACE } },
	{ "GLFW",              TagDetails{  true, Level::ERROR } },
    { "Graphic",           TagDetails{  true, Level::TRACE  } },
	{ "Mesh",              TagDetails{  true, Level::WARN  } },
	{ "Physics",           TagDetails{  true, Level::WARN  } },
	{ "Project",           TagDetails{  true, Level::WARN  } },
	{ "Renderer",          TagDetails{  true, Level::INFO  } },
	{ "Scene",             TagDetails{  true, Level::INFO  } },
};

void Logger::Init()
{
    // Create "logs" directory if doesn't exist
    std::string logsDirectory = "logs";
    if (!std::filesystem::exists(logsDirectory))
        std::filesystem::create_directories(logsDirectory);
    
    std::vector<spdlog::sink_ptr> quarkSinks =
    {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/QUARK.log", true),
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
    };

    std::vector<spdlog::sink_ptr> appSinks =
    {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/APP.log", true),
		std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
    };

    quarkSinks[0]->set_pattern("[%T] [%l] %n: %v");
    appSinks[0]->set_pattern("[%T] [%l] %n: %v");

    quarkSinks[1]->set_pattern("%^[%T] %n: %v%$");
    appSinks[1]->set_pattern("%^[%T] %n: %v%$");

    s_CoreLogger = std::make_shared<spdlog::logger>("QUARK", quarkSinks.begin(), quarkSinks.end());
    s_CoreLogger->set_level(spdlog::level::trace);

    s_ClientLogger = std::make_shared<spdlog::logger>("APP", appSinks.begin(), appSinks.end());
    s_ClientLogger->set_level(spdlog::level::trace);

    s_EnabledTags = s_DefaultTagDetails;

}

void Logger::ShutDown()
{
    s_CoreLogger.reset();
    s_ClientLogger.reset();
    spdlog::drop_all();
}

}