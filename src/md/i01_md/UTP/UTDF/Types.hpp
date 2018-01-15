#pragma once

/* UTDF
 * 13.2 / April 2014
 */

#include <cstdint>
#include <array>
#include <i01_core/macro.hpp>

namespace i01 { namespace MD { namespace UTP { namespace UTDF { namespace Types {

    enum class MessageCategory : std::uint8_t {
        TRADE           = 'T'
      , ADMINISTRATIVE  = 'A'
      , CONTROL         = 'C'
      , VOLUME          = 'V'
    };

    enum class MessageType : std::uint8_t {
        TRADE_SHORT_FORM                          = 'A'
      , TRADE_LONG_FORM                           = 'W'
      , TRADE_CORRECTION                          = 'Y'
      , TRADE_CANCEL_ERROR                        = 'Z'
      , TRADE_PRIOR_DAY_AS_OF                     = 'H'
      , ADMIN_GENERAL                             = 'A'
      , ADMIN_ISSUE_SYMBOL_DIRECTORY              = 'B'
      , ADMIN_CROSS_SRO_TRADING                   = 'H'
      , ADMIN_MARKET_CENTER_TRADING_ACTION        = 'K'
      , REGSHO_SHORTSALE_PRICE_TEST_RESTRICTED    = 'V'
      , ADMIN_CLOSING_TRADE_SUMMARY               = 'Z'
      , VOLUME_TOTAL_CONSOLIDATED_AND_MARKET_CENTER_VOLUME = 'M' /* Category = V */
      , ADMIN_MWCB_DECLINE_LEVEL                  = 'C'
      , ADMIN_MWCB_STATUS                         = 'D'
      , ADMIN_PRICE_BAND                          = 'P'
      , CONTROL_START_OF_DAY                      = 'I'
      , CONTROL_END_OF_DAY                        = 'J'
      , CONTROL_MARKET_SESSION_OPEN               = 'O'
      , CONTROL_MARKET_SESSION_CLOSE              = 'C'
      , CONTROL_END_OF_RETRANS_REQUESTS           = 'K'
      , CONTROL_END_OF_TRANSMISSIONS              = 'Z'
      , CONTROL_LINE_INTEGRITY                    = 'T'
      , CONTROL_SEQNUM_RESET                      = 'L'
      , CONTROL_END_OF_CONSOLIDATED_LAST_SALE_ELIGIBILITY = 'S'
    };

    enum class SessionIdentifier : std::uint8_t {
        ALL_MARKET_SESSIONS_OR_SESSION_INDEPENDENT  = 'A'
      , US_MARKET_SESSION                           = 'U'
    };

    enum class RetransmissionRequesterGeneric : std::uint16_t {
        ORIGINAL_TRANSMISSION_TO_ALL_RECIPIENTS   = 0x4F20 // "O "
      , RETRANSMISSION_TO_ALL_RECIPIENTS          = 0x5220 // "R "
      , TEST_CYCLE_TRANSMISSION_TO_ALL            = 0x5420 // "T "
    };

    union RetransmissionRequester {
        RetransmissionRequesterGeneric generic;
        std::uint16_t vendor_specific;
    } __attribute__((packed));
    I01_ASSERT_SIZE(RetransmissionRequester, 2);

    union SeqNum {
        std::uint8_t u8[8];
        std::uint64_t u64;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SeqNum, 8);

    enum class MarketCenterID : std::uint8_t {
        NYSE_MKT          = 'A'
      , NASDAQ_OMX_BX     = 'B'
      , NSX               = 'C'
      , FINRA_ADF         = 'D'
      , INDEPENDENT_SIP   = 'E'
      , ISE               = 'I'
      , EDGA              = 'J'
      , EDGX              = 'K'
      , CHX               = 'M'
      , NYSE_EURONEXT     = 'N'
      , NYSE_ARCA         = 'P'
      , NASDAQ_OMX        = 'Q'
      , CBOE              = 'W'
      , NASDAQ_OMX_PHLX   = 'X'
      , BATS_Y            = 'Y'
      , BATS_Z            = 'Z'
      , NOT_APPLICABLE    = ' '
    };

