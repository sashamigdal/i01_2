#pragma once

#include <cstdint>

#include <i01_core/macro.hpp>

#include "Types.hpp"

namespace i01 { namespace OE { namespace EDGE { namespace XPRS130 { namespace Messages {
    using namespace i01::OE::EDGE::XPRS130::Types;

    /* SESSION API: */

    struct MEPLoginRequest {
        PackageLength                   package_length;
        PackageType                     package_type;
        Username                        username;
        Password                        password;
        Session                         requested_session;
        SequenceNumber                  requested_sequence_number;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPLoginRequest, 49);

    struct MEPLoginAccepted {
        PackageLength                   package_length;
        PackageType                     package_type;
        Session                         session;
        SequenceNumber                  sequence_number;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPLoginAccepted, 33);

    struct MEPLoginRejected {
        PackageLength                   package_length;
        PackageType                     package_type;
        RejectReasonCode                reject_reason_code;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPLoginRejected, 4);

    struct MEPSequencedData {
        PackageLength                   package_length;
        PackageType                     package_type;
        std::uint8_t                    message[];
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPSequencedData, 3);

    struct MEPUnsequencedData {
        PackageLength                   package_length;
        PackageType                     package_type;
        std::uint8_t                    message[];
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPUnsequencedData, 3);

    struct MEPServerHeartbeat {
        PackageLength                   package_length;
        PackageType                     package_type;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPServerHeartbeat, 3);

    struct MEPClientHeartbeat {
        PackageLength                   package_length;
        PackageType                     package_type;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPClientHeartbeat, 3);

    struct MEPLogoutRequest {
        PackageLength                   package_length;
        PackageType                     package_type;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPLogoutRequest, 3);

    struct MEPDebug {
        PackageLength                   package_length;
        PackageType                     package_type;
        std::uint8_t                    text[];
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPDebug, 3);

    struct MEPEndOfSession {
        PackageLength                   package_length;
        PackageType                     package_type;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MEPEndOfSession, 3);

    /* ORDER API: */

    struct EnterOrderShortFormat {
        MessageType                     message_type;
        OrderToken                      order_token;
        BuySellIndicator                buy_sell_indicator;
        Quantity                        quantity;
        Symbol                          symbol;
        Price                           price;
        TimeInForce                     time_in_force;
        Display                         display;
        SpecialOrderType                special_order_type;
        ExtendedHoursEligible           extended_hours_eligible;
        Capacity                        capacity;
        RouteOutEligibility             route_out_eligibility;
        ISOEligibility                  iso_eligibility;
    } __attribute__((packed));
    I01_ASSERT_SIZE(EnterOrderShortFormat, 37);

    struct EnterOrderExtendedFormat {
        MessageType                     message_type;
        OrderToken                      order_token;
        BuySellIndicator                buy_sell_indicator;
        Quantity                        quantity;
        Symbol                          symbol;
        Price                           price;
        TimeInForce                     time_in_force;
        Display                         display;
        SpecialOrderType                special_order_type;
        ExtendedHoursEligible           extended_hours_eligible;
        Capacity                        capacity;
        RouteOutEligibility             route_out_eligibility;
        ISOEligibility                  iso_eligibility;
        RoutingDeliveryMethod           routing_delivery_method;
        RouteStrategy                   route_strategy;
        Quantity                        minimum_quantity;
        Quantity                        max_floor;
        PegDifference                   peg_difference;
        DiscretionaryOffset             discretionary_offset;
        Timestamp                       expire_time;
        Symbol                          symbol_suffix;
    } __attribute__((packed));
    I01_ASSERT_SIZE(EnterOrderExtendedFormat, 64);

    struct AcceptedMessageShortFormat {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        BuySellIndicator                buy_sell_indicator;
        Quantity                        quantity;
        Symbol                          symbol;
        Price                           price;
        TimeInForce                     time_in_force;
        Display                         display;
        SpecialOrderType                special_order_type;
        ExtendedHoursEligible           extended_hours_eligible;
        OrderReferenceNumber            order_reference_number;
        Capacity                        capacity;
        RouteOutEligibility             route_out_eligibility;
        ISOEligibility                  iso_eligibility;
    } __attribute__((packed));
    I01_ASSERT_SIZE(AcceptedMessageShortFormat, 53);

