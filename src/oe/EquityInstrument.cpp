#include <ostream>

#include <i01_core/Log.hpp>
#include <i01_oe/EquityInstrument.hpp>

namespace i01 { namespace OE {

bool EquityInstrument::validate(Order* o)
{
    if (LIKELY(m_order_rate_limit > -1))
    {
        while(!m_order_rate.empty() &&
            ((m_order_rate.front().tv_sec * 1000000000L + m_order_rate.front().tv_nsec) <
            ((o->creation_time().tv_sec - 1) * 1000000000L + o->creation_time().tv_nsec)))
        {
            m_order_rate.pop();
        }

        if (UNLIKELY((m_order_rate.size() + 1) > static_cast<unsigned int>(m_order_rate_limit)))
        {
            std::cerr << "Validate (Order Rate Limit) failed: [" << o->localID() << "] " << m_order_rate.size() + 1 << " > " << m_order_rate_limit << std::endl;
            return false;
        }
        // NB: See below (just before return true) for m_order_rate update.
    }

    if (UNLIKELY(o->size() > m_order_size_limit))
    {
        std::cerr << "Validate (Order Size Limit) failed: [" << o->localID() << "] " << o->size() << " > " << m_order_size_limit << std::endl;
        return false;
    }

    if (UNLIKELY(o->price() > m_order_price_limit))
    {
        std::cerr << "Validate (Order Price Limit) failed: [" << o->localID() << "] " << o->price() << " > " << m_order_price_limit << std::endl;
        return false;
    }

    Price orderValue = o->size() * o->price();
    if (UNLIKELY(orderValue > m_order_value_limit))
    {
        std::cerr << "Validate (Order Value Limit) failed: [" << o->localID() << "] " << orderValue << " > " << m_order_value_limit << std::endl;
        return false;
    }

    Quantity postPosition = m_position.quantity() + m_position.remote_quantity();
    if (o->side() == Side::BUY)
        postPosition += o->size();
    else
        postPosition -= o->size();

    if (UNLIKELY(labs(postPosition) > m_position_limit))
    {
        std::cerr << "Validate (Position Limit) failed: [" << o->localID() << "] " << postPosition << " > " << m_position_limit << std::endl;
        return false;
    }

    Price positionValue = static_cast<Price>(labs(postPosition)) * o->price();
    if (UNLIKELY(positionValue > m_position_value_limit))
    {
        std::cerr << "Validate (Position Value) failed: [" << o->localID() << "] " << positionValue << " > " << m_position_value_limit << std::endl;
        return false;
    }

    if (o->side() == Side::SELL || o->side() == Side::SHORT || o->side() == Side::SHORT_EXEMPT)
    {
        postPosition -= m_position.short_open_size();
        Size requiredLocateSize = 0;
        if (postPosition < 0)
        {
            o->side(Side::SHORT); // correct incorrect SELL -> SHORT.
            requiredLocateSize = static_cast<Size>(-postPosition);
        } else {
            o->side(Side::SELL); // correct potentially incorrect SHORT -> SELL.
        }
        if (LIKELY(m_position.m_locates_remaining != Position::LOCATES_REMAINING_SENTINEL)) {
            if (UNLIKELY(requiredLocateSize > m_position.m_locates_remaining))
            {
                std::cerr << "Validate (Locates) failed: [" << o->localID() << "] " << requiredLocateSize << " > " << m_position.m_locates_remaining << std::endl;
                return false;
            }
        } else {
                std::cerr << "Validate (Locates Sentinel) failed: [" << o->localID() << "] Locates not set for symbol " << m_symbol << std::endl;
                return false;
        }

        m_position.m_locates_remaining -= requiredLocateSize;
        m_position.m_locates_used += requiredLocateSize;
    }

    if (LIKELY(m_order_rate_limit > -1))
    {
        m_order_rate.push(o->creation_time());
    }
    return true;
}

std::ostream & operator<<(std::ostream &os, const EquityInstrument &ei)
{
    return os << static_cast<const Instrument &>(ei) << ","
              << "LOT," << ei.m_lot_size << ","
              << "LOC," << ei.m_locate_size << ","
              << "NREC," << (ei.m_norecycle ? "1," : "0,")
              << "ADV," << ei.m_adv << ","
              << "ADR," << ei.m_adr;
}

void EquityInstrument::update_locates_live(Quantity new_locate_size, bool only_if_increasing)
{
    if (new_locate_size == m_locate_size)
        return;
    core::ScopedLock<mutex_type> lock(m_mutex);
    Quantity locate_diff = new_locate_size - m_locate_size;
    if ((locate_diff == 0) || (only_if_increasing && (locate_diff <= 0)))
        return;
    m_locate_size = new_locate_size;
    m_position.m_locates_remaining += locate_diff;
    std::cerr << "Updated locates live for " << m_esi << " symbol: " << m_symbol << " new:" << m_locate_size << " diff:" << locate_diff << " remaining:" << m_position.m_locates_remaining << std::endl;
}

}}
