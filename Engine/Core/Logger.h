#pragma once

#include <memory>

#include <spdlog/spdlog.h>

// Disable debug and trace logging for release builds.
#ifdef NDEBUG
    #define LOG_DEBUG_ENABLED 0
    #define LOG_TRACE_ENABLED 0
#else
    #define LOG_DEBUG_ENABLED 1
    #define LOG_TRACE_ENABLED 1
#endif
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
#ifndef NDEBUG
    #define CORE_LOG_DEBUG(...) Logger::GetCoreLogger()->debug(__VA_ARGS__);
#else
    #define CORE_LOG_DEBUG(...)
#endif
#define CORE_LOG_INFO(...) Logger::GetCoreLogger()->info(__VA_ARGS__);
#define CORE_LOG_WARN(...) Logger::GetCoreLogger()->warn(__VA_ARGS__);
#define CORE_LOG_ERROR(...) Logger::GetCoreLogger()->error(__VA_ARGS__);
#define CORE_LOG_CRITICAL(...) Logger::GetCoreLogger()->critical(__VA_ARGS__);

// Client logging
#ifndef NDEBUG
    #define CLIENT_LOG_TRACE(...) Logger::GetCoreLogger()->trace(__VA_ARGS__);
    #define CLIENT_LOG_DEBUG(...) Logger::GetCoreLogger()->debug(__VA_ARGS__);
#else
    #define CLIENT_LOG_TRACE(...)
    #define CLIENT_LOG_DEBUG(...)
#endif

#define CLINET_LOG_INFO(...) Logger::GetClientLogger()->info(__VA_ARGS__);
#define CLINET_LOG_WARN(...) Logger::GetClientLogger()->warn(__VA_ARGS__);
#define CLINET_LOG_ERROR(...) Logger::GetClientLogger()->error(__VA_ARGS__);
#define CLINET_LOG_CRITICAL(...) Logger::GetClientLogger()->critical(__VA_ARGS__);