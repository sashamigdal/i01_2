#pragma once

#include <cstdint>
#include <iostream>

#include <i01_core/macro.hpp>

#include <i01_md/BATS/PITCH2/Types.hpp>
#include <i01_md/util.hpp>

#include <i01_oe/BOE20Order.hpp>

namespace i01 { namespace OE { namespace BATS { namespace BOE20 { namespace Types {


template<int WIDTH> using BOEAlphanumeric = BOE20Order::BOEAlphanumeric<WIDTH>;
template<int WIDTH> using FixedWidthText = BOE20Order::FixedWidthText<WIDTH>;

    typedef std::uint16_t           StartOfMessage; /* 0xBABA */
    constexpr const StartOfMessage START_OF_MESSAGE_SENTINEL = 0xBABA;
    typedef std::uint16_t           MessageLength;
    enum class MessageType : std::uint8_t {
          LOGIN_REQUEST_V2          = 0x37 /* session */
        , LOGOUT_REQUEST            = 0x02 /* session */
        , CLIENT_HEARTBEAT          = 0x03 /* session */
        , NEW_ORDER_V2              = 0x38 /* application */
        , CANCEL_ORDER_V2           = 0x39 /* application */
        , MODIFY_ORDER_V2           = 0x3A /* application */
        , LOGIN_RESPONSE_V2         = 0x24 /* session */
        , LOGOUT                    = 0x08 /* session */
        , SERVER_HEARTBEAT          = 0x09 /* session */
        , REPLAY_COMPLETE           = 0x13 /* application */
        , ORDER_ACKNOWLEDGMENT_V2  = 0x25 /* application */
        , ORDER_REJECTED_V2         = 0x26 /* application */
        , ORDER_MODIFIED_V2         = 0x27 /* application */
        , ORDER_RESTATED_V2         = 0x28 /* application */
        , USER_MODIFY_REJECTED_V2   = 0x29 /* application */
        , ORDER_CANCELLED_V2        = 0x2A /* application */
        , CANCEL_REJECTED_V2        = 0x2B /* application */
        , ORDER_EXECUTION_V2        = 0x2C /* application */
        , TRADE_CANCEL_OR_CORRECT_V2= 0x2D /* application */
    };
std::ostream& operator<<(std::ostream& os, const MessageType& mt);

    typedef std::uint8_t            MatchingUnit;
    typedef std::uint32_t           SequenceNumber;

const int NUMBER_OF_MATCHING_UNITS = 32;
const int MAX_NUMBER_OF_MATCHING_UNITS = std::numeric_limits<MatchingUnit>::max()+1;

using SessionSubID = MD::BATS::PITCH2::Types::SessionSubID;

using Username = MD::BATS::PITCH2::Types::Username;

using Password = MD::BATS::PITCH2::Types::Password;

typedef std::uint8_t            NumParamGroups;
typedef std::uint16_t           ParamGroupLength;
// typedef std::uint8_t            ParamGroupType;

enum class ParamGroupType : std::uint8_t {
    UNIT_SEQUENCES              = 0x80 ,
    RETURN_BITFIELDS            = 0x81,
    };


    enum class BooleanFlag : std::uint8_t {
        FALSE   = 0,
        TRUE    = 1
    };

enum class YesNoFlag : std::uint8_t {
    NO = 'N' ,
    YES = 'Y' ,
    };

std::ostream & operator<<(std::ostream &os, const BooleanFlag &m);

typedef std::uint8_t NumberOfUnits;

typedef std::uint64_t Price;


    enum class LoginResponseStatus : std::uint8_t {
          LOGIN_ACCEPTED                            = 'A'
        , SESSION_IN_USE                            = 'B'
        , SESSION_IS_DISABLED                       = 'D'
        , INVALID_RETURN_BITFIELD_IN_LOGIN_MESSAGE  = 'F'
        , INVALID_UNIT_GIVEN_IN_LOGIN_MESSAGE       = 'I'
        , INVALID_LOGIN_REQUEST_MESSAGE_STRUCTURE   = 'M'
        , NOT_AUTHORIZED_INVALID_USERNAME_PASSWORD  = 'N'
        , SEQUENCE_AHEAD_IN_LOGIN_MESSAGE           = 'Q'
        , INVALID_SESSION                           = 'S'
    };


typedef FixedWidthText<60> ResponseText;
I01_ASSERT_SIZE(ResponseText, 60);

