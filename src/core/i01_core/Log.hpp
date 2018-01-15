#pragma once

#if defined(_ICC) || defined(__INTEL_COMPILER)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include "spdlog/include/spdlog/spdlog.h"

#if defined(_ICC) || defined(__INTEL_COMPILER)
#else
#pragma GCC diagnostic pop
#endif

#ifdef I01_DEBUG_MESSAGING
#   ifndef SPDLOG_TRACE_ON
#       define SPDLOG_TRACE_ON 1
#   endif
#   ifndef SPDLOG_DEBUG_ON
#       define SPDLOG_DEBUG_ON 1
#   endif
#endif

// WARNING: I01_LOG_TRACE and I01_LOG_DEBUG arguments are NOT EVALUATED if
// I01_DEBUG_MESSAGING is disabled.
#define I01_LOG_TRACE(...)    SPDLOG_TRACE(Log::instance().console(), __VA_ARGS__)
#define I01_LOG_DEBUG(...)    SPDLOG_DEBUG(Log::instance().console(), __VA_ARGS__)
// ...but this is not the case for I01_LOG_everythingelse...
#define I01_LOG_INFO(...)     Log::instance().console()->info(__VA_ARGS__)
#define I01_LOG_NOTICE(...)   Log::instance().console()->notice(__VA_ARGS__)
#define I01_LOG_WARN(...)     Log::instance().console()->warn(__VA_ARGS__)
#define I01_LOG_ERROR(...)    Log::instance().console()->error(__VA_ARGS__)
#define I01_LOG_CRITICAL(...) Log::instance().console()->critical(__VA_ARGS__)

#include <i01_core/Config.hpp>
#include <i01_core/Singleton.hpp>

namespace i01 { namespace core {

class Log : public Singleton<Log> {
public:
    Log() : m_console_sp(spdlog::stderr_logger_mt("console")) {
        m_console_sp->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%l] [%n] [%t] %v");
        if (Config::instance().get_shared_state()->get<bool>("verbose")) {
#ifndef NDEBUG
            m_console_sp->set_level(spdlog::level::debug);
#else
            m_console_sp->set_level(spdlog::level::info);
#endif
        } else {
            m_console_sp->set_level(spdlog::level::notice);
        }
    }

    std::shared_ptr<spdlog::logger> console() { return m_console_sp; }

private:
    std::shared_ptr<spdlog::logger> m_console_sp;
};

} }
