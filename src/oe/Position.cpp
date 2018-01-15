#include <cassert>
#include <ostream>

#include <i01_oe/Order.hpp>
#include <i01_oe/Position.hpp>
#include <i01_oe/Instrument.hpp>

namespace i01 { namespace OE {

Position::Position(Instrument *inst, Quantity startq, Price adj_close) :
    m_trading_quantity{startq},
    m_start_quantity{startq},
    m_open_long_size{0},
    m_open_short_size{0},
    m_open_long_exposure{0},
    m_open_short_exposure{0},
    m_realized{0},
    m_unrealized{0},
    m_vwap{0},
    m_mark{0},
    m_prior_adjusted_close{adj_close},
    // NOTE: These are initialized after the fact/in EquityInstrument!
    m_locates_remaining(LOCATES_REMAINING_SENTINEL),
    m_locates_used(0),
    m_norecycle(false),
    m_remote_trading_quantity(0),
    m_remote_vwap(0.0),
    m_remote_positions{}
{
}

void Position::update_realized_and_unrealized_pnl(const Side side, const Size size,
                                                  const Price price, const Dollars fee)
{
    auto net_delta = price*size;
    auto realized_delta = Dollars{0};
    // assumption is that pos has not been updated to show this fill yet
    auto new_pos = Quantity{trading_quantity()};
    // new mark price
    m_mark = price;
    if (side == Side::BUY) {
        net_delta += fee;
        new_pos += size;

        if (trading_quantity() >= 0) {
            // BUY TO ENTER
            // adding to existing position
            m_vwap = (static_cast<Dollars>(trading_quantity())*m_vwap + net_delta) / static_cast<Dollars>(new_pos);

            // does not change realized pnl
        } else {
            // BUY TO EXIT
            // this is closing part of whole of existing position
            if (new_pos <= 0) {
                // partial or fully closed
                realized_delta = (m_vwap - price) * size;
                m_realized += realized_delta;

                // does not change vwap unless fully closed
                if (0 == new_pos) {
                    m_vwap = 0;
                }
            } else {
                // reversed
                realized_delta = (m_vwap - price) * -static_cast<Dollars>(trading_quantity());
                m_realized += realized_delta;
                m_vwap = price;
            }
        }
    } else {
        net_delta -= fee;
        new_pos -= size;
        if (trading_quantity() <= 0) {
            // SELL TO ENTER
            m_vwap = (net_delta - static_cast<Dollars>(trading_quantity())*m_vwap) / -static_cast<Dollars>(new_pos);
            // realized unchanged
            realized_delta = 0;
        } else {
            // SELL TO EXIT
            if (new_pos >= 0) {
                // partial or fully close
                realized_delta = (price - m_vwap) * size;
                m_realized += realized_delta;

                if (0 == new_pos) {
                    m_vwap = 0;
                }
            } else {
                // reversed the position
                realized_delta = (price - m_vwap) * static_cast<Dollars>(trading_quantity());
                m_realized += realized_delta;
                m_vwap = price;
            }
        }
    }
    m_unrealized = (m_mark - m_vwap) * static_cast<Dollars>(new_pos);
    m_trading_quantity = new_pos;
}

void Position::add_open_buys(const Price p, const Size s)
{
    m_open_long_size += s;
    m_open_long_exposure += static_cast<Dollars>(s) * p;
}

void Position::sub_open_buys(const Price p, const Size s)
{
    assert(m_open_long_size >= s);
    m_open_long_size -= s;
    m_open_long_exposure -= static_cast<Dollars>(s) * p;
}

void Position::add_open_sells(const Price p, const Size s)
{
    m_open_short_size += s;
    m_open_short_exposure -= static_cast<Dollars>(s) * p;
}

void Position::sub_open_sells(const Price p, const Size s)
{
    assert(m_open_short_size >= s);
    m_open_short_size -= s;
    m_open_short_exposure += static_cast<Dollars>(s) * p;
}

void Position::add_trading_quantity(const Quantity q)
{
    m_trading_quantity += q;
}

Dollars Position::realized_pnl() const
{
    return m_realized;
}

Dollars Position::unrealized_pnl() const
{
    return m_unrealized;
}

Dollars Position::notional() const
{
    return m_mark * static_cast<Dollars>(m_trading_quantity);
}

Dollars Position::start_of_day_long_exposure() const
{
    if (m_start_quantity > 0) {
        return static_cast<Dollars>(m_start_quantity) * m_prior_adjusted_close;
    } else {
        return 0;
    }
}

Dollars Position::start_of_day_short_exposure() const
{
    if (m_start_quantity < 0) {
        return static_cast<Dollars>(m_start_quantity) * m_prior_adjusted_close;
    } else {
        return 0;
    }
}

Dollars Position::long_open_exposure() const
{
    return m_open_long_exposure + start_of_day_long_exposure();
}

Dollars Position::short_open_exposure() const
{
    return m_open_short_exposure + start_of_day_short_exposure();
}

void Position::on_update_mark(const Price p)
{
    m_mark = p;
    m_unrealized = (m_mark - m_vwap) * static_cast<Dollars>(m_trading_quantity);
}

void Position::print(std::ostream &os) const
{
    os << "SOD," << m_start_quantity
       << ",CLS," << m_prior_adjusted_close
       << ",Q," << m_trading_quantity
       << ",OPNL," << m_open_long_size
       << ",OPNS," << m_open_short_size
       << ",REAL," << m_realized
       << ",UNREAL," << m_unrealized
       << ",TRADEVWAP," << m_vwap
       << ",REMTRADEVWAP," << m_remote_vwap
       << ",MARK," << m_mark
       << ",RLOC," << m_locates_remaining
       << ",ULOC," << m_locates_used;
}

void Position::update_remote_position(const EngineID source, Quantity qty, Price avg_px, Price mark_px, Quantity locates_remaining, Quantity locates_used)
{
    if (source >= c_MIN_ENGINE_ID  && source <= c_MAX_ENGINE_ID) {
        auto& rempos = m_remote_positions[source];
        auto oldq = rempos.trading_quantity = qty;
        rempos.vwap = avg_px;
        rempos.mark = mark_px;
        rempos.locates_remaining = locates_remaining;
        rempos.locates_used = locates_used;
        m_remote_trading_quantity += qty - oldq;
        m_remote_vwap = calc_remote_vwap();
    }
}

std::ostream & operator<<(std::ostream &os, const Position& p)
{
    p.print(os);
    return os;
}

}}