    enum class LogoutReason : std::uint8_t {
          USER_REQUESTED        = 'U'
        , END_OF_DAY            = 'E'
        , ADMINISTRATIVE        = 'A'
        , PROTOCOL_VIOLATION    = '!'
    };



static const std::uint8_t HPR_REJECT_BROKER_LOC = 'Z';
static const std::uint32_t HPR_REJECT_ORDER_SIZE = 1;
static const BOEAlphanumeric<8> HPR_REJECT_SYMBOL = {{'Z', 'V', 'Z', 'Z', 'T'}};

static const int CL_ORD_ID_WIDTH = 20;
static const int HPR_ORD_ID_SYMBOL_WIDTH = 6;
static const int OUR_ORD_ID_WIDTH = 13;

union LocalClOrdID {
    std::uint64_t               u64;
    struct Bitfield {
        std::uint32_t           local_id;
        std::uint16_t           local_account;
    } __attribute__((packed))   bits;
    void set(std::uint16_t acct, std::uint32_t id) {
         u64 = ((std::uint64_t) acct << 32) | (std::uint64_t) id;
   }

    struct hash {
        size_t operator() (const LocalClOrdID &l) const {
            return std::hash<std::uint64_t>()(l.u64);
        }
    };

    static LocalClOrdID create(std::uint16_t acct, std::uint32_t id) {
        LocalClOrdID lid;
        lid.set(acct,id);
        return lid;
    }
};
I01_ASSERT_SIZE(LocalClOrdID, 8);

using OurClOrdID = core::Alphanumeric<OUR_ORD_ID_WIDTH,true,true,'0'>;

union ClOrdID {
    std::array<uint8_t,CL_ORD_ID_WIDTH>         arr;
    using HPRSymbol = std::array<std::uint8_t, HPR_ORD_ID_SYMBOL_WIDTH>;
    struct HPRFields {
        std::uint8_t            broker_loc;
        HPRSymbol               symbol;
        OurClOrdID              order_id;
    } __attribute__((packed))   fields;
    I01_ASSERT_SIZE(HPRFields, CL_ORD_ID_WIDTH);
    void set(const std::uint8_t &loc, std::uint16_t acct, std::uint32_t id, const HPRSymbol &sym = {{'0', '0', '0', '0', '0', '0'}}) {
        fields.broker_loc = loc;
        fields.order_id.set(LocalClOrdID::create(acct, id).u64);
        fields.symbol = sym;
    }
    template<typename RetType>
    RetType get_local_id() const {
        return fields.order_id.get<RetType>();
    }
};
I01_ASSERT_SIZE(ClOrdID, 20);
std::ostream & operator<<(std::ostream &os, const ClOrdID &m);

    enum class Side : std::uint8_t {
          BUY                   = '1'
        , SELL                  = '2'
        , SELL_SHORT            = '5'
        , SELL_SHORT_EXEMPT     = '6'
    };
std::ostream & operator<<(std::ostream &os, const Side &m);

typedef std::uint32_t Quantity;
static const Quantity MAX_ORDER_QTY = 999999;

enum class WhichBitfields : std::uint8_t {
    BITFIELD_1 = 1 ,
    BITFIELD_2 = 2 ,
    BITFIELD_3 = 4 ,
    BITFIELD_4 = 8 ,
    BITFIELD_5 = 16 ,
    BITFIELD_6 = 32 ,
    BITFIELD_7 = 64 ,
    BITFIELD_8 = 128 ,
    };

