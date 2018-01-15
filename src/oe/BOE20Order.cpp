#include <algorithm>
#include <iostream>

#include <i01_oe/BOE20Order.hpp>

namespace i01 { namespace OE {

BOE20Order::BOE20Order(Instrument* const iinst,
           const Price iprice,
           const Size isize,
           const Side iside,
           const TimeInForce itif,
           const OrderType itype,
           OrderListener* ilistener,
           const UserData iuserdata) :
    Order(iinst, iprice, isize, iside, itif, itype, ilistener, iuserdata),
    m_display_indicator(DisplayIndicator::UNKNOWN),
    m_exec_inst(ExecInst::UNKNOWN),
    m_routing_inst{0}
{

}

std::ostream& operator<<(std::ostream& os, const BOE20Order::DisplayIndicator& d)
{
    switch (d) {
    case BOE20Order::DisplayIndicator::UNKNOWN:
        os << "UNKNOWN";
        break;
    case BOE20Order::DisplayIndicator::DEFAULT:
        os << "DEFAULT";
        break;
    case BOE20Order::DisplayIndicator::PRICE_ADJUST:
        os << "PRICE_ADJUST";
        break;
    case BOE20Order::DisplayIndicator::MULTIPLE_PRICE_ADJUST:
        os << "MULTIPLE_PRICE_ADJUST";
        break;
    case BOE20Order::DisplayIndicator::CANCEL_BACK_IF_NEEDS_ADJUSTMENT:
        os << "CANCEL_BACK_IF_NEEDS_ADJUSTMENT";
        break;
    case BOE20Order::DisplayIndicator::HIDDEN:
        os << "HIDDEN";
        break;
    case BOE20Order::DisplayIndicator::DISPLAY_PRICE_SLIDING:
        os << "DISPLAY_PRICE_SLIDING";
        break;
    case BOE20Order::DisplayIndicator::DISPLAY_PRICE_SLIDING_NO_CROSS:
        os << "DISPLAY_PRICE_SLIDING_NO_CROSS";
        break;
    case BOE20Order::DisplayIndicator::MULTIPLE_DISPLAY_PRICE_SLIDING:
        os << "MULTIPLE_DISPLAY_PRICE_SLIDING";
        break;
    case BOE20Order::DisplayIndicator::HIDE_NOT_SLIDE_DEPRECATED:
        os << "HIDE_NOT_SLIDE_DEPRECATED";
        break;
    case BOE20Order::DisplayIndicator::VISIBLE:
        os << "VISIBLE";
        break;
    case BOE20Order::DisplayIndicator::INVISIBLE:
        os << "INVISIBLE";
        break;
    case BOE20Order::DisplayIndicator::NO_RESCRAPE_AT_LIMIT:
        os << "NO_RESCRAPE_AT_LIMIT";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const BOE20Order::ExecInst& e)
{
    switch (e) {
    case BOE20Order::ExecInst::UNKNOWN:
        os << "UNKNOWN";
        break;
    case BOE20Order::ExecInst::DEFAULT:
        os << "DEFAULT";
        break;
    case BOE20Order::ExecInst::INTERMARKET_SWEEP:
        os << "INTERMARKET_SWEEP";
        break;
    case BOE20Order::ExecInst::MARKET_PEG:
        os << "MARKET_PEG";
        break;
    case BOE20Order::ExecInst::MARKET_MAKER_PEG:
        os << "MARKET_MAKER_PEG";
        break;
    case BOE20Order::ExecInst::PRIMARY_PEG:
        os << "PRIMARY_PEG";
        break;
    case BOE20Order::ExecInst::SUPPLEMENTAL_PEG:
        os << "SUPPLEMENTAL_PEG";
        break;
    case BOE20Order::ExecInst::LISTING_MARKET_OPENING:
        os << "LISTING_MARKET_OPENING";
        break;
    case BOE20Order::ExecInst::LISTING_MARKET_CLOSING:
        os << "LISTING_MARKET_CLOSING";
        break;
    case BOE20Order::ExecInst::BOTH_LISTING_MARKET_OPEN_N_CLOSE:
        os << "BOTH_LISTING_MARKET_OPEN_N_CLOSE";
        break;
    case BOE20Order::ExecInst::MIDPOINT_DISCRETIONARY:
        os << "MIDPOINT_DISCRETIONARY";
        break;
    case BOE20Order::ExecInst::MIDPOINT_PEG_TO_NBBO_MID:
        os << "MIDPOINT_PEG_TO_NBBO_MID";
        break;
    case BOE20Order::ExecInst::MIDPOINT_PEG_TO_NBBO_MID_NO_LOCK:
        os << "MIDPOINT_PEG_TO_NBBO_MID_NO_LOCK";
        break;
    case BOE20Order::ExecInst::ALTERNATIVE_MIDPOINT:
        os << "ALTERNATIVE_MIDPOINT";
        break;
    case BOE20Order::ExecInst::LATE:
        os << "LATE";
        break;
    case BOE20Order::ExecInst::MIDPOINT_MATCH_DEPRECATED:
        os << "MIDPOINT_MATCH_DEPRECATED";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const BOE20Order::RoutingInstFirstChar& ri)
{
    switch (ri) {
    case BOE20Order::RoutingInstFirstChar::UNKNOWN:
        os << "UNKNOWN";
        break;
    case BOE20Order::RoutingInstFirstChar::BOOK_ONLY:
        os << "BOOK_ONLY";
        break;
    case BOE20Order::RoutingInstFirstChar::POST_ONLY:
        os << "POST_ONLY";
        break;
    case BOE20Order::RoutingInstFirstChar::POST_ONLY_AT_LIMIT:
        os << "POST_ONLY_AT_LIMIT";
        break;
    case BOE20Order::RoutingInstFirstChar::ROUTABLE:
        os << "ROUTABLE";
        break;
    case BOE20Order::RoutingInstFirstChar::SUPER_AGGRESSIVE:
        os << "SUPER_AGGRESSIVE";
        break;
    case BOE20Order::RoutingInstFirstChar::AGGRESSIVE:
        os << "AGGRESSIVE";
        break;
    case BOE20Order::RoutingInstFirstChar::SUPER_AGGRESSIVE_WHEN_ODD_LOT:
        os << "SUPER_AGGRESSIVE_WHEN_ODD_LOT";
        break;
    case BOE20Order::RoutingInstFirstChar::POST_TO_AWAY:
        os << "POST_TO_AWAY";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const BOE20Order::RoutingInstSecondChar& ri)
{
    switch (ri) {
    case BOE20Order::RoutingInstSecondChar::UNKNOWN:
        os << "UNKNOWN";
        break;
    case BOE20Order::RoutingInstSecondChar::ELIGIBLE_TO_ROUTE_TO_DRT_CLC:
        os << "ELIGIBLE_TO_ROUTE_TO_DRT_CLC";
        break;
    case BOE20Order::RoutingInstSecondChar::ROUTE_TO_DISPLAYED_MKTS_ONLY:
        os << "ROUTE_TO_DISPLAYED_MKTS_ONLY";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const BOE20Order::RoutingInst& ri)
{
    return os << static_cast<BOE20Order::RoutingInstFirstChar>(ri.text.raw[0]) << "-"
              << static_cast<BOE20Order::RoutingInstSecondChar>(ri.text.raw[1]);
}

void BOE20Order::RoutingInst::set(const BOE20Order::RoutingInstFirstChar w,
                                  const BOE20Order::RoutingInstSecondChar x)
{
    u32 = 0;
    layout.first = w;
    layout.second = x;
}

void BOE20Order::set_post_only()
{
    m_routing_inst.set(RoutingInstFirstChar::POST_ONLY);
}

std::ostream& operator<<(std::ostream& os, const BOE20Order& o)
{
    const auto& bo = static_cast<const Order&>(o);
    return os << bo
              << ",DISPLAYIND," << o.m_display_indicator
              << ",EXECINST," << o.m_exec_inst
              << ",ROUTINGINST," << o.m_routing_inst;
}

bool BOE20Order::set_attribute(const std::string& name, const std::string& value)
{
    auto nn = name;
    std::transform(name.begin(), name.end(), nn.begin(), ::toupper);

    if ("DISPLAYINDICATOR" == nn
        || "DISPIND" == nn) {
        return boe_display_indicator(value);
    } else if ("ROUTINGINST" == nn) {
        return boe_routing_inst(value);
    }

    return true;
}

bool BOE20Order::boe_display_indicator(const std::string& value)
{
    if (value.size() != 1) {
        return false;
    }

    auto c = value[0];

    switch (c) {
    case static_cast<char>(DisplayIndicator::UNKNOWN):
        boe_display_indicator(DisplayIndicator::UNKNOWN);
        break;
    case static_cast<char>(DisplayIndicator::DEFAULT):
        boe_display_indicator(DisplayIndicator::DEFAULT);
        break;
    case static_cast<char>(DisplayIndicator::PRICE_ADJUST):
        boe_display_indicator(DisplayIndicator::PRICE_ADJUST);
        break;
    case static_cast<char>(DisplayIndicator::MULTIPLE_PRICE_ADJUST):
        boe_display_indicator(DisplayIndicator::MULTIPLE_PRICE_ADJUST);
        break;
    case static_cast<char>(DisplayIndicator::CANCEL_BACK_IF_NEEDS_ADJUSTMENT):
        boe_display_indicator(DisplayIndicator::CANCEL_BACK_IF_NEEDS_ADJUSTMENT);
        break;
    case static_cast<char>(DisplayIndicator::HIDDEN):
        boe_display_indicator(DisplayIndicator::HIDDEN);
        break;
    case static_cast<char>(DisplayIndicator::DISPLAY_PRICE_SLIDING):
        boe_display_indicator(DisplayIndicator::DISPLAY_PRICE_SLIDING);
        break;
    case static_cast<char>(DisplayIndicator::DISPLAY_PRICE_SLIDING_NO_CROSS):
        boe_display_indicator(DisplayIndicator::DISPLAY_PRICE_SLIDING_NO_CROSS);
        break;
    case static_cast<char>(DisplayIndicator::MULTIPLE_DISPLAY_PRICE_SLIDING):
        boe_display_indicator(DisplayIndicator::MULTIPLE_DISPLAY_PRICE_SLIDING);
        break;
    case static_cast<char>(DisplayIndicator::HIDE_NOT_SLIDE_DEPRECATED):
        boe_display_indicator(DisplayIndicator::HIDE_NOT_SLIDE_DEPRECATED);
        break;
    case static_cast<char>(DisplayIndicator::VISIBLE):
        boe_display_indicator(DisplayIndicator::VISIBLE);
        break;
    case static_cast<char>(DisplayIndicator::INVISIBLE):
        boe_display_indicator(DisplayIndicator::INVISIBLE);
        break;
    case static_cast<char>(DisplayIndicator::NO_RESCRAPE_AT_LIMIT):
        boe_display_indicator(DisplayIndicator::NO_RESCRAPE_AT_LIMIT);
        break;
    default:
        return false;
    }
    return true;
}

bool BOE20Order::boe_routing_inst(const std::string& value)
{
    if (value.size() < 1 || value.size() > 2) {
        return false;
    }
    auto c = value[0];
    auto ri = RoutingInst{};
    switch (c) {
    case static_cast<char>(RoutingInstFirstChar::UNKNOWN):
        ri.layout.first = RoutingInstFirstChar::UNKNOWN;
        break;
    case static_cast<char>(RoutingInstFirstChar::BOOK_ONLY):
        ri.layout.first = RoutingInstFirstChar::BOOK_ONLY;
        break;
    case static_cast<char>(RoutingInstFirstChar::POST_ONLY):
        ri.layout.first = RoutingInstFirstChar::POST_ONLY;
        break;
    case static_cast<char>(RoutingInstFirstChar::POST_ONLY_AT_LIMIT):
        ri.layout.first = RoutingInstFirstChar::POST_ONLY_AT_LIMIT;
        break;
    case static_cast<char>(RoutingInstFirstChar::ROUTABLE):
        ri.layout.first = RoutingInstFirstChar::ROUTABLE;
        break;
    case static_cast<char>(RoutingInstFirstChar::SUPER_AGGRESSIVE):
        ri.layout.first = RoutingInstFirstChar::SUPER_AGGRESSIVE;
        break;
    case static_cast<char>(RoutingInstFirstChar::AGGRESSIVE):
        ri.layout.first = RoutingInstFirstChar::AGGRESSIVE;
        break;
    case static_cast<char>(RoutingInstFirstChar::SUPER_AGGRESSIVE_WHEN_ODD_LOT):
        ri.layout.first = RoutingInstFirstChar::SUPER_AGGRESSIVE_WHEN_ODD_LOT;
        break;
    case static_cast<char>(RoutingInstFirstChar::POST_TO_AWAY):
        ri.layout.first = RoutingInstFirstChar::POST_TO_AWAY;
        break;
    default:
        return false;
    }

    if (value.size() >1) {
        c = value[1];
        switch(c) {
        case static_cast<char>(RoutingInstSecondChar::UNKNOWN):
            ri.layout.second = RoutingInstSecondChar::UNKNOWN;
            break;
        case static_cast<char>(RoutingInstSecondChar::ELIGIBLE_TO_ROUTE_TO_DRT_CLC):
            ri.layout.second = RoutingInstSecondChar::ELIGIBLE_TO_ROUTE_TO_DRT_CLC;
            break;
        case static_cast<char>(RoutingInstSecondChar::ROUTE_TO_DISPLAYED_MKTS_ONLY):
            ri.layout.second = RoutingInstSecondChar::ROUTE_TO_DISPLAYED_MKTS_ONLY;
            break;
        default:
            return false;
        }
    }
    boe_routing_inst(ri);
    return true;
}

}}
