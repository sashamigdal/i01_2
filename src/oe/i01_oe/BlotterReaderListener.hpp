#pragma once

// if we don't include this and only fwd declare the OrderLogFormat
// structs, it gets really annoying in other units.
#include <i01_oe/OrderLogFormat.hpp>

namespace i01 {

namespace core {
class Timestamp;
}

namespace OE {

class BlotterReaderListener {
public:
    using Timestamp = core::Timestamp;

    virtual void on_log_start(const Timestamp&) = 0;
    virtual void on_log_end(const Timestamp&) = 0;
    virtual void on_log_new_instrument(const Timestamp&, const OrderLogFormat::NewInstrumentBody&) {}
    virtual void on_log_new_order(const Timestamp&, const OrderLogFormat::NewOrderBody&) {}
    virtual void on_log_order_sent(const Timestamp&, const OrderLogFormat::OrderSentBody&) = 0;
    virtual void on_log_local_reject(const Timestamp&, const OrderLogFormat::OrderLocalRejectBody&) = 0;
    virtual void on_log_pending_cancel(const Timestamp&, const OrderLogFormat::OrderCxlReqBody&) = 0;
    virtual void on_log_acknowledged(const Timestamp&, const OrderLogFormat::OrderAckBody&) = 0;
    virtual void on_log_rejected(const Timestamp&, const OrderLogFormat::OrderRejectBody&) = 0;
    virtual void on_log_filled(const Timestamp&, bool partial_fill, const OrderLogFormat::OrderFillBody&) = 0;
    virtual void on_log_partial_cancel(const Timestamp&, const OrderLogFormat::OrderCxlBody&) {}
    virtual void on_log_cancelled(const Timestamp&, const OrderLogFormat::OrderCxlBody&) = 0;
    virtual void on_log_cxlreplace_request(const Timestamp&, const OrderLogFormat::OrderCxlReplaceReqBody&)  {}
    virtual void on_log_cxlreplaced(const Timestamp&, const OrderLogFormat::OrderCxlReplaceBody&) {}
    virtual void on_log_cancel_rejected(const Timestamp&, const OrderLogFormat::OrderCxlRejectBody&) = 0;
    virtual void on_log_destroy(const Timestamp&, const OrderLogFormat::OrderDestroyBody&) = 0;

    virtual void on_log_position(const Timestamp&, const OrderLogFormat::PositionBody&) {}
    virtual void on_log_manual_position_adj(const Timestamp&, const OrderLogFormat::ManualPositionAdjBody&) = 0;

    virtual void on_log_add_session(const Timestamp&, const OrderLogFormat::NewSessionBody&) = 0;
    virtual void on_log_order_session_data(const Timestamp&, const OrderLogFormat::OrderSessionDataHeader&) {}
    virtual void on_log_add_strategy(const Timestamp&, const OrderLogFormat::NewAccountBody&) = 0;

};


}}
