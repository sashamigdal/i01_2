#pragma once

#include <cstdint>

#include <i01_core/macro.hpp>

#include "Types.hpp"

namespace i01 { namespace OE { namespace ARCA { namespace ArcaDirect41 { namespace Messages {
    using namespace i01::OE::ARCA::ArcaDirect41::Types;

    /* SESSION MESSAGES */

    struct LogonRequestVariant1 {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;

        /// 0 = replay all messages, -1 = do not replay but continue,
        //  N>0 = replay from specified seqnum
        SeqNum                            last_seqnum;
        Username                          username;
        Symbology                         symbology;
        MessageVersionProfile             message_version_profile;
        CancelOnDisconnect                cancel_on_disconnect;
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(LogonRequestVariant1, 48);

    struct LogonRequestVariant2Full {
        MessageType                       message_type;
        Variant                           variant; /* = 2 */
        Length                            length;
        SeqNum                            seqnum;

        /// 0 = replay all messages, -1 = do not replay but continue,
        //  N>0 = replay from specified seqnum
        SeqNum                            last_seqnum;
        SessionProfileBitmap              session_profile_bitmap;
        Username                          username;
        MessageVersionProfile             message_version_profile; /* = 15 (be) */
        CancelOnDisconnect                cancel_on_disconnect;
        DefaultExtendedExecInst           default_extended_exec_inst;
        ProactiveIfLocked                 default_proactive_if_locked;
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(LogonRequestVariant2Full, 22 + 31);

    struct LogonReject {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum; /* ignore */

        /// 0 = replay all messages, -1 = do not replay but continue,
        //  N>0 = replay from specified seqnum
        SeqNum                            last_seqnum_server_received;
        SeqNum                            last_seqnum_server_sent;
        RejectType                        reject_type;
        std::uint8_t                      text[40];
        std::uint8_t                      filler;
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(LogonReject, 60);

    struct TestRequest { 
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum; /* ignore */
        std::uint8_t                      filler[3];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(TestRequest, 12);
    typedef TestRequest HeartbeatRequest;
    I01_ASSERT_SIZE(HeartbeatRequest, 12);

    struct Heartbeat {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum; /* ignore */
        std::uint8_t                      filler[3];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(Heartbeat, 12);

    /* APPLICATION MESSAGES */

    /// New Order Message Variant 1: Equities Only -- Small Message.
    struct NewOrderMessageVariant1 {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum; /* ignore */
        ClientOrderID                     client_order_id;
        PCSLinkID                         pcs_link_id;
        Quantity                          order_quantity;
        Price                             order_price;
        ExDestination                     ex_destination;
        PriceScale                        price_scale;
        Symbol                            symbol;
        CompID                            company_group_id;
        CompID                            deliver_to_comp_id;
        SubID                             sender_sub_id;
        ExecInst                          exec_inst;
        Side                              side;
        OrderType                         order_type;
        TimeInForce                       time_in_force;
        Rule80A                           rule_80a;
        SessionID                         trading_session_id;
        Account                           account;
        ISO                               iso;
        ExtendedExecInst                  extended_exec_inst;
        ExtendedPNP                       extended_pnp;
        NoSelfTrade                       no_self_trade;
        ProactiveIfLocked                 proactive_if_locked;
        std::uint8_t                      filler;
        MessageTerminator                 message_terminator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(NewOrderMessageVariant1, 76);

    /* TODO: New order message variants 2, 3, 4 */

    struct Cancel {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        OrderID                           order_id;
        ClientOrderID                     original_client_order_id;
        Price                             strike_price; /* null if equities */
        UnderQty                          under_qty;
        ExDestination                     ex_destination;
        CorporateAction                   corporate_action; /* null if equities */
        PutOrCall                         put_or_call; /* null if equities */
        BulkCancel                        bulk_cancel;
        OpenOrClose                       open_or_close; /* null if equities */
        Symbol                            symbol;
        StrikeDate                        strike_date;
        Side                              side;
        CompID                            deliver_to_comp_id;
        Account                           account;
        std::uint8_t                      filler[7];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(Cancel, 72);

