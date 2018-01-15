#include <i01_ts/Strategy.hpp>
#include <i01_ts/LogReaderStrategy.hpp>
#include <i01_ts/ManualStrategy.hpp>
#include <i01_ts/NBBOSamplerStrategy.hpp>
#include <i01_ts/SamplerStrategy.hpp>
#include <i01_ts/SimTestStrategy.hpp>
#include <i01_ts/AsyncTPSStrategy.hpp>

namespace i01 { namespace TS {

std::map<const std::string, Strategy *> Strategy::s_strategies;
core::SpinMutex Strategy::s_strategies_mutex;

Strategy * Strategy::factory(
      OE::OrderManager* omp
      , MD::DataManager* dmp
    , const std::string& name
    , const std::string& type)
{
    // TODO: any static execution strategies (e.g. manual trader) can go here
    // in a similar fashion to OrderSession::factory.
    Strategy * sp;
    try {
        if (type == "sampler") {
            sp = new SamplerStrategy(omp, dmp, name);
        } else if (type == "simtest") {
            sp = new SimTestStrategy(omp, dmp, name);
        } else if (type == "manual") {
            sp = new ManualStrategy(omp, dmp, name);
        } else if ("log_reader" == type) {
            sp = new LogReaderStrategy(omp, dmp, name);
        } else if ("nbbo_sampler" == type) {
            sp = new NBBOSamplerStrategy(omp, dmp, name);
        } else if ("async_tps" == type) {
            sp = new AsyncTPSStrategy(omp, dmp, name);
        } else {
            throw std::runtime_error("Unknown strategy type \"" + type +"\"; has it been added to Strategy::factory?");
        }
        return sp;
    } catch (std::exception &e) {
        std::cerr << "Exception caught while constructing strategy " << name << " with type " << type << ": " << e.what() << std::endl;
    }

   return nullptr;
}

Strategy::Strategy(i01::OE::OrderManager * omp, i01::MD::DataManager *dmp, const std::string& n, const OE::LocalAccount& la)
  : m_name(n), m_localaccount(la), m_om_p(omp), m_dm_p(dmp)
{
    core::LockGuard<core::SpinMutex> lock(s_strategies_mutex);
    s_strategies.insert(std::make_pair(m_name, this));
}

Strategy * Strategy::get_strategy(const std::string& n)
{
    core::LockGuard<core::SpinMutex> lock(s_strategies_mutex);
    auto it = s_strategies.find(n);
    if (it != s_strategies.end())
        return it->second;
    return nullptr;
}

} }
