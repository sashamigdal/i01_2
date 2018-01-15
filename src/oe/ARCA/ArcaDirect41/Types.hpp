#pragma once

#include <cstdint>
#include <array>

namespace i01 { namespace OE { namespace ARCA { namespace ArcaDirect41 { namespace Types {

    enum class MessageType : std::uint8_t {
        LOGON                               = 'A' /* Variants 1, 2 */
      , LOGON_REJECT                        = 'L' /* Variants 1 */
      , TEST_MESSAGE                        = '1' /* Variants 1 */
      , HEARTBEAT_MESSAGE                   = '0' /* Variants 1 */
      , ORDER_CROSS                         = 'B' /* Variants 1, 2 */
      , BUST_OR_CORRECT                     = 'C' /* Variants 1 */
      , NEW_ORDER                           = 'D' /* Variants 1, 2, 3, 4 */
      , CANCEL_REPLACE_ACK                  = 'E' /* Variants 1 */
      , CANCEL                              = 'F' /* Variants 1 */
      , CANCEL_REPLACE                      = 'G' /* Variants 1, 2, 3, 4 */
      , ALLOCATION                          = 'J' /* Variants 1 */
      , ALLOCATION_ACK                      = 'P' /* Variants 1 */
      , ORDER_ACK                           = 'a' /* Variants 1 */
      , ORDER_FILLED                        = '2' /* Variants 1, 2, 3, 4 */
      , ORDER_KILLED                        = '4' /* Variants 1, 2 */
      , ORDER_REPLACED                      = '5' /* Variants 1 */
      , CANCEL_REQUEST_ACK                  = '6' /* Variants 1 */
      , ORDER_REJECTED                      = '8' /* Variants 1 */
    };

    typedef std::uint8_t Variant;
    typedef std::uint16_t Length;
    typedef std::uint32_t SeqNum;
    typedef std::uint8_t Username[5];
    enum class Symbology : std::uint8_t {
        OCC_OSI_EXPLICIT_SYMBOLOGY = 2 /* for option series identification */
    };

    struct BytePair {
        std::uint8_t key;
        std::uint8_t value;
    };

    union MessageVersionProfile {
        std::uint8_t                  raw[28];
        BytePair                      pairs[14];
    };

    enum class CancelOnDisconnect : std::uint8_t {
        DO_NOT_CANCEL_ON_DISCONNECT   = 0
      , ENABLE_CANCEL_ON_DISCONNECT   = 1
    };

    enum class MessageTerminator : std::uint8_t {
        NEW_LINE = '\n'
    };

    enum class SessionProfileBitmap : std::uint32_t {
        MESSAGE_VERSION               = 1 << 0
      , CANCEL_ON_DISCONNECT          = 1 << 1
      , EXTENDED_EXEC_INST            = 1 << 2
      , PROACTIVE_IF_LOCKED           = 1 << 3
    };

    enum class DefaultExtendedExecInst : std::uint8_t {
        NO_INTERACTION_WITH_MPL_ORDERS                    = '0'
      , OPT_OUT_OF_INTERACTION_WITH_IOI_DARK_POOL_QUOTES  = '2'
      , ADD_LIQUIDITY_ONLY                                = 'A'
      , NONE                                              = 0x00
    };

    enum class ProactiveIfLocked : std::uint8_t {
        PROACTIVE_IF_LOCKED                                     = 'Y'
      , RE_PRICE_IF_PRICED_THROUGH_LULD                         = '1'
      , PROACTIVE_IF_LOCKED_AND_RE_PRICE_IF_PRICED_THROUGH_LULD = '2'
      , NONE                                                    = 0x00
    };

    enum class RejectType : std::uint16_t {
        SUCCESS                       = 0
      , SYSTEM_UNAVAILABLE            = 1
      , INVALID_SEQUENCE_NUMBER       = 2
      , CLIENT_SESSION_ALREADY_EXISTS = 3
      , CLIENT_SESSION_DISABLED       = 4
      , CONNECTION_TYPE               = 5
      , INVALID_IP_ADDRESS            = 10
    };

    /* APPLICATION */

    typedef std::uint32_t ClientOrderID;
    typedef std::uint32_t PCSLinkID;

    typedef std::uint32_t Quantity;
    typedef std::uint16_t UnderQty; // TODO: ask NYSE if this should be 2 or 4 bytes...

    typedef std::uint32_t Price;
    enum class ExDestination : std::uint16_t {
        NYSE_ARCA_EQUITIES    = 102
      , NYSE_ARCA_OPTIONS     = 103
      , NYSE_AMEX_OPTIONS     = 104
    };

    enum class PriceScale : std::uint8_t {
        ZERO   = '0'
      , ONE    = '1'
      , TWO    = '2'
      , THREE  = '3'
      , FOUR   = '4'
    };

    typedef std::uint8_t SymbolRaw[8];
    typedef std::uint64_t SymbolIntegral;
    union Symbol {
        SymbolRaw           raw;
        SymbolIntegral      integral;
    };

    typedef std::uint8_t CompID[5];
    typedef CompID SubID;

