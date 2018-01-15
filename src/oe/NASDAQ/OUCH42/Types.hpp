#pragma once

#include <cstdint>
#include <array>
#include <arpa/inet.h>
#include <endian.h>
#include <algorithm>
#include <iosfwd>

#include <i01_core/macro.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/Alphanumeric.hpp>
#include <i01_oe/NASDAQOrder.hpp>

namespace i01 { namespace OE { namespace NASDAQ { namespace OUCH42 { namespace Types {

    typedef std::uint32_t PriceRaw;
    enum class PriceSpecial : PriceRaw {
        MAXIMUM_PRICE               = 0x7735939C /* = $199,999.9900 */
      , MARKET_CROSS_SPECIAL_PRICE  = 0x7FFFFFFF /* = $214,748.3647 */
    };
    union Price {
        PriceRaw              raw;
        PriceSpecial          special;

        void set_limit(const double new_price)
        { raw = htonl(static_cast<PriceRaw>((new_price * 10000.0) + 0.5)); }
        void set_market()
        { raw = htonl(static_cast<PriceRaw>(PriceSpecial::MARKET_CROSS_SPECIAL_PRICE)); }
        bool is_market() const
        { return special == PriceSpecial::MARKET_CROSS_SPECIAL_PRICE; }
        double get_limit() const
        { return ((double)std::min((PriceRaw)PriceSpecial::MAXIMUM_PRICE, be32toh(raw))) / 10000.0; }
    };
std::ostream& operator<<(std::ostream& os, const Price& p);

    typedef std::uint32_t TimeInForceRaw;
    enum class TimeInForceSpecial : TimeInForceRaw {
        IMMEDIATE_OR_CANCEL       = 0x0
     ,  MARKET_HOURS              = 0x1869E /* = 99,998 */
     ,  SYSTEM_HOURS              = 0x1869F /* = 99,999; invalid above this */
     ,  BIG_ENDIAN_MARKET_HOURS   = 0x9E860100
     ,  BIG_ENDIAN_SYSTEM_HOURS   = 0x9F860100
    };
    union TimeInForce {
        TimeInForceRaw        raw;
        TimeInForceSpecial    special;

        void set(std::uint32_t tif) {
            raw = htonl(tif);
        }
        void set_ioc() {
            special = TimeInForceSpecial::IMMEDIATE_OR_CANCEL;
        }
        void set_market_hours() {
            special = TimeInForceSpecial::BIG_ENDIAN_MARKET_HOURS;
        }
        void set_system_hours() {
            special = TimeInForceSpecial::BIG_ENDIAN_SYSTEM_HOURS;
        }

        OE::TimeInForce to_i01() const {
            switch (special) {
            case TimeInForceSpecial::IMMEDIATE_OR_CANCEL:
                return OE::TimeInForce::IMMEDIATE_OR_CANCEL;
            case TimeInForceSpecial::BIG_ENDIAN_MARKET_HOURS:
            case TimeInForceSpecial::MARKET_HOURS:
                return OE::TimeInForce::DAY;
            case TimeInForceSpecial::BIG_ENDIAN_SYSTEM_HOURS:
            case TimeInForceSpecial::SYSTEM_HOURS:
                return OE::TimeInForce::EXT;
            default:
                return OE::TimeInForce::TIMED;
            }
        }
    };
std::ostream& operator<<(std::ostream& os, const TimeInForce& tif);

    typedef std::uint8_t MessageTypeRaw;
    enum class InboundMessageType : MessageTypeRaw {
        ENTER_ORDER         = 'O'
      , REPLACE_ORDER       = 'U'
      , CANCEL_ORDER        = 'X'
      , MODIFY_ORDER        = 'M'
    };
std::ostream& operator<<(std::ostream& os, const InboundMessageType& msg);

    enum class OutboundMessageType : MessageTypeRaw {
        SYSTEM_EVENT            = 'S'
      , ACCEPTED                = 'A'
      , REPLACED                = 'U'
      , CANCELED                = 'C'
      , AIQ_CANCELLED           = 'D'
      , EXECUTED                = 'E'
      , BROKEN_TRADE            = 'B'
      , REJECTED                = 'J'
      , CANCEL_PENDING          = 'P'
      , CANCEL_REJECT           = 'I'
      , ORDER_PRIORITY_UPDATE   = 'T'
      , ORDER_MODIFIED          = 'M'
    };
std::ostream& operator<<(std::ostream& os, const OutboundMessageType& msg);

    /// Day-unique, case-sensitive, alphanumeric token sent to the exchange to
    //  identify our order.  We use this primarily as a base-62 number in the
    //  set of characters [0-9A-Za-z] to allow us to reference this order.
    //  In base-62, a 64 bit number consumes 11 characters, leaving 3
    //  characters for use as account identifiers.
    //  $\ceil{\log_{62}{(2^64)}} = 11, \ceil{\log_{62}{(2^16)}} = 3$
    //  This number will be right-padded with spaces.
    union OrderToken {
        typedef std::uint8_t Raw[14];
        typedef std::array<std::uint8_t, 14> Array;
        struct HPRData {
            std::uint8_t hpr_marking;
            std::array<std::uint8_t, 5> symbol;
            i01::core::Alphanumeric<8, true, true, '0'> id;
        };
        struct NonHPRData {
            i01::core::Alphanumeric<3> account;
            i01::core::Alphanumeric<11> id;
        };

        Raw raw;
        Array arr;
        HPRData hprdata;
        NonHPRData nonhprdata;
    };
    I01_ASSERT_SIZE(OrderToken, 14);
std::ostream& operator<<(std::ostream& os, const OrderToken& tok);

