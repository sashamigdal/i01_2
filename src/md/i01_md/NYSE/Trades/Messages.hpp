#pragma once

#include <cstdint>
#include <i01_core/macro.hpp>
#include "Types.hpp"

/* XDP Common Client
 * 1.6a / 3 June 2014
 */
/* XDP Trades
 * 2.1 / 29 November 2013
 */

namespace i01 { namespace MD { namespace NYSE { namespace Trades { namespace Messages {

    using namespace i01::MD::NYSE::Trades::Types;

    struct PacketHeader {
        static const char* name() { return "XDP::PacketHeader"; }
        PktSize pkt_size;
        DeliveryFlag delivery_flag;
        NumberMsgs num_msgs;
        SeqNum seqnum;
        Time send_time;
        TimeNS send_time_ns;
    } __attribute__((packed));
    I01_ASSERT_SIZE(PacketHeader, 16);

    struct MessageHeader {
        static const char* name() { return "XDP::MessageHeader"; }
        MsgSize msg_size;
        MsgType msg_type;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MessageHeader, 4);

    struct SeqNumReset { /* PacketHeader DeliveryFlag = 12 */
        static const char* name() { return "XDP::SeqNumReset"; }
        MessageHeader msghdr; /* msg_type = 1 */
        Time source_time;
        TimeNS source_time_ns;
        ProductID product_id;
        ChannelID channel_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SeqNumReset, 14);

    struct SourceTimeReference {
        static const char* name() { return "XDP::SourceTimeReference"; }
        MessageHeader msghdr; /* msg_type = 2 */
        SymbolIndex symbol_index;
        SymbolSeqNum symbol_seqnum;
        Time time_reference;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SourceTimeReference, 16);

//    struct Heartbeat { /* PacketHeader DeliveryFlag = 1 */
//        static const char* name() { return "XDP::Heartbeat"; }
//        /* PacketHeader only. */
//    } __attribute__((packed));
//    I01_ASSERT_SIZE(Heartbeat, 0);

// TODO: HeartbeatResponse
// TODO: RetransmissionRequest
// TODO: RefreshRequest
// TODO: RequestResponse
// TODO: Retransmission messages (2.11 in XDP Common Client spec)
// TODO: SymbolIndexMappingRequest
// TODO: SymbolIndexMapping
// TODO: VendorMapping
// TODO: MessageUnavailable

    struct SymbolClear {
        static const char* name() { return "XDP::SymbolClear"; }
        MessageHeader msghdr; /* msg_type = 32 */
        Time source_time;
        TimeNS source_time_ns;
        SymbolIndex symbol_index;
        SeqNum next_source_seqnum;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SymbolClear, 20);

    struct TradingSessionChange {
        static const char* name() { return "XDP::TradingSessionChange"; }
        MessageHeader msghdr; /* msg_type = 33 */
        Time source_time;
        TimeNS source_time_ns;
        SymbolIndex symbol_index;
        SymbolSeqNum symbol_seqnum;
        TradingSession trading_session;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TradingSessionChange, 21);

    struct SecurityStatus {
        static const char* name() { return "XDP::SecurityStatus"; }
        MessageHeader msghdr; /* msg_type = 33 */
        Time source_time;
        TimeNS source_time_ns;
        SymbolIndex symbol_index;
        SymbolSeqNum symbol_seqnum;
        SecurityStatusType security_status; /**/
        HaltCondition halt_condition;
        TransactionID transaction_id;
        Price price1;
        Price price2;
        SSRTriggeringExchangeID ssr_triggering_exchange_id;
        Volume ssr_triggering_volume;
        Time time;
        SSRState ssr_state;
        MarketState market_state;
        SessionState session_state;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SecurityStatus, 46);

// TODO: RefreshHeader

    
    struct Trade {
        static const char* name() { return "XDP::Trade"; }
        MessageHeader msghdr;
        Time source_time;
        TimeNS source_time_ns;
        SymbolIndex symbol_index;
        SymbolSeqNum symbol_seqnum;
        TradeID trade_id;
        Price price;
        Volume volume;
        TradeCondition1 trade_condition1;
        TradeCondition2 trade_condition2;
        TradeCondition3 trade_condition3;
        TradeCondition4 trade_condition4;
        TradeThroughExempt trade_through_exempt;
        LiquidityIndicatorFlag liquidity_indicator_flag;
        Price ask_price;
        Volume ask_volume;
        Price bid_price;
        Volume bid_volume;
        TransactionID transaction_id;
        Tick tick;
        SellerDays seller_days;
        StopStockIndicator stop_stock_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(Trade, 61);
    
    struct TradeCancelOrBust {
        static const char* name() { return "XDP::TradeCancelOrBust"; }
        MessageHeader msghdr;
        Time source_time;
        TimeNS source_time_ns;
        SymbolIndex symbol_index;
        SymbolSeqNum symbol_seqnum;
        TradeID original_trade_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TradeCancelOrBust, 24);
    
    struct TradeCorrection {
        static const char* name() { return "XDP::TradeCorrection"; }
        MessageHeader msghdr;
        Time source_time;
        TimeNS source_time_ns;
        SymbolIndex symbol_index;
        SymbolSeqNum symbol_seqnum;
        TradeID original_trade_id;
        TradeID new_trade_id;
        Price price;
        Volume volume;
        TradeCondition1 trade_condition1;
        TradeCondition2 trade_condition2;
        TradeCondition3 trade_condition3;
        TradeCondition4 trade_condition4;
        TradeThroughExempt trade_through_exempt;
        TransactionID transaction_id;
        Tick tick;
        SellerDays seller_days;
        StopStockIndicator stop_stock_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TradeCorrection, 48);
    
    struct StockSummary {
        static const char* name() { return "XDP::StockSummary"; }
        MessageHeader msghdr;
        Time source_time;
        TimeNS source_time_ns;
        SymbolIndex symbol_index;
        Price high_price;
        Price low_price;
        Price open_price;
        Price close_price;
        Volume total_volume;
    } __attribute__((packed));
    I01_ASSERT_SIZE(StockSummary, 36);

} } } } }
