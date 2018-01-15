#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <unistd.h>
#include <sched.h>
#include <memory.h>
#include <malloc.h>
#include <sys/mman.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <i01_core/Version.hpp>
#include <i01_core/Application.hpp>
#include <i01_core/Config.hpp>
#include <i01_core/Log.hpp>

namespace po = boost::program_options;

namespace i01 { namespace core {

Application::Application()
    : m_argc(0)
    , m_argv_p(nullptr)
    , m_argv()
    , m_opt_desc()
    , m_pos_desc()
    , m_vm()
    , m_pmqos_cpudmalatency()
    , m_verbose(false)
    , m_daemonized(false)
    , m_optimized(false)
{
    m_opt_desc.add_options()
        ("help,h", "Display this help message")
        ("version", "Display version information")
        ("verbose"
          , "Enable verbose logging and output")
        ("config-file"
          , po::value<std::vector<std::string> >()
          , "boost::program_options configuration file (may be specified multiple times)")
        ("lua-file"
          , po::value<std::vector<std::string> >()
          , "Lua configuration file (may be specified multiple times)")
        ("conf"
         , po::value<std::vector<std::string> >()
         , "key=value format configuration (may be specified multiple times)")
        ("daemonize"
          , "Run in background (as a daemon)")
        ("optimize", "Enable latency and performance optimizations")
        ("ignore-environ", "Ignore environment variables during initial configuration")
        ;
    po::options_description perf_desc("Performance optimizations");
    perf_desc.add_options()
        ("reserve-memory"
          , po::value<int>()
                ->default_value(1024)->implicit_value(1024)
          , "Amount of memory to prefault/reserve, in megabytes.")
        ("scheduler-priority"
          , po::value<int>()
                ->default_value(51)->implicit_value(51)
          , "Scheduler RT priority to use: 0 - 99.")
        ("scheduler-policy"
          , po::value<std::string>()
                ->default_value("fifo")
          , "Scheduler policy to use: fifo, rr, idle, batch, other.")
        ;
    m_opt_desc.add(perf_desc);
}

Application::Application(int argc, const char *argv[])
    : Application()
{
    if (!init(argc, argv)) {
        std::cerr << "Initialization failed." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

bool Application::init(int argc, const char *argv[])
{
    m_argc = argc;
    m_argv_p = argv;
    m_argv = std::vector<std::string>(argv, argv + argc);

    m_vm.clear();

    try {
        // Parse environment options:
        po::store(po::parse_environment(m_opt_desc, "I01_"), m_vm);
        // Parse command line options:
        po::store(po::command_line_parser(argc, argv)
                .options(m_opt_desc).positional(m_pos_desc).run()
            , m_vm);

        if (m_vm.count("version"))
        {
            std::cout << "IDENT: " << i01::g_IDENT << std::endl;
            std::cout << "COMMIT: " << i01::g_COMMIT << std::endl;
            std::cout << "VERSION: " << i01::g_VERSION << std::endl;
            std::cout << "BUILD_VERSION: " << i01::g_BUILD_VERSION << std::endl;
            std::cout << "INTEGER_VERSION: " << i01::g_INTEGER_VERSION << std::endl;
            std::exit(EXIT_SUCCESS);
        }

        if (m_vm.count("help"))
        {
            std::cout << "Usage: " << m_argv[0] << " [options]" << std::endl
                      << "Version: " << i01::g_VERSION
                      << " (" << i01::g_BUILD_VERSION << ")" << std::endl
                      << m_opt_desc << std::endl;
            std::exit(EXIT_SUCCESS);
        }

        // Parse config file options, if provided:
        if (m_vm.count("config-file"))
        {
            for (std::string f : m_vm["config-file"].as< std::vector<std::string> >())
            {
                if(!load_config_file(f.c_str())) {
                    std::cerr << "Application: failed to load_config_file("
                              << f << ")." << std::endl;
                }
            }
        }

        if (!m_vm.count("ignore-environ"))
            Config::instance().load_environ();

        if (m_vm.count("conf")) {
            for (auto s : m_vm["conf"].as<std::vector<std::string> >()) {
                // look for whitespace
                std::string ws = " \t\f\v\n\r";
                auto p = s.find_last_not_of(ws);
                if (p != s.size()-1) {
                    throw std::runtime_error("--conf argument '" + s + "' contains whitespace");
                }

                // should be of the form <key>=<value>
                boost::char_separator<char> sep(" \t\f\v\n\r=");
                boost::tokenizer<boost::char_separator<char>> toks(s, sep);
                int num = 0;
                for (auto t : toks) {
                    num++;
                }
                if (num != 2) {
                    throw std::runtime_error("--conf argument '" + s + " has extraneous whitespace or =");
                }
                // auto it = toks.begin();
                auto key = *toks.begin();
                auto val = *++toks.begin();
                Config::instance().load_strings({{key, val}});
            }
            m_vm.erase("conf");
        }

        Config::instance().load_variables_map(m_vm);

        if (m_vm.count("lua-file"))
        {
            for (std::string f : m_vm["lua-file"].as<std::vector<std::string> >())
            {
                Config::instance().load_lua_file(f);
            }
        }


    } catch(po::error& e) {
        std::cerr << "Error parsing program options: " << e.what() << std::endl;
        std::cerr << m_opt_desc << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (Config::instance().get_shared_state()->get<bool>("daemonize").get_value_or(false))
        daemonize_(true);

    if (Config::instance().get_shared_state()->get<bool>("optimize").get_value_or(false))
        optimize_(true);

    if (Config::instance().get_shared_state()->get<bool>("verbose").get_value_or(false))
        m_verbose = true;

    ::openlog( ::basename(argv[0])
             , LOG_PID | (verbose() ? LOG_PERROR : 0)
             , LOG_USER);

    ::syslog( LOG_NOTICE
            , "Application ready. (version=%s, build=%s, commit=%s, uid=%d, euid=%d)"
            , i01::g_VERSION
            , i01::g_BUILD_VERSION
            , i01::g_COMMIT
            , (int)::getuid()
            , (int)::geteuid() );

    I01_LOG_NOTICE("Application ready. (version={}, build={}, commit={}, uid={:d}, euid={:d}, pid={:d})"
            , i01::g_VERSION
            , i01::g_BUILD_VERSION
            , i01::g_COMMIT
            , (int)::getuid()
            , (int)::geteuid()
            , (int)::getpid());

    // Call any notifiers provided for user-specified values...
    po::notify(m_vm);

    return true;
}

bool Application::load_config_file(const char *file_path)
{
    std::ifstream ifs(file_path);
    try {
        po::store( po::parse_config_file(ifs, m_opt_desc)
                 , m_vm);
        return true;
    } catch(po::error& e) {
        std::cerr << "While parsing " << file_path
                  << "got error: " << e.what() << std::endl;
    }
    return false;
}

Application::~Application()
{
}

void Application::fatal_error(const char * logmsg)
{
    std::cerr << "FATAL: " << logmsg << std::endl;
    ::syslog( LOG_ERR
            , "FATAL: %s"
            , logmsg );
    ::exit(EXIT_FAILURE);
}

void Application::daemonize_(bool daemonize_value)
{
    if (!daemonize_value || daemonized())
        return;
    pid_t pid, sid;
    if (getppid() != 1) /* not yet daemonized */
    {
        pid = ::fork();
        if (pid < 0)
        {
            std::cerr << "fork failed during daemonization." << std::endl;
            perror("fork");
            std::exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
        ::umask(0);
        sid = ::setsid();
        if (sid < 0) {
            std::cerr << "setsid failed during daemonization." << std::endl;
            perror("setsid");
            std::exit(EXIT_FAILURE);
        }
        if ((::chdir("/")) < 0) {
            std::cerr << "chdir failed during daemonization." << std::endl;
            perror("chdir");
            std::exit(EXIT_FAILURE);
        }
        stdin = freopen("/dev/null", "r", stdin);
        /* Do not redirect stdout, stderr as these are handled well by
         * systemd/journalctl. */
        //freopen("/dev/null", "w", stdout);
        //freopen("/dev/null", "w", stderr);

    }
    m_daemonized = true;
}

void Application::optimize_(bool optimize_value)
{
    if (!optimize_value)
        return;
    /* Performance optimizations */

    if (!m_pmqos_cpudmalatency.set(0))
    {
        std::cerr << "Error setting /dev/cpu_dma_latency." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (!mallopt(M_TRIM_THRESHOLD, -1))
    {
        std::cerr << "Error in mallopt(M_TRIM_THRESHOLD, ...)." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (!mallopt(M_MMAP_MAX, 0))
    {
        std::cerr << "Error in mallopt(M_MMAP_MAX, ...)." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    long psz = -1;
    if ((psz = ::sysconf(_SC_PAGESIZE)) <= 0)
    {
        std::cerr << "Error getting page size." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    unsigned long mlsz = (unsigned long)m_vm["reserve-memory"].as<int>();
    if (mlsz > 0)
    {
        char * buf = (char*) ::malloc(mlsz * 1024 * 1024);
        if (buf == nullptr)
        {
            std::cerr << "Error malloc()ing for prefaulting." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        for (volatile unsigned long i = 0; i < mlsz * 1024 * 1024; i += (unsigned long)psz)
        {
            buf[i] = 0;
        }
        if (::mlockall(MCL_CURRENT) != 0)
        {
            std::cerr << "Error locking memory. " << strerror(errno) << std::endl;
            std::exit(EXIT_FAILURE);
        }
        ::free(buf);
    }

    int rtprio = m_vm["scheduler-priority"].as<int>();
    std::string policy = m_vm["scheduler-policy"].as<std::string>();
    int policint = SCHED_OTHER;
    struct sched_param param;
    ::memset(&param, 0, sizeof(param));
    if (rtprio < 0 || rtprio > 99)
    {
        std::cerr << "scheduler-priority must be between 0-99." << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        param.sched_priority = rtprio;
    }
    if (policy == "fifo")
        policint = SCHED_FIFO;
    else if (policy == "rr")
        policint = SCHED_RR;
    else if (policy == "idle")
        policint = SCHED_IDLE;
    else if (policy == "batch")
        policint = SCHED_BATCH;
    else if (policy == "other")
        policint = SCHED_OTHER;
    else
    {
        std::cerr << "scheduler-policy must be fifo/rr/idle/batch/other." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (::sched_setscheduler(0, policint, &param) != 0)
    {
        std::cerr << "sched_setscheduler failed." << std::endl;
        perror("sched_setscheduler");
        std::exit(EXIT_FAILURE);
    }

    m_optimized = true;
}

} }
