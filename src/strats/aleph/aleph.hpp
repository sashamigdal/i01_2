#pragma once

#include <cstring>
#include <iostream>

#include <i01_core/Version.hpp>
#include <i01_ts/StrategyPlugin.hpp>
#include <i01_ts/EquitiesStrategy.hpp>

namespace i01 { namespace TS {

class AlephStrategy : public EquitiesStrategy {
public:
    AlephStrategy(OE::OrderManager* omp, MD::DataManager * dmp, const std::string& n);
    virtual ~AlephStrategy();

    /* OrderListener */
    virtual void on_order_acknowledged(const OE::Order*) override final;
    virtual void on_order_fill(const OE::Order*, const OE::Size, const OE::Price, const OE::Price fee) override final;
    virtual void on_order_cancel(const OE::Order*, const OE::Size) override final;
    virtual void on_order_cancel_rejected(const OE::Order*) override final;
    virtual void on_order_cancel_replaced(const OE::Order*) override final;
    virtual void on_order_rejected(const OE::Order*) override final;

    /* BookMuxListener */
    virtual void on_symbol_definition(const MD::SymbolDefEvent &) override final;
    virtual void on_book_added(const MD::L3AddEvent& new_o) override final;
    virtual void on_book_canceled(const MD::L3CancelEvent& cxl_o) override final;
    virtual void on_book_modified(const MD::L3ModifyEvent& mod_e) override final;
    virtual void on_book_executed(const MD::L3ExecutionEvent& exe_e) override final;
    virtual void on_book_crossed(const MD::L3CrossedEvent&) override final;
    /// NYSE is a precious snowflake.
    virtual void on_l2_update(const MD::L2BookEvent&) override final;
    virtual void on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent&) override final;
    virtual void on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent&) override final;
    virtual void on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent&) override final;
    virtual void on_trade(const MD::TradeEvent&) override final;
    virtual void on_gap(const MD::GapEvent&) override final;
    virtual void on_trading_status_update(const MD::TradingStatusEvent&) override final;
    virtual void on_feed_event(const MD::FeedEvent&) override final;
    virtual void on_market_event(const MD::MWCBEvent&) override final;
    virtual void on_start_of_data(const MD::PacketEvent&) override final;
    virtual void on_end_of_data(const MD::PacketEvent&) override final;
    virtual void on_timeout_event(const MD::TimeoutEvent&) override final;
    /* TimerListener */
    virtual void on_timer(const core::Timestamp&, void *, std::uint64_t) override final;
};

} }
