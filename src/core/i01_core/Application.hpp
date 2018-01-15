#pragma once

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>

#include <i01_core/macro.hpp>
// #include <i01_core/Logger.hpp>
#include <i01_core/PMQOSInterface.hpp>

namespace po = boost::program_options;

namespace i01 { namespace core {

class Application : private boost::noncopyable {
public:
    Application();
    Application(int argc, const char *argv[]);
    virtual ~Application();

    /// If you override init, remember to call Application::init first and
    /// check if it returns false.
    virtual bool init(int argc, const char *argv[]);
    bool load_config_file(const char *file_path);

    po::options_description& options_description()
    { return m_opt_desc; }
    po::positional_options_description& positional_options_description()
    { return m_pos_desc; }
    const po::variables_map& variables_map() const { return m_vm; }

    static void fatal_error(const char * logmsg) __attribute__((noreturn));

    bool verbose() const { return m_verbose; }
    bool daemonized() const { return m_daemonized; }
    bool optimized() const { return m_optimized; }

    virtual int run() { return 0; }

protected:
    int m_argc;
    const char ** m_argv_p;
    std::vector<std::string> m_argv;

    po::options_description m_opt_desc;
    po::positional_options_description m_pos_desc;
    po::variables_map m_vm;

    PMQOSInterface<CPUDMALatency> m_pmqos_cpudmalatency;

    bool m_verbose;
    bool m_daemonized;
    bool m_optimized;

    void daemonize_(bool);
    void optimize_(bool);
};

class NoopApplication : public Application {
public:
    using Application::Application;
    int run() override final { return 0; }
};

} }
