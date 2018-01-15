#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <dlfcn.h>

#include <iostream>
#include <sstream>

#include <boost/filesystem.hpp>

#include <i01_core/Version.hpp>
#include <i01_core/Config.hpp>
#include <i01_core/Log.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/FD.hpp>
#include <i01_core/NamedThread.hpp>
#include <i01_core/Application.hpp>
#include <i01_core/Date.hpp>

#include <systemd/sd-daemon.h>

#include "logger.hpp"

using i01::core::Log;
using i01::core::Application;
using i01::core::Config;
using i01::core::Timestamp;
using i01::core::NamedThreadBase;
using i01::core::NamedThread;

namespace i01 { namespace apps { namespace logger {

LoggerApp * LoggerApp::s_logger_app_p = nullptr;

LoggerApp::LoggerApp()
    : Application()
    , m_logger_name()
    , m_log(Log::instance())
{
    if (s_logger_app_p)
        throw std::runtime_error("Error: LoggerApp constructed twice.");

    s_logger_app_p = this;
    const char * default_name = nullptr;
    struct utsname uname_result;
    if (0 == ::uname(&uname_result))
        default_name = uname_result.nodename;

    int fd = -1;
    int n = ::sd_listen_fds(0);
    if (n > 1) {
            fprintf(stderr, "Too many file descriptors received.\n");
            exit(1);
    } else if (n == 1) {
            fd = SD_LISTEN_FDS_START + 0;
            if (0 >= ::sd_is_socket(fd, AF_UNIX, SOCK_STREAM, -1)) {
                fprintf(stderr, "File descriptor %d is not AF_UNIX and SOCK_STREAM.\n", fd);
                exit(1);
            }
    } else {
            union {
                    struct sockaddr sa;
                    struct sockaddr_un un;
            } sa;

            fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) {
                    fprintf(stderr, "socket(): %m\n");
                    exit(1);
            }

            ::memset(&sa, 0, sizeof(sa));
            sa.un.sun_family = AF_UNIX;
            ::strncpy(sa.un.sun_path, "/run/i01logger.sock", sizeof(sa.un.sun_path));

            if (::bind(fd, &sa.sa, sizeof(sa)) < 0) {
                    fprintf(stderr, "bind(): %m\n");
                    exit(1);
            }

            if (::listen(fd, SOMAXCONN) < 0) {
                    fprintf(stderr, "listen(): %m\n");
                    exit(1);
            }
    }

    po::options_description logger_desc("Logger options");
    logger_desc.add_options()
        ("logger.name"
          , po::value<std::string>()
                ->default_value(default_name == nullptr ? "" : default_name)
          , "Name of the logger.")
        ("logger.working-dir"
          , po::value<std::string>()
                ->default_value(boost::filesystem::current_path().string())
          , "Logger working directory.")
        ;
    m_opt_desc.add(logger_desc);
}

LoggerApp::LoggerApp(int argc, const char *argv[])
    : LoggerApp()
{ if (!init(argc, argv)) std::exit(EXIT_FAILURE); }

bool LoggerApp::init(int argc, const char *argv[]) {
    if (!Application::init(argc, argv))
        return false;

    auto cfg = Config::instance().get_shared_state();

    cfg->get("logger.name", m_logger_name);
    if (m_logger_name.empty()) {
        std::cerr << "Error: logger.name is not configured." << std::endl;
        return false;
    }

    if (auto working_dir = cfg->get<std::string>("logger.working-dir")) {
        try {
            boost::filesystem::current_path(boost::filesystem::path(*working_dir));
        } catch (boost::filesystem::filesystem_error& e) {
            std::cerr << "Error: Could not chdir into logger.working-dir: " << e.what() << std::endl;
            return false;
        }
    }

    return true;
}

LoggerApp::~LoggerApp() {
    log().console()->notice() << "Attempting graceful shutdown of logger" << m_logger_name << ".";
}

int LoggerApp::run()
{
    return EXIT_SUCCESS;
    return EXIT_FAILURE; // should never reach here...
}

} } }

int main(int argc, const char *argv[])
{
    i01::apps::logger::LoggerApp app(argc, argv);
    return app.run();
}
