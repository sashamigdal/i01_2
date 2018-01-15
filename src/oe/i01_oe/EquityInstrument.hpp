#pragma once
#include <iosfwd>
#include <string>

#include <i01_oe/Order.hpp>
#include <i01_oe/Instrument.hpp>
#include <i01_oe/InstrumentStatistics.hpp>

namespace i01 { namespace OE {

class EquityInstrument : public Instrument
{
public:
    struct Params : public Instrument::Params {
        Size lot_size;
        Quantity locate_size;
        ADV adv;
        ADR adr;
        bool norecycle;
    };

public:
    EquityInstrument(
            const std::string& isymbol,
            const MD::EphemeralSymbolIndex iesi,
            const core::MIC ilisting_market,
            const Size iorder_size_limit,
            const Price iorder_price_limit,
            const Price iorder_value_limit,
            const int iorder_rate_limit,
            const Size iposition_limit,
            const Price iposition_value_limit,
            const Quantity start_quantity,
            const Price prior_adj_close,
            const Size ilot_size,
            const Quantity ilocate_size,
            const ADV &iadv,
            const ADR &iadr,
            const bool inorecycle)
    : Instrument(isymbol, iesi, ilisting_market, iorder_size_limit, iorder_price_limit, iorder_value_limit,
                 iorder_rate_limit, iposition_limit, iposition_value_limit,
                 start_quantity, prior_adj_close)
        , m_lot_size(ilot_size)
        , m_locate_size(ilocate_size)
        , m_adv(iadv)
        , m_adr(iadr)
        , m_norecycle(inorecycle)
    { m_position.m_locates_remaining = m_locate_size; }

    EquityInstrument(const std::string &isymbol, const MD::EphemeralSymbolIndex iesi, const core::MIC ilisting_market, const Params &p) :
        EquityInstrument(isymbol, iesi, ilisting_market, p.order_size_limit, p.order_price_limit, p.order_value_limit,
                         p.order_rate_limit, p.position_limit, p.position_value_limit,
                         p.start_quantity, p.prior_adjusted_close,
                         p.lot_size, p.locate_size, p.adv, p.adr, p.norecycle)
    {}

    EquityInstrument(const EquityInstrument&) = delete;
    EquityInstrument(EquityInstrument&&) = delete;
    EquityInstrument& operator=(const EquityInstrument&) = delete;
    EquityInstrument& operator=(EquityInstrument&&) = delete;
    bool validate(Order*) override;

    Size lot_size() const { return m_lot_size; }
    Quantity locate_size() const { return m_locate_size; }
    ADV adv() const { return m_adv; }
    ADR adr() const { return m_adr; }
    bool norecycle() const { return m_norecycle; }

    friend std::ostream & operator<<(std::ostream &os, const EquityInstrument &ei);
    friend class OrderManager;
protected:
    void update_locates_live(Quantity new_locate_size, bool only_if_increase);

    Size m_lot_size;
    Quantity m_locate_size;
    ADV m_adv;
    ADR m_adr;
    bool m_norecycle;
};

}}
