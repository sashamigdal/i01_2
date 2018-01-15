#pragma once

#include <i01_oe/Blotter.hpp>
#include <i01_oe/Types.hpp>
#include <i01_core/AsyncMappedFile.hpp>

namespace i01 { namespace OE {

class Order;
class OrderSession;
class OrderManager;
class FileBlotter : public Blotter {
public:
    FileBlotter() = delete;
    /// Blotter must be initialized with a reference to its respective
    /// owning `OrderManager`.
    FileBlotter(OrderManager& om, const std::string& path = OE::DEFAULT_ORDERLOG_PATH);
    virtual ~FileBlotter();

    // OrderManager related:
    virtual void log_start() override final;

    // Order related:
    virtual void log_new_order(const Order* order_p) override final;
    virtual void log_order_sent(const Order* order_p) override final;
    virtual void log_local_reject(const Order* order_p) override final;
    virtual void log_pending_cancel(const Order* order_p, Size new_qty) override final;
    virtual void log_destroy(const Order* order_p) override final;
    virtual void log_acknowledged(const Order* order_p) override final;
    virtual void log_rejected(const Order* order_p) override final;
    virtual void log_filled(const Order* order_p, const Size fillSize, const Price fillPrice, const Timestamp& ts, const FillFeeCode fill_fee_code) override final;
    virtual void log_cancel_rejected(const Order* order_p) override final;
    virtual void log_cancelled(const Order* order_p, Size cxled_qty) override final;

    // Session related:
    virtual void log_add_session(const OrderSession* osp) override final;
    virtual void log_position(const EngineID, const Instrument*, Quantity, Price, Price) override final;

    // Strategy related:
    virtual void log_add_strategy(const std::string&, const OE::LocalAccount&) override final;
private:
    core::AsyncMappedFile m_orderlog;
    bool m_started;
};

} }
