#pragma once

// OUCH 4.2
//    http://www.nasdaqtrader.com/content/technicalsupport/specifications/TradingProducts/OUCH4.2.pdf

#include <cstdint>
#include <iosfwd>

#include "Types.hpp"

namespace i01 { namespace OE { namespace NASDAQ { namespace OUCH42 { namespace Messages {
    using namespace i01::OE::NASDAQ::OUCH42::Types;

    /* INBOUND */

    struct EnterOrder {
        InboundMessageType                  message_type;
        OrderToken                          order_token;
        BuySellIndicator                    buy_sell_indicator;
        Shares                              shares;
        Stock                               stock;
        Price                               price;
        TimeInForce                         time_in_force;
        Firm                                firm;
        Display                             display;
        Capacity                            capacity;
        IntermarketSweepEligibility         iso_eligibility;
        Shares                              minimum_quantity;
        CrossType                           cross_type;
        CustomerType                        customer_type;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const EnterOrder& o);

    struct ReplaceOrder {
        InboundMessageType                  message_type;
        OrderToken                          existing_order_token;
        OrderToken                          replacement_order_token;
        Shares                              shares;
        Price                               price;
        TimeInForce                         time_in_force;
        Display                             display;
        IntermarketSweepEligibility         iso_eligibility;
        Shares                              minimum_quantity;
    } __attribute__((packed));

    struct CancelOrder {
        InboundMessageType                  message_type;
        OrderToken                          order_token;

        /// New intended order size.  Zero (0) to cancel order completely.
        Shares                              shares;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const CancelOrder& o);

    struct ModifyOrder {
        InboundMessageType                  message_type;
        OrderToken                          order_token;

        /// Only transitions between SELL, SHORT, and SHORT_EXEMPT allowed.
        BuySellIndicator                    buy_sell_indicator;
        Shares                              shares;
    } __attribute__((packed));

    /* OUTBOUND */

    struct OutboundMessageHeader {
        OutboundMessageType                 message_type;
        ExchTime                            exch_time;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OutboundMessageHeader& msg);

    struct OutboundOrderMessageHeader {
        OutboundMessageType                 message_type;
        ExchTime                            exch_time;
        OrderToken                          order_token;
    } __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const OutboundOrderMessageHeader& msg);

template<typename OutboundOrderMsgType>
std::ostream& operator<<(std::ostream& os, const OutboundOrderMsgType& msg)
{
    return os << msg.hdr;
}

    struct SystemEvent {
        OutboundMessageHeader               hdr;
        SystemEventCode                     system_event_code;
    } __attribute__((packed));

    struct Accepted {
        OutboundOrderMessageHeader          hdr;
        BuySellIndicator                    buy_sell_indicator;
        Shares                              shares;
        Stock                               stock;
        Price                               price;
        TimeInForce                         time_in_force;
        Firm                                firm;
        Display                             display;
        OrderReferenceNumber                order_reference_number;
        Capacity                            capacity;
        IntermarketSweepEligibility         iso_eligibility;
        Shares                              minimum_quantity;
        CrossType                           cross_type;
        OrderState                          order_state;
        BBOWeightIndicator                  bbo_weight_indicator;
    } __attribute__((packed));

    struct Replaced {
        OutboundOrderMessageHeader          hdr;
        BuySellIndicator                    buy_sell_indicator;
        Shares                              shares;
        Stock                               stock;
        Price                               price;
        TimeInForce                         time_in_force;
        Firm                                firm;
        Display                             display;
        OrderReferenceNumber                order_reference_number;
        Capacity                            capacity;
        IntermarketSweepEligibility         iso_eligibility;
        Shares                              minimum_quantity;
        CrossType                           cross_type;
        OrderState                          order_state;
        OrderToken                          previous_order_token;
        BBOWeightIndicator                  bbo_weight_indicator;
    } __attribute__((packed));

    struct Canceled {
        OutboundOrderMessageHeader          hdr;
        Shares                              decremented_shares;
        CanceledReason                      reason;
    } __attribute__((packed));

    struct AIQCanceled {
        OutboundOrderMessageHeader          hdr;
        Shares                              decremented_shares;

        /// Always SELF_MATCH_PREVENTION ('Q') for AIQCanceled.
        CanceledReason                      reason;

        /// Always the same for 'decrement both', may differ if
        //  'cancel oldest', from decremented_shares.
        Shares                              quantity_prevented_from_trading;
        Price                               execution_price;
        LiquidityFlag                       liquidity_flag;
    } __attribute__((packed));

    struct Executed {
        OutboundOrderMessageHeader          hdr;
        Shares                              executed_shares;
        Price                               execution_price;
        LiquidityFlag                       liquidity_flag;
        MatchNumber                         match_number;
    } __attribute__((packed));

    struct BrokenTrade {
        OutboundOrderMessageHeader          hdr;
        MatchNumber                         match_number;
        BrokenTradeReason                   reason;
    } __attribute__((packed));

    struct Rejected {
        OutboundOrderMessageHeader          hdr;
        RejectedReason                      reason;
    } __attribute__((packed));

    struct CancelPending {
        OutboundOrderMessageHeader          hdr;
    } __attribute__((packed));

    struct CancelReject {
        OutboundOrderMessageHeader          hdr;
    } __attribute__((packed));

    struct OrderPriorityUpdate {
        OutboundOrderMessageHeader          hdr;
        Price                               execution_price;
        Display                             display;
        OrderReferenceNumber                order_reference_number;
    } __attribute__((packed));

    struct Modified {
        OutboundOrderMessageHeader          hdr;
        BuySellIndicator                    buy_sell_indicator;
        Shares                              shares;
    } __attribute__((packed));

} } } } }
