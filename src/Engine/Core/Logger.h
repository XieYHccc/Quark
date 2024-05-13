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
    #define UE_CORE_TRACE(...) Logger::GetCoreLogger()->trace(__VA_ARGS__);
    #define UE_CORE_DEBUG(...) Logger::GetCoreLogger()->debug(__VA_ARGS__);
#else
    #define UE_CORE_TRACE(...)
    #define UE_CORE_DEBUG(...)
#endif
#define UE_CORE_INFO(...) Logger::GetCoreLogger()->info(__VA_ARGS__);
#define UE_CORE_WARN(...) Logger::GetCoreLogger()->warn(__VA_ARGS__);
#define UE_CORE_ERROR(...) Logger::GetCoreLogger()->error(__VA_ARGS__);
#define UE_CORE_CRITICAL(...) Logger::GetCoreLogger()->critical(__VA_ARGS__);

// Client logging
#ifndef NDEBUG
    #define UE_CLIENT_TRACE(...) Logger::GetCoreLogger()->trace(__VA_ARGS__);
    #define UE_CLIENT_DEBUG(...) Logger::GetCoreLogger()->debug(__VA_ARGS__);
#else
    #define UE_CLIENT_TRACE(...)
    #define UE_CLIENT_DEBUG(...)
#endif
#define UE_CLINET_INFO(...) Logger::GetClientLogger()->info(__VA_ARGS__);
#define UE_CLINET_WARN(...) Logger::GetClientLogger()->warn(__VA_ARGS__);
#define UE_CLINET_ERROR(...) Logger::GetClientLogger()->error(__VA_ARGS__);
#define UE_CLINET_CRITICAL(...) Logger::GetClientLogger()->critical(__VA_ARGS__);