#pragma once

#include <array>

#include <i01_core/Lock.hpp>
#include <i01_core/Time.hpp>

#include <i01_md/BookMuxEvents.hpp>
#include <i01_md/OrderData.hpp>
#include <i01_md/Symbol.hpp>
#include <i01_md/SymbolState.hpp>

#include <i01_ts/EquitiesStrategy.hpp>

namespace i01 { namespace core {
struct MIC;
}}

namespace i01 { namespace TS {

class L1EquitiesStrategy : public EquitiesStrategy {
public:
    using Mutex = core::SpinMutex;

public:
    using EquitiesStrategy::EquitiesStrategy;
    virtual ~L1EquitiesStrategy() {}

    /* BookMuxListener */
    virtual void on_symbol_definition(const MD::SymbolDefEvent &) override {}
    virtual void on_book_added(const MD::L3AddEvent& new_o) override;
    virtual void on_book_canceled(const MD::L3CancelEvent& cxl_o) override;
    virtual void on_book_modified(const MD::L3ModifyEvent& mod_e) override;
    virtual void on_book_executed(const MD::L3ExecutionEvent& exe_e) override;
    virtual void on_book_crossed(const MD::L3CrossedEvent&) override {}
    /// NYSE is a precious snowflake.
    virtual void on_l2_update(const MD::L2BookEvent&) override;
    virtual void on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent&) override {}
    virtual void on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent&) override {}
    virtual void on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent&) override {}
    virtual void on_trade(const MD::TradeEvent&) override {}
    virtual void on_gap(const MD::GapEvent&) override {}
    virtual void on_trading_status_update(const MD::TradingStatusEvent&) override {}
    virtual void on_feed_event(const MD::FeedEvent&) override {}
    virtual void on_market_event(const MD::MWCBEvent&) override {}
    virtual void on_start_of_data(const MD::PacketEvent&) override {}
    virtual void on_end_of_data(const MD::PacketEvent&) override;
    virtual void on_timeout_event(const MD::TimeoutEvent&) override {}
    virtual void on_timer(const core::Timestamp&, void*, std::uint64_t) override {}

    // when only one side has changed
    virtual void on_bbo_update(const core::Timestamp&
                               , const core::MIC&
                               , const MD::EphemeralSymbolIndex&
                               , const MD::L3OrderData::Side&
                               , const MD::L2Quote& ) = 0;

    // when both sides have changed from the last update
    virtual void on_bbo_update(const core::Timestamp&
                               , const core::MIC&
                               , const MD::EphemeralSymbolIndex&
                               , const MD::FullL2Quote& ) = 0;

protected:
    MD::FullL2Quote unsafe_best_quote(const core::MIC& mic, MD::EphemeralSymbolIndex esi) const;
    MD::L2Quote unsafe_best_bid_quote(const core::MIC& mic, MD::EphemeralSymbolIndex esi) const;
    MD::L2Quote unsafe_best_ask_quote(const core::MIC& mic, MD::EphemeralSymbolIndex esi) const;

    core::Timestamp unsafe_last_ts(const core::MIC& mic, MD::EphemeralSymbolIndex esi) const;

protected:

    struct StockSide {
        MD::EphemeralSymbolIndex esi;
        std::uint8_t side_flags;

        bool operator<(const StockSide& ss) const {
            return esi < ss.esi ? true : (side_flags < ss.side_flags);
        }
    };

    struct UpdateStocksSet : std::vector<StockSide> {
        void insert(StockSide);
    };

    // using UpdateStocksSet = std::set<StockSide>;

    struct QuoteEntry {
        MD::FullL2Quote quote;
        core::Timestamp bid_ts;
        core::Timestamp ask_ts;
        Mutex mutex;
    };

    using MutexByMIC = std::array<Mutex, (int) core::MIC::Enum::NUM_MIC>;
    using UpdatedStocksByMIC = std::array<UpdateStocksSet, (int)core::MIC::Enum::NUM_MIC>;
    using QuoteArray = std::array<QuoteEntry, MD::NUM_SYMBOL_INDEX>;
    using QuotesByMIC = std::array<QuoteArray, (int) core::MIC::Enum::NUM_MIC>;

private:
    MutexByMIC m_mic_mutexes;
    UpdatedStocksByMIC m_updated_stocks;
    QuotesByMIC m_quotes;
};

} }
