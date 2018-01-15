#include <i01_core/Time.hpp>
#include <i01_core/util.hpp>

#include "Types.hpp"

namespace i01 { namespace OE { namespace BATS { namespace BOE20 { namespace Types {

using i01::core::operator<<;

std::ostream& operator<<(std::ostream& os, const MessageType& mt)
{
    switch (mt) {
    case MessageType::LOGIN_REQUEST_V2:
        os << "LOGIN_REQUEST_V2";
        break;
    case MessageType::LOGOUT_REQUEST:
        os << "LOGOUT_REQUEST";
        break;
    case MessageType::CLIENT_HEARTBEAT:
        os << "CLIENT_HEARTBEAT";
        break;
    case MessageType::NEW_ORDER_V2:
        os << "NEW_ORDER_V2";
        break;
    case MessageType::CANCEL_ORDER_V2:
        os << "CANCEL_ORDER_V2";
        break;
    case MessageType::MODIFY_ORDER_V2:
        os << "MODIFY_ORDER_V2";
        break;
    case MessageType::LOGIN_RESPONSE_V2:
        os << "LOGIN_RESPONSE_V2";
        break;
    case MessageType::LOGOUT:
        os << "LOGOUT";
        break;
    case MessageType::SERVER_HEARTBEAT:
        os << "SERVER_HEARTBEAT";
        break;
    case MessageType::REPLAY_COMPLETE:
        os << "REPLAY_COMPLETE";
        break;
    case MessageType::ORDER_ACKNOWLEDGMENT_V2:
        os << "ORDER_ACKNOWLEDGMENT_V2";
        break;
    case MessageType::ORDER_REJECTED_V2:
        os << "ORDER_REJECTED_V2";
        break;
    case MessageType::ORDER_MODIFIED_V2:
        os << "ORDER_MODIFIED_V2";
        break;
    case MessageType::ORDER_RESTATED_V2:
        os << "ORDER_RESTATED_V2";
        break;
    case MessageType::USER_MODIFY_REJECTED_V2:
        os << "USER_MODIFY_REJECTED_V2";
        break;
    case MessageType::ORDER_CANCELLED_V2:
        os << "ORDER_CANCELLED_V2";
        break;
    case MessageType::CANCEL_REJECTED_V2:
        os << "CANCEL_REJECTED_V2";
        break;
    case MessageType::ORDER_EXECUTION_V2:
        os << "ORDER_EXECUTION_V2";
        break;
    case MessageType::TRADE_CANCEL_OR_CORRECT_V2:
        os << "TRADE_CANCEL_OR_CORRECT_V2";
        break;
    default:
        os << "UNKNOWN MESSAGE TYPE 0x" << std::hex << (int) mt << std::dec;
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const BooleanFlag &m)
{
    return os << std::string(m == BooleanFlag::TRUE ? "T" : "F");
}

std::ostream & operator<<(std::ostream &os, const ClOrdID &m)
{
    return os << m.arr;
}

std::ostream & operator<<(std::ostream &os, const Side &m)
{
    switch(m) {
    case Side::BUY:
        return os << "B";
    case Side::SELL:
        return os << "S";
    case Side::SELL_SHORT:
        return os << "SS";
    case Side::SELL_SHORT_EXEMPT:
        return os << "SSE";
    default:
        return os;
    }
    // notreached
}

std::ostream & operator<<(std::ostream &os, const BaseLiquidityIndicator &m)
{
    switch (m) {
    case BaseLiquidityIndicator::ADDED_LIQUIDITY:
        return os << "ADDED_LIQUIDITY";
    case BaseLiquidityIndicator::REMOVED_LIQUIDITY:
        return os << "REMOVED_LIQUIDITY";
    case BaseLiquidityIndicator::ROUTED_TO_ANOTHER_MARKET:
        return os << "ROUTED_TO_ANOTHER_MARKET";
    case BaseLiquidityIndicator::BZX_AUCTION_TRADE:
        return os << "BZX_AUCTION_TRADE";
    default:
        return os;
    }
}
std::ostream & operator<<(std::ostream &os, const Capacity &m)
{
    switch (m) {
    case Capacity::AGENCY:
        return os << "AGENCY";
    case Capacity::PRINCIPAL:
        return os << "PRINCIPAL";
    case Capacity::RISKLESS_PRINCIPAL:
        return os << "RISKLESS_PRINCIPAL";
    default:
        return os;
    }
}

std::ostream & operator<<(std::ostream &os, const OrdType &m)
{
    switch (m) {
    case OrdType::MARKET:
        return os << "MARKET";
    case OrdType::LIMIT:
        return os << "LIMIT";
    case OrdType::STOP:
        return os << "STOP";
    case OrdType::STOP_LIMIT:
        return os << "STOP_LIMIT";
    case OrdType::PEGGED:
        return os << "PEGGED";
    default:
        return os;
    }
}

std::ostream & operator<<(std::ostream &os, const SubLiquidityIndicator &m)
{
    switch (m) {
    case SubLiquidityIndicator::NO_ADDITIONAL_INFORMATION:
        return os << "NO_ADDITIONAL_INFORMATION";
    case SubLiquidityIndicator::TRADE_ADDED_RPI_LIQUIDITY:
        return os << "TRADE_ADDED_RPI_LIQUIDITY";
    case SubLiquidityIndicator::TRADE_ADDED_HIDDEN_LIQUIDITY:
        return os << "TRADE_ADDED_HIDDEN_LIQUIDITY";
    case SubLiquidityIndicator::TRADE_ADDED_HIDDEN_LIQUIDITY_THAT_WAS_PRICE_IMPROVED:
        return os << "TRADE_ADDED_HIDDEN_LIQUIDITY_THAT_WAS_PRICE_IMPROVED";
    case SubLiquidityIndicator::EXECUTION_FROM_FIRST_ORDER_TO_JOIN_THE_NBBO:
        return os << "EXECUTION_FROM_FIRST_ORDER_TO_JOIN_THE_NBBO";
    case SubLiquidityIndicator::EXECUTION_FROM_ORDER_THAT_SET_THE_NBBO:
        return os << "EXECUTION_FROM_ORDER_THAT_SET_THE_NBBO";
    case SubLiquidityIndicator::TRADE_ADDED_VISIBLE_LIQUIDITY_THAT_WAS_PRICE_IMPROVED:
        return os << "TRADE_ADDED_VISIBLE_LIQUIDITY_THAT_WAS_PRICE_IMPROVED";
    case SubLiquidityIndicator::MIDPOINT_PEG_ORDER_ADDED_LIQUIDITY:
        return os << "MIDPOINT_PEG_ORDER_ADDED_LIQUIDITY";
    default:
        return os;
    }
}

std::ostream & operator<<(std::ostream &os, const TimeInForce &m)
{
    switch (m) {
    case TimeInForce::DAY:
        return os << "DAY";
    case TimeInForce::GTC:
        return os << "GTC";
    case TimeInForce::AT_THE_OPEN:
        return os << "AT_THE_OPEN";
    case TimeInForce::IOC:
        return os << "IOC";
    case TimeInForce::FOK:
        return os << "FOK";
    case TimeInForce::GTX:
        return os << "GTX";
    case TimeInForce::GTD:
        return os << "GTD";
    case TimeInForce::AT_THE_CLOSE:
        return os << "AT_THE_CLOSE";
    case TimeInForce::REG_HOURS_ONLY:
        return os << "REG_HOURS_ONLY";
    default:
        return os;
    }

    return os << (char) m;
}

// std::ostream & operator<<(std::ostream &os, const DateTime &dt)
// {
//     // nanos since the epoch
//     return os << core::Timestamp(dt/1000000000ULL, dt % 1000000000ULL);
// }

std::ostream & operator<<(std::ostream &os, const ReasonCode &m)
{
    os << (char) m << ":";
    switch (m) {
    case ReasonCode::ADMIN:
        os << "ADMIN";
        break;
    case ReasonCode::DUPLICATE_CL_ORD_ID:
        os << "DUPLICATE_CL_ORD_ID";
        break;
    case ReasonCode::HALTED:
        os << "HALTED";
        break;
    case ReasonCode::INCORRECT_DATA_CENTER:
        os << "INCORRECT_DATA_CENTER";
        break;
    case ReasonCode::TOO_LATE_TO_CANCEL:
        os << "TOO_LATE_TO_CANCEL";
        break;
    case ReasonCode::ORDER_RATE_THRESHOLD_EXCEEDED:
        os << "ORDER_RATE_THRESHOLD_EXCEEDED";
        break;
    case ReasonCode::PRICE_EXCEEDS_CROSS_RANGE:
        os << "PRICE_EXCEEDS_CROSS_RANGE";
        break;
    case ReasonCode::RAN_OUT_OF_LIQUIDITY_TO_EXECUTE_AGAINST:
        os << "RAN_OUT_OF_LIQUIDITY_TO_EXECUTE_AGAINST";
        break;
    case ReasonCode::CL_ORD_ID_DOESNT_MATCH_A_KNOWN_ORDER:
        os << "CL_ORD_ID_DOESNT_MATCH_A_KNOWN_ORDER";
        break;
    case ReasonCode::CANT_MODIFY_AN_ORDER_THAT_IS_PENDING_FILL:
        os << "CANT_MODIFY_AN_ORDER_THAT_IS_PENDING_FILL";
        break;
    case ReasonCode::WAITING_FOR_FIRST_TRADE:
        os << "WAITING_FOR_FIRST_TRADE";
        break;
    case ReasonCode::ROUTING_UNAVAILABLE:
        os << "ROUTING_UNAVAILABLE";
        break;
    case ReasonCode::FILL_WOULD_TRADE_THRU_NBBO:
        os << "FILL_WOULD_TRADE_THRU_NBBO";
        break;
    case ReasonCode::USER_REQUESTED:
        os << "USER_REQUESTED";
        break;
    case ReasonCode::WOULD_WASH:
        os << "WOULD_WASH";
        break;
    case ReasonCode::ADD_LIQUIDITY_ONLY_ORDER_WOULD_REMOVE:
        os << "ADD_LIQUIDITY_ONLY_ORDER_WOULD_REMOVE";
        break;
    case ReasonCode::ORDER_EXPIRED:
        os << "ORDER_EXPIRED";
        break;
    case ReasonCode::SYMBOL_NOT_SUPPORTED:
        os << "SYMBOL_NOT_SUPPORTED";
        break;
    case ReasonCode::RESERVE_RELOAD:
        os << "RESERVE_RELOAD";
        break;
    case ReasonCode::MARKET_ACCESS_RISK_LIMIT_EXCEEDED:
        os << "MARKET_ACCESS_RISK_LIMIT_EXCEEDED";
        break;
    case ReasonCode::MAX_OPEN_ORDERS_COUNT_EXCEEDED:
        os << "MAX_OPEN_ORDERS_COUNT_EXCEEDED";
        break;
    case ReasonCode::LIMIT_UP_DOWN:
        os << "LIMIT_UP_DOWN";
        break;
    case ReasonCode::WOULD_REMOVE_ON_UNSLIDE:
        os << "WOULD_REMOVE_ON_UNSLIDE";
        break;
    case ReasonCode::CROSSED_MARKET:
        os << "CROSSED_MARKET";
        break;
    case ReasonCode::ORDER_RECEIVED_BY_BATS_DURING_REPLAY:
        os << "ORDER_RECEIVED_BY_BATS_DURING_REPLAY";
        break;
    default:
        break;
    }
    return os;
}

std::ostream & operator<<(std::ostream &os, const RestatementReason &m)
{
    switch (m) {
    case RestatementReason::RELOAD:
        return os << "RELOAD";
    case RestatementReason::LIQUIDITY_UPDATED:
        return os << "LIQUIDITY_UPDATED";
    case RestatementReason::REROUTE:
        return os << "REROUTE";
    case RestatementReason::SIZE_REDUCED_DUE_TO_SQP:
        return os << "SIZE_REDUCED_DUE_TO_SQP";
    case RestatementReason::WASH:
        return os << "WASH";
    case RestatementReason::LOCKED_IN_CROSS:
        return os << "LOCKED_IN_CROSS";
    default:
        return os;
    }
}

}}}}}
