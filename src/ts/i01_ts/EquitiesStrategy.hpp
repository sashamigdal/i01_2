#pragma once


#include <i01_core/TimerListener.hpp>
#include <i01_md/BookMuxListener.hpp>
#include <i01_oe/OrderListener.hpp>

#include <i01_ts/Strategy.hpp>

namespace i01 { namespace TS {

class EquitiesStrategy : public Strategy, public OE::OrderListener, public MD::BookMuxListener, public core::TimerListener {
protected:
public:
    EquitiesStrategy(OE::OrderManager * omp, MD::DataManager * dmp, const std::string& n);
    virtual ~EquitiesStrategy() {}
};

} }
