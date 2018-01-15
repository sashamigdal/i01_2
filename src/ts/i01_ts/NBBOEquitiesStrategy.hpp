#pragma once

#include <i01_md/Symbol.hpp>

#include <i01_ts/L1EquitiesStrategy.hpp>

namespace i01 { namespace MD {
struct FullL2Quote;
struct L2Quote;
enum class Side;
}}

namespace i01 { namespace TS {

class NBBOEquitiesStrategy : public L1EquitiesStrategy {
public:
    using Timestamp = core::Timestamp;

    NBBOEquitiesStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string& n);
    // when only one side has changed
    virtual void on_bbo_update(const Timestamp&
                               , const core::MIC&
                               , const MD::EphemeralSymbolIndex&
                               , const MD::L3OrderData::Side&
                               , const MD::L2Quote& ) override final;

    // when both sides have changed from the last update
    virtual void on_bbo_update(const Timestamp&
                               , const core::MIC&
                               , const MD::EphemeralSymbolIndex&
                               , const MD::FullL2Quote& ) override final;

    // virtual void on_nbbo_update(const Timestamp&, const core::MIC&, MD::EphemeralSymbolIndex, const MD::Side&, const MD::L2Quote&) = 0;
    virtual void on_nbbo_update(const Timestamp&, const core::MIC&, MD::EphemeralSymbolIndex, const MD::FullL2Quote&) = 0;

    virtual void on_trading_status_update(const MD::TradingStatusEvent&) override;

    Timestamp last_data_timestamp() const { return m_last_data_timestamp; }

    void enable_quotes_in_state(const MD::SymbolState::TradingState s);
    void disable_quotes_in_state(const MD::SymbolState::TradingState s);

private:
    const static std::uint32_t SIDE_MASK = 1 << 16;

protected:
    const MD::OrderBook& bbo_book(MD::EphemeralSymbolIndex esi) const;


    static constexpr MD::L3OrderData::RefNum side_to_bits(MD::L3OrderData::Side side) {
        return MD::L3OrderData::Side::BUY == side ? 0 : SIDE_MASK;
    }

    static constexpr MD::L3OrderData::Side refnum_to_side(MD::L3OrderData::RefNum r) {
        return (r & SIDE_MASK) == 0 ? MD::L3OrderData::Side::BUY : MD::L3OrderData::Side::SELL;
    }

    static constexpr std::uint32_t refnum_to_index(MD::L3OrderData::RefNum r) {
        return r & (~SIDE_MASK);
    }

    static constexpr MD::L3OrderData::RefNum make_refnum(std::uint32_t index, MD::L3OrderData::Side side) {
        return index | side_to_bits(side);
    }

private:

    bool update_book(const Timestamp& ts, const core::MIC& mic, MD::EphemeralSymbolIndex esi,
                     MD::L3OrderData::Side side, const MD::L2Quote& q);
    MD::OrderBook::Order make_new_order(MD::L3OrderData::RefNum refnum, const Timestamp& ts, const MD::L2Quote& q);

    void enable_venue(const int mic, const MD::EphemeralSymbolIndex esi);
    void disable_venue(const int mic, const MD::EphemeralSymbolIndex esi);

    void update_venue_from_status(const core::MIC& mic, const MD::EphemeralSymbolIndex index,
                                  const MD::SymbolState::TradingState ts);

private:
    using BooksArray = std::array<MD::OrderBook,MD::NUM_SYMBOL_INDEX>;
    using TradingStateBitfield = std::underlying_type<MD::SymbolState::TradingState>::type;
    using BoolArray = std::array<bool, MD::NUM_SYMBOL_INDEX>;
    using BoolByMIC = std::array<BoolArray, (int) core::MIC::Enum::NUM_MIC>;

private:
    Timestamp m_last_data_timestamp;
    BooksArray m_books;
    BoolByMIC m_venue_enabled;

    TradingStateBitfield m_status_policy_bitfield;

};

}}
