#include <ostream>
#include <i01_oe/Instrument.hpp>

namespace i01 { namespace OE {

std::ostream & operator<<(std::ostream &os, const Instrument &i)
{
    return os << i.m_symbol << ","
              << i.m_listing_market << ","
              << i.m_order_size_limit << ","
              << i.m_order_price_limit << ","
              << i.m_order_value_limit << ","
              << i.m_order_rate_limit << ","
              << i.m_position_limit << ","
              << i.m_position_value_limit << ","
              << i.m_position;
}

}}
