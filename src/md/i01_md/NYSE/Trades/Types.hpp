#pragma once

#include <cstdint>
#include <i01_core/macro.hpp>

namespace i01 { namespace MD { namespace NYSE { namespace Trades { namespace Types {

    /* Packet Header: */
    typedef std::uint16_t PktSize;
    enum class DeliveryFlag : std::uint8_t {
        HEARTBEAT                     = 1
      , XDP_FAILOVER                  = 10
      , ORIGINAL                      = 11
      , SEQUENCE_NUMBER_RESET         = 12
      , ONE_PACKET_IN_RETRANS_UNCOMP  = 13
      , PART_OF_RETRANS_SEQ_UNCOMP    = 15
      , ONE_PACKET_IN_REFRESH_UNCOMP  = 17
      , START_OF_REFRESH_UNCOMP       = 18
      , PART_OF_REFRESH_UNCOMP        = 19
      , END_OF_REFRESH_UNCOMP         = 20
      , MESSAGE_UNAVAILABLE_21        = 21
      , COMPACTED_XDP_FAILOVER        = 30
      , MESSAGE_UNAVAILABLE_41        = 41
    };
    typedef std::uint8_t NumberMsgs;
    typedef std::uint32_t SeqNum;
    typedef std::uint32_t Time;
    typedef std::uint32_t TimeNS;

    /* Message Header: */
    typedef std::uint16_t MsgSize;
    enum class MsgType : std::uint16_t {
        SEQUENCE_NUMBER_RESET         = 1
      , TIME_REFERENCE                = 2
      , SYMBOL_INDEX_MAPPING          = 3
      , VENDOR_MAPPING                = 4
      , OPTION_SERIES_INDEX_MAPPING   = 5
      , RETRANSMISSION_REQUEST        = 10
      , REQUEST_RESPONSE              = 11
      , HEARTBEAT_RESPONSE            = 12
      , SYMBOL_INDEX_MAPPING_REQUEST  = 13
      , REFRESH_REQUEST               = 15
      , MESSAGE_UNAVAILABLE           = 31
      , SYMBOL_CLEAR                  = 32
      , TRADING_SESSION_CHANGE        = 33
      , SECURITY_STATUS               = 34
      , REFRESH_HEADER                = 35
      , TRADE                         = 220
      , TRADE_CANCEL_OR_BUST          = 221
      , TRADE_CORRECTION              = 222
      , STOCK_SUMMARY                 = 223
    };

    enum class TradingSession : std::uint8_t {
        MORNING     = 0x01 /* 0400-0930 ET */
      , NATIONAL    = 0x02 /* 0930-1600 ET */
      , LATE        = 0x04 /* 1600-2000 ET */
    };
    struct TradingSessionBitfield {
        std::uint8_t u8;

        bool morning() const { return u8 & (std::uint8_t)TradingSession::MORNING; }
        bool national() const { return u8 & (std::uint8_t)TradingSession::NATIONAL; }
        bool late() const { return u8 & (std::uint8_t)TradingSession::LATE; }
    } __attribute__((packed));
    union TradeSession {
        std::uint8_t u8;
        TradingSession ts;
        TradingSessionBitfield tsbf;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TradeSession, 1);

    typedef std::uint8_t ProductID;
    typedef std::uint8_t ChannelID;

    typedef std::uint32_t SymbolIndex;
    typedef std::uint32_t SymbolSeqNum;