    enum class NewOrderBitfield1 : std::uint8_t {
          CLEARING_FIRM     = 1 << 0
        , CLEARING_ACCOUNT  = 1 << 1
        , PRICE             = 1 << 2 /* limit orders */
        , EXEC_INST         = 1 << 3
        , ORD_TYPE          = 1 << 4 /* limit, market, peg */
        , TIME_IN_FORCE     = 1 << 5
        , MIN_QTY           = 1 << 6
        , MAX_FLOOR         = 1 << 7
    };
    enum class NewOrderBitfield2 : std::uint8_t {
          SYMBOL            = 1 << 0 /* required */
        , SYMBOL_SFX        = 1 << 1 /* not required for NASDAQ symbols */
        , CAPACITY          = 1 << 6 /* required */
        , ROUTING_INST      = 1 << 7
    };
    enum class NewOrderBitfield3 : std::uint8_t {
          ACCOUNT               = 1 << 0
        , DISPLAY_INDICATOR     = 1 << 1
        , MAX_REMOVE_PCT        = 1 << 2
        , DISCRETION_AMOUNT     = 1 << 3
        , PEG_DIFFERENCE        = 1 << 4
        , PREVENT_MEMBER_MATCH  = 1 << 5
        , LOCATE_REQD           = 1 << 6
        , EXPIRE_TIME           = 1 << 7
    };
    enum class NewOrderBitfield4 : std::uint8_t {
    };
    enum class NewOrderBitfield5 : std::uint8_t {
          ATTRIBUTED_QUOTE      = 1 << 1
        , EXT_EXEC_INST         = 1 << 3
    };
    enum class NewOrderBitfield6 : std::uint8_t {
        DISPLAY_RANGE           = 1 << 0
      , STOP_PX                 = 1 << 1
      , ROUT_STRATEGY           = 1 << 2
      , ROUTE_DELIVERY_METHOD   = 1 << 3
      , EX_DESTINATION          = 1 << 4
      , ECHO_TEXT               = 1 << 5
    };

    enum class CancelOrderBitfield1 : std::uint8_t {
          CLEARING_FIRM = 1 << 0
    };

    enum class ModifyOrderBitfield1 : std::uint8_t {
          CLEARING_FIRM         = 1 << 0
        , ORDER_QTY             = 1 << 2 /* required */
        , PRICE                 = 1 << 3 /* required for limit orders */
        , ORD_TYPE              = 1 << 4
        , CANCEL_ORIG_ON_REJECT = 1 << 5
        , EXEC_INST             = 1 << 6
        , SIDE                  = 1 << 7
    };
    enum class ModifyOrderBitfield2 : std::uint8_t {
        MAX_FLOOR               = 1 << 0
      , STOP_PX                 = 1 << 1
    };

    typedef std::uint64_t DateTime;

