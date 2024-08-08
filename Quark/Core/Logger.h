#pragma once
#include "Quark/Core/Base.h"

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

namespace quark {
#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1

// Disable debug and trace logging for release builds.
#ifdef QK_DEBUG_BUILD
    #define LOG_DEBUG_ENABLED 1
    #define LOG_TRACE_ENABLED 1
#else
    #define LOG_DEBUG_ENABLED 0
    #define LOG_TRACE_ENABLED 0
#endif

// #undef LOG_TRACE_ENABLED
// #define LOG_TRACE_ENABLED 0

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
#if LOG_DEBUG_ENABLED == 1
    #define CORE_LOGD(...) Logger::GetCoreLogger()->debug(__VA_ARGS__);
    #define LOGD(...) Logger::GetClientLogger()->debug(__VA_ARGS__);
#else
    #define CORE_LOGD(...)
    #define LOGD(...)
#endif

#if LOG_TRACE_ENABLED == 1
    #define CORE_LOGT(...) Logger::GetCoreLogger()->trace(__VA_ARGS__);
    #define LOGT(...) Logger::GetClientLogger()->trace(__VA_ARGS__);
#else
    #define CORE_LOGT(...)
    #define LOGT(...)
#endif

#define CORE_LOGI(...) Logger::GetCoreLogger()->info(__VA_ARGS__);
#define CORE_LOGW(...) Logger::GetCoreLogger()->warn(__VA_ARGS__);
#define CORE_LOGE(...) Logger::GetCoreLogger()->error(__VA_ARGS__);
#define CORE_LOGC(...) Logger::GetCoreLogger()->critical(__VA_ARGS__);

#define LOGI(...) Logger::GetClientLogger()->info(__VA_ARGS__);
#define LOGW(...) Logger::GetClientLogger()->warn(__VA_ARGS__);
#define LOGE(...) Logger::GetClientLogger()->error(__VA_ARGS__);
#define LOGC(...) Logger::GetClientLogger()->critical(__VA_ARGS__);

}