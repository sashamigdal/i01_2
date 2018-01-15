#pragma once

#include <cstdint>
#include <iostream>

#include <i01_core/macro.hpp>

#include "Types.hpp"

namespace i01 { namespace OE { namespace BATS { namespace BOE20 { namespace Messages {
    using namespace i01::OE::BATS::BOE20::Types;

    struct MessageHeader {
        StartOfMessage              start_of_message;
        MessageLength               message_length;
        MessageType                 message_type;
        MatchingUnit                matching_unit;
        SequenceNumber              sequence_number;
        // need this struct to have trivial constructor
        static MessageHeader create(MessageType mt = MessageType::CLIENT_HEARTBEAT, SequenceNumber sn = 0);
    } __attribute__((packed));
    I01_ASSERT_SIZE(MessageHeader, 10);
std::ostream & operator<<(std::ostream &os, const MessageHeader &m);

struct UnitNumberSequencePair {
    MatchingUnit                    unit_number;
    SequenceNumber                  unit_sequence;
} __attribute__((packed));
I01_ASSERT_SIZE(UnitNumberSequencePair, 5);
std::ostream & operator<<(std::ostream &os, const UnitNumberSequencePair &m);

struct UnitSequenceNumberSection {
    NumberOfUnits                       number_of_units;
    UnitNumberSequencePair              units[];
} __attribute__((packed));
I01_ASSERT_SIZE(UnitSequenceNumberSection, 1);
std::ostream & operator<<(std::ostream &os, const UnitSequenceNumberSection &m);

struct ParamGroupHeader {
    ParamGroupLength            length;
    ParamGroupType              type;
} __attribute__((packed));
I01_ASSERT_SIZE(ParamGroupHeader, 3);
std::ostream & operator<<(std::ostream &os, const ParamGroupHeader &m);

struct ParamGroup {
    ParamGroupHeader            header;
    std::uint8_t                data[];
} __attribute__((packed));
I01_ASSERT_SIZE(ParamGroup, 3);
std::ostream & operator<<(std::ostream &os, const ParamGroup &m);

struct UnitSequencesParamGroup {
    ParamGroupHeader            header;
    BooleanFlag                 no_unspecified_unit_replay;
    UnitSequenceNumberSection   unit_sequence_number;
} __attribute__((packed));
I01_ASSERT_SIZE(UnitSequencesParamGroup, 5);
std::ostream & operator<<(std::ostream &os, const UnitSequencesParamGroup &m);

struct ReturnBitfield {
    std::uint8_t                num_bitfields;
    union Bits {
        std::uint64_t           u64;
        std::uint8_t            u8[8];
    } __attribute__((packed)) bitfields;
} __attribute__((packed));
I01_ASSERT_SIZE(ReturnBitfield, 9);
std::ostream & operator<<(std::ostream &os, const ReturnBitfield &m);

struct ReturnBitfieldsParamGroup {
    ParamGroupHeader            header;
    MessageType                 message_type;
    ReturnBitfield              bitfields;
} __attribute__((packed));
I01_ASSERT_SIZE(ReturnBitfieldsParamGroup, 13);
std::ostream & operator<<(std::ostream &os, const ReturnBitfieldsParamGroup &m);

struct VariableLengthBitfield {
    std::uint8_t                num_bitfields;
    std::uint8_t                bitfields[];
} __attribute__((packed));
I01_ASSERT_SIZE(VariableLengthBitfield, 1);
std::ostream & operator<<(std::ostream &os, const VariableLengthBitfield &m);

    struct LoginRequest {
        MessageHeader                   header;
        SessionSubID                    session_sub_id;
        Username                        username;
        Password                        password;
        NumParamGroups                  num_param_groups;
        std::uint8_t                    param_groups[];

    } __attribute__((packed));
    I01_ASSERT_SIZE(LoginRequest, 29);
std::ostream & operator<<(std::ostream &os, const LoginRequest &m);

    struct LogoutRequest {
        MessageHeader                   message_header;
        static LogoutRequest create();
    } __attribute__((packed));
    I01_ASSERT_SIZE(LogoutRequest, 10);
std::ostream & operator<<(std::ostream &os, const LogoutRequest &m);


    struct ClientHeartbeat {
        MessageHeader                   message_header;
        // ClientHeartbeat() : message_header(MessageType::CLIENT_HEARTBEAT) {}
        static ClientHeartbeat create();
    } __attribute__((packed));
    I01_ASSERT_SIZE(ClientHeartbeat, 10);
std::ostream & operator<<(std::ostream &os, const ClientHeartbeat &m);