    enum class SubMarketCenterID : std::uint8_t {
        NONE_OR_FINRA_ADF = ' '
      , NASDAQ_OMX = 'Q'
      , NYSE_MKT = 'A'
      , NASDAQ_OMX_BX = 'B'
      , NSX = 'C'
      , ISE = 'I'
      , CHX = 'M'
      , NYSE_EURONEXT = 'N'
      , NYSE_ARCA = 'P'
      , CBOE = 'W'
      , NASDAQ_OMX_PHLX = 'X'
      , BATS_Z = 'Z'
      , BATS_Y = 'Y'
      , EDGA = 'J'
      , EDGX = 'K'
    };

    union Time {
        std::uint8_t u8[9]; /* HHMMSSCCC */
        int hours() const { return (u8[0]-'0') * 10 + (u8[1]-'0'); }
        int minutes() const { return (u8[2]-'0') * 10 + (u8[3]-'0'); }
        int seconds() const { return (u8[4]-'0') * 10 + (u8[5]-'0'); }
        int milliseconds() const { return (u8[6]-'0') * 100 + (u8[7]-'0') * 10 + (u8[8]-'0'); }
        std::uint64_t milliseconds_since_midnight() const { return hours()*3600*1000 + minutes()*60*1000 + seconds()*1000 + milliseconds(); }
    } __attribute__((packed));
    I01_ASSERT_SIZE(Time, 9);

    typedef std::uint8_t IssueSymbol5[5];
    typedef std::uint8_t IssueSymbol11[11];

    enum class NBBOAppendageIndicator : std::uint8_t {
        NO_NBBO_CHANGE = '0'
      , NO_NBBO_CAN_BE_CALCULATED = '1'
      , SHORT_FORM_NBBO_APPENDAGE = '2'
      , LONG_FORM_NBBO_APPENDAGE = '3'
      , QUOTE_CONTAINS_ALL_NBBO_INFO = '4' /* not subject to NASD Short Sale Rule 3350 */
    };

    enum class LULDBBOIndicator : std::uint8_t {
        NOT_APPLICABLE = ' '
      , BID_PRICE_ABOVE_UPPER_LIMIT_NONEXECUTABLE = 'A'
      , ASK_PRICE_BELOW_LOWER_LIMIT_NONEXECUTABLE = 'B'
      , BID_AND_ASK_OUTSIDE_BAND_NONEXECUTABLE = 'C'
    };

    enum class LULDNBBOIndicator : std::uint8_t {
        NOT_APPLICABLE = ' '
      , NBBO_EXECUTABLE = 'A'
      , NBB_BELOW_LOWER_LIMIT_NONEXECUTABLE = 'B'
      , NBO_ABOVE_UPPER_LIMIT_NONEXECUTABLE = 'C'
      , NBBO_OUTSIDE_BAND_NONEXECUTABLE = 'D'
      , NBB_EQUALS_UPPER_LIMIT_LIMITSTATE = 'E'
      , NBO_EQUALS_LOWER_LIMIT_LIMITSTATE = 'F'
      , NBB_EQUALS_UPPER_LIMIT_LIMITSTATE_NBO_LOWER_NONEXECUTABLE = 'G'
      , NBO_EQUALS_LOWER_LIMIT_LIMITSTATE_NBB_UPPER_NONEXECUTABLE = 'H'
      , NBB_EQUALS_UPPER_NBO_EQUALS_LOWER_CROSSED_NOT_LIMITSTATE = 'I'
    };

    enum class FINRAADFMPIDAppendageIndicator : std::uint8_t {
        NO_ADF_MPID_CHANGES = '0'
      , NO_ADF_MPID_EXISTS = '1'
      , ADF_MPID_ATTACHED = '2'
      , NOT_APPLICABLE = ' '
    };

    typedef std::uint8_t Currency[3];

    union MPID {
        std::uint8_t u8[4];
        std::uint32_t u32;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MPID, 4);

    enum class Action : std::uint8_t {
        TRADING_HALT = 'H'
      , QUOTATION_RESUMPTION = 'Q'
      , TRADING_RESUMPTION_UTPS = 'T'
      , VOLATILITY_TRADING_PAUSE = 'P'
    };