    typedef std::uint64_t OrderID;

// Numbering of the bitfields based on the BOE manual numbering
enum class OrderAcknowledgmentBitfield1 : std::uint8_t {
    SIDE                = 1 ,
    PEG_DIFFERENCE      = 2 ,
    PRICE               = 4 ,
    EXEC_INST           = 8 ,
    ORD_TYPE            = 16 ,
    TIME_IN_FORCE       = 32 ,
    MIN_QTY             = 64 ,
    MAX_REMOVE_PCT      = 128 ,
    };

enum class OrderAcknowledgmentBitfield2 : std::uint8_t {
    SYMBOL              = 1 ,
    SYMBOL_SFX          = 2 ,
    CAPACITY            = 64 ,
    };

enum class OrderAcknowledgmentBitfield3: std::uint8_t {
    ACCOUNT             = 1 ,
    CLEARING_FIRM       = 2 ,
    CLEARING_ACCOUNT    = 4 ,
    DISPLAY_INDICATOR   = 8 ,
    MAX_FLOOR           = 16 ,
    DISCRETION_AMOUNT   = 32 ,
    ORDER_QTY           = 64 ,
    PREVENT_MEMBER_MATCH= 128 ,
    };

enum class OrderAcknowledgmentBitfield4 : std::uint8_t {
};

enum class OrderAcknowledgmentBitfield5 : std::uint8_t {
    ORIG_CL_ORD_ID      = 1 ,
    LEAVES_QTY          = 2 ,
    LAST_SHARES         = 4 ,
    LAST_PRICE          = 8 ,
    DISPLAY_PRICE       = 16 ,
    WORKING_PRICE       = 32 ,
    BASE_LIQUID_INDIC   = 64 ,
    EXPIRE_TIME         = 128 ,
};

enum class OrderAcknowledgmentBitfield6 : std::uint8_t {
    SECONDARY_ORDER_ID  = 1 ,
    ATTRIBUTED_QUOTE    = 8 ,
    EXT_EXEC_INST       = 16 ,
};

enum class OrderAcknowledgmentBitfield7 : std::uint8_t {
    SUB_LIQUID_INDIC    = 1 ,
};

enum class OrderAcknowledgmentBitfield8 : std::uint8_t {
    ECHO_TEXT           = 2 ,
    STOP_PX             = 4 ,
    ROUTING_INST        = 8 ,
    ROUT_STRATEGY       = 16 ,
    ROUTE_DELIV_METHOD  = 32 ,
    EX_DESTINATION      = 64 ,
};

// ORDER REJECTED BITFIELDS

typedef OrderAcknowledgmentBitfield1 OrderRejectedBitfield1;
typedef OrderAcknowledgmentBitfield2 OrderRejectedBitfield2;
typedef OrderAcknowledgmentBitfield3 OrderRejectedBitfield3;
typedef OrderAcknowledgmentBitfield4 OrderRejectedBitfield4;
typedef OrderAcknowledgmentBitfield5 OrderRejectedBitfield5;
typedef OrderAcknowledgmentBitfield6 OrderRejectedBitfield6;
// no bits in bitfield7
typedef OrderAcknowledgmentBitfield8 OrderRejectedBitfield8;

// ORDER MODIFIED BITFIELDS

typedef OrderAcknowledgmentBitfield1 OrderModifiedBitfield1;

enum class OrderModifiedBitfield2 : std::uint8_t {
};

typedef OrderAcknowledgmentBitfield3 OrderModifiedBitfield3;
typedef OrderAcknowledgmentBitfield4 OrderModifiedBitfield4;
typedef OrderAcknowledgmentBitfield5 OrderModifiedBitfield5;
typedef OrderAcknowledgmentBitfield6 OrderModifiedBitfield6;

enum class OrderModifiedBitfield7 : std::uint8_t {
};

typedef OrderAcknowledgmentBitfield8 OrderModifiedBitfield8;

// ORDER RESTATED BITFIELDS

typedef OrderAcknowledgmentBitfield1 OrderRestatedBitfield1;
typedef OrderAcknowledgmentBitfield2 OrderRestatedBitfield2;
typedef OrderAcknowledgmentBitfield3 OrderRestatedBitfield3;
typedef OrderAcknowledgmentBitfield4 OrderRestatedBitfield4;
typedef OrderAcknowledgmentBitfield5 OrderRestatedBitfield5;
typedef OrderAcknowledgmentBitfield6 OrderRestatedBitfield6;

enum class OrderRestatedBitfield7 : std::uint8_t {
};

typedef OrderAcknowledgmentBitfield8 OrderRestatedBitfield8;

// USER MODIFY REJECTED BITFIELDS

// none

// ORDER CANCELLED BITFIELDS

typedef OrderAcknowledgmentBitfield1 OrderCancelledBitfield1;
typedef OrderAcknowledgmentBitfield2 OrderCancelledBitfield2;
typedef OrderAcknowledgmentBitfield3 OrderCancelledBitfield3;
typedef OrderAcknowledgmentBitfield4 OrderCancelledBitfield4;
typedef OrderAcknowledgmentBitfield5 OrderCancelledBitfield5;
typedef OrderAcknowledgmentBitfield6 OrderCancelledBitfield6;

enum class OrderCancelledBitfield7 : std::uint8_t {
};

typedef OrderAcknowledgmentBitfield8 OrderCancelledBitfield8;

// CANCEL REJECTED BITFIELDS

typedef OrderAcknowledgmentBitfield1 CancelRejectedBitfield1;
typedef OrderAcknowledgmentBitfield2 CancelRejectedBitfield2;

enum class CancelRejectedBitfield8 : std::uint8_t {
    ECHO_TEXT           = 2 ,
    STOP_PX             = 4 ,
    };

// ORDER EXECUTION BITFIELDS

typedef OrderAcknowledgmentBitfield1 OrderExecutionBitfield1;
typedef OrderAcknowledgmentBitfield2 OrderExecutionBitfield2;
typedef OrderAcknowledgmentBitfield3 OrderExecutionBitfield3;

enum class OrderExecutionBitfield6 : std::uint8_t {
    ATTRIBUTED_QUOTE    = 8 ,
    EXT_EXEC_INST       = 16 ,
    };

enum class OrderExecutionBitfield8 : std::uint8_t {
    FEE_CODE            = 1 ,
    ECHO_TEXT           = 2 ,
    STOP_PX             = 4 ,
    ROUTING_INST        = 8 ,
    ROUT_STRATEGY       = 16 ,
    ROUTE_DELIV_METHOD  = 32 ,
    EX_DESTINATION      = 64 ,
    };

// TRADE CANCEL OR CORRECT BITFIELDS

typedef OrderAcknowledgmentBitfield2 TradeCancelOrCorrectBitfield2;

// OPTIONAL FIELDS

using Account                           = FixedWidthText<16>;
using AttributedQuote                   = std::uint8_t;

enum class BaseLiquidityIndicator : std::uint8_t {
    ADDED_LIQUIDITY = 'A'
 , REMOVED_LIQUIDITY = 'R'
 , ROUTED_TO_ANOTHER_MARKET = 'X'
 , BZX_AUCTION_TRADE = 'C'
 };
std::ostream & operator<<(std::ostream &os, const BaseLiquidityIndicator &m);

using CancelOrigOnReject                = std::uint8_t;

enum class Capacity : std::uint8_t {
    AGENCY = 'A' ,
    PRINCIPAL = 'P' ,
    RISKLESS_PRINCIPAL = 'R' ,
    };
std::ostream & operator<<(std::ostream &os, const Capacity &m);

using ClearingAccount = FixedWidthText<4>;
using ClearingFirm = BOEAlphanumeric<4>;
using DiscretionAmount = std::uint16_t;
using DisplayIndicator = BOE20Order::DisplayIndicator;
using DisplayPrice = Price;
using DisplayRange = std::uint32_t;
using EchoText = FixedWidthText<64>;
using ExDestination = std::uint8_t;
using ExecInst = BOE20Order::ExecInst;
using ExpireTime = DateTime;
using ExtExecInst = std::uint8_t;
using FeeCode = BOEAlphanumeric<2>;
using LastPx = Price;
using LastShares = Quantity;
using LeavesQty = Quantity;
using LocateReqd = YesNoFlag;
using MaxFloor = Quantity;
using MaxRemovePct = std::uint8_t;
using MinQty = Quantity;
using OrderQty = Quantity;
enum class OrdType : std::uint8_t {
    MARKET              = '1' ,
    LIMIT               = '2' ,
    STOP                = '3' ,
    STOP_LIMIT          = '4' ,
    PEGGED              = 'P' ,
    };
std::ostream & operator<<(std::ostream &os, const OrdType &m);

using OrigClOrdID = ClOrdID;
using PegDifference = std::uint64_t;
using PreventMatch = BOEAlphanumeric<3>;
using RouteDeliverMethod = FixedWidthText<3>;
using RoutingInst = BOE20Order::RoutingInst;
using RoutStrategy = FixedWidthText<6>;
using SecondaryOrderID = std::uint64_t;
using StopPx = Price;

enum class SubLiquidityIndicator : std::uint8_t {
    NO_ADDITIONAL_INFORMATION = '\0'
 , TRADE_ADDED_RPI_LIQUIDITY = 'E'
 , TRADE_ADDED_HIDDEN_LIQUIDITY = 'H'
 , TRADE_ADDED_HIDDEN_LIQUIDITY_THAT_WAS_PRICE_IMPROVED = 'I'
 , EXECUTION_FROM_FIRST_ORDER_TO_JOIN_THE_NBBO = 'J'
 , EXECUTION_FROM_ORDER_THAT_SET_THE_NBBO = 'S'
 , TRADE_ADDED_VISIBLE_LIQUIDITY_THAT_WAS_PRICE_IMPROVED = 'V'
 , MIDPOINT_PEG_ORDER_ADDED_LIQUIDITY = 'm'
 };
std::ostream & operator<<(std::ostream &os, const SubLiquidityIndicator &m);

using Symbol = BOEAlphanumeric<8>;
using SymbolSfx = Symbol;

enum class TimeInForce : std::uint8_t {
    DAY = '0' ,
    GTC = '1' ,
    AT_THE_OPEN = '2' ,
    IOC = '3' ,
    FOK = '4' ,
    GTX = '5' , // expires end of extended day
    GTD = '6' , // expires at earlier of ExpireTime or end of Ext Day
    AT_THE_CLOSE = '7' ,
    REG_HOURS_ONLY = 'R' ,
    };
std::ostream & operator<<(std::ostream &os, const TimeInForce &m);


using WorkingPrice = Price;

enum class ReasonCode : std::uint8_t {
    ADMIN = 'A'
 , DUPLICATE_CL_ORD_ID = 'D'
 , HALTED = 'H'
 , INCORRECT_DATA_CENTER = 'I'
 , TOO_LATE_TO_CANCEL = 'J'
 , ORDER_RATE_THRESHOLD_EXCEEDED = 'K'
 , PRICE_EXCEEDS_CROSS_RANGE = 'L'
 , RAN_OUT_OF_LIQUIDITY_TO_EXECUTE_AGAINST = 'N'
 , CL_ORD_ID_DOESNT_MATCH_A_KNOWN_ORDER = 'O'
 , CANT_MODIFY_AN_ORDER_THAT_IS_PENDING_FILL = 'P'
 , WAITING_FOR_FIRST_TRADE = 'Q'
 , ROUTING_UNAVAILABLE = 'R'
 , FILL_WOULD_TRADE_THRU_NBBO = 'T'
 , USER_REQUESTED = 'U'
 , WOULD_WASH = 'V'
 , ADD_LIQUIDITY_ONLY_ORDER_WOULD_REMOVE = 'W'
 , ORDER_EXPIRED = 'X'
 , SYMBOL_NOT_SUPPORTED = 'Y'
 , RESERVE_RELOAD = 'r'
 , MARKET_ACCESS_RISK_LIMIT_EXCEEDED = 'm'
 , MAX_OPEN_ORDERS_COUNT_EXCEEDED = 'o'
 , LIMIT_UP_DOWN = 'u'
 , WOULD_REMOVE_ON_UNSLIDE = 'w'
 , CROSSED_MARKET = 'x'
 , ORDER_RECEIVED_BY_BATS_DURING_REPLAY = 'y'
};
std::ostream & operator<<(std::ostream &os, const ReasonCode &m);

