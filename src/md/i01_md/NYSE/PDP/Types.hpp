#pragma once

#include <array>
#include <cstdint>
#include <i01_core/macro.hpp>

namespace i01 { namespace MD { namespace NYSE { namespace PDP { namespace Types {

    typedef std::uint16_t MsgSize;

    enum class MsgType : std::uint16_t {
        SEQUENCE_NUMBER_RESET             = 1
      , HEARTBEAT                         = 2
      , MESSAGE_UNAVAILABLE               = 5
      , RETRANSMISSION_RESPONSE           = 10
      , RETRANSMISSION_REQUEST            = 20
      , REFRESH_REQUEST                   = 22
      , HEARTBEAT_RESPONSE                = 24
      , OPENBOOK_FULL_UPDATE              = 230
      , OPENBOOK_DELTA_UPDATE             = 231
    , NYSE_OPENING_IMBALANCE            = 240
    , NYSE_CLOSING_IMBALANCE            = 241

    };

    typedef std::uint32_t MsgSeqNum;
    typedef std::uint32_t MillisecondsSinceMidnight;
    typedef std::uint16_t Microseconds;
    typedef std::uint8_t ProductID;

enum class ProductIDValues : std::uint8_t {
    OPENBOOK_ULTRA              = 115 ,
    IMBALANCES                  = 116 ,
    };

    enum class RetransFlag : std::uint8_t {
        ORIGINAL                    = 1
      , RETRANSMITTED               = 2
      , REPLAY                      = 3
      , RETRANSMITTED_REPLAY        = 4
      , RETRANSMITTED_REFRESH       = 5
      , ORIGINAL_TEST               = 129
      , RETRANSMITTED_TEST          = 130
      , REPLAY_TEST                 = 131
      , RETRANSMITTED_REPLAY_TEST   = 132
    };

    typedef std::uint8_t NumBodyEntries;
    typedef std::uint8_t LinkFlag;
    typedef std::uint8_t SourceID[20];
    typedef std::uint16_t SecurityIndex;

    typedef std::uint32_t SymbolSeqNum;
    typedef std::uint8_t SourceSessionID;

static const int SYMBOL_LENGTH = 11;
typedef std::array<std::uint8_t, SYMBOL_LENGTH> SymbolArray;
union Symbol {
    std::uint8_t u[SYMBOL_LENGTH];
    SymbolArray arr;
} __attribute__((packed));
I01_ASSERT_SIZE(Symbol, SYMBOL_LENGTH);


    typedef std::uint8_t PriceScaleCode;
typedef std::uint32_t Quantity;

    enum class QuoteCondition : std::uint8_t {
        NONE                                      = ' '
      , SLOW_ON_BID_DUE_TO_LRP_OR_GAP             = 'E'
      , SLOW_ON_ASK_DUE_TO_LRP_OR_GAP             = 'F'
      , SLOW_ON_BID_AND_ASK_DUE_TO_LPR_OR_GAP     = 'U'
      , SLOW_QUOTE_DUE_TO_SET_SLOW_ON_BOTH_SIDES  = 'W'
    };

    enum class TradingStatus : std::uint8_t {
        PRE_OPENING = 'P'
      , OPENED      = 'O'
      , CLOSING     = 'C'
      , HALTED      = 'H'
    };

    typedef std::uint16_t MPV;
    typedef std::uint32_t PriceNumerator;
    typedef std::uint32_t Volume;
    typedef std::uint16_t NumOrders;

    enum class Side : std::uint8_t {
        BUY   = 'B'
      , SELL  = 'S'
    };

    enum class DeltaReasonCode : std::uint8_t {
        NEW_ORDER_OR_ADDITIONAL_INTEREST  = 'O'
      , CANCEL                            = 'C'
      , EXECUTION                         = 'E'
      , MULTIPLE_EVENTS                   = 'X'
    };

    typedef std::uint32_t LinkID;

    typedef std::uint8_t RefreshSymbol[16];

    enum class AcceptReject : std::uint8_t {
        ACCEPTED = 'A'
      , REJECTED = 'R'
    };

    enum class RejectReason : std::uint8_t {
        ACCEPTED                = '0'
      , PERMISSIONS             = '1'
      , INVALID_SEQUENCE_RANGE  = '2'
      , MAX_SEQUENCE_RANGE      = '3'
      , MAX_REQUEST_PER_DAY     = '4'
      , MAX_REFRESH_PER_DAY     = '5'
      , SEQNUM_TTL_TOO_OLD      = '6'
    };

enum class Indicator01 : std::uint8_t {
    NO    = 0
 , YES   = 1
 };
enum class ImbalanceSide : std::uint8_t {
    BUY = 'B'
 , SELL = 'S'
 , NO_IMBALANCE = ' '
 };


}}}}}
