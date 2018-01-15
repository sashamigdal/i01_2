#include <iostream>
#include <i01_core/util.hpp>

#include "Types.hpp"

namespace i01 { namespace OE { namespace NASDAQ { namespace OUCH42 { namespace Types {

using i01::core::operator<<;

std::ostream& operator<<(std::ostream& os, const Price& p)
{
    if (p.is_market()) {
        return os << "MARKET";
    }

    return os << p.get_limit();
}

std::ostream& operator<<(std::ostream& os, const TimeInForce& tif)
{
    return os << tif.to_i01();
}

std::ostream& operator<<(std::ostream& os, const InboundMessageType& msg)
{
    switch (msg) {
    case InboundMessageType::ENTER_ORDER:
        os << "ENTER_ORDER";
        break;
    case InboundMessageType::REPLACE_ORDER:
        os << "REPLACE_ORDER";
        break;
    case InboundMessageType::CANCEL_ORDER:
        os << "CANCEL_ORDER";
        break;
    case InboundMessageType::MODIFY_ORDER:
        os << "MODIFY_ORDER";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const OutboundMessageType& msg)
{
    switch (msg) {
    case OutboundMessageType::SYSTEM_EVENT:
        os << "SYSTEM_EVENT";
        break;
    case OutboundMessageType::ACCEPTED:
        os << "ACCEPTED";
        break;
    case OutboundMessageType::REPLACED:
        os << "REPLACED";
        break;
    case OutboundMessageType::CANCELED:
        os << "CANCELED";
        break;
    case OutboundMessageType::AIQ_CANCELLED:
        os << "AIQ_CANCELLED";
        break;
    case OutboundMessageType::EXECUTED:
        os << "EXECUTED";
        break;
    case OutboundMessageType::BROKEN_TRADE:
        os << "BROKEN_TRADE";
        break;
    case OutboundMessageType::REJECTED:
        os << "REJECTED";
        break;
    case OutboundMessageType::CANCEL_PENDING:
        os << "CANCEL_PENDING";
        break;
    case OutboundMessageType::CANCEL_REJECT:
        os << "CANCEL_REJECT";
        break;
    case OutboundMessageType::ORDER_PRIORITY_UPDATE:
        os << "ORDER_PRIORITY_UPDATE";
        break;
    case OutboundMessageType::ORDER_MODIFIED:
        os << "ORDER_MODIFIED";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const OrderToken& tok)
{
    return os << tok.arr;
}

OE::Side to_i01_side(BuySellIndicator bs)
{
    switch(bs) {
    case BuySellIndicator::BUY:
        return OE::Side::BUY;
    case BuySellIndicator::SELL:
        return OE::Side::SELL;
    case BuySellIndicator::SHORT:
        return OE::Side::SHORT;
    case BuySellIndicator::SHORT_EXEMPT:
        return OE::Side::SHORT_EXEMPT;
    default:
        return OE::Side::UNKNOWN;
    }
}

std::ostream& operator<<(std::ostream& os, const Stock& s)
{
    return os << s.arr;
}

std::ostream& operator<<(std::ostream& os, const Firm& f)
{
    return os << f.arr;
}

}}}}}
