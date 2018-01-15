#pragma once

#include <vector>

#include <i01_core/Log.hpp>
#include <i01_core/NamedThread.hpp>
#include <i01_core/Application.hpp>

#include <i01_md/DataManager.hpp>

#include <i01_oe/OrderManager.hpp>

#include <i01_ts/Strategy.hpp>
#include <i01_ts/StrategyPlugin.hpp>

extern "C" {
// Fwd declare these global handler functions.
I01_API bool i01_c_api_register_strategy_from_plugin(i01::TS::Strategy *);
I01_API bool i01_c_api_register_named_thread_from_plugin(i01::core::NamedThreadBase *t);
}

namespace i01 { namespace apps { namespace engine {

class EngineApp : public i01::core::Application {
    private:
        /// \internal Host name.
        std::string m_hostname;
        /// \internal Engine name.
        std::string m_engine_name;
        /// \internal Engine ID.
        std::uint8_t m_engine_id;
        /// \internal Current date being simulated (if in simulation mode).  Format:
        /// YYYYMMDD
        std::uint32_t m_engine_date;
        /// \internal Time in nanoseconds since midnight upon which to shut down the
        /// engine.
        std::uint64_t m_shutdown_time_ns_since_midnight;
        core::Timestamp m_shutdown_ts;

    /// \internal Boolean on whether the OrderManager should replay from the orderlog on start.
    bool m_om_replay;

        /// \internal Boolean indicating whether the engine is being run in
        /// simulation mode or not.
        bool m_simulated;
        /// \internal Global log reference.
        i01::core::Log& m_log;
        /// \internal Thread container to allow for graceful shutdown of all
        /// threads upon engine shutdown.  Users should assume thread
        /// ownership belongs to the engine once registered.
        std::set<i01::core::NamedThreadBase *> m_threads;

        /// \internal DataManager pointer for strategies to use to get market data. Only one per engine currently.
        i01::MD::DataManager * m_dm_p;

        /// \internal OrderManager pointer for strategies to use to place
        /// orders, and to spawn OrderSessions with.  Currently only one OM
        //per engine is supported.
        i01::OE::OrderManager * m_om_p;

        /// \internal Strategy container, to allow for graceful shutdown of strategies.
        std::map<std::string, i01::TS::Strategy *> m_strategies;
        /// \internal Plugin container, to allow for graceful shutdown of plugins.
        std::vector<std::pair<void *, std::string>> m_plugins;

    std::vector<std::string> m_pcap_filenames;
    /// \internal Sim Session type... one type for all sessions.
    std::string m_sim_session_type;

        /// \internal Plugin context object to pass to plugins loaded by this
        /// engine.
        const i01_StrategyPluginContext m_ctx;


        /// \internal Static pointer to EngineApp required for i01_register_*_from_plugin.
        static EngineApp * s_engine_app_p;

    public:
        EngineApp();
        EngineApp(int argc, const char *argv[]);
        bool init(int argc, const char *argv[]) override final;
        virtual ~EngineApp();
        /// Global engine log, use this to write log messages to the console.
        inline i01::core::Log& log() { return m_log; }
        int run() override final;
        /// Non-global function to register a strategy with the engine.
        /// Takes ownership of the Strategy.
        bool register_strategy(i01::TS::Strategy* s);
        /// Non-global function to register a NamedThread with the engine.
        /// Takes ownership of the NamedThread.
        bool register_thread(i01::core::NamedThreadBase* t);
        /// Non-global function to register an OrderSession with the engine's
        /// OrderManager.
        /// Takes ownership of the OrderSession.
        bool register_session(i01::OE::OrderSessionPtr s);
        /// Returns a const reference to the engine's StrategyPlugin context,
        const i01_StrategyPluginContext& strategyplugin_ctx() const { return m_ctx; }
        /// Returns a pointer to the order manager.
        i01::OE::OrderManager * ordermanager() { return m_om_p; }

    i01::MD::DataManager * datamanager() { return m_dm_p; }

        friend bool ::i01_c_api_register_strategy_from_plugin(i01::TS::Strategy *);
        friend bool ::i01_c_api_register_named_thread_from_plugin(i01::core::NamedThreadBase *);
};

} } }