    struct CancelReplaceVariant1 {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        OrderID                           order_id;
        ClientOrderID                     new_client_order_id;
        ClientOrderID                     original_client_order_id;
        Quantity                          order_qty;
        Price                             strike_price; /* null if equities */
        Price                             price;
        ExDestination                     ex_destination;
        UnderQty                          under_qty;
        PriceScale                        price_scale;
        PutOrCall                         put_or_call; /* null if equities */
        CorporateAction                   corporate_action; /* null if equities */
        OpenOrClose                       open_or_close; /* null if equities */
        Symbol                            symbol;
        StrikeDate                        strike_date;
        ExecInst                          exec_inst;
        Side                              side;
        OrderType                         order_type;
        TimeInForce                       time_in_force;
        Rule80A                           rule_80a;
        SessionID                         trading_session_id;
        CompID                            deliver_to_comp_id;
        Account                           account;
        std::uint8_t                      filler[3];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(CancelReplaceVariant1, 88);

    /* TODO: Order Cancel/Replace Verbose (Variant 2)
     *       Fast Cancel/Replace Message Options (Variant 3)
     */

    struct CancelReplaceVariant4 {
        MessageType                       message_type;
        Variant                           variant; /* = 4 */
        Length                            length;
        SeqNum                            seqnum;
        OrderID                           order_id;
        ClientOrderID                     original_client_order_id;
        Quantity                          order_qty;
        Price                             price;
        ExDestination                     ex_destination;
        PriceScale                        price_scale;
        Symbol                            symbol;
        Side                              side;
        SuppressAck                       suppress_ack; /* 'Y' to suppress ack */
        CompID                            deliver_to_comp_id;
        Account                           account;
        std::uint8_t                      filler[7];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(CancelReplaceVariant4, 64);

    struct CancelRequestAck {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     client_order_id;
        OrderID                           order_id;
        std::uint8_t                      filler[3];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(CancelRequestAck, 40);

    struct Killed {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     client_order_id;
        OrderID                           order_id;
        InformationText                   information_text; /* = '0' or '1' */
        std::uint8_t                      filler[2];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(Killed, 40);

    struct KilledSTP {
        MessageType                       message_type;
        Variant                           variant; /* = 2 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     client_order_id;
        OrderID                           order_id;
        Quantity                          last_shares;
        InformationText                   information_text; /* = '2' */
        std::uint8_t                      text[40];
        LiquidityIndicator                liquidity_indicator;
        std::uint8_t                      filler[5];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(KilledSTP, 88);

    struct CancelReplaceAck {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     original_client_order_id;
        OrderID                           order_id;
        std::uint8_t                      filler[3];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(CancelReplaceAck, 40);

    struct Replaced {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     new_client_order_id;
        OrderID                           order_id;
        std::uint8_t                      filler[3];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(Replaced, 40);

    struct CancelReplaceReject {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     client_order_id;
        ClientOrderID                     original_client_order_id;
        RejectedMessageType               rejected_message_type;
        std::uint8_t                      text[40];
        RejectReason                      reject_reason;
        std::uint8_t                      filler[5];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(CancelReplaceReject, 80);

    struct BustedOrCorrected {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     client_order_id;
        ExecutionID                       execution_id;
        Quantity                          order_quantity;
        Price                             price;
        PriceScale                        price_scale;
        BustType                          type;
        std::uint8_t                      filler[1];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(BustedOrCorrected, 48);

    struct Ack {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     original_client_order_id;
        OrderID                           order_id;
        Price                             price;
        PriceScale                        price_scale;
        AckLiquidityIndicator             liquidity_indicator;
        std::uint8_t                      filler[5];
        MessageTerminator                 message_terminator; /* '\n' */
    } __attribute__((packed));
    I01_ASSERT_SIZE(Ack, 48);