    struct ActionDateTime { /* TODO: translation functions, see appendix F in spec */
        std::uint8_t year_u8[2];
        std::uint8_t month_u8;
        std::uint8_t day_u8;
        std::uint8_t hour_u8;
        std::uint8_t minute_u8;
        std::uint8_t second_u8;
        int year() const { return (year_u8[0] - '0')*10 + year_u8[1]; }
        int month() const { return month_u8 - '0'; }
        int day() const { return day_u8 - '0'; }
        int hour() const { return hour_u8 - '0'; }
        int minute() const { return minute_u8 - '0'; }
        int second() const { return second_u8 - '0'; }
        std::uint32_t seconds_since_midnight() const { return hour()*3600 + minute()*60 + second(); }
    } __attribute__((packed));
    I01_ASSERT_SIZE(ActionDateTime, 7);

    enum class PriceDenominator : std::uint8_t { /* appendix A */
        VAL_10        = 'A'
      , VAL_100       = 'B'
      , VAL_1000      = 'C'
      , VAL_10000     = 'D'
      , VAL_100000    = 'E'
      , VAL_1000000   = 'F'
      , VAL_10000000  = 'G'
      , VAL_100000000 = 'H'
    };

    //double pd2dbl(PriceDenominator& denom)
    //{ 
    //    switch (denom) {
    //    case PriceDenominator::VAL_10:
    //        return 10.0;
    //    case PriceDenominator::VAL_100:
    //        return 100.0;
    //    case PriceDenominator::VAL_1000:
    //        return 1000.0;
    //    case PriceDenominator::VAL_10000:
    //        return 10000.0;
    //    case PriceDenominator::VAL_100000:
    //        return 100000.0;
    //    case PriceDenominator::VAL_1000000:
    //        return 1000000.0;
    //    case PriceDenominator::VAL_10000000:
    //        return 10000000.0;
    //    case PriceDenominator::VAL_100000000:
    //        return 100000000.0;
    //    };
    //}

    typedef std::uint8_t Price6[6];
    typedef std::uint8_t Price10[10];
    typedef std::uint8_t Price12[12];
    typedef std::uint8_t Size2[2];
    typedef std::uint8_t Size5[5];
    typedef std::uint8_t Size7[7];

    enum class Authenticity : std::uint8_t {
        LIVE_PRODUCTION = 'P'
      , TEST = 'T'
      , DEMO = 'D'
      , DELETED = 'X'
    };

    enum class FinancialStatusIndicator : std::uint8_t {
        CREATIONS_SUSPENDED = 'C'
      , DEFICIENT = 'D'
      , DELINQUENT = 'E'
      , BANKRUPT = 'Q'
      , NORMAL_DEFAULT = 'N'
      , DEFICIENT_AND_BANKRUPT = 'G'
      , DEFICIENT_AND_DELINQUENT = 'H'
      , DELINQUENT_AND_BANKRUPT = 'J'
      , DEFICIENT_DELINQUENT_AND_BANKRUPT = 'K'
    };

    typedef std::uint8_t IssueName[30];

    enum class IssueType : std::uint8_t {
        AMERICAN_DEPOSITORY_RECEIPT   = 'A'
      , BOND                          = 'B'
      , COMMON_SHARES                 = 'C'
      , BOND_DERIVATIVE               = 'D'
      , EQUITY_DERIVATIVE             = 'E'
      , DEPOSITORY_RECEIPT            = 'F'
      , CORPORATE_BOND                = 'G'
      , LIMITED_PARTNERSHIP           = 'L'
      , MISCELLANEOUS                 = 'M'
      , NOTE                          = 'N'
      , ORDINARY_SHARES               = 'O'
      , PREFERRED_SHARES              = 'P'
      , RIGHTS                        = 'R'
      , SHARES_OF_BENEFICIAL_INTEREST = 'S'
      , CONVERTIBLE_DEBENTURE         = 'T'
      , UNIT                          = 'U'
      , UNITS_OF_BENEFICIAL_INTEREST  = 'V'
      , WARRANT                       = 'W'
      , INDEX_WARRANT                 = 'X'
      , PUT_WARRANT                   = 'Y'
      , UNCLASSIFIED                  = 'Z'
    };

