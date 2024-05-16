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
#ifdef QK_DEBUG_BUILD
    #define CORE_LOGD(...) Logger::GetCoreLogger()->debug(__VA_ARGS__);
    #define CORE_LOGT(...) Logger::GetCoreLogger()->trace(__VA_ARGS__);
#else
    #define CORE_LOGD(...)
    #define CORE_LOGT(...)
#endif
#define CORE_LOGI(...) Logger::GetCoreLogger()->info(__VA_ARGS__);
#define CORE_LOGW(...) Logger::GetCoreLogger()->warn(__VA_ARGS__);
#define CORE_LOGE(...) Logger::GetCoreLogger()->error(__VA_ARGS__);
#define CORE_LOGC(...) Logger::GetCoreLogger()->critical(__VA_ARGS__);

// Client logging
#ifdef QK_DEBUG_BUILD
    #define LOGT(...) Logger::GetClientLogger()->trace(__VA_ARGS__);
    #define LOGD(...) Logger::GetClientLogger()->debug(__VA_ARGS__);
#else
    #define LOGT(...)
    #define LOGD(...)
#endif

#define LOGI(...) Logger::GetClientLogger()->info(__VA_ARGS__);
#define LOGW(...) Logger::GetClientLogger()->warn(__VA_ARGS__);
#define LOGE(...) Logger::GetClientLogger()->error(__VA_ARGS__);
#define LOGC(...) Logger::GetClientLogger()->critical(__VA_ARGS__);