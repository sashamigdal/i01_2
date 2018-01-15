#pragma once

#include <i01_core/macro.hpp>
#include "Types.hpp"

namespace i01 { namespace MD { namespace UTP { namespace UQDF { namespace Messages {

    using namespace i01::MD::UTP::UQDF::Types;

    struct MessageHeader {
        MessageCategory category;
        MessageType type;
        SessionIdentifier session_id;
        RetransmissionRequester retrans_requester;
        SeqNum seqnum;
        MarketCenterID market_center_originator_id;
        Time timestamp;
        std::uint8_t filler;
    } __attribute__((packed));
    I01_ASSERT_SIZE(MessageHeader, 24);

    struct QuoteBBOShortForm { /* Category Q - Type E */
        IssueSymbol5 issue_symbol;
        std::uint8_t reserved;
        SIPGeneratedUpdate sip_generated_update;
        QuoteCondition quote_condition;
        std::uint8_t luld_bbo_indicator;
        PriceDenominator bid_price_denominator;
        Price6 bid_price;
        Size2 bid_size;
        PriceDenominator ask_price_denominator;
        Price6 ask_price;
        Size2 ask_size;
        NBBOAppendageIndicator national_bbo_appendage_indicator;
        LULDNBBOIndicator luld_national_bbo_indicator;
        FINRAADFMPIDAppendageIndicator finra_adf_mpid_appendage_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(QuoteBBOShortForm, 30);

    struct QuoteBBOLongForm { /* Category Q - Type F */
        IssueSymbol11 issue_symbol;
        std::uint8_t reserved;
        SIPGeneratedUpdate sip_generated_update;
        QuoteCondition quote_condition;
        std::uint8_t luld_bbo_indicator;
        RetailInterestIndicator retail_interest_indicator;
        PriceDenominator bid_price_denominator;
        Price10 bid_price;
        Size7 bid_size;
        PriceDenominator ask_price_denominator;
        Price10 ask_price;
        Size7 ask_size;
        Currency currency;
        NBBOAppendageIndicator national_bbo_appendage_indicator;
        LULDNBBOIndicator luld_national_bbo_indicator;
        FINRAADFMPIDAppendageIndicator finra_adf_mpid_appendage_indicator;
    } __attribute__((packed));
    I01_ASSERT_SIZE(QuoteBBOLongForm, 58);

    struct ShortFormNBBOAppendage {
        NQuoteCondition nbbo_quote_condition;
        MarketCenterID nbb_market_center;
        PriceDenominator nbb_price_denominator;
        Price6 nbb_price;
        Size2 nbb_size;
        std::uint8_t reserved;
        MarketCenterID nbo_market_center;
        PriceDenominator nbo_price_denominator;
        Price6 nbo_price;
        Size2 nbo_size;
    } __attribute__((packed));
    I01_ASSERT_SIZE(ShortFormNBBOAppendage, 22);

    struct LongFormNBBOAppendage {
        NQuoteCondition nbbo_quote_condition;
        MarketCenterID nbb_market_center;
        PriceDenominator nbb_price_denominator;
        Price10 nbb_price;
        Size7 nbb_size;
        std::uint8_t reserved;
        MarketCenterID nbo_market_center;
        PriceDenominator nbo_price_denominator;
        Price10 nbo_price;
        Size7 nbo_size;
        Currency currency;
    } __attribute__((packed));
    I01_ASSERT_SIZE(LongFormNBBOAppendage, 43);

    struct FINRAADFMPIDAppendage {
        MPID bid_adf_mpid;
        MPID ask_adf_mpid;
    } __attribute__((packed));
    I01_ASSERT_SIZE(FINRAADFMPIDAppendage, 8);

    struct GeneralAdministrative {
        std::uint8_t text[300]; /* UP TO 300 */
    } __attribute__((packed));
    I01_ASSERT_SIZE(GeneralAdministrative, 300); /* UP TO 300 */

    struct CrossSROTradingAction {
        IssueSymbol11 issue_symbol;
        Action action;
        ActionDateTime action_datetime;
        ReasonCode reason_code;
    } __attribute__((packed));
    I01_ASSERT_SIZE(CrossSROTradingAction, 25);

    struct SessionCloseRecap_ClosingQuote {
        IssueSymbol11 issue_symbol;
        MarketCenterID nbb_market_center;
        PriceDenominator nbb_price_denominator;
        Price10 nbb_price;
        Size7 nbb_size;
        std::uint8_t reserved;
        MarketCenterID nbo_market_center;
        PriceDenominator nbo_price_denominator;
        Price10 nbo_price;
        Size7 nbo_size;
        Currency currency;
        SpecialCondition special_condition;
        std::uint8_t number_of_market_center_attachments[2];
    } __attribute__((packed));
    I01_ASSERT_SIZE(SessionCloseRecap_ClosingQuote, 56);

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