    enum class BuySellIndicator : std::uint8_t {
        BUY           = 'B'
      , SELL          = 'S'
      , SHORT         = 'T'
      , SHORT_EXEMPT  = 'E'
    };
OE::Side to_i01_side(BuySellIndicator bs);

    typedef std::uint32_t Shares;

    typedef char StockRaw[8];
    typedef std::array<char, 8> StockArray;
    typedef std::uint64_t StockIntegral;
    union Stock {
        StockRaw          raw;
        StockIntegral     integral;
        StockArray        arr;
    };
std::ostream& operator<<(std::ostream& os, const Stock& s);

    typedef char FirmRaw[4];
    typedef std::array<char, 4> FirmArray;
    typedef std::uint32_t FirmIntegral;
    union Firm {
        FirmRaw           raw;
        FirmArray         arr;
        FirmIntegral      integral;
    };
std::ostream& operator<<(std::ostream& os, const Firm& f);

    typedef NASDAQOrder::Display Display;
    typedef NASDAQOrder::Capacity Capacity;

    enum class IntermarketSweepEligibility : std::uint8_t {
        ELIGIBLE     = 'Y'
      , NOT_ELIGIBLE = 'N'
    };

    enum class CrossType : std::uint8_t {
        NO_CROSS            = 'N'
      , OPENING_CROSS       = 'O'
      , CLOSING_CROSS       = 'C'
      , HALT_OR_IPO_CROSS   = 'H' /* must be market price */
      , SUPPLEMENTAL_ORDER  = 'S'
      , RETAIL              = 'R'
    };

    enum class CustomerType : std::uint8_t {
        RETAIL_DESIGNATED       = 'R'
      , NOT_RETAIL_DESIGNATED   = 'N'
      , USE_DEFAULT             = ' '
    };

    struct ExchTime {
        std::uint64_t u64; /* nanoseconds past midnight, be64 */
        std::uint64_t get() const { return be64toh(u64); }
    };
    I01_ASSERT_SIZE(ExchTime, sizeof(std::uint64_t));

    enum class SystemEventCode : std::uint8_t {
        START_OF_DAY = 'S'
      , END_OF_DAY   = 'E'
    };
std::ostream& operator<<(std::ostream& os, const SystemEventCode& code);

    typedef std::uint64_t OrderReferenceNumber;

    enum class OrderState : std::uint8_t {
        LIVE = 'L'
      , DEAD = 'D'
    };

    enum class BBOWeightIndicator : std::uint8_t {
        RANGE_ZERO_TO_ZERO_POINT_TWO_PERCENT = '0'
      , RANGE_ZERO_POINT_TWO_TO_1_PERCENT    = '1'
      , RANGE_ONE_TO_TWO_PERCENT             = '2'
      , RANGE_GREATER_THAN_TWO_PERFECT       = '3'
      , UNSPECIFIED                          = ' '
      , SETS_THE_QBBO_WHILE_JOINING_THE_NBBO = 'S'
      , IMPROVES_THE_NBBO_UPON_ENTRY         = 'N'
    };

    enum class CanceledReason : std::uint8_t {
        USER_REQUESTED            = 'U'
      , IMMEDIATE_OR_CANCEL       = 'I'
      , TIMEOUT                   = 'T'
      , SUPERVISORY               = 'S'
      , REGULATORY_RESTRICTION    = 'D'
      , SELF_MATCH_PREVENTION     = 'Q'
      , SYSTEM_CANCEL             = 'Z'
    };

    typedef NASDAQOrder::LiquidityFlag LiquidityFlag;

    typedef std::uint64_t MatchNumber;

    enum class BrokenTradeReason : std::uint8_t {
        ERRONEOUS       = 'E'
      , CONSENT         = 'C'
      , SUPERVISORY     = 'S'
      , EXTERNAL        = 'X'
    };

    enum class RejectedReason : std::uint8_t {
      /* Rejected Order Reasons: */
        TEST_MODE                                       = 'T'
      , HALTED                                          = 'H'
      , SHARES_EXCEEDS_SAFETY_THRESHOLD                 = 'Z'
      , INVALID_STOCK                                   = 'S'
      , INVALID_DISPLAY_TYPE                            = 'D'
      , CLOSED                                          = 'C'
      , FIRM_NOT_AUTHORIZED_FOR_CLEARING_TYPE           = 'L'
      , OUTSIDE_PERMITTED_TIMES_FOR_CLEARING_TYPE       = 'M'
      , NOT_ALLOWED_IN_TYPE_OF_CROSS                    = 'R'
      , INVALID_PRICE                                   = 'X'
      , INVALID_MINIMUM_QUANTITY                        = 'N'
      , OTHER                                           = 'O'
      /* PRM Rejected Order Reasons: */
      , REJECTED_ALL_ENABLED                            = 'a'
      , EASY_TO_BORROW_REJECT                           = 'b'
      , RESTRICTED_SYMBOL_LIST_REJECT                   = 'c'
      , ISO_ORDER_RESTRICTION                           = 'd'
      , ODD_LOT_ORDER_RESTRICTION                       = 'e'
      , MIDPOINT_ORDER_RESTRICTION                      = 'f'
      , PREMARKET_ORDER_RESTRICTION                     = 'g'
      , POSTMARKET_ORDER_RESTRICTION                    = 'h'
      , SHORT_SALE_ORDER_RESTRICTION                    = 'i'
      , ON_OPEN_ORDER_RESTRICTION                       = 'j'
      , ON_CLOSE_ORDER_RESTRICTION                      = 'k'
      , TWO_SIDED_QUOTE_REJECT                          = 'l'
      , EXCEEDED_SHARES_LIMIT                           = 'm'
      , EXCEEDED_DOLLAR_VALUE_LIMIT                     = 'n'
    };

} } } } }