    struct LoginResponse {
        MessageHeader                   message_header;
        LoginResponseStatus             login_response_status;
        ResponseText                    login_response_text;
        BooleanFlag                     no_unspecified_unit_replay;
        SequenceNumber                  last_received_sequence_number;
        UnitSequenceNumberSection       unit_sequence_number;
        // NumParamGroups                  num_param_groups;
        // std::uint8_t                    param_groups[];
    } __attribute__((packed));
    I01_ASSERT_SIZE(LoginResponse, 77);
std::ostream & operator<<(std::ostream &os, const LoginResponse &m);

    struct Logout {
        MessageHeader                   message_header;
        LogoutReason                    logout_reason;
        ResponseText                    logout_reason_text;
        SequenceNumber                  last_received_sequence_number;
        UnitSequenceNumberSection       unit_sequence_number;
    } __attribute__((packed));
    I01_ASSERT_SIZE(Logout, 76);
std::ostream & operator<<(std::ostream &os, const Logout &m);

    struct ServerHeartbeat {
        MessageHeader                   message_header;
    } __attribute__((packed));
    I01_ASSERT_SIZE(ServerHeartbeat, 10);
std::ostream & operator<<(std::ostream &os, const ServerHeartbeat &m);

    struct ReplayComplete {
        MessageHeader                   message_header;
    } __attribute__((packed));
    I01_ASSERT_SIZE(ReplayComplete, 10);
std::ostream & operator<<(std::ostream &os, const ReplayComplete &m);

    struct NewOrder {
        MessageHeader                   message_header;
        ClOrdID                         cl_ord_id;
        Side                            side;
        Quantity                        order_qty;
        std::uint8_t                    num_bitfields;
        // OPTIONAL FIELDS
        // VariableLengthBitfield          new_order_bitfields;
        struct OptionalFields {
            ClearingFirm clearing_firm;
            ClearingAccount clearing_account;
            Price price;
            ExecInst exec_inst;
            OrdType ord_type;
            TimeInForce time_in_force;
            MinQty min_qty;
            MaxFloor max_floor;
            Symbol symbol;
            Capacity capacity;
            RoutingInst routing_inst;
            Account account;
            DisplayIndicator display_indicator;
            MaxRemovePct max_remove_pct;
            DiscretionAmount discretion_amount;
            PegDifference peg_difference;
            AttributedQuote attributed_quote;
            ExtExecInst ext_exec_inst;
        } __attribute__ ((packed));

    } __attribute__((packed));
    I01_ASSERT_SIZE(NewOrder, 36);
std::ostream & operator<<(std::ostream &os, const NewOrder &m);

    struct CancelOrder {
        MessageHeader                   message_header;
        ClOrdID                         orig_cl_ord_id;
        std::uint8_t                    num_bitfields;
        // VariableLengthBitfield          cancel_order_bitfields;

        struct OptionalFields {
            ClearingFirm clearing_firm;
        } __attribute__((packed));

    } __attribute__((packed));
    I01_ASSERT_SIZE(CancelOrder, 31);
std::ostream & operator<<(std::ostream &os, const CancelOrder &m);

    struct ModifyOrder {
        MessageHeader                   message_header;
        ClOrdID                         new_cl_ord_id;
        ClOrdID                         orig_cl_ord_id;
        std::uint8_t                    num_bitfields;

        struct OptionalFields {
            ClearingFirm clearing_firm;
            OrderQty order_qty;
            Price price;
            OrdType ord_type;
            CancelOrigOnReject cancel_orig_on_reject;
            ExecInst exec_inst;
            Side side;
            // bitfield 2
            MaxFloor max_floor;
        } __attribute__((packed));
    } __attribute__((packed));
    I01_ASSERT_SIZE(ModifyOrder, 51);
std::ostream & operator<<(std::ostream &os, const ModifyOrder &m);

    struct OrderAcknowledgment {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        OrderID                         order_id;
        std::uint8_t                    reserved;
        std::uint8_t                    number_of_return_bitfields;

