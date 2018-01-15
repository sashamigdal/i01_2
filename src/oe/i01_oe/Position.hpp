#pragma once

#include <iosfwd>
#include <array>
#include <limits>
#include <numeric>
#include <i01_core/Lock.hpp>
#include <i01_oe/Types.hpp>

namespace i01 { namespace OE {

class FirmRiskCheck;
class Instrument;
class OrderManager;
class Order;
class PortfolioRiskCheck;

class Position
{
public:
    Position(Instrument *inst, Quantity startq, Price adj_close);
    Position(const Position &) = delete;
    Position(Position&&) = delete;
    Position& operator=(const Position &) = delete;
    Position& operator=(Position&&) = delete;
    virtual ~Position() {}

    Quantity quantity() const { return trading_quantity(); }
    Quantity trading_quantity() const { return m_trading_quantity; }

    Dollars realized_pnl() const;
    Dollars unrealized_pnl() const;

    /// Notional of the position
    Dollars notional() const;

    Dollars long_open_exposure() const;
    Dollars short_open_exposure() const;

    Size long_open_size() const { return m_open_long_size; }
    Size short_open_size() const { return m_open_short_size; }

    Quantity start_quantity() const { return m_start_quantity; }
    Price prior_adjusted_close() const { return m_prior_adjusted_close; }
    Price mark() const { return m_mark; }
    Price vwap() const { return m_vwap; }

    Quantity remote_quantity() const { return m_remote_trading_quantity; }
    Price remote_vwap() const { return m_remote_vwap; }

    void print(std::ostream &os) const;
    friend std::ostream & operator<<(std::ostream &os, const Position &p);

private:
    Dollars start_of_day_long_exposure() const;
    Dollars start_of_day_short_exposure() const;

    void add_trading_quantity(const Quantity q);
    void add_open_buys(const Price p, const Size s);
    void add_open_sells(const Price p, const Size s);

    void sub_trading_quantity(const Quantity q) { m_trading_quantity -= q; }
    void sub_open_buys(const Price p, const Size s);
    void sub_open_sells(const Price p, const Size s);

    void on_update_mark(const Price p);

    void update_realized_and_unrealized_pnl(const Side side, const Size size, const Price price,
                                            const Dollars fee);

    void start_quantity(Size sq) { m_start_quantity = sq; }
    void prior_adjusted_close(Price pac) { m_prior_adjusted_close = pac; }

    void update_remote_position(const EngineID source, Quantity qty, Price avg_px, Price mark_px, Quantity locates_remaining, Quantity locates_used);

private:
    Quantity m_trading_quantity;
    Quantity m_start_quantity;
    Size m_open_long_size;
    Size m_open_short_size;

    Dollars m_open_long_exposure;
    Dollars m_open_short_exposure;

    // // these are net of fee
    Dollars m_realized;
    Dollars m_unrealized;

    /// VWAP of trading quantity
    Price m_vwap;
    Price m_mark;
    Price m_prior_adjusted_close;

    /// Locates:
    Quantity m_locates_remaining;
    static_assert(std::is_signed<decltype(m_locates_remaining)>::value, "Position::m_locates_remaining must be signed.");
    static const constexpr Quantity LOCATES_REMAINING_SENTINEL = std::numeric_limits<Quantity>::min();
    static_assert(LOCATES_REMAINING_SENTINEL < 0, "LOCATES_REMAINING_SENTINEL must be negative.");

    Quantity m_locates_used;
    bool m_norecycle;

    // Remote positions and locates:
    Quantity m_remote_trading_quantity;
    Price m_remote_vwap;
    struct RemotePosition {
        Quantity trading_quantity;
        Price vwap;
        Price mark;
        Quantity locates_remaining;
        Quantity locates_used;
        RemotePosition()
            : trading_quantity(0), vwap(0), locates_remaining(0), locates_used(0)
        {}
    };
    std::array<RemotePosition, c_MAX_ENGINE_ID + 1> m_remote_positions;

    Quantity calc_remote_quantity() const { 
        Quantity ret = 0;
        for (int i = c_MIN_ENGINE_ID; i <= c_MAX_ENGINE_ID; ++i)
            ret += m_remote_positions[i].trading_quantity;
        return ret;
    }
    Price calc_remote_vwap() const {
        Price ret = 0.0;
        for (int i = c_MIN_ENGINE_ID; i <= c_MAX_ENGINE_ID; ++i)
            ret += ((double)m_remote_positions[i].trading_quantity) * m_remote_positions[i].vwap;
        return ret;
    }

    friend class OrderManager;
    friend class FirmRiskCheck;
    friend class PortfolioRiskCheck;
    friend class EquityInstrument;
};

}}