    enum class SecurityStatusType : std::uint8_t {
      /* Halt status codes */
        OPENING_DELAY                     = '3'
      , TRADING_HALT                      = '4' // valid for ARCA
      , RESUME                            = '5' // valid for ARCA
      , NO_OPEN_NO_RESUME                 = '6'
      , SSR_ACTIVATED_DAY_1               = 'A'
      , SSR_CONTINUED_DAY_2               = 'C'
      , SSR_DEACTIVATED                   = 'D'
      , SSR_IN_EFFECT                     = 'E' // valid for ARCA
      /* NYSE Market State values */
      , OPENED                            = 'O'
      , PRE_OPENING                       = 'P'
      , CLOSED                            = 'X'
      /* Price indication */
      , TIME                              = 'T'
      , PRICE_INDICATION                  = 'I'
      , PRE_OPENING_PRICE_INDICATION      = 'G'
      , RULE_15_INDICATION                = 'R'
    };
    enum class HaltCondition : std::uint8_t {
        NOT_APPLICABLE                    = 0x20
      , NOT_DELAYED_NOT_HALTED            = '~'
      , NEWS_DISSEMINATION                = 'D'
      , ORDER_IMBALANCE                   = 'I'
      , NEWS_PENDING                      = 'P'
      , LULD_PAUSE                        = 'M'
      , RELATED_SECURITY                  = 'S'
      , EQUIPMENT_CHANGEOVER              = 'X'
      , NO_OPEN_NO_RESUME                 = 'Z'
      , MARKET_WIDE_CIRCUIT_BREAKER_LVL1  = '1'
      , MARKET_WIDE_CIRCUIT_BREAKER_LVL2  = '2'
      , MARKET_WIDE_CIRCUIT_BREAKER_LVL3  = '3'
    };
    typedef std::uint32_t TransactionID;
    typedef std::uint32_t Price;
    enum class SSRTriggeringExchangeID : std::uint8_t {
        NYSE = 'N'
      , NYSE_ARCA = 'P'
      , NASDAQ = 'Q'
      , NYSE_MKT = 'A'
      , OTCBB_GLOBAL_OTC = 'U'
      , OTHER_OTC = 'V'
      , NASDAQ_OMX_BX = 'B'
      , NSX = 'C'
      , FINRA = 'D'
      , ISE = 'I'
      , EDGA = 'J'
      , EDGX = 'K'
      , CHX = 'M'
      , CTS = 'S'
      , NASDAQ_OMX = 'T'
      , CBSX = 'W'
      , NASDAQ_OMX_PSX = 'X'
      , BATS_Y = 'Y'
      , BATS = 'Z'
    };
    typedef std::uint32_t Volume;
    enum class SSRState : std::uint8_t {
        NO_SHORT_SALE_IN_EFFECT = '~'
      , SSR_IN_EFFECT = 'E' // ARCA
    };
    enum class MarketState : std::uint8_t {
        OPENED = 'O'
      , PRE_OPENING = 'P'
      , CLOSED = 'X'
    };
    enum class SessionState : std::uint8_t {
        EARLY = 'X'
      , CORE = 'Y'
      , LATE = 'Z'
    };

    enum class Side : std::uint8_t {
        BUY   = 'B'
      , SELL  = 'S'
      , NO_IMBALANCE = ' ' /* only for imbalance messages */
    };

    typedef std::uint32_t TradeID;

    enum class TradeCondition1 : std::uint8_t {
        REGULAR_SALE              = '@'
      , CASH                      = 'C'
      , NEXT_DAY_TRADE            = 'N'
      , SELLER                    = 'R'
    };
    enum class TradeCondition2 : std::uint8_t {
        NOT_APPLICABLE            = 0x20
      , ISO                       = 'F'
      , OPENING_TRADE             = 'O'
      , REOPENING_TRADE           = '5'
      , CLOSING_TRADE             = '6'
      , CORRECTED_LAST_SALE_PRICE = '9'
    };
    enum class TradeCondition3 : std::uint8_t {
        NOT_APPLICABLE            = 0x20
      , SOLD_LAST                 = 'L'
      , EXTENDED_HOURS_TRADE      = 'T'
      , EXTENDED_HOURS_SOLD       = 'U'
      , SOLD                      = 'Z'
    };
    enum class TradeCondition4 : std::uint8_t {
        NOT_APPLICABLE            = 0x20
      , REGULAR_SALE              = '@'
      , AUTOMATIC_EXECUTION       = 'E'
      , ODD_LOT_TRADE             = 'I'
      , RULE_127_OR_155           = 'K'
      , OFFICIAL_CLOSING_PRICE    = 'M'
      , OFFICIAL_OPEN_PRICE       = 'Q'
    };

    enum class TradeThroughExempt : std::uint8_t {
        NOT_APPLICABLE            = 0x20
      , TRADE_THROUGH_EXEMPT_611  = 'X'
    };

    enum class LiquidityIndicatorFlag : std::uint8_t {
        BUY_SIDE            = 0x01
      , SELL_SIDE           = 0x02
      , NO_LIQUIDITY_ADDED  = 0x04
    };

    enum class Tick : std::uint8_t {
        NO_TICK_FIRST_TIME = 0
      , ZERO_DOWN_TICK     = 1
      , DOWN_TICK          = 2
      , UP_TICK            = 3
      , ZERO_UP_TICK       = 4
    };

    typedef std::uint8_t SellerDays; /* 2-180 if TradeCond1=R */
    enum class StopStockIndicator : std::uint8_t {
        PRICE_NOT_STOPPED = 0
      , PRICE_STOPPED     = 1
    };

} } } } }