        // these are the optional fields we are using in i01
        struct OptionalFields {
            Side side;
            PegDifference peg_difference;
            Price price;
            ExecInst exec_inst;
            OrdType ord_type;
            TimeInForce time_in_force;
            MinQty min_qty;
            MaxRemovePct max_remove_pct;
            Symbol symbol;
            Capacity capacity;
            Account account;
            ClearingFirm clearing_firm;
            ClearingAccount clearing_account;
            DisplayIndicator display_indicator;
            MaxFloor max_floor;
            DiscretionAmount discretion_amount;
            OrderQty order_qty;
            LeavesQty leaves_qty;
            DisplayPrice display_price;
            WorkingPrice working_price;
            AttributedQuote attributed_quote;
            ExtExecInst ext_exec_inst;
            RoutingInst routing_inst;
        } __attribute__ ((packed));
        // I01_ASSERT_SIZE(OptionalFields, 4);

    } __attribute__((packed));
    I01_ASSERT_SIZE(OrderAcknowledgment, 48);
std::ostream & operator<<(std::ostream &os, const OrderAcknowledgment &m);


    struct OrderRejected {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        ReasonCode                      order_reject_reason;
        ResponseText                    text;
        std::uint8_t                    reserved;
        // VariableLengthBitfield          order_rejected_bitfields;
        std::uint8_t                    number_of_return_bitfields;

        struct OptionalFields {
            Side side;
            PegDifference peg_difference;
            Price price;
            ExecInst exec_inst;
            OrdType ord_type;
            TimeInForce time_in_force;
            MinQty min_qty;
            MaxRemovePct max_remove_pct;
            Symbol symbol;
            Capacity capacity;
            Account account;
            ClearingFirm clearing_firm;
            ClearingAccount clearing_account;
            DisplayIndicator display_indicator;
            MaxFloor max_floor;
            DiscretionAmount discretion_amount;
            OrderQty order_qty;
            AttributedQuote attributed_quote;
            ExtExecInst ext_exec_inst;
            RoutingInst routing_inst;
        } __attribute__ ((packed));


    } __attribute__((packed));
    I01_ASSERT_SIZE(OrderRejected, 101);
std::ostream & operator<<(std::ostream &os, const OrderRejected &m);

    struct OrderModified {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        OrderID                         order_id;
        std::uint8_t                    reserved;
        std::uint8_t                    number_of_return_bitfields;

        struct OptionalFields {
            Side side;
            PegDifference peg_difference;
            Price price;
            ExecInst exec_inst;
            OrdType ord_type;
            TimeInForce time_in_force;
            MinQty min_qty;
            MaxRemovePct max_remove_pct;
            Account account;
            ClearingFirm clearing_firm;
            ClearingAccount clearing_account;
            DisplayIndicator display_indicator;
            MaxFloor max_floor;
            DiscretionAmount discretion_amount;
            OrderQty order_qty;
            OrigClOrdID orig_cl_ord_id;
            LeavesQty leaves_qty;
            DisplayPrice display_price;
            WorkingPrice working_price;
            AttributedQuote attributed_quote;
            ExtExecInst ext_exec_inst;
            RoutingInst routing_inst;
        } __attribute__ ((packed));


    } __attribute__((packed));
    I01_ASSERT_SIZE(OrderModified, 48);
std::ostream & operator<<(std::ostream &os, const OrderModified &m);

    struct OrderRestated {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        OrderID                         order_id;
        RestatementReason               restatement_reason;
        std::uint8_t                    reserved;
        std::uint8_t                    number_of_return_bitfields;

        struct OptionalFields {
            Side side;
            PegDifference peg_difference;
            Price price;
            ExecInst exec_inst;
            OrdType ord_type;
            TimeInForce time_in_force;
            MinQty min_qty;
            MaxRemovePct max_remove_pct;
            Symbol symbol;
            Capacity capacity;
            Account account;
            ClearingFirm clearing_firm;
            ClearingAccount clearing_account;
            DisplayIndicator display_indicator;
            MaxFloor max_floor;
            DiscretionAmount discretion_amount;
            OrderQty order_qty;
            OrigClOrdID orig_cl_ord_id;
            LeavesQty leaves_qty;
            DisplayPrice display_price;
            WorkingPrice working_price;
            AttributedQuote attributed_quote;
            ExtExecInst ext_exec_inst;
            RoutingInst routing_inst;
        } __attribute__ ((packed));

    } __attribute__((packed));
    I01_ASSERT_SIZE(OrderRestated, 49);
std::ostream & operator<<(std::ostream &os, const OrderRestated &m);

