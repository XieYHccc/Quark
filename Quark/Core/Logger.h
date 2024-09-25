#pragma once
#include "Quark/Core/Base.h"

#include <map>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

// Disable debug and trace logging for release builds.
#ifdef QK_DEBUG_BUILD
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 0
#else
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

// #undef LOG_TRACE_ENABLED
// #define LOG_TRACE_ENABLED 0

namespace quark {

class Logger 
{
public:
    enum class Type : uint8_t
    {
        CORE = 0, CLIENT = 1
    };
    enum class Level : uint8_t
    {
        TRACE = 0, INFO, WARN, ERROR, FATAL
    };
    struct TagDetails
    {
        bool enabled = true;
        Level levelFilter = Level::TRACE;
    };

    static void Init();
    static void ShutDown();

    template<typename... Args>
    static void PrintMessageTag(Logger::Type type, Logger::Level level, std::string_view tag, std::format_string<Args...> format, Args&&... args);

    static void PrintMessageTag(Logger::Type type, Logger::Level level, std::string_view tag, std::string_view message);

    template<typename... Args>
    static void PrintAssertMessage(Logger::Type type, std::string_view prefix, std::string_view condition, std::string_view inFile, uint32_t inLine, std::format_string<Args...> message, Args&&... args);

    static void PrintAssertMessage(Logger::Type type, std::string_view prefix, std::string_view condition, std::string_view inFile, uint32_t inLine);

    const static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
    const static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

    static bool HasTag(const std::string& tag) { return s_EnabledTags.find(tag) != s_EnabledTags.end(); }
    static std::map<std::string, TagDetails>& EnabledTags() { return s_EnabledTags; }

private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
    static std::shared_ptr<spdlog::logger> s_ClientLogger;

    inline static std::map<std::string, TagDetails> s_EnabledTags;
    static std::map<std::string, TagDetails> s_DefaultTagDetails;
};

}
////////////////////////////////////// OLD API /////////////////////////////////////
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

#define CORE_LOGI(...) ::quark::Logger::GetCoreLogger()->info(__VA_ARGS__);
#define CORE_LOGW(...) ::quark::Logger::GetCoreLogger()->warn(__VA_ARGS__);
#define CORE_LOGE(...) ::quark::Logger::GetCoreLogger()->error(__VA_ARGS__);
#define CORE_LOGC(...) ::quark::Logger::GetCoreLogger()->critical(__VA_ARGS__);

#define LOGI(...) ::quark::Logger::GetClientLogger()->info(__VA_ARGS__);
#define LOGW(...) ::quark::Logger::GetClientLogger()->warn(__VA_ARGS__);
#define LOGE(...) ::quark::Logger::GetClientLogger()->error(__VA_ARGS__);
#define LOGC(...) ::quark::Logger::GetClientLogger()->critical(__VA_ARGS__);

// Core logging
#define QK_CORE_LOGT_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CORE, ::quark::Logger::Level::TRACE, tag, __VA_ARGS__)
#define QK_CORE_LOGI_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CORE, ::quark::Logger::Level::INFO, tag, __VA_ARGS__)
#define QK_CORE_LOGW_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CORE, ::quark::Logger::Level::WARN, tag, __VA_ARGS__)
#define QK_CORE_LOGE_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CORE, ::quark::Logger::Level::ERROR, tag, __VA_ARGS__)
#define QK_CORE_LOGF_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CORE, ::quark::Logger::Level::FATAL, tag, __VA_ARGS__)

// Client logging
#define QK_APP_LOGT_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CLIENT, ::quark::Logger::Level::TRACE, tag, __VA_ARGS__)
#define QK_APP_LOGI_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CLIENT, ::quark::Logger::Level::INFO, tag, __VA_ARGS__)
#define QK_APP_LOGW_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CLIENT, ::quark::Logger::Level::WARN, tag, __VA_ARGS__)
#define QK_APP_LOGE_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CLIENT, ::quark::Logger::Level::ERROR, tag, __VA_ARGS__)
#define QK_APP_LOGF_TAG(tag, ...) ::quark::Logger::PrintMessageTag(::quark::Logger::Type::CLIENT, ::quark::Logger::Level::FATAL, tag, __VA_ARGS__)

namespace quark
{
template<typename... Args>
void Logger::PrintMessageTag(Logger::Type type, Logger::Level level, std::string_view tag, const std::format_string<Args...> format, Args&&... args)
{
    auto detail = s_EnabledTags[std::string(tag)];
    if (detail.enabled && detail.levelFilter <= level)
    {
        auto logger = (type == Type::CORE) ? GetCoreLogger() : GetClientLogger();
        std::string formatted = std::format(format, std::forward<Args>(args)...);
        switch (level)
        {
        case Level::TRACE:
            logger->trace("[{0}] {1}", tag, formatted);
            break;
        case Level::INFO:
            logger->info("[{0}] {1}", tag, formatted);
            break;
        case Level::WARN:
            logger->warn("[{0}] {1}", tag, formatted);
            break;
        case Level::ERROR:
            logger->error("[{0}] {1}", tag, formatted);
            break;
        case Level::FATAL:
            logger->critical("[{0}] {1}", tag, formatted);
            break;
        }
    }
}

inline void Logger::PrintMessageTag(Logger::Type type, Logger::Level level, std::string_view tag, std::string_view message)
{
    auto detail = s_EnabledTags[std::string(tag)];
    if (detail.enabled && detail.levelFilter <= level)
    {
        auto logger = (type == Type::CORE) ? GetCoreLogger() : GetClientLogger();
        switch (level)
        {
        case Level::TRACE:
            logger->trace("[{0}] {1}", tag, message);
            break;
        case Level::INFO:
            logger->info("[{0}] {1}", tag, message);
            break;
        case Level::WARN:
            logger->warn("[{0}] {1}", tag, message);
            break;
        case Level::ERROR:
            logger->error("[{0}] {1}", tag, message);
            break;
        case Level::FATAL:
            logger->critical("[{0}] {1}", tag, message);
            break;
        }
    }
}

template<typename... Args>
void Logger::PrintAssertMessage(Logger::Type type, std::string_view prefix, std::string_view condition, std::string_view inFile, uint32_t inLine, std::format_string<Args...> message, Args&&... args)
{
    auto logger = (type == Type::CORE) ? GetCoreLogger() : GetClientLogger();
    auto formatted = std::format(message, std::forward<Args>(args)...);
    logger->error("{0}: {1}, message: {2}, in file {3}, in line {4}", prefix, condition, formatted, inFile, inLine);
}


inline void Logger::PrintAssertMessage(Logger::Type type, std::string_view prefix, std::string_view condition, std::string_view inFile, uint32_t inLine)
{
    auto logger = (type == Type::CORE) ? GetCoreLogger() : GetClientLogger();
    logger->error("{0}: {1}, in file {2}, in line {3}", prefix, condition, inFile, inLine);
}

}
