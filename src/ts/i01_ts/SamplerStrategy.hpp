#pragma once

#include <i01_core/Application.hpp>
#include <i01_core/Config.hpp>
#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/Version.hpp>

#include <i01_md/DataManager.hpp>

#include <i01_ts/EquitiesStrategy.hpp>

namespace i01 { namespace TS {


class SamplerStrategy : public EquitiesStrategy {
public:
    enum class OutputFormat {
        ORDERBOOK = 1,
        L1_TICKS = 2,
    };

    typedef i01::core::MIC MIC;
    struct SymbolEntry {
        std::string m_cta_symbol;
        std::uint32_t m_fdo_stock_id;
        std::string m_id_string;
    };
    typedef std::vector<SymbolEntry> symbols_container_type;
    typedef i01::core::Timestamp Timestamp;
    typedef std::map<int,MD::FullSummary> stock_summary_map;
    typedef std::vector<stock_summary_map> FullSummaryByMICAndStockMap;

    typedef std::unordered_map<MD::EphemeralSymbolIndex, core::Timestamp> stock_ts_map;
    typedef std::vector<stock_ts_map> TimestampByMICAndStockMap;
    typedef std::set<const MD::BookBase *> OrderBookSet;
    typedef std::vector<OrderBookSet> OrderBooksByMICMap;
    typedef std::unordered_map<MD::BookBase::SymbolIndex, MD::TradingStatusEvent::Event> BookTradingStatusContainer;
    typedef std::vector<BookTradingStatusContainer> TradingStatusByMICAndStock;

private:
    void output_l1_(const MIC &mic, std::uint32_t si, const MD::FullSummary &fs);

    void orderbook_format_(const MD::L2Book *book, const Timestamp &ts, bool is_bid, bool is_bbo, std::uint64_t price, std::uint64_t size, std::uint32_t num_orders);
    void ticks_format_(OutputFormat format, const MD::L2Book *book, const Timestamp &ts, bool is_bid, bool is_bbo, std::uint64_t price, std::uint64_t size, std::uint32_t num_orders);
    void check_offset_(const MIC &, const Timestamp &ts);

    const std::string & id_str_(std::uint32_t esi) const;

public:
    SamplerStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string & n);
    virtual ~SamplerStrategy() {}

    /* OrderListener */
    virtual void on_order_acknowledged(const OE::Order*) override final {}
    virtual void on_order_fill(const OE::Order*, const OE::Size, const OE::Price, const OE::Price) override final {}
    virtual void on_order_cancel(const OE::Order*, const OE::Size) override final {}
    virtual void on_order_cancel_rejected(const OE::Order*) override final {}
    virtual void on_order_cancel_replaced(const OE::Order*) override final {}
    virtual void on_order_rejected(const OE::Order*) override final {}

    /* BookMuxListener */
    virtual void on_symbol_definition(const MD::SymbolDefEvent &) override final;
    virtual void on_book_added(const MD::L3AddEvent& new_o) override final;
    virtual void on_book_canceled(const MD::L3CancelEvent& cxl_o) override final;
    virtual void on_book_modified(const MD::L3ModifyEvent& mod_e) override final;
    virtual void on_book_executed(const MD::L3ExecutionEvent& exe_e) override final;
    virtual void on_book_crossed(const MD::L3CrossedEvent&) override final {}
    /// NYSE is a precious snowflake.
    virtual void on_l2_update(const MD::L2BookEvent&) override final;
    virtual void on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent&) override final {}
    virtual void on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent&) override final {}
    virtual void on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent&) override final {}
    virtual void on_trade(const MD::TradeEvent&) override final;
    virtual void on_gap(const MD::GapEvent&) override final;
    virtual void on_trading_status_update(const MD::TradingStatusEvent&) override final;
    virtual void on_feed_event(const MD::FeedEvent&) override final {}
    virtual void on_timeout_event(const MD::TimeoutEvent&) override final {}
    virtual void on_market_event(const MD::MWCBEvent&) override final {}
    virtual void on_start_of_data(const MD::PacketEvent&) override final;
    virtual void on_end_of_data(const MD::PacketEvent&) override final;

    /* TimerListener */
    virtual void on_timer(const core::Timestamp& ts, void * userdata, std::uint64_t iter) override final {}

private:
    void init_symbols();

private:

    OutputFormat m_output_format;
    bool m_show_gaps;
    bool m_show_imbalances;
    std::string m_offset_hhmmss;
    std::vector<std::uint64_t> m_offset_millis;
    std::vector<bool> m_valid_offset;

    symbols_container_type m_symbols;

    std::vector<Timestamp> m_next_interval;
    std::uint32_t m_interval_seconds;

    OrderBooksByMICMap m_stocks_with_event;
    FullSummaryByMICAndStockMap m_last_best;
    TradingStatusByMICAndStock m_trading_status;

    bool m_quiet_mode;
    bool m_verbose;
    bool m_show_trades;
};

}}