    struct FillVariant1 {
        MessageType                       message_type;
        Variant                           variant; /* = 1 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     client_order_id;
        OrderID                           order_id;
        ExecutionID                       execution_id;
        ArcaExID                          arca_ex_id;
        Quantity                          last_shares;
        Price                             last_price;
        PriceScale                        price_scale;
        LiquidityIndicator                liquidity_indicator;
        Side                              side;
        LastMkt                           last_mkt;
        std::uint8_t                      filler[10];
        MessageTerminator                 message_terminator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(FillVariant1, 88);

    struct FillVariant2 {
        MessageType                       message_type;
        Variant                           variant; /* = 2 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     client_order_id;
        OrderID                           order_id;
        ExecutionID                       execution_id;
        ExecutionID                       execution_ref_id;
        ArcaExID                          arca_ex_id;
        Quantity                          order_quantity;
        Price                             price;
        Quantity                          leaves;
        Quantity                          cum_qty;
        Price                             avg_px;
        Price                             stop_price;
        Price                             discretion_offset;
        Price                             peg_difference;
        Quantity                          last_shares;
        Price                             last_price;
        Price                             strike_price;
        PutOrCall                         put_or_call;
        OpenOrClose                       open_or_close;
        Symbol                            symbol;
        StrikeDate                        strike_date;
        ExecTransType                     exec_trans_type;
        OrderRejectReason                 order_reject_reason;
        OrderStatus                       order_status;
        ExecutionType                     execution_type;
        Side                              side;
        OrderType                         order_type;
        TimeInForce                       time_in_force;
        Account                           account;
        std::uint8_t                      text[40];
        DiscretionInstruction             discretion_instruction;

        /// For Order Ack, Cancel Pending, Cancelled, CancelReplaced Pending,
        //  and Replaced Acks (Arca Equities), use AckLiquidityIndicator.
        //  For partial fills and fills, use LiquidityIndicator.
        AckLiquidityIndicator             liquidity_indicator; // TODO: FIXME
        std::uint8_t                      exec_broker[5]; // TODO: FIXME
        LastMkt                           last_mkt;
        std::uint8_t                      filler[7];
        MessageTerminator                 message_terminator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(FillVariant2, 208);

    struct FillVariant3 {
        MessageType                       message_type;
        Variant                           variant; /* = 3 */
        Length                            length;
        SeqNum                            seqnum;
        SendingTime                       sending_time;
        TransactionTime                   transaction_time;
        ClientOrderID                     client_order_id;
        OrderID                           order_id;
        ExecutionID                       execution_id;
        ExecutionID                       execution_ref_id;
        ArcaExID                          arca_ex_id;
        LegRefID                          leg_ref_id;
        Quantity                          last_shares;
        Price                             last_price;
        ExecTransType                     exec_trans_type;
        OrderRejectReason                 order_reject_reason;
        ExecutionType                     execution_type;
        Side                              side;
        std::uint8_t                      text[40];

        /// TODO: FIXME: Add Options values (apdx. B)
        LiquidityIndicator                liquidity_indicator; // TODO: FIXME
        std::uint8_t                      filler[6];
        MessageTerminator                 message_terminator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(FillVariant3, 136);

    // TODO: Options Cross Execution Report (FillVariant4)

    // TODO: Order Cross (Options) - Single (Variant 1)
    // TODO: Order Cross (Options) - Complex (Variant 2)
    // TODO: Allocation (Options) - Variant 1
    // TODO: Allocation Ack (Options) - Variant 1
    // TODO: Risk Management Request Message (Options) - Variant 1
    // TODO: Risk Management Request Ack (Options) - Variant 1
    // TODO: Risk Management Alert Message (Options) - Variant 1

//////////////////
    // TODO: mapping NYSE Arca Equities Order Types to field values...

} } } } }

