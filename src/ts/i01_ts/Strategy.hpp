#pragma once

#include <map>
#include <string>

#include <i01_core/Lock.hpp>
#include <i01_oe/Types.hpp>


namespace i01 { namespace MD {
class DataManager;
}}

namespace i01 { namespace OE {
class OrderManager;
}}

namespace i01 { namespace TS {

class Strategy {
    const std::string m_name;
    const OE::LocalAccount m_localaccount;
    static std::map<const std::string, Strategy *> s_strategies;
    static core::SpinMutex s_strategies_mutex;
protected:
    i01::OE::OrderManager * m_om_p;
    i01::MD::DataManager * m_dm_p;

public:
    Strategy(i01::OE::OrderManager * omp, i01::MD::DataManager *dmp, const std::string& n, const OE::LocalAccount& la = 0);
    virtual ~Strategy() {}

    const std::string& name() const { return m_name; }
    const OE::LocalAccount& local_account() const { return m_localaccount; }

    /// Called on engine start after data feeds and order manager are started.
    /// Start any threads here and register them with the engine so that they
    /// are pthread_join()ed on prior to engine shutdown.
    virtual void start() {}

    static Strategy * get_strategy(const std::string& n);
    static Strategy * factory(OE::OrderManager * omp, MD::DataManager * dmp, const std::string& n, const std::string& type);

    friend class EngineApp;
};


} }
