#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string>

#include <i01_core/Time.hpp>

namespace i01 { namespace OE {

    typedef std::uint8_t EngineID;
    const EngineID c_MIN_ENGINE_ID = 1;
    const EngineID c_MAX_ENGINE_ID = 7;

    typedef i01::core::Timestamp Timestamp;

    typedef std::string Symbol;

    typedef std::uint32_t Size;

    typedef std::uint16_t LocalAccount;

    typedef std::uint32_t LocalID;

    typedef std::uint64_t ExchangeID;

    typedef double Price;

    typedef std::int64_t Quantity;

typedef std::uint64_t ExchangeTimestamp;

typedef double Dollars;

const std::size_t CLIENT_ORDER_ID_LENGTH = 20;
using ClientOrderID = std::array<std::uint8_t,CLIENT_ORDER_ID_LENGTH>;
std::ostream& operator<<(std::ostream& os, const ClientOrderID& id);

    enum class Side {
        UNKNOWN           = 0
      , BUY               = 1 //< Buy
      , SELL              = 2 //< Sell
      , SHORT             = 3 //< Sell short
      , SHORT_EXEMPT      = 4 //< Sell short exempt
    };
inline Side operator-(const Side& s) noexcept {
    switch (s) {
        case Side::BUY:
            return Side::SELL;
        case Side::SELL:
        case Side::SHORT:
        case Side::SHORT_EXEMPT:
            return Side::BUY;
        default:
            return Side::UNKNOWN;
    }
}
inline int side_to_sign(const Side& s) noexcept {
    switch (s) {
        case Side::BUY:
            return +1;
        case Side::SELL:
        case Side::SHORT:
        case Side::SHORT_EXEMPT:
            return -1;
        default:
            return 0;
    }
}
std::ostream & operator<<(std::ostream &os, const Side &s);

    enum class OrderType {
        UNKNOWN           = 0
      , MARKET            = 1 //< Market order
      , LIMIT             = 2 //< Limit order
      , STOP              = 4 //< Stop order
      , MIDPOINT_PEG      = 8 //< Midpoint peg order
    };
std::ostream & operator<<(std::ostream &os, const OrderType &ot);

    enum class TimeInForce {
        UNKNOWN               = 0
      , DAY                   = 1 //< Day (Market Hours)
      , EXT                   = 2 //< Extended Hours
      , GTC                   = 3 //< Good until cancelled
      , AUCTION_OPEN          = 4 //< Opening auction/cross
      , AUCTION_CLOSE         = 5 //< Closing auction/cross
      , AUCTION_HALT          = 6 //< Halt auction/cross
      , TIMED                 = 7 //< Good until date/time
      , IMMEDIATE_OR_CANCEL   = 8 //< Immediate or cancel
      , FILL_OR_KILL          = 9 //< Fill or kill
    };
std::ostream & operator<<(std::ostream &os, const TimeInForce &s);

    /// State of the order.
    //  Initial state: NEW_AND_UNSENT
    //  Terminal states: UNKNOWN, LOCALLY_REJECTED, FULLY_FILLED,
    //                   CANCELLED, CANCEL_REPLACED, REMOTELY_REJECTED
    //
    enum class OrderState {
        UNKNOWN                     = 0
      , NEW_AND_UNSENT              = 1 //< Freshly created, unsent.
      , LOCALLY_REJECTED            = 2 //< Local rejection (unsent).
      , SENT                        = 3 //< Sent and unacknowledged.
      , ACKNOWLEDGED                = 4 //< Acknowledged by exchange.
      , PARTIALLY_FILLED            = 5 //< Partially filled.
      , FILLED                      = 6 //< Filled.
      , PENDING_CANCEL              = 7 //< Cancel sent.
      , PARTIALLY_CANCELLED         = 8 //< Partially cancelled.
      , CANCELLED                   = 9 //< Cancelled.
      , PENDING_CANCEL_REPLACE      = 10 //< Cancel replace sent.
      , CANCEL_REPLACED             = 11 //< Cancel replace confirmed.
      , REMOTELY_REJECTED           = 12 //< Rejected by exchange/remote.
      , CANCEL_REJECTED             = 13 //< Cancel rejected by remote.
    };
std::ostream & operator<<(std::ostream &os, const OrderState &s);

    typedef void* UserData;

    // TODO: consider making this a big enum with one entry per each
    // exchange's fee type, e.g. NYSE DMM take, SLP take, Floor take, etc.
    // which we can then index into an array of fee values to calculate PNL in
    // realtime.  for v1, we'll only support realtime pre-fee gross pnl
    // tracking.
    enum class FillFeeCode : std::uint32_t {
        UNKNOWN                                           = 0
        // 0x00000001 - 0x000000FF are reserved for OUCH 4.2 LiquidityFlag:
      , XNAS_ADDED                                             = 'A'
      , XNAS_REMOVED                                           = 'R'
      , XNAS_OPENING_CROSS_BILLABLE                            = 'O'
      , XNAS_OPENING_CROSS_NON_BILLABLE                        = 'M'
      , XNAS_CLOSING_CROSS_BILLABLE                            = 'C'
      , XNAS_CLOSING_CROSS_NON_BILLABLE                        = 'L'
      , XNAS_HALT_OR_IPO_CROSS_BILLABLE                        = 'H'
      , XNAS_HALT_OR_IPO_CROSS_NON_BILLABLE                    = 'K'
      , XNAS_NON_DISPLAYED_ADDING_LIQUIDITY                    = 'J'
      , XNAS_ADDED_POST_ONLY                                   = 'W' /* N/A */
      , XNAS_REMOVED_LIQUIDITY_AT_A_MIDPOINT                   = 'm'
      , XNAS_ADDED_LIQUIDITY_VIA_MIDPOINT_ORDER                = 'k'
      , XNAS_SUPPLEMENTAL_ORDER_EXECUTION                      = '0' /* ASCII zero */
      , XNAS_DISPLAYED_ADDING_IMPROVES_NBBO                    = '7'
      , XNAS_DISPLAYED_ADDING_SETS_QBBO_JOINS_NBBO             = '8'
      , XNAS_RETAIL_DESIGNATED_EXECUTION_REMOVED_LIQUIDITY     = 'd' /* N/A */
      , XNAS_RETAIL_DESIGNATED_ADDED_DISPLAYED_LIQUIDITY       = 'e'
      , XNAS_RETAIL_DESIGNATED_ADDED_NONDISPLAYED_LIQUIDITY    = 'f' /* N/A */
      , XNAS_RETAIL_PRICE_IMPROVING_PROVIDES_LIQUIDITY         = 'j'
      , XNAS_RETAIL_REMOVES_RPI_LIQUIDITY                      = 'r'
      , XNAS_RETAIL_REMOVES_OTHER_PRICE_IMPROVING_NONDISPLAYED = 't'
      , XNAS_ADDED_DISPLAYED_LIQUIDITY_IN_SELECT_SYMBOL        = '4'
      , XNAS_ADDED_NON_DISPLAYED_LIQUIDITY_IN_SELECT_SYMBOL    = '5'
      , XNAS_LIQUIDITY_REMOVING_ORDER_IN_SELECT_SYMBOL         = '6'
      , XNAS_ADDED_NON_DISPLAYED_MIDPOINT_LIQUIDITY_IN_SELECT  = 'g'
      , XNAS_RESERVED                                          = 0xFF
    };
std::ostream & operator<<(std::ostream &os, const FillFeeCode &s);

const char * const  DEFAULT_ORDERLOG_PATH = "orderlog";

} }