    enum class LULDPriceBandIndicator : std::uint8_t {
        OPENING_UPDATE = 'A'
      , INTRADAY_UPDATE = 'B'
      , RESTATED_VALUE = 'C'
      , SUSPENDED_DURING_HALT_OR_PAUSE = 'D'
      , REOPENING_UPDATE = 'E'
      , OUTSIDE_PRICE_BAND_RULE_HOURS = 'F'
      , NONE = ' '
    };

    enum class MarketCategory : std::uint8_t {
        NASDAQ_GLOBAL_SELECT_MARKET = 'Q'
      , NASDAQ_GLOBAL_MARKET = 'G'
      , NASDAQ_CAPITAL_MARKET = 'S'
    };

    enum class MWCBStatusLevelIndicator : std::uint8_t {
        MWCB_DECLINE_LEVEL_1_BREACHED_07_PERCENT_DECLINE = '1'
      , MWCB_DECLINE_LEVEL_2_BREACHED_13_PERCENT_DECLINE = '2'
      , MWCB_DECLINE_LEVEL_3_BREACHED_20_PERCENT_DECLINE = '3'
    };

    enum class QuoteCondition : std::uint8_t {
        MANUAL_ASK_AUTOMATED_BID = 'A'
      , MANUAL_BID_AUTOMATED_ASK = 'B'
      , FAST_TRADING = 'F'
      , MANUAL_BID_AND_ASK = 'H'
      , ORDER_IMBALANCE = 'I'
      , CLOSED_QUOTE = 'L'
      , NON_FIRM_QUOTE = 'N'
      , OPENING_QUOTE_AUTOMATED = 'O'
      , REGULAR_TWO_SIDED_OPEN_QUOTE_AUTOMATED = 'R'
      , MANUAL_BID_AND_ASK_NON_FIRM = 'U'
      , Y_ONE_SIDED_AUTOMATED = 'Y'
      , ORDER_INFLUX = 'X'
      , NO_OPEN_NO_RESUME = 'Z'
    };

    enum class NQuoteCondition : std::uint8_t {
        NBBO_CLOSED = 'L'
      , NBBO_REGULAR_TWO_SIDED_OPEN = 'R'
      , NBBO_REGULAR_ONE_SIDED_OPEN = 'Y'
    };

    union ReasonCode {
        std::uint8_t u8[6];
        // TODO: function to convert to enum?
    } __attribute__((packed));
    I01_ASSERT_SIZE(ReasonCode, 6);

    enum class RegSHOAction : std::uint8_t {
        NO_PRICE_TEST_IN_EFFECT = '0'
      , REG_SHO_SHORT_SALE_PRICE_TEST_INTRADAY_PRICE_DROP = '1'
      , REG_SHO_SHORT_SALE_PRICE_TEST_REMAINS_IN_EFFECT = '2'
    };

    enum class RetailInterestIndicator : std::uint8_t {
        NOT_APPLICABLE = ' '
      , RETAIL_INTEREST_ON_BID_QUOTE = 'A'
      , RETAIL_INTEREST_ON_ASK_QUOTE = 'B'
      , RETAIL_INTEREST_ON_BOTH_SIDES = 'C'
    };

    enum class YesNoNA : std::uint8_t { YES = 'Y', NO = 'N', NA = ' ' };

    enum class SIPGeneratedUpdate : std::uint8_t {
        MARKET_PARTICIPANT_GENERATED = ' '
      , SIP_GENERATED_TRANSACTION = 'E' /* e.g. price band change */
    };

    enum class SpecialCondition : std::uint8_t {
        ONE_SIDED_NBBO_AT_MARKET_CLOSE = 'O'
      , TRADING_HALT_IN_EFFECT_AT_MARKET_CLOSE = 'H'
      , NO_ELIGIBLE_MARKET_PARTICIPANT_QUOTES_IN_ISSUE_AT_MARKET_CLOSE = 'M'
      , NOT_APPLICABLE = ' '
    };

