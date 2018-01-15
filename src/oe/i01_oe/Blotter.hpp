#pragma once

#include <vector>

#include <boost/noncopyable.hpp>

#include <i01_core/Lock.hpp>
#include <i01_oe/Types.hpp>
#include <i01_oe/Instrument.hpp>

namespace i01 { namespace OE {

class Order;
class OrderSession;
class OrderManager;

class Blotter : private boost::noncopyable {
protected:
    OrderManager& m_om;
public:
    Blotter() = delete;
    /// Blotter must be initialized with a reference to its respective
    /// owning `OrderManager`.
    Blotter(OrderManager& om);
    virtual ~Blotter() = default;

    // OrderManager related:
    virtual void log_start() = 0;

    // Order related:
    virtual void log_new_order(const Order* order_p) = 0;
    virtual void log_order_sent(const Order* order_p) = 0;
    virtual void log_local_reject(const Order* order_p) = 0;
    virtual void log_pending_cancel(const Order* order_p, Size new_qty) = 0;
    virtual void log_destroy(const Order* order_p) = 0;
    virtual void log_acknowledged(const Order* order_p) = 0;
    virtual void log_rejected(const Order* order_p) = 0;
    virtual void log_filled(const Order* order_p, const Size fillSize, const Price fillPrice, const Timestamp& ts, const FillFeeCode fill_fee_code) = 0;
    virtual void log_cancel_rejected(const Order* order_p) = 0;
    virtual void log_cancelled(const Order* order_p, Size) = 0;

    // Session related:
    virtual void log_add_session(const OrderSession* osp) = 0;
    virtual void log_position(const EngineID source, const Instrument* instrument, Quantity position, Price avg_price, Price mark_px) = 0;
    
    // Strategy related:
    virtual void log_add_strategy(const std::string& strategy_name, const OE::LocalAccount& la) = 0;
};

class NullBlotter : public Blotter {
public:
    /// Blotter must be initialized with a reference to its respective
    /// owning `OrderManager`.
    NullBlotter(OrderManager& om);
    virtual ~NullBlotter() = default;

    // OrderManager related:
    virtual void log_start() override final {}

    // Order related:
    virtual void log_new_order(const Order* ) override final {}
    virtual void log_order_sent(const Order* ) override final {}
    virtual void log_local_reject(const Order* ) override final {}
    virtual void log_pending_cancel(const Order* , Size) override final {}
    virtual void log_destroy(const Order* ) override final {}
    virtual void log_acknowledged(const Order* ) override final {}
    virtual void log_rejected(const Order* ) override final {}
    virtual void log_filled(const Order* , const Size , const Price , const Timestamp& , const FillFeeCode ) override final {}
    virtual void log_cancel_rejected(const Order* ) override final {}
    virtual void log_cancelled(const Order*, Size) override final {}

    // Session related:
    virtual void log_add_session(const OrderSession* ) override final {}
    virtual void log_position(const EngineID, const Instrument*, Quantity, Price, Price) override final {}

    // Strategy related:
    virtual void log_add_strategy(const std::string&, const OE::LocalAccount&) override final {}
};


} }
