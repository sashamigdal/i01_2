#pragma once

#include <i01_core/macro.hpp>
#include "Types.hpp"

namespace i01 { namespace MD { namespace UTP { namespace UTDF { namespace Messages {

    using namespace i01::MD::UTP::UTDF::Types;

    struct MessageHeader {
        MessageCategory category;
        MessageType type;
        SessionIdentifier session_id;
        RetransmissionRequester retrans_requester;
        SeqNum seqnum;
        MarketCenterID market_center_originator_id;
        Time timestamp;
        SubMarketCenterID sub_market_center_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MessageHeader, 24);

    struct TradeShortForm { /* Category T - Type A */
        IssueSymbol5 issue_symbol;
        SaleCondition1 sale_condition;
        PriceDenominator price_denominator;
        Price6 price;
        Size6 report_volume;
        ConsolidatedPriceChangeIndicator consolidated_price_change_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TradeShortForm, 20);

    struct InlineTrade {
        TradeThroughExemptFlag trade_through_exempt_flag;
        SaleCondition4 sale_condition;
        std::uint8_t sellers_sale_days[2];
        PriceDenominator price_denominator;
        Price10 price;
        Currency currency;
        Size9 report_volume;
    } __attribute__((packed));
    I01_ASSERT_SIZE(InlineTrade, 30);

    struct TradeLongForm { /* Category T - Type W */
        IssueSymbol11 issue_symbol;
        InlineTrade trade;
        ConsolidatedPriceChangeIndicator consolidated_price_change_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TradeLongForm, 42);

    struct TradeCorrection {
        SeqNum original_seqnum;
        IssueSymbol11 issue_symbol;
        InlineTrade original_trade;
        InlineTrade corrected_trade;
        PriceDenominator consolidated_high_price_denominator;
        Price10 consolidated_high_price;
        PriceDenominator consolidated_low_price_denominator;
        Price10 consolidated_low_price;
        PriceDenominator consolidated_last_sale_price_denominator;
        Price10 consolidated_last_sale_price;
        MarketCenterID consolidated_last_sale_market_center;
        Currency currency;
        Size11 consolidated_volume;
        ConsolidatedPriceChangeIndicator consolidated_price_change_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TradeCorrection, 128);

    struct TradeCancelError {
        SeqNum original_seqnum;
        IssueSymbol11 issue_symbol;
        Function function;
        InlineTrade original_trade;
        PriceDenominator consolidated_high_price_denominator;
        Price10 consolidated_high_price;
        PriceDenominator consolidated_low_price_denominator;
        Price10 consolidated_low_price;
        PriceDenominator consolidated_last_sale_price_denominator;
        Price10 consolidated_last_sale_price;
        MarketCenterID consolidated_last_sale_market_center;
        Currency currency;
        Size11 consolidated_volume;
        ConsolidatedPriceChangeIndicator consolidated_price_change_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(TradeCancelError, 99);

    struct PriorDayAsOfTrade {
        IssueSymbol11 issue_symbol;
        SeqNum prior_day_seqnum;
        InlineTrade prior_day_trade;
        ActionDateTime prior_day_trade_datetime;
        AsOfAction as_of_action;
    } __attribute__((packed));
    I01_ASSERT_SIZE(PriorDayAsOfTrade, 57);

    struct GeneralAdministrative {
        std::uint8_t text[300]; /* UP TO 300 */
    } __attribute__((packed));
    I01_ASSERT_SIZE(GeneralAdministrative, 300); /* UP TO 300 */

    struct ConsolidatedTradeSummary {
        IssueSymbol11 issue_symbol;
        PriceDenominator daily_consolidated_high_price_denominator;
        Price10 daily_consolidated_high_price;
        PriceDenominator daily_consolidated_low_price_denominator;
        Price10 daily_consolidated_low_price;
        PriceDenominator daily_consolidated_closing_price_denominator;
        Price10 daily_consolidated_closing_price;
        MarketCenterID consolidated_closing_price_market_center;
        Currency currency;
        Size11 consolidated_volume;
        TradingAction trading_action;
        std::uint8_t number_of_market_center_attachments[2];
    } __attribute__((packed));
    I01_ASSERT_SIZE(ConsolidatedTradeSummary, 62);
    struct MarketCenterClosingPriceVolumeSummary {
        MarketCenterID market_center;
        PriceDenominator closing_price_denominator;
        Price10 closing_price;
        Size11 volume;
        MarketCenterCloseIndicator close_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MarketCenterClosingPriceVolumeSummary, 24);

    struct TotalConsolidatedAndMarketCenterVolume {
        Size12 total_consolidated_volume;
        std::uint8_t numer_of_market_center_attachments[2];
    } __attribute__((packed));
    I01_ASSERT_SIZE(TotalConsolidatedAndMarketCenterVolume, 14);
    struct MarketCenterVolume {
        MarketCenterID market_center;
        Size12 volume;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MarketCenterVolume, 13);

    struct CrossSROTradingAction {
        IssueSymbol11 issue_symbol;
        Action action;
        ActionDateTime action_datetime;
        ReasonCode reason_code;
    } __attribute__((packed));
    I01_ASSERT_SIZE(CrossSROTradingAction, 25);

    struct MarketCenterAttachment {
        MarketCenterID market_center_id;
        PriceDenominator bid_price_denominator;
        Price10 bid_price;
        Size7 bid_size;
        PriceDenominator ask_price_denominator;
        Price10 ask_price;
        Size7 ask_size;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MarketCenterAttachment, 37);

    struct MarketCenterTradingAction {
        IssueSymbol11 issue_symbol;
        Action action;
        ActionDateTime action_datetime;
        MarketCenterID market_center_id;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MarketCenterTradingAction, 20);

    struct IssueSymbolDirectory {
        IssueSymbol11 issue_symbol;
        IssueSymbol11 old_issue_symbol;
        IssueName issue_name;
        IssueType issue_type;
        MarketCategory market_category;
        Authenticity authenticity;
        YesNoNA short_sale_threshold_indicator; /* Restricted under SEC Rule 203(b)(3)? [(Y)es/(N)o/( )NA] */
        Size5 round_lot_size;
        FinancialStatusIndicator financial_status_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(IssueSymbolDirectory, 62);

    struct RegSHOShortSalePriceTestRestricted {
        IssueSymbol11 issue_symbol;
        RegSHOAction reg_sho_action;
    } __attribute__((packed));
    I01_ASSERT_SIZE(RegSHOShortSalePriceTestRestricted, 12);

    struct LULDPriceBand {
        IssueSymbol11 issue_symbol;
        LULDPriceBandIndicator luld_price_band_indicator;
        Time luld_price_band_effective_time;
        PriceDenominator ld_price_denominator;
        Price10 ld_price;
        PriceDenominator lu_price_denominator;
        Price10 lu_price;
    } __attribute__((packed));
    I01_ASSERT_SIZE(LULDPriceBand, 43);

    struct MWCBDeclineLevel {
        PriceDenominator mwcb_denominator;
        Price12 mwcb_level1;
        std::uint8_t reserved1[3];
        Price12 mwcb_level2;
        std::uint8_t reserved2[3];
        Price12 mwcb_level3;
        std::uint8_t reserved3[3];
    } __attribute__((packed));
    I01_ASSERT_SIZE(MWCBDeclineLevel, 46);

    struct MWCBStatus {
        MWCBStatusLevelIndicator mwcb_status_level;
        std::uint8_t reserved[3];
    } __attribute__((packed));
    I01_ASSERT_SIZE(MWCBStatus, 4);

    // TODO: Control messages are message headers only...

} } } } }

