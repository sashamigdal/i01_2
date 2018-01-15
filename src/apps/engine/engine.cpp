#include <sys/utsname.h>
#include <dlfcn.h>

#include <string>
#include <iostream>
#include <sstream>

#include <boost/filesystem.hpp>

#include <fix8/f8includes.hpp>

#include <i01_core/Version.hpp>
#include <i01_core/Config.hpp>
#include <i01_core/Log.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/FD.hpp>
#include <i01_core/NamedThread.hpp>
#include <i01_core/Application.hpp>
#include <i01_core/Date.hpp>

#include <i01_oe/Types.hpp>

#include <i01_ts/Strategy.hpp>
#include <i01_ts/StrategyPlugin.hpp>
#include <i01_oe/OrderManager.hpp>
#include <i01_oe/OrderLogFormat.hpp>
#include <i01_oe/L2SimSession.hpp>

#include <i01_ts/engine.hpp>

using i01::core::Log;
using i01::core::Application;
using i01::core::Config;
using i01::core::Timestamp;
using i01::core::NamedThreadBase;
using i01::core::NamedThread;
using i01::OE::OrderManager;
using i01::OE::OrderSession;
using i01::OE::OrderSessionPtr;
using i01::TS::Strategy;
using i01::MD::DataManager;

namespace i01 { namespace apps { namespace engine {

EngineApp * EngineApp::s_engine_app_p = nullptr;

EngineApp::EngineApp()
    : Application()
    , m_engine_name()
    , m_engine_date(0)
    , m_shutdown_time_ns_since_midnight(0)
    , m_om_replay(false)
    , m_simulated(false)
    , m_log(Log::instance())
    , m_dm_p(new DataManager)
    , m_om_p(new OrderManager(m_dm_p))
    , m_sim_session_type("L2SimSession")
    , m_ctx{
        .data_manager = (i01_data_manager_t *)m_dm_p,
        .order_manager = (i01_order_manager_t *)m_om_p,
        .config_instance = (i01_config_t *)&Config::instance(),
        .engine_app = (i01_engine_app_t *)this,
        .git_commit = i01::g_COMMIT,
        .register_strategy_op =
            (i01_register_strategy_op_t)
                &::i01_c_api_register_strategy_from_plugin,
        .register_named_thread_op =
            (i01_register_named_thread_op_t)
                &::i01_c_api_register_named_thread_from_plugin
        }
{
    if (s_engine_app_p)
        throw std::runtime_error("Error: EngineApp constructed twice.");
    s_engine_app_p = this;
    const char * default_name = nullptr;
    struct utsname uname_result;
    if (0 == ::uname(&uname_result)) {
        default_name = uname_result.nodename;
        m_hostname = uname_result.nodename;
    }

    po::options_description engine_desc("Engine options");
    engine_desc.add_options()
        ("engine.name"
          , po::value<std::string>()
                ->default_value(default_name == nullptr ? "" : std::string(default_name, 8))
          , "Name of the engine, must be globally unique.")
        ("engine.id"
          , po::value<int>()->default_value(0)
          , "Unique number between 1 and 7 representing the engine in position broadcasts.")
        ("engine.working-dir"
          , po::value<std::string>()
                ->default_value(boost::filesystem::current_path().string())
          , "Engine working directory.")
        ("engine.date"
          , po::value<std::uint32_t>(&m_engine_date)
                ->default_value(0)
          , "Date to simulate (format: YYYYMMDD).  Warning: If set, engine will be in simulation mode!")
        ("engine.stop-at"
          , po::value<std::string>()
                ->default_value("")
          , "Engine shutdown time (format: HH:MM:SS), e.g. 16:30:01.  Leave empty to disable shutdown timer.")
        ("engine.plugins-path-prefix"
          , po::value<std::string>()
                ->default_value("")
          , "If strategy plugin path does not start with /, prefix this to it.")
        ("engine.no-replay"
         // TODO: this is int instead of bool b/c lexical_cast does not handle bool well
        , po::value<int>()->default_value(0)->implicit_value(1)
        , "Should the OrderManager NOT replay and recover orders from the orderlog?")
        // FIXME handle historical data differently...
        ("engine.pcap-files", po::value<std::vector<std::string> >(&m_pcap_filenames), "pcap files")
        ;
    positional_options_description().add("engine.pcap-files",-1);
    m_opt_desc.add(engine_desc);
}

EngineApp::EngineApp(int argc, const char *argv[])
    : EngineApp()
{ if (!init(argc, argv)) std::exit(EXIT_FAILURE); }

bool EngineApp::init(int argc, const char *argv[]) {
    if (!Application::init(argc, argv))
        return false;

    auto cfg = Config::instance().get_shared_state();
    m_engine_date = cfg->get_or_default<std::uint32_t>("engine.date", 0);

    m_om_p->init(*cfg);

    m_dm_p->hostname(m_hostname);

    if (m_engine_date != 0) {
        if (m_engine_date > 20000101 && m_engine_date <= core::Date::today().date()) {
            m_simulated = true;
        } else {
            std::cerr << "Error: engine.date invalid or in the future. (format: YYYYMMDD)" << std::endl;
            return false;
        }
    }

    cfg->get("engine.name", m_engine_name);
    if ((!m_simulated) && m_engine_name.empty()) {
        std::cerr << "Error: engine.name is not configured." << std::endl;
        return false;
    }

    m_engine_id = static_cast<std::uint8_t>(cfg->get_or_default("engine.id", 0));
    if ((!m_simulated) && !(m_engine_id >= OE::c_MIN_ENGINE_ID && m_engine_id <= OE::c_MAX_ENGINE_ID)) {
        std::cerr << "Error: engine.id is not configured or too large." << std::endl;
        return false;
    }

    bool no_replay_arg=false;
    cfg->get("engine.no-replay", no_replay_arg);
    m_om_replay = !no_replay_arg;

    if (auto working_dir = cfg->get<std::string>("engine.working-dir")) {
        try {
            boost::filesystem::current_path(boost::filesystem::path(*working_dir));
        } catch (boost::filesystem::filesystem_error& e) {
            std::cerr << "Error: Could not chdir into engine.working-dir: " << e.what() << std::endl;
            return false;
        }
    }

    if (auto shutdown_time = cfg->get<std::string>("engine.stop-at")) {
        if (!shutdown_time->empty()) {
            // this argument is given in local time and should be converted to UTC internally
            time_t now(::time(NULL));
            struct tm * stp(::localtime(&now));

            if (m_engine_date) {
                std::stringstream ss;
                ss << m_engine_date << shutdown_time;
                if (::strptime(ss.str().c_str(), "%Y%m%d%H:%M:%S", stp) == NULL) {
                    std::cerr << "Error: historical engine.stop-at invalid." << std::endl;
                    return false;
                }
            } else {
                if (::strptime(shutdown_time->c_str(), "%H:%M:%S", stp) == NULL) {
                    std::cerr << "Error: engine.stop-at invalid." << std::endl;
                    return false;
                }
            }
            struct tm local_shut = *stp;
            auto local_shut_ns = (uint64_t)(local_shut.tm_hour * 3600 + local_shut.tm_min * 60 + local_shut.tm_sec) * 1000000000ULL;
            auto tt = ::mktime(stp);
            m_shutdown_ts = core::Timestamp{tt,0};
            stp = ::gmtime(&tt);
            m_shutdown_time_ns_since_midnight = (uint64_t)(stp->tm_hour * 3600 + stp->tm_min * 60 + stp->tm_sec) * 1000000000ULL;
            log().console()->notice("Engine will shut down at {} ({:02d}:{:02d}:{:02d}) [{} ({:02d}:{:02d}:{:02d}) UTC]."
                                    , local_shut_ns / 1000000000ULL
                                    , local_shut.tm_hour
                                    , local_shut.tm_min
                                    , local_shut.tm_sec
                                    , m_shutdown_time_ns_since_midnight / 1000000000ULL
                                    , stp->tm_hour
                                    , stp->tm_min
                                    , stp->tm_sec);
            if (!m_simulated && (m_shutdown_time_ns_since_midnight < Timestamp::now().nanoseconds_since_midnight())) {
                std::cerr << "Error: engine.stop-at is in the past." << std::endl;
                return false;
            }
        }
    }

    //////// Load sessions:
    // if (auto fgl = cfg->get<std::string>("fix8.globallogger.path")) {
    //     if (!fgl->empty())
    //         FIX8::GlobalLogger::set_global_filename(*fgl);
    // }
    // FIX8::GlobalLogger::set_levels(FIX8::Logger::Levels(FIX8::Logger::All));

    auto cs1(cfg->copy_prefix_domain("oe.sessions."));
    auto s(cs1->get_key_prefix_set());
    for (const auto& session_name : s) {
        if (session_name.empty())
            continue;
        if (session_name.length() > sizeof(OE::OrderLogFormat::SessionName)) {
            std::cerr << "Session name " << session_name << " is too long (max = " << sizeof(OE::OrderLogFormat::SessionName) << "), skipping" << std::endl;
            continue;
        }
        auto cs(cs1->copy_prefix_domain(session_name + "."));
        std::string active;
        if (!(cs->get("active", active))) {
            std::cerr << "Session active boolean not specified: oe.sessions." << session_name << ".active, assuming false" << std::endl;
            continue;
        }
        if (active != "true") { // skip sessions with active = false...
            continue;
        }
        std::string session_type;
        if (!(cs->get<std::string>("type", session_type))) {
            std::cerr << "Session type not specified: oe.sessions." << session_name << ".type" << std::endl;
            continue;
        }
        if (m_simulated) {
            // we've secretly replaced the type of this session with a
            // simulated session. Let's see if any diners in this
            // fancy restaurant notice.
            session_type = m_sim_session_type;

            // // take engine.{ack,cxl,fill}_latency_ns and place into Config under the session's name (if it's not already there)

            Config::instance().copy_key("engine.ack_latency_ns", "oe.sessions." + session_name);
            Config::instance().copy_key("engine.cxl_latency_ns", "oe.sessions." + session_name);
        }
        auto osp_b = m_om_p->add_session(session_name, session_type);
        if (osp_b.first == nullptr)
            return false;
        if (!osp_b.second) {
            std::cerr << "Error: Duplicate session detected: " << session_name << std::endl;
            return false;
        }
        // simulated sessions need to be added to the DataManager ... maybe all sessions should have the option?
        if (m_simulated || m_sim_session_type == session_type) {
            m_dm_p->register_listener(static_cast<OE::L2SimSession *>(osp_b.first.get()));
        }

    }

    //////// Load strategies:
    auto cs2(cfg->copy_prefix_domain("ts.strategies."));
    auto s2(cs2->get_key_prefix_set());
    for (const auto& strategy_name : s2) {
        if (strategy_name.empty())
            continue;
        auto cs(cs2->copy_prefix_domain(strategy_name + "."));
        std::string strategy_library;
        if ((cs->get<std::string>("library", strategy_library)) && !strategy_library.empty()) {
            std::string library_path_prefix;
            cfg->get<std::string>("engine.plugins-path-prefix", library_path_prefix);
            std::string library_path = (strategy_library[0] != '/' && !library_path_prefix.empty()
                    ? library_path_prefix + "/" + strategy_library
                    : strategy_library);
            // Loads the strategy with LAZY function binding and LOCAL symbol
            // resolution (so symbols will NOT be available to subsequently
            // loaded libraries).
            void * strategy = ::dlopen(library_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (!strategy) {
                std::cerr << "Could not load strategy " << strategy_name << " library: " << dlerror() << std::endl;
                return false;
            }
            i01_StrategyPluginInitFunc init_func = (i01_StrategyPluginInitFunc) ::dlsym(strategy, "i01_strategy_plugin_init");
            const char * dlsym_err = ::dlerror();
            if (dlsym_err) {
                std::cerr << "Could not initialize strategy plugin " << strategy_name << ": " << dlsym_err << std::endl;
                return false;
            }
            if (! (*init_func)(&m_ctx, strategy_name.c_str()) ) {
                std::cerr << "Strategy plugin init function failed " << strategy_name << ": " << strategy_library << std::endl;
                ::dlclose(strategy);
                return false;
            }
            m_plugins.emplace_back(std::make_pair(strategy, strategy_name));
        } else if ((cs->get<std::string>("type", strategy_library)) && !strategy_library.empty()) {
            Strategy * sp = Strategy::factory(m_om_p, m_dm_p, strategy_name, strategy_library);
            if (!sp) {
                std::cerr << "Could not get strategy " << strategy_name << " statically." << std::endl;
                return false;
            }
            register_strategy(sp);
        } else {
            std::cerr << "Strategy " << strategy_name << " missing type or library configuration parameter." << std::endl;
            return false;
        }
    }

    // set the live/historical state for the DM ... and creates bookmuxen here
    auto md_cfg(cfg->copy_prefix_domain("md."));
    m_dm_p->init(*md_cfg, m_engine_date);
    if (m_pcap_filenames.size()) {
        m_dm_p->use_files(std::set<std::string>(m_pcap_filenames.begin(), m_pcap_filenames.end()));
    }

    return true;
}

EngineApp::~EngineApp() {
    m_om_p->disable_all_trading();
    log().console()->notice() << "Attempting graceful shutdown of engine " << m_engine_name << ".";
    for (auto& th : m_threads) {
        th->shutdown(/* blocking = */ false);
    }
    for (auto& th : m_threads) {
        log().console()->notice() << "Joining on thread " << th->name() << ".";
        th->join();
        // Deleting will pthread_cancel the thread if it's still active:
        delete th;
    }
    m_threads.clear();
    for (auto& ns : m_strategies) {
        delete ns.second;
        ns.second = nullptr;
    }
    m_strategies.clear();
    for (auto it = m_plugins.rbegin(); it != m_plugins.rend(); ++it) {
        if (it->first) {
            i01_StrategyPluginExitFunc exit_func = (i01_StrategyPluginExitFunc) ::dlsym(it->first, "i01_strategy_plugin_exit");
            const char * dlsym_err = ::dlerror();
            if (dlsym_err) {
                std::cerr << "Could not find strategy plugin " << it->second << " exit function: " << dlsym_err << std::endl;
            } else {
                (*exit_func)(&m_ctx, it->second.c_str());
            }
            ::dlclose(it->first);
        }
    }
    m_plugins.clear();
    delete m_om_p;
    delete m_dm_p;
}

bool EngineApp::register_strategy(Strategy* s) {
    return m_strategies.emplace(std::make_pair(s->name(), s)).second;
}
bool EngineApp::register_thread(NamedThreadBase* t) {
    return m_threads.emplace(t).second;
}

int EngineApp::run()
{
    // TODO: Bring up market data feed handlers based on Config:


    // Bring up order entry sessions:
    m_om_p->start(m_om_replay);
    // Bring up strategies in m_strategies:
    for (auto& strat : m_strategies) {
        if (strat.second)
            strat.second->start();
    }

    m_dm_p->start_event_pollers();

    if (m_shutdown_time_ns_since_midnight) {
        // shutdown timer specified, exit at specified time:
        if (!m_simulated) { // use current realtime clock if not simulated:
            while (m_shutdown_time_ns_since_midnight > Timestamp::now().nanoseconds_since_midnight()) {
                ::sleep(1);
            }
        } else { // use last event time if simulated:

            m_dm_p->read_until(m_shutdown_ts);
        }
        
        return EXIT_SUCCESS; // engine destruction will cause threads to shut down.
    } else {
        // no shutdown timer...
        if (m_simulated) {
            // read all simulated data
            m_dm_p->read_data();
        } else {
            // exit when all threads are finished:
            for (auto& thrd : m_threads) {
                if (thrd)
                    if (thrd->join())
                        log().console()->notice() << "Thread " << thrd->name() << " exited successfully at engine shutdown time.";
            }
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE; // should never reach here...
}

} } }

extern "C" {
bool i01_c_api_register_strategy_from_plugin(Strategy *s) {
    if (!i01::apps::engine::EngineApp::s_engine_app_p)
        return false;
    return i01::apps::engine::EngineApp::s_engine_app_p->register_strategy(s);
}

bool i01_c_api_register_named_thread_from_plugin(NamedThreadBase *t) {
    if (!i01::apps::engine::EngineApp::s_engine_app_p)
        return false;
    return i01::apps::engine::EngineApp::s_engine_app_p->register_thread(t);
}
}
int main(int argc, const char *argv[])
{
    i01::apps::engine::EngineApp app(argc, argv);
    return app.run();
}
