#include <i01_core/util.hpp>

#include <i01_md/NASDAQ/ITCH50/Messages.hpp>

using namespace i01::MD::NASDAQ::ITCH50::Types;

namespace i01 { namespace MD { namespace NASDAQ { namespace ITCH50 { namespace Messages {

using i01::core::operator<<;

std::ostream & operator<<(std::ostream &os, const ExtMessageHeader &evt)
{
    std::uint64_t tt = evt.timestamp.u64_48;
    return os << (char) evt.msg_type << ","
              << evt.stock_locate << ","
              << evt.tracking_number << ","
              << tt;
}
std::ostream & operator<<(std::ostream &os, const SystemEvent &evt)
{
    return os << evt.hdr << ","
              << (char) evt.event_code;
}

std::ostream & operator<<(std::ostream &os, const StockDirectory &evt)
{
    return os << evt.hdr << ","
              << evt.stock << ","
              << (char) evt.market_category << ","
              << (char) evt.financial_status_indicator << ","
              << evt.round_lot_size << ","
              << (char) evt.round_lots_only << ","
              << (char) evt.issue_classification << ","
              << evt.issue_subtype.arr << ","
              << (char) evt.authenticity << ","
              << (char) evt.short_sale_threshold_indicator << ","
              << (char) evt.ipo_flag << ","
              << (char) evt.luld_reference_price_tier << ","
              << (char) evt.etp_flag << ","
              << evt.etp_leverage_factor << ","
              << (char) evt.inverse_indicator;
}

std::ostream & operator<<(std::ostream &os, const StockTradingAction &evt)
{
    return os << evt.hdr << ","
              << evt.stock << ","
              << (char) evt.trading_state << ","
              << evt.reason.arr;
}

std::ostream & operator<<(std::ostream &os, const RegSHORestriction &evt)
{
    return os << evt.hdr << ","
              << evt.stock << ","
              << (char) evt.reg_sho_action;
}

std::ostream & operator<<(std::ostream &os, const MarketParticipantPosition &evt)
{
    return os << evt.hdr << ","
              << evt.mpid.arr << ","
              << evt.stock << ","
              << (char) evt.primary_market_maker << ","
              << (char) evt.market_maker_mode << ","
              << (char) evt.market_participant_state;
}

std::ostream & operator<<(std::ostream &os, const MWCBDeclineLevel &evt)
{
    return os << evt.hdr << ","
              << evt.level1 << ","
              << evt.level2 << ","
              << evt.level3;
}

std::ostream & operator<<(std::ostream &os, const MWCBBreach &evt)
{
    return os << evt.hdr << ","
              << (char) evt.breached_level;
}

std::ostream & operator<<(std::ostream &os, const IPOQuotingPeriodUpdate &evt)
{
    return os << evt.hdr << ","
              << evt.stock << ","
              << evt.ipo_quotation_release_time << ","
              << (char) evt.ipo_quotation_release_qualifier << ","
              << evt.price;
}

std::ostream & operator<<(std::ostream &os, const AddOrder &evt)
{
    return os << evt.hdr << ","
              << evt.order_refnum << ","
              << (char) evt.buy_sell_indicator << ","
              << evt.shares << ","
              << evt.stock << ","
              << evt.price;
}

std::ostream & operator<<(std::ostream &os, const AddOrderMPIDAttribution &evt)
{
    return os << evt.add_order << ","
              << evt.attribution.arr;
}

std::ostream & operator<<(std::ostream &os, const OrderExecuted &evt)
{
    return os << evt.hdr << ","
              << evt.order_refnum << ","
              << evt.executed_shares << ","
              << evt.match_number;
}

std::ostream & operator<<(std::ostream &os, const OrderExecutedWithPrice &evt)
{
    return os << evt.order_executed << ","
              << (char) evt.printable << ","
              << evt.execution_price;
}

std::ostream & operator<<(std::ostream &os, const OrderCancel &evt)
{
    return os << evt.hdr << ","
              << evt.order_refnum << ","
              << evt.canceled_shares;
}

std::ostream & operator<<(std::ostream &os, const OrderDelete &evt)
{
    return os << evt.hdr << ","
              << evt.order_refnum;
}

std::ostream & operator<<(std::ostream &os, const OrderReplace &evt)
{
    return os << evt.hdr << ","
              << evt.original_order_refnum << ","
              << evt.new_order_refnum << ","
              << evt.shares << ","
              << evt.price;
}

std::ostream & operator<<(std::ostream &os, const TradeNonCross &evt)
{
    return os << evt.hdr << ","
              << evt.order_refnum << ","
              << (char) evt.buy_sell << ","
              << evt.shares << ","
              << evt.stock << ","
              << evt.price << ","
              << evt.match_number;
}

std::ostream & operator<<(std::ostream &os, const TradeCross &evt)
{
    return os << evt.hdr << ","
              << evt.shares << ","
              << evt.stock << ","
              << evt.cross_price << ","
              << evt.match_number << ","
              << evt.cross_type;
}

std::ostream & operator<<(std::ostream &os, const BrokenTrade &evt)
{
    return os << evt.hdr << ","
              << evt.match_number;
}

std::ostream & operator<<(std::ostream &os, const NetOrderImbalanceIndicator &evt)
{
    return os << evt.hdr << ","
              << evt.paired_shares << ","
              << evt.imbalance_shares << ","
              << evt.imbalance_direction << ","
              << evt.stock << ","
              << evt.far_price << ","
              << evt.near_price << ","
              << evt.current_reference_price << ","
              << evt.cross_type << ","
              << evt.price_variation_indicator;
}

std::ostream & operator<<(std::ostream &os, const RetailPriceImprovementIndicator &evt)
{
    return os << evt.hdr << ","
              << evt.stock << ","
              << (char) evt.interest_flag;
}

}}}}}
