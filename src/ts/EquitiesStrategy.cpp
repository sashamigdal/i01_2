#include <i01_md/DataManager.hpp>

#include <i01_ts/EquitiesStrategy.hpp>

namespace i01 { namespace TS {

EquitiesStrategy::EquitiesStrategy(OE::OrderManager * omp, MD::DataManager * dmp,
                                   const std::string& n) :
    Strategy(omp, dmp, n)
{
    if (m_dm_p) {
        m_dm_p->register_listener(this);
    }
}

}}
