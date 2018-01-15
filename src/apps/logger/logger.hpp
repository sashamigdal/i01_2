#pragma once

#include <string>

#include <i01_core/Log.hpp>
#include <i01_core/Application.hpp>

namespace i01 { namespace apps { namespace logger {

class LoggerApp : public i01::core::Application {
    private:
        /// \internal Logger name.
        std::string m_logger_name;

        /// \internal Global log reference.
        i01::core::Log& m_log;

        /// \internal Static pointer to EngineApp required for i01_register_*_from_plugin.
        static LoggerApp * s_logger_app_p;

    public:
        LoggerApp();
        LoggerApp(int argc, const char *argv[]);
        bool init(int argc, const char *argv[]) override final;
        virtual ~LoggerApp();
        /// Global engine log, use this to write log messages to the console.
        inline i01::core::Log& log() { return m_log; }
        int run() override final;
};

} } }
