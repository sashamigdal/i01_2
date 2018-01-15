#include <iostream>

#include <i01_core/util.hpp>

#include "Messages.hpp"

namespace i01 { namespace OE { namespace NASDAQ { namespace OUCH42 { namespace Messages {


std::ostream& operator<<(std::ostream& os, const EnterOrder& o)
{
    using i01::core::operator<<;
    os << o.message_type << ","
       << o.order_token << ","
       << (char) o.buy_sell_indicator << ","
       << o.shares << ","
       << o.stock << ","
       << o.price << ","
       << o.time_in_force << ","
       << o.firm << ","
       << o.display << ","
       << o.capacity << ","
       << (char) o.iso_eligibility << ","
       << o.minimum_quantity << ","
       << (char) o.cross_type << ","
       << (char) o.customer_type;
    return os;
}

std::ostream& operator<<(std::ostream& os, const CancelOrder& o)
{
    using i01::core::operator<<;
    os << o.message_type << ","
       << o.order_token << ","
       << o.shares;
    return os;
}

std::ostream& operator<<(std::ostream& os, const OutboundMessageHeader& msg)
{
    using i01::core::operator<<;
    return os << msg.message_type << ","
              << msg.exch_time.get();
}

std::ostream& operator<<(std::ostream& os, const OutboundOrderMessageHeader& msg)
{
    using i01::core::operator<<;
    return os << msg.message_type << ","
              << msg.exch_time.get() << ","
              << msg.order_token.arr;
}

}}}}}
