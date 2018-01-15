#include <ostream>

#include <i01_core/util.hpp>

#include <i01_oe/Types.hpp>

namespace i01 { namespace OE {

std::ostream& operator<<(std::ostream& os, const ClientOrderID& id)
{
    using i01::core::operator<<;
    return os << id;
}

std::ostream & operator<<(std::ostream &os, const Side &s)
{
    switch (s) {
    case Side::UNKNOWN:
        os << "UNKNOWN";
        break;
    case Side::BUY:
        os << "BUY";
        break;
    case Side::SELL:
        os << "SELL";
        break;
    case Side::SHORT:
        os << "SHORT";
        break;
    case Side::SHORT_EXEMPT:
        os << "SHORT_EXEMPT";
        break;
    default:
        break;
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const OrderType &o)
{
    switch (o) {
    case OrderType::UNKNOWN:
        os << "UNKNOWN";
        break;
    case OrderType::MARKET:
        os << "MARKET";
        break;
    case OrderType::LIMIT:
        os << "LIMIT";
        break;
    case OrderType::STOP:
        os << "STOP";
        break;
    case OrderType::MIDPOINT_PEG:
        os << "MIDPOINT_PEG";
        break;
    default:
        break;
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const TimeInForce &t)
{
    switch (t) {
    case TimeInForce::UNKNOWN:
        os << "UNKNOWN";
        break;
    case TimeInForce::DAY:
        os << "DAY";
        break;
    case TimeInForce::EXT:
        os << "EXT";
        break;
    case TimeInForce::GTC:
        os << "GTC";
        break;
    case TimeInForce::AUCTION_OPEN:
        os << "AUCTION_OPEN";
        break;
    case TimeInForce::AUCTION_CLOSE:
        os << "AUCTION_CLOSE";
        break;
    case TimeInForce::AUCTION_HALT:
        os << "AUCTION_HALT";
        break;
    case TimeInForce::TIMED:
        os << "TIMED";
        break;
    case TimeInForce::IMMEDIATE_OR_CANCEL:
        os << "IOC";
        break;
    case TimeInForce::FILL_OR_KILL:
        os << "FOK";
        break;
    default:
        break;
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const OrderState &o)
{
    switch (o) {
    case OrderState::UNKNOWN:
        os << "UNKNOWN";
        break;
    case OrderState::NEW_AND_UNSENT:
        os << "NEW_AND_UNSENT";
        break;
    case OrderState::LOCALLY_REJECTED:
        os << "LOCALLY_REJECTED";
        break;
    case OrderState::SENT:
        os << "SENT";
        break;
    case OrderState::ACKNOWLEDGED:
        os << "ACKNOWLEDGED";
        break;
    case OrderState::PARTIALLY_FILLED:
        os << "PARTIALLY_FILLED";
        break;
    case OrderState::FILLED:
        os << "FILLED";
        break;
    case OrderState::PENDING_CANCEL:
        os << "PENDING_CANCEL";
        break;
    case OrderState::PARTIALLY_CANCELLED:
        os << "PARTIALLY_CANCELLED";
        break;
    case OrderState::CANCELLED:
        os << "CANCELLED";
        break;
    case OrderState::PENDING_CANCEL_REPLACE:
        os << "PENDING_CANCEL_REPLACE";
        break;
    case OrderState::CANCEL_REPLACED:
        os << "CANCEL_REPLACED";
        break;
    case OrderState::REMOTELY_REJECTED:
        os << "REMOTELY_REJECTED";
        break;
    case OrderState::CANCEL_REJECTED:
        os << "CANCEL_REJECTED";
        break;
    default:
        break;
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const FillFeeCode &ffc)
{
    if (ffc == FillFeeCode::UNKNOWN)
        os << "UNKNOWN";
    else
        os << (uint32_t)ffc;
    return os;
}
}}