    struct UserModifyRejected {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        ReasonCode                      modify_reject_reason;
        ResponseText                    text;
        std::uint8_t                    reserved;
        VariableLengthBitfield          order_rejected_bitfields;
    } __attribute__((packed));
    I01_ASSERT_SIZE(UserModifyRejected, 101);
std::ostream & operator<<(std::ostream &os, const UserModifyRejected &m);

    struct OrderCancelled {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        ReasonCode                      cancel_reason;
        std::uint8_t                    reserved;
        // VariableLengthBitfield          order_cancelled_bitfields;
        std::uint8_t                    number_of_return_bitfields;

        struct OptionalFields {
            Side side;
            PegDifference peg_difference;
            Price price;
            ExecInst exec_inst;
            OrdType ord_type;
            TimeInForce time_in_force;
            MinQty min_qty;
            MaxRemovePct max_remove_pct;
            Symbol symbol;
            Capacity capacity;
            Account account;
            ClearingFirm clearing_firm;
            ClearingAccount clearing_account;
            DisplayIndicator display_indicator;
            MaxFloor max_floor;
            DiscretionAmount discretion_amount;
            OrderQty order_qty;
            LeavesQty leaves_qty;
            AttributedQuote attributed_quote;
            ExtExecInst ext_exec_inst;
            RoutingInst routing_inst;
        } __attribute__ ((packed));


    } __attribute__((packed));
    I01_ASSERT_SIZE(OrderCancelled, 41);
std::ostream & operator<<(std::ostream &os, const OrderCancelled &m);

    struct CancelRejected {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        ReasonCode                      cancel_reject_reason;
        ResponseText                    text;
        std::uint8_t                    reserved;
        std::uint8_t                    number_of_return_bitfields;

        struct OptionalFields {
            Side side;
            PegDifference peg_difference;
            Price price;
            ExecInst exec_inst;
            OrdType ord_type;
            TimeInForce time_in_force;
            MinQty min_qty;
            MaxRemovePct max_remove_pct;
            Symbol symbol;
            Capacity capacity;
        } __attribute__ ((packed));

    } __attribute__((packed));
    I01_ASSERT_SIZE(CancelRejected, 101);
std::ostream & operator<<(std::ostream &os, const CancelRejected &m);

    struct OrderExecution {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        ExecID                          exec_id;
        Quantity                        last_shares;
        Price                           last_px;
        Quantity                        leaves_qty;
        BaseLiquidityIndicator          base_liquidity_indicator;
        SubLiquidityIndicator           sub_liquidity_indicator;
        ContraBroker                    contra_broker;
        std::uint8_t                    reserved;
        std::uint8_t                    number_of_return_bitfields;


        struct OptionalFields {
            Side side;
            PegDifference peg_difference;
            Price price;
            ExecInst exec_inst;
            OrdType ord_type;
            TimeInForce time_in_force;
            MinQty min_qty;
            MaxRemovePct max_remove_pct;
            Symbol symbol;
            Capacity capacity;
            Account account;
            ClearingFirm clearing_firm;
            ClearingAccount clearing_account;
            DisplayIndicator display_indicator;
            MaxFloor max_floor;
            DiscretionAmount discretion_amount;
            OrderQty order_qty;
            AttributedQuote attributed_quote;
            ExtExecInst ext_exec_inst;
            FeeCode fee_code;
            RoutingInst routing_inst;
        } __attribute__ ((packed));

    } __attribute__((packed));
    I01_ASSERT_SIZE(OrderExecution, 70);
std::ostream & operator<<(std::ostream &os, const OrderExecution &m);

    struct TradeCancelOrCorrect {
        MessageHeader                   message_header;
        DateTime                        transaction_time;
        ClOrdID                         cl_ord_id;
        OrderID                         order_id;
        ExecID                          exec_ref_id;
        Side                            side;
        BaseLiquidityIndicator          base_liquidity_indicator;
        ClearingFirm                    clearing_firm;
        ClearingAccount                 clearing_account;
        Quantity                        last_shares;
        Price                           last_px;
        Price                           corrected_price;
        DateTime                        orig_time;
        std::uint8_t                    reserved;
        std::uint8_t                    number_of_return_bitfields;

        struct OptionalFields {
            Symbol symbol;
            Capacity capacity;
        } __attribute__ ((packed));

    } __attribute__((packed));
    I01_ASSERT_SIZE(TradeCancelOrCorrect, 94);
std::ostream & operator<<(std::ostream &os, const TradeCancelOrCorrect &m);

} } } } }
