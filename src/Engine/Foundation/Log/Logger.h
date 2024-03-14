#pragma once

#include <memory>

#include <spdlog/spdlog.h>

class Logger {
public:
    static void Init();

    const static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return m_CoreLogger; }
    const static std::shared_ptr<spdlog::logger>& GetClientLogger() { return m_ClientLogger; }

private:
    static std::shared_ptr<spdlog::logger> m_CoreLogger;
    static std::shared_ptr<spdlog::logger> m_ClientLogger;
};


// Engine logging
#define XE_CORE_TRACE(...) Logger::GetCoreLogger()->trace(__VA_ARGS__);
#define XE_CORE_INFO(...) Logger::GetCoreLogger()->info(__VA_ARGS__);
#define XE_CORE_WARN(...) Logger::GetCoreLogger()->warn(__VA_ARGS__);
#define XE_CORE_ERROR(...) Logger::GetCoreLogger()->error(__VA_ARGS__);
#define XE_CORE_CRITICAL(...) Logger::GetCoreLogger()->critical(__VA_ARGS__);

// Client logging
#define XE_CLINET_INFO(...) Logger::GetClientLogger()->info(__VA_ARGS__);
#define XE_CLINET_WARN(...) Logger::GetClientLogger()->warn(__VA_ARGS__);
#define XE_CLINET_ERROR(...) Logger::GetClientLogger()->error(__VA_ARGS__);
#define XE_CLINET_CRITICAL(...) Logger::GetClientLogger()->critical(__VA_ARGS__);