    /* UTDF */

    enum class SaleCondition1 : std::uint8_t {
        REGULAR_SALE = '@'
      , ACQUISITION = 'A'
      , BUNCHED_TRADE = 'B'
      , CASH_SALE = 'C'
      , DISTRIBUTION = 'D'
      , PLACEHOLDER_FOR_FUTURE_USE = 'E'
      , INTERMARKET_SWEEP = 'F'
      , BUNCHED_SOLD_TRADE = 'G'
      , PRICE_VARIATION_TRADE = 'H'
      , ODD_LOT_TRADE = 'I'
      , RULE_155_TRADE_AMEX = 'K'
      , SOLD_LAST = 'L'
      , MARKET_CENTER_OFFICIAL_CLOSE = 'M'
      , NEXT_DAY = 'N'
      , OPENING_PRINTS = 'O'
      , PRIOR_REFERENCE_PRICE = 'P'
      , MARKET_CENTER_OFFICIAL_OPEN = 'Q'
      , SELLER = 'R'
      , SPLIT_TRADE = 'S'
      , FORM_T = 'T'
      , EXTENDED_TRADING_HOURS_OUT_OF_SEQUENCE = 'U'
      , STOCK_OPTION_TRADE = 'V'
      , AVERAGE_PRICE_TRADE = 'W'
      , CROSS_TRADE = 'X'
      , YELLOW_FLAG_REGULAR_TRADE = 'Y'
      , SOLD_OUT_OF_SEQUENCE = 'Z'
      , STOPPED_STOCK_REGULAR_TRADE = '1'
      /* Stopped stock: sold last and sold out of sequence not supported */
      , DERIVATIVELY_PRICED = '4'
      , REOPENING_PRINTS = '5'
      , CLOSING_PRINTS = '6'
      , PLACEHOLDER_FOR_611_EXEMPT_1 = '7'
      , PLACEHOLDER_FOR_611_EXEMPT_2 = '8'
      , CORRECTED_CONSOLIDATED_CLOSE = '9'
    };

    union SaleCondition4 {
        std::uint8_t u8[4];
        std::uint32_t u32;
    } __attribute__((packed));
    I01_ASSERT_SIZE(SaleCondition4, 4);

    typedef std::uint8_t Size6[6];
    typedef std::uint8_t Size9[9];
    typedef std::uint8_t Size11[11];
    typedef std::uint8_t Size12[12];

    enum class ConsolidatedPriceChangeIndicator : std::uint8_t {
        NO_PRICES_CHANGED = '0'
      , CONSOLIDATED_LAST_PRICE_CHANGED = '1'
      , CONSOLIDATED_LOW_PRICE_CHANGED = '2'
      , CONSOLIDATED_LAST_AND_LOW_PRICE_CHANGED = '3'
      , CONSOLIDATED_HIGH_PRICE_CHANGED = '4'
      , CONSOLIDATED_LAST_AND_HIGH_CHANGED = '5'
      , CONSOLIDATED_HIGH_AND_LOW_CHANGED = '6'
      , ALL_CONSOLIDATED_CHANGED = '7'
    };

    enum class TradeThroughExemptFlag : std::uint8_t {
        TRADE_THROUGH_EXEMPT_611 = 'X'
      , NOT_TRADE_THROUGH_EXEMPT_611 = ' '
    };

    enum class Function : std::uint8_t {
        CANCEL = 'C'
      , ERROR = 'E'
    };

    enum class AsOfAction : std::uint8_t {
        AS_OF_TRADE_ADDITION = 'A'
      , AS_OF_TRADE_CANCEL = 'C'
    };
    enum class TradingAction : std::uint8_t {
        HALT_IN_EFFECT = 'H'
      , REGULAR_TRADING = ' '
    };

    enum class MarketCenterCloseIndicator : std::uint8_t {
       CLOSING_PRICE_BASED_ON_M_SALE_CONDITION_MODIFIER = 'M'
     , NOT_BASED_ON_M_SALE_CONDITION = ' '
    };

} } } } }
