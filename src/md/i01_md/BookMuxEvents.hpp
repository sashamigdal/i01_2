#pragma once

// for std::ostream fwd declarations
#include <iosfwd>
#include <string>

#include <i01_md/BookBase.hpp>
#include <i01_md/OrderBook.hpp>
#include <i01_md/Symbol.hpp>

namespace i01 { namespace core {
struct MIC;
class Timestamp;
}}

namespace i01 { namespace MD {

namespace NYSE { namespace PDP { namespace Messages {
struct OpeningImbalance;
struct ClosingImbalance;
}}}

namespace NASDAQ { namespace ITCH50 { namespace Messages {
struct NetOrderImbalanceIndicator;
}}}

class L2Book;

struct SymbolDefEvent {
    typedef core::MIC MIC;

    const MIC &mic;
    const EphemeralSymbolIndex esi;
    const std::string &ticker;
};
std::ostream & operator<<(std::ostream &, const SymbolDefEvent &evt);

struct GapEvent {
    typedef core::Timestamp Timestamp;
    typedef core::MIC MIC;
    const Timestamp & m_timestamp;
    const MIC &m_mic;
    const std::uint64_t m_expected_seqnum;
    const std::uint64_t m_received_seqnum;
    const Timestamp & m_prior_timestamp;
};
std::ostream & operator<<(std::ostream &, const GapEvent &evt);

struct L3AddEvent {
    typedef core::Timestamp Timestamp;
    const Timestamp &m_timestamp;
    const OrderBook &m_book;
    const OrderBook::Order &m_order;
};
std::ostream & operator<<(std::ostream &, const L3AddEvent &evt);

struct L3CancelEvent {
    typedef core::Timestamp Timestamp;

    const Timestamp &m_timestamp;
    const OrderBook &m_book;
    const OrderBook::Order &m_order;
    const OrderBook::Order::Size m_old_size;
};
std::ostream & operator<<(std::ostream &, const L3CancelEvent &evt);

struct L3ModifyEvent {
    typedef core::Timestamp Timestamp;
    typedef OrderBook::Order Order;
    typedef Order::RefNum RefNum;
    typedef Order::Price Price;
    typedef Order::Size Size;

    const Timestamp &m_timestamp;
    const OrderBook &m_book;
    const Order &m_new_order;
    RefNum m_old_refnum;
    Price m_old_price;
    Size m_old_size;
};
std::ostream & operator<<(std::ostream &, const L3ModifyEvent &evt);

struct L3ExecutionEvent {
    typedef core::Timestamp Timestamp;
    typedef OrderBook::Order Order;
    typedef Order::Price Price;
    typedef Order::Size Size;
    typedef Order::RefNum TradeRefNum;

    const Timestamp &m_timestamp;
    const Timestamp &m_exchange_timestamp;
    const OrderBook &m_book;
    const Order &m_order;
    const TradeRefNum m_trade_id;
    const Price m_exec_price;
    const Size m_exec_size;
    const Size m_old_size;
    const bool m_nonprintable;
};
std::ostream & operator<<(std::ostream &, const L3ExecutionEvent &evt);

struct L3CrossedEvent {
    const core::Timestamp& timestamp;
    const OrderBook& book;
    const OrderBook::Order& order;
};
std::ostream& operator<<(std::ostream&, const L3CrossedEvent& evt);

struct TradeEvent {
    typedef core::Timestamp Timestamp;
    typedef OrderBook::Order::Price Price;
    typedef std::uint64_t Size;
    typedef OrderBook::Order::RefNum TradeRefNum;
    typedef OrderBook::Order::Side PassiveSide;

    const Timestamp &m_timestamp;
    const Timestamp &m_exchange_timestamp;
    const BookBase &m_book;
    const TradeRefNum m_trade_id;
    const Price m_price;
    const Size m_size;
    const PassiveSide m_passive_side;
    const bool m_cross;
    const void * m_msg;
};
std::ostream & operator<<(std::ostream &, const TradeEvent &evt);

struct TradingStatusEvent {
    typedef core::Timestamp Timestamp;
    const Timestamp m_timestamp;
    const BookBase &m_book;

    enum class Event : std::uint8_t {
        UNKNOWN = 0,
        TRADING = 1,
        HALT = 2,
        QUOTE_ONLY = 3,
        SSR_IN_EFFECT = 4,
        SSR_NOT_IN_EFFECT = 5,
        PRE_MKT_SESSION = 6,
        REGULAR_MKT_SESSION = 7,
        POST_MKT_SESSION = 8,
    };

    Event m_event;
};
std::ostream & operator<<(std::ostream &, const TradingStatusEvent &evt);

struct NYSEOpeningImbalanceEvent {
    typedef core::Timestamp Timestamp;

    const Timestamp &m_timestamp;
    const L2Book &m_book;
    const NYSE::PDP::Messages::OpeningImbalance & m_msg;
};
std::ostream & operator<<(std::ostream &, const NYSEOpeningImbalanceEvent &evt);

struct NYSEClosingImbalanceEvent {
    typedef core::Timestamp Timestamp;

    const Timestamp &m_timestamp;
    const L2Book &m_book;
    const NYSE::PDP::Messages::ClosingImbalance & m_msg;
};
std::ostream & operator<<(std::ostream &, const NYSEClosingImbalanceEvent &evt);

struct NASDAQImbalanceEvent {
    typedef core::Timestamp Timestamp;

    const Timestamp& m_timestamp;
    const Timestamp& m_ecn_timestamp;
    const OrderBook& m_book;
    const NASDAQ::ITCH50::Messages::NetOrderImbalanceIndicator* msg;
};

struct FeedEvent {
    enum class EventCode {
        UNKNOWN               = 0
      , START_OF_MESSAGES     = 1
      , START_OF_SYSTEM_HOURS = 2
      , START_OF_MARKET_HOURS = 3
      , END_OF_MARKET_HOURS   = 4
      , END_OF_SYSTEM_HOURS   = 5
      , END_OF_MESSAGES       = 6
    };

    const core::Timestamp& timestamp;
    const core::Timestamp& ecn_timestamp;
    const core::MIC& mic;
    const EventCode& event_code;
};
std::ostream & operator<<(std::ostream &, const FeedEvent &evt);

struct TimeoutEvent {
    enum class EventCode {
        UNKNOWN         = 0
            , TIMEOUT_START     = 1
            , TIMEOUT_END       = 2
            };
    const core::Timestamp& timestamp;
    const core::MIC& mic;
    const core::Timestamp& last_timestamp;
    const int unit_index;
    const std::string& unit_name;
    const EventCode event_code;
};
std::ostream& operator<<(std::ostream&, const TimeoutEvent::EventCode&);
std::ostream& operator<<(std::ostream&, const TimeoutEvent& evt);

// TODO FIXME: implement MWCB
struct MWCBEvent { const core::Timestamp timestamp; int level; };
std::ostream & operator<<(std::ostream &, const MWCBEvent &evt);

struct L2BookEvent {
    typedef core::Timestamp Timestamp;
    enum class DeltaReasonCode {
        NONE              = 0
      , NEW_ORDER         = 1
      , CANCEL            = 2
      , EXECUTION         = 3
      , MULTIPLE_EVENTS   = 4
    };

    const Timestamp &m_timestamp;
    const L2Book &m_book;
    bool m_is_buy;
    bool m_is_bbo;
    bool m_is_reduced;
    Price m_price;
    Size m_size;
    NumOrders m_num_orders;
    Size m_delta_size;
    DeltaReasonCode m_reason;
};
std::ostream & operator<<(std::ostream &, const L2BookEvent &evt);

struct PacketEvent {
    typedef core::Timestamp Timestamp;
    typedef core::MIC MIC;

    const Timestamp& timestamp;
    const MIC& mic;
};
std::ostream & operator<<(std::ostream & os, const PacketEvent &evt);

}}