    // TODO: convert ExecInst and OrderType to enum class
    typedef std::uint8_t ExecInst;
    typedef std::uint8_t OrderType;

    enum class Side : std::uint8_t {
        BUY                         = '1'
      , SELL                        = '2'
      , SHORT                       = '5'
      , SHORT_EXEMPT /* SSR 201 */  = '6'
      , SHORT_AND_CANCEL            = 'S' /* when SSR 201 is in effect */
      , CROSS                       = '8'
      , CROSS_SHORT                 = '9'
    };

    enum class TimeInForce : std::uint8_t {
        DAY                         = '0'
      , OPENING_AUCTION_ONLY        = '2' /* arca primaries only */
      , IOC                         = '3'
      , FOK                         = '4'
      , CLOSING_AUCTION_ONLY        = '7' /* arca primary and PO+ only */
      , ROT_1_IOC                   = '8'
      , ROT_2_IOC                   = '9'
      , ROT_2_DAY_OR_MARKET         = 'A'
    };

    enum class Rule80A : std::uint8_t { /* value names from FIX spec: */
        AGENCY_SINGLE_ORDER                             = 'A'
      , PRINCIPAL                                       = 'P'
      , SHORT_EXEMPT_TRANSACTION_FOR_PRINCIPAL          = 'E'
      /* EXTENSION PER EQUITY CROSSING ORDERS SECTION OF SPEC: */
      , SHORT_EXEMPT_TRANSACTION_W_TYPE                 = 'F'
      , PROPRIETARY_ALGO                                = 'J'
      , SHORT_EXEMPT_TRANSACTION_MEMBER_NOT_AFFILIATED  = 'X'
      , AGENCY_ALGO                                     = 'K'
      , SHORT_EXEMPT_TRANSACTION_I_TYPE                 = 'H'
    };

    typedef std::uint8_t SessionIDRaw[4];
    typedef std::uint32_t SessionIDIntegral;
    union SessionID {
        SessionIDRaw      raw;
        SessionIDIntegral integral;
    };

    typedef std::uint8_t Account[10];
    enum class ISO : std::uint8_t {
        ISO_SWEEP   = 'Y'
      , NO_ISO_FLAG = 'N'
    };

    enum class ExtendedExecInst : std::uint8_t {
        NOT_SET                                           = 0x00
      , NO_MIDPOINT_EXECUTION                             = '0'
      , IOI_OPTING_OUT                                    = '2'
      , ADD_LIQUIDITY_ONLY                                = 'A'
      , PRIMARY_ORDER_PLUS                                = 'P'
      , PRIMARY_ORDER_PLUS_SHIPPING_INST                  = 'p'
      , RPI_ORDER                                         = 'R'
      , PL_SELECT_ORDER                                   = 'S'
    };

    enum class ExtendedPNP : std::uint8_t {
        NOT_SET             = 0x00
      , EXTENDED_PNP_PLUS   = 'P'
      , EXTENDED_PNP_BLIND  = 'B'
    };

    enum class NoSelfTrade : std::uint8_t {
        CANCEL_NEW          = 'N'
      , CANCEL_OLD          = 'O'
      , CANCEL_BOTH         = 'B'
      , CANCEL_DECREMENT    = 'D'
    };

    typedef std::uint64_t OrderID;

    enum class CorporateAction : std::uint8_t {
        NO_CORPORATE_CHANGES                  = 0
      , CREATED_FOR_A_CORPORATE_ACTION        = 1
    };

    enum class PutOrCall : std::uint8_t {
        PUT   = 0
      , CALL  = 1
    };

    enum class BulkCancel : std::uint8_t {
        NOT_SET               = 0
      , PERFORM_BULK_CANCEL   = 1
      , BULK_CANCEL_BY_ETPID  = 2
    };

    enum class OpenOrClose : std::uint8_t {
        OPEN      = 'O'
      , CLOSE     = 'C'
      , EQUITIES  = 0x00
    };

    typedef std::uint8_t StrikeDateRaw[8];
    typedef std::uint64_t StrikeDateIntegral;
    union StrikeDate {
        StrikeDateRaw raw;
        StrikeDateIntegral integral;
    };

    enum class SuppressAck : std::uint8_t {
        YES = 'Y'
      , NO  = 'N' /* anything other than 'Y' */
    };

    typedef std::uint64_t Timestamp;
    typedef Timestamp SendingTime;
    typedef Timestamp TransactionTime;

    enum class InformationText : std::uint8_t {
        USER_INITIATED_KILL                         = '0'
      , EXCHANGE_INITIATED_FOR_PNP_CROSSED_MARKET   = '1'
      , SELF_TRADE_PREVENTION_ACTIVATED_KILL        = '2'
    };

