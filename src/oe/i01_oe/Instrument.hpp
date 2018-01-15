#pragma once

#include <iosfwd>

#include <string>
#include <queue>

#include <i01_core/MIC.hpp>

#include <i01_md/Symbol.hpp>

#include <i01_oe/Types.hpp>
#include <i01_oe/Position.hpp>

namespace i01 { namespace OE {

class Order;

class Instrument
{
public:
    typedef core::RecursiveMutex mutex_type;

    struct Params {
        Quantity start_quantity;
        Price prior_adjusted_close;
        Size order_size_limit;
        Price order_price_limit;
        Price order_value_limit;
        int order_rate_limit;
        Size position_limit;
        Price position_value_limit;
    };


public:
    Instrument(const std::string& isymbol,
               const MD::EphemeralSymbolIndex iesi,
               const core::MIC ilisting_market,
               const Size iorder_size_limit,
               const Price iorder_price_limit,
               const Price iorder_value_limit,
               const int iorder_rate_limit,
               const Size iposition_limit,
               const Price iposition_value_limit,
               const Quantity start_quantity,
               const Price prior_adj_close) :
        m_symbol(isymbol),
        m_esi(iesi),
        m_listing_market(ilisting_market),
        m_order_size_limit(iorder_size_limit),
        m_order_price_limit(iorder_price_limit),
        m_order_value_limit(iorder_value_limit),
        m_order_rate_limit(iorder_rate_limit),
        m_position_limit(iposition_limit),
        m_position_value_limit(iposition_value_limit),
        m_position(this, start_quantity, prior_adj_close)
    {
    }

    Instrument(const std::string &sym, const MD::EphemeralSymbolIndex iesi, const core::MIC& ilisting_market, const Params &p) :
        Instrument(sym, iesi, ilisting_market
                , p.order_size_limit
                , p.order_price_limit
                , p.order_value_limit
                , p.order_rate_limit
                , p.position_limit
                , p.position_value_limit
                , p.start_quantity
                , p.prior_adjusted_close)
    {
    }

    Instrument(const Instrument&) = delete;
    Instrument(Instrument&&) = delete;
    Instrument& operator=(const Instrument&) = delete;
    Instrument& operator=(Instrument&&) = delete;

    virtual ~Instrument() {}

    mutex_type& mutex() const { return m_mutex; }

    const std::string& symbol() const { return m_symbol; }
    Size order_size_limit() const { return m_order_size_limit; }
    Price order_price_limit() const { return m_order_price_limit; }
    Price order_value_limit() const { return m_order_value_limit; }
    int order_rate_limit() const { return m_order_rate_limit; }
    Size position_limit() const { return m_position_limit; }
    Price position_value_limit() const { return m_position_value_limit; }

    const Position& position() const { return m_position; }
    Position& position() { return m_position; }

    MD::EphemeralSymbolIndex esi() const { return m_esi; }
    core::MIC listing_market() const { return m_listing_market; }

    virtual bool validate(Order*) = 0;

    friend std::ostream & operator<<(std::ostream &os, const Instrument &i);

protected:
    mutable mutex_type m_mutex;

    const std::string m_symbol;
    MD::EphemeralSymbolIndex m_esi;
    core::MIC m_listing_market;
    Size m_order_size_limit;
    Price m_order_price_limit;
    Price m_order_value_limit;
    int m_order_rate_limit;
    Size m_position_limit;
    Price m_position_value_limit;

    Position m_position;

    std::queue<Timestamp> m_order_rate;
};

}}