    struct AcceptedMessageExtendedFormat {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        BuySellIndicator                buy_sell_indicator;
        Quantity                        quantity;
        Symbol                          symbol;
        Price                           price;
        TimeInForce                     time_in_force;
        Display                         display;
        SpecialOrderType                special_order_type;
        ExtendedHoursEligible           extended_hours_eligible;
        OrderReferenceNumber            order_reference_number;
        Capacity                        capacity;
        RouteOutEligibility             route_out_eligibility;
        ISOEligibility                  iso_eligibility;
        RoutingDeliveryMethod           routing_delivery_method;
        RouteStrategy                   route_strategy;
        Quantity                        minimum_quantity;
        Quantity                        max_floor;
        PegDifference                   peg_difference;
        DiscretionaryOffset             discretionary_offset;
        Timestamp                       expire_time;
        Symbol                          symbol_suffix;
    } __attribute__((packed));
    I01_ASSERT_SIZE(EnterOrderExtendedFormat, 64);

    struct ExecutedOrder {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        Quantity                        executed_quantity;
        Price                           execution_price;
        LiquidityFlag                   liquidity_flag;
        MatchNumber                     match_number;
    } __attribute__((packed));
    I01_ASSERT_SIZE(ExecutedOrder, 44);

    struct RejectedMessage {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        RejectReason                    reject_reason;
    } __attribute__((packed));
    I01_ASSERT_SIZE(RejectedMessage, 24);

    struct ExtendedRejectedMessage {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        RejectReason                    reject_reason;
        InResponseTo                    in_response_to;
    } __attribute__((packed));
    I01_ASSERT_SIZE(ExtendedRejectedMessage, 25);

    struct Cancel {
        MessageType                     message_type;
        OrderToken                      order_token;
        Quantity                        quantity;
    } __attribute__((packed));
    I01_ASSERT_SIZE(Cancel, 19);

    struct CanceledMessage {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        Quantity                        decremented_quantity;
        CanceledReason                  canceled_reason;
    } __attribute__((packed));
    I01_ASSERT_SIZE(CanceledMessage, 28);

    struct CancelPending {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
    } __attribute__((packed));
    I01_ASSERT_SIZE(CancelPending, 23);

    struct Replace {
        MessageType                     message_type;
        OrderToken                      existing_order_token;
        OrderToken                      replacement_order_token;
        Quantity                        quantity;
        Price                           price;
    } __attribute__((packed));
    I01_ASSERT_SIZE(Replace, 37);

    struct ReplacedMessage {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      replacement_order_token;
        BuySellIndicator                buy_sell_indicator;
        Quantity                        quantity;
        Symbol                          symbol;
        Price                           price;
        OrderReferenceNumber            order_reference_number;
        Capacity                        capacity;
        OrderToken                      previous_order_token;
    } __attribute__((packed));
    I01_ASSERT_SIZE(ReplacedMessage, 61);

    struct PendingReplace {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
    } __attribute__((packed));
    I01_ASSERT_SIZE(PendingReplace, 23);

    struct SystemEvent {
        MessageType                     message_type;
        Timestamp                       timestamp;
        EventCode                       event_code;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SystemEvent, 10);

    struct BrokenTrade {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        MatchNumber                     match_number;
        BrokenTradeReason               broken_trade_reason;
    } __attribute__((packed));
    I01_ASSERT_SIZE(BrokenTrade, 32);

    struct PriceCorrection {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        MatchNumber                     match_number;
        Price                           new_execution_price;
        PriceCorrectionReason           price_correction_reason;
    } __attribute__((packed));
    I01_ASSERT_SIZE(PriceCorrection, 36);

    struct AntiInternalizationModifier {
        MessageType                     message_type;
        AIMethod                        ai_method;
        AIIdentifier                    ai_identifier;
        AIGroupID                       ai_group_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(AntiInternalizationModifier, 5);

    struct AIAdditionalInfoMessage {
        MessageType                     message_type;
        Timestamp                       timestamp;
        OrderToken                      order_token;
        Quantity                        quantity;
        Price                           price;
        CanceledOrderState              canceled_order_state;
        MemberID                        contra_member_id;
        ContraTokenOrClOrderID          contra_token_or_cl_order_id;
        AIMethod                        ai_method;
        AIIdentifier                    ai_identifier;
        AIGroupID                       ai_group_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(AIAdditionalInfoMessage, 62);


} } } } }