    enum class LiquidityIndicator : std::uint8_t {
        ADD_ON_TAPE                                                       = 'A'
      , ADD_BLIND_ON_TAPE                                                 = 'B'
      , TAPE_EXECUTION_ROUTED_TO_NYSE_OR_AMEX_PARTICIPATED_OPEN_OR_REOPEN = 'C'
      , ADD_ON_TAPE_SUB_DOLLAR                                            = 'D'
      , REMOVING_ON_TAPE_SUB_DOLLAR                                       = 'E'
      , TAPE_EXECUTION_ROUTED_TO_NYSE_OR_AMEX_WAS_LIQUIDITY_ADDING        = 'F'
      , TAPE_EXECUTION_LIMIT_AUCTION_ONLY_IN_ARCA_OPEN_MKT_OR_HALT_AUCTION= 'G'
//    , TAPE_EXECUTION_MARKET_AUCTION_ONLY_IN_ARCA_OPEN_MKT_HALT_AUCTION  = 'G'
      , ROUTED_ON_TAPE_SUB_DOLLAR                                         = 'H'
      , ADD_ON_TAPE_RETAIL                                                = 'I'
      , TAPE_EXECUTION_TRACKING_ORDER                                     = 'J'
      , REMOVING_ON_TAPE_RETAIL                                           = 'K'
      , REMOVING_ON_TAPE_MPL_ORDER                                        = 'L'
      , ADD_ON_TAPE_MPL_ORDER                                             = 'M'
      , TAPE_EXECUTION_ROUTED_TO_NYSE_OR_AMEX_TAKING_OR_345_REGARDLESS    = 'N'
      , NEUTRAL_ON_TAPE                                                   = 'O'
      , REMOVING_ON_TAPE                                                  = 'R'
      , TAPE_EXECUTION_SET_NEW_ARCA_BBO_ADDING_LIQUIDITY_ARCA             = 'S'
      , TAPE_EXECUTION_MOC_LOC_ON_NYSE                                    = 'U'
      , TAPE_EXECUTION_ROUTED_NYSE_ARCA_REROUTED_EXTERNAL_AND_FILLED      = 'W'
      , TAPE_EXECUTION_ROUTED                                             = 'X'
      , TAPE_EXECUTION_MOC_LOC_ON_AMEX                                    = 'Y'
      , TAPE_EXECUTION_MOC_LOC_ON_ARCA                                    = 'Z'
//    , TAPE_EXECUTION_LIMIT_AUCTION_ONLY_IN_CLOSING_AUCTION_ON_ARCA      = 'Z'
    };

    enum class RejectedMessageType : std::uint8_t {
        ORDER_REJECT                  = '1'
      , CANCEL_REJECT                 = '2'
      , CANCEL_REPLACE_REJECT         = '3'
    };

    enum class RejectReason : std::uint8_t {
        TOO_LATE_TO_CANCEL  = '0'
      , UNKNOWN_ORDER       = '1'
    };

    typedef std::uint64_t ExecutionID;

    enum class BustType : std::uint8_t {
        BUST      = '1'
      , CORRECT   = '2'
    };

    enum class AckLiquidityIndicator : std::uint8_t {
        CANDIDATE_FOR_LIQUIDITY_INDICATOR_S   = '1'
      , BLIND                                 = '2'
      , NOT_BLIND                             = '3'
    };

    typedef std::uint8_t ArcaExID[20];

    inline
    constexpr std::uint16_t LastMktValue (
                                     const std::uint8_t w = 0,
                                     const std::uint8_t x = 0)
    {
        return (std::uint16_t)(((std::uint16_t)w << 0) + ((std::uint16_t)x << 8));
    }

    enum class LastMkt : std::uint16_t {
        TAPE_A = LastMktValue('P', 'A')
      , TAPE_B = LastMktValue('P', 'B')
      , TAPE_C = LastMktValue('P', 'C')
    };

    enum class ExecTransType : std::uint8_t {
        NEW       = '0'
      , CANCEL    = '1'
      , CORRECT   = '2'
      , STATUS_3  = '3'
      , STATUS_4  = '4' // complex order
    };

    enum class OrderRejectReason : std::uint8_t {
        SEE_TEXT_FIELD_1 = '1'
      , SEE_TEXT_FIELD_3 = '3'
    };

    enum class OrderStatus : std::uint8_t {
        NEW_OR_OPEN                   = '0'
      , PARTIALLY_FILLED              = '1'
      , FILLED                        = '2'
      , EXPIRED                       = '3'
      , CANCELED                      = '4'
      , REPLACED                      = '5'
      , CANCELED_PENDING              = '6'
      , REJECTED                      = '8'
      , SELF_TRADE_PREVENTION_CANCEL  = 'C'
      , PENDING_REPLACE               = 'E'
    };

    enum class ExecutionType : std::uint8_t {
        NEW                           = '0'
      , PARTIAL_FILL                  = '1'
      , FILL                          = '2'
      , DONE_FOR_DAY                  = '3'
      , CANCELED                      = '4'
      , REPLACED                      = '5'
      , PENDING_CANCEL                = '6'
      , REJECTED                      = '8'
      , SUSPENDED                     = '9'
      , SELF_TRADE_PREVENTION_CANCEL  = 'C'
    };

    enum class DiscretionInstruction : std::uint8_t {
        NOT_USED = 0x00
    };

    typedef std::uint32_t LegRefID;



} } } } }

