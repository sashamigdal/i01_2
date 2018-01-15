#pragma once

/* NYSE OpenBook Ultra
 * 1.9 / 7 Oct 2013
 * and NYSE Order Imbalances Client Spec 1.12 / 9 Nov 2012
 */

#include <cstdint>
#include <i01_core/macro.hpp>
#include <i01_md/NYSE/PDP/Types.hpp>

namespace i01 { namespace MD { namespace NYSE { namespace PDP { namespace Messages {

    using namespace i01::MD::NYSE::PDP::Types;

    struct MessageHeader {
        MsgSize msg_size;
        MsgType msg_type;
        MsgSeqNum msg_seq_num;
        MillisecondsSinceMidnight send_time;
        ProductID product_id; /* = 116 for Order Imbalances */
        RetransFlag retrans_flag;
        NumBodyEntries num_body_entries;
        LinkFlag link_flag;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MessageHeader, 16);

    struct FullUpdate {
        static const MsgType i01_msg_type = MsgType::OPENBOOK_FULL_UPDATE;
        MsgSize msg_size;
        SecurityIndex security_index;
        MillisecondsSinceMidnight source_time;
        Microseconds source_time_microseconds;
        SymbolSeqNum symbol_seqnum;
        SourceSessionID source_session_id;
        Symbol symbol;
        PriceScaleCode price_scale_code;
        QuoteCondition quote_condition;
        TradingStatus trading_status;
        std::uint8_t filler;
        MPV mpv;
        /* Followed by 0 or more FullUpdatePricePoints... */
    } __attribute__((packed));
    I01_ASSERT_SIZE(FullUpdate, 32);
std::ostream & operator<<(std::ostream &os, const FullUpdate &msg);

    struct FullUpdatePricePoint {
        PriceNumerator price_numerator;
        Volume volume;
        NumOrders num_orders;
        Side side;
        std::uint8_t filler;
    } __attribute__((packed));
    I01_ASSERT_SIZE(FullUpdatePricePoint, 12);
std::ostream & operator<<(std::ostream &os, const FullUpdatePricePoint &msg);

    struct DeltaUpdate {
        static const MsgType i01_msg_type = MsgType::OPENBOOK_DELTA_UPDATE;
        MsgSize msg_size;
        SecurityIndex security_index;
        MillisecondsSinceMidnight source_time;
        Microseconds source_time_microseconds;
        SymbolSeqNum symbol_seqnum;
        SourceSessionID source_session_id;
        QuoteCondition quote_condition;
        TradingStatus trading_status;
        PriceScaleCode price_scale_code;
    } __attribute__((packed));
    I01_ASSERT_SIZE(DeltaUpdate, 18);
std::ostream & operator<<(std::ostream &os, const DeltaUpdate &msg);

    struct DeltaUpdatePricePoint {
        PriceNumerator price_numerator;
        Volume volume;
        Volume chg_qty;
        NumOrders num_orders;
        Side side;
        DeltaReasonCode reason_code;
        LinkID link_id1;
        LinkID link_id2;
        LinkID link_id3;
    } __attribute__((packed));
    I01_ASSERT_SIZE(DeltaUpdatePricePoint, 28);
std::ostream & operator<<(std::ostream &os, const DeltaUpdatePricePoint &msg);

    struct SequenceNumberReset {
        static const MsgType i01_msg_type = MsgType::SEQUENCE_NUMBER_RESET;
        MsgSeqNum next_seqnum;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SequenceNumberReset, 4);
std::ostream & operator<<(std::ostream &os, const SequenceNumberReset &msg);

    // Retransmission / Refresh related:

    struct HeartbeatSubscription {
        SourceID source_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(HeartbeatSubscription, 20);

    // TODO: Heartbeat.

    struct HeartbeatResponse {
        static const MsgType i01_msg_type = MsgType::HEARTBEAT_RESPONSE;
        SourceID source_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(HeartbeatResponse, 20);

    struct RetransmissionRequest {
        static const MsgType i01_msg_type = MsgType::RETRANSMISSION_REQUEST;
        MsgSeqNum begin_seqnum;
        MsgSeqNum end_seqnum;
        SourceID source_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(RetransmissionRequest, 28);

    struct SymbolIndexMappingRequest {
        SourceID source_id;
        SecurityIndex security_index;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SymbolIndexMappingRequest, 22);

    struct BookRefreshRequest {
        static const MsgType i01_msg_type = MsgType::REFRESH_REQUEST;
        RefreshSymbol symbol;
        SourceID source_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(BookRefreshRequest, 36);

    struct ExtendedBookRefreshRequest {
        SourceID source_id;
        SecurityIndex security_index;
        MsgType msg_type; /* not implemented; 0 = all, or 230 = full update */
    } __attribute__((packed));
    I01_ASSERT_SIZE(ExtendedBookRefreshRequest, 24);

    struct RetransmissionResponse {
        static const MsgType i01_msg_type = MsgType::RETRANSMISSION_RESPONSE;
        MsgSeqNum source_seqnum;
        SourceID source_id;
        AcceptReject status;
        RejectReason reject_reason;
        std::uint8_t filler[2];
    } __attribute__((packed));
    I01_ASSERT_SIZE(RetransmissionResponse, 28);

    struct SymbolIndexMapping {
        Symbol symbol;
        std::uint8_t filler;
        SecurityIndex security_index;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SymbolIndexMapping, 14);
std::ostream & operator<<(std::ostream &os, const SymbolIndexMapping &msg);

    struct MessageUnavailable {
        static const MsgType i01_msg_type = MsgType::MESSAGE_UNAVAILABLE;
        MsgSeqNum begin_seqnum;
        MsgSeqNum end_seqnum;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MessageUnavailable, 8);

    struct OpeningImbalance {
        static const MsgType i01_msg_type = MsgType::NYSE_OPENING_IMBALANCE;
        Symbol symbol;
        Indicator01 stock_open_indicator;
        ImbalanceSide imbalance_side;
        PriceScaleCode price_scale_code;
        PriceNumerator reference_price_numerator;
        Quantity imbalance_quantity;
        Quantity paired_quantity;
        PriceNumerator clearing_price_numerator;
        MillisecondsSinceMidnight source_time;
        PriceNumerator ssr_filing_price_numerator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(OpeningImbalance, 38);
std::ostream & operator<<(std::ostream &os, const OpeningImbalance &msg);

    struct ClosingImbalance {
        static const MsgType i01_msg_type = MsgType::NYSE_CLOSING_IMBALANCE;
        Symbol symbol;
        Indicator01 regulatory_imbalance_indicator;
        ImbalanceSide imbalance_side;
        PriceScaleCode price_scale_code;
        PriceNumerator reference_price_numerator;
        Quantity imbalance_quantity;
        Quantity paired_quantity;
        PriceNumerator continuous_book_clearing_price_numerator;
        PriceNumerator closing_only_clearing_price_numerator;
        MillisecondsSinceMidnight source_time;
    } __attribute__((packed));
    I01_ASSERT_SIZE(ClosingImbalance, 38);
std::ostream & operator<<(std::ostream &os, const ClosingImbalance &msg);

}}}}}
