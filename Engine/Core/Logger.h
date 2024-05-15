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
    #define CORE_LOGD(...) Logger::GetCoreLogger()->debug(__VA_ARGS__);
#else
    #define CORE_LOGD(...)
#endif
#define CORE_LOGI(...) Logger::GetCoreLogger()->info(__VA_ARGS__);
#define CORE_LOGW(...) Logger::GetCoreLogger()->warn(__VA_ARGS__);
#define CORE_LOGE(...) Logger::GetCoreLogger()->error(__VA_ARGS__);
#define CORE_LOGC(...) Logger::GetCoreLogger()->critical(__VA_ARGS__);

// Client logging
#ifndef NDEBUG
    #define CLIENT_LOGT(...) Logger::GetCoreLogger()->trace(__VA_ARGS__);
    #define CLIENT_LOGD(...) Logger::GetCoreLogger()->debug(__VA_ARGS__);
#else
    #define CLIENT_LOGT(...)
    #define CLIENT_LOGD(...)
#endif

#define CLINET_LOGI(...) Logger::GetClientLogger()->info(__VA_ARGS__);
#define CLINET_LOGW(...) Logger::GetClientLogger()->warn(__VA_ARGS__);
#define CLINET_LOGE(...) Logger::GetClientLogger()->error(__VA_ARGS__);
#define CLINET_LOGC(...) Logger::GetClientLogger()->critical(__VA_ARGS__);