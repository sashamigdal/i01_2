#include <iostream>
#include <i01_oe/NASDAQOrder.hpp>

namespace i01 { namespace OE {

std::ostream& operator<<(std::ostream& os, const NASDAQOrder::Display& d)
{
    switch (d) {
    case NASDAQOrder::Display::UNKNOWN:
        os << "UNKNOWN";
        break;
    case NASDAQOrder::Display::ATTRIBUTABLE_PRICE_TO_DISPLAY:
        os << "ATTRIBUTABLE_PRICE_TO_DISPLAY";
        break;
    case NASDAQOrder::Display::ANONYMOUS_PRICE_TO_COMPLY:
        os << "ANONYMOUS_PRICE_TO_COMPLY";
        break;
    case NASDAQOrder::Display::NON_DISPLAY:
        os << "NON_DISPLAY";
        break;
    case NASDAQOrder::Display::POST_ONLY:
        os << "POST_ONLY";
        break;
    case NASDAQOrder::Display::IMBALANCE_ONLY:
        os << "IMBALANCE_ONLY";
        break;
    case NASDAQOrder::Display::MIDPOINT_PEG:
        os << "MIDPOINT_PEG";
        break;
    case NASDAQOrder::Display::MIDPOINT_PEG_POST_ONLY:
        os << "MIDPOINT_PEG_POST_ONLY";
        break;
    case NASDAQOrder::Display::POST_ONLY_AND_ATTRIBUTABLE_PRICE_TO_DISPLAY:
        os << "POST_ONLY_AND_ATTRIBUTABLE_PRICE_TO_DISPLAY";
        break;
    case NASDAQOrder::Display::RETAIL_ORDER_TYPE_1:
        os << "RETAIL_ORDER_TYPE_1";
        break;
    case NASDAQOrder::Display::RETAIL_ORDER_TYPE_2:
        os << "RETAIL_ORDER_TYPE_2";
        break;
    case NASDAQOrder::Display::RETAIL_PRICE_IMPROVEMENT_ORDER:
        os << "RETAIL_PRICE_IMPROVEMENT_ORDER";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const NASDAQOrder::Capacity& c)
{
    switch (c) {
    case NASDAQOrder::Capacity::AGENCY:
        os << "AGENCY";
        break;
    case NASDAQOrder::Capacity::PRINCIPAL:
        os << "PRINCIPAL";
        break;
    case NASDAQOrder::Capacity::RISKLESS:
        os << "RISKLESS";
        break;
    case NASDAQOrder::Capacity::OTHER:
        os << "OTHER";
        break;
    default:
        break;
    }
    return os;
}

}}