    enum class RestatementReason : std::uint8_t {
          RELOAD = 'L'
        , LIQUIDITY_UPDATED = 'Q'
        , REROUTE = 'R'
        , SIZE_REDUCED_DUE_TO_SQP = 'S'
        , WASH = 'W'
        , LOCKED_IN_CROSS = 'X'
    };
std::ostream & operator<<(std::ostream &os, const RestatementReason &m);

    typedef i01::MD::BATS::PITCH2::Types::ExecutionId ExecID;

static const double PRICE_DIVISOR = 1e4;
static const double PRICE_DIVISOR_INV = 1e-4;



    inline
    constexpr std::uint32_t ContraBrokerValue (
                                     const std::uint8_t w = 0,
                                     const std::uint8_t x = 0,
                                     const std::uint8_t y = 0,
                                     const std::uint8_t z = 0)
    {
        return (((std::uint32_t)w << 0) + ((std::uint32_t)x << 8) + ((std::uint32_t)y << 16) + ((std::uint32_t)z << 24));
    }

    enum class ContraBroker : std::uint32_t {
          BATS_BZX_EXCHANGE             = ContraBrokerValue('B', 'A', 'T', 'S')
        , BATS_BYX_EXCHANGE             = ContraBrokerValue('B', 'Y', 'X', 'X')
        , ROUTED_TO_NASDAQ              = ContraBrokerValue('I', 'N', 'E', 'T')
        , ROUTED_TO_NYSE_ARCA           = ContraBrokerValue('A', 'R', 'C', 'A')
        , ROUTED_EXECUTION_FROM_NSX     = ContraBrokerValue('N', 'S', 'X')
        , ROUTED_TO_NYSE_MKT            = ContraBrokerValue('A', 'M', 'E', 'X')
        , ROUTED_TO_NASDAQ_BX           = ContraBrokerValue('B', 'E', 'X')
        , ROUTED_TO_CBOE_STOCK_EXCHANGE = ContraBrokerValue('C', 'B', 'S', 'X')
        , ROUTED_TO_CHICAGO             = ContraBrokerValue('C', 'H', 'X')
        , DIRECTEDGE_A_EXCHANGE         = ContraBrokerValue('E', 'D', 'G', 'A')
        , DIRECTEDGE_X_EXCHANGE         = ContraBrokerValue('E', 'D', 'G', 'X')
        , ROUTED_TO_LAVAFLOW            = ContraBrokerValue('F', 'L', 'O', 'W')
        , ROUTED_TO_NEW_YORK            = ContraBrokerValue('N', 'Y', 'S', 'E')
        , ROUTED_TO_NASDAQ_PSX          = ContraBrokerValue('P', 'S', 'X')
        , ROUTED_TO_DRT_POOL            = ContraBrokerValue('D', 'R', 'T')
    };
    // typedef std::int64_t AccessFee; /* five implied decimal places, negative for rebates */


} } } } }
