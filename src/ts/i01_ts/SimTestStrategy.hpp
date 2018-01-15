#pragma once

#include <i01_md/BookBase.hpp>
#include <i01_md/OrderBook.hpp>
#include <i01_md/SymbolState.hpp>

#include <i01_oe/BATSOrder.hpp>
#include <i01_oe/EquityInstrument.hpp>
#include <i01_oe/OrderSessionPtr.hpp>

#include <i01_ts/EquitiesStrategy.hpp>

namespace i01 { namespace TS {

class SimTestStrategy : public EquitiesStrategy {

public:
    using Timestamp = i01::core::Timestamp;
    using BookBase = MD::BookBase;
    using OB = MD::OrderBook;
    using OrderType = OE::BATSOrder;
    using OrderSessionPtr = OE::OrderSessionPtr;
    using OrderState = OE::OrderState;


    struct Event {
        bool is_order;
        OE::Price price;
        OE::Size size;
        OE::Side side;
        OE::TimeInForce tif;
        i01::OE::OrderType type;
        OE::Order *order_p;
    };

    using EventContainer = std::map<Timestamp, Event>;

public:
    SimTestStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string &n);
    void add_event(const Timestamp &ts, const Event &evt);
    void on_start_of_data(const MD::PacketEvent &evt) override final;
    void on_trading_status_update(const MD::TradingStatusEvent &evt) override final;
    void book_added_helper(const MD::FullL2Quote &q, OE::Order*& o, const MD::L3AddEvent &evt);
    void on_book_added(const MD::L3AddEvent &evt) override final;
    void check_for_cancel(OB::Order::Side order_side,
                          const MD::FullL2Quote & q,
                          const BookBase & book);

    void on_book_canceled(const MD::L3CancelEvent &evt) override final;
    void on_book_modified(const MD::L3ModifyEvent &evt) override final;
    void on_book_executed(const MD::L3ExecutionEvent &evt) override final;
    virtual void on_book_crossed(const MD::L3CrossedEvent&) override final {}
    virtual void on_timeout_event(const MD::TimeoutEvent&) override final {}

    void moc_helper(OE::Order *& order_p, OE::Size sz, OE::Side s);
    void l2_update_helper(OE::Order *& order_p, const MD::L2BookEvent &evt);
    void on_l2_update(const MD::L2BookEvent &evt) override final;
    void on_trade(const MD::TradeEvent &evt) override final;
    void on_end_of_data(const MD::PacketEvent &evt) override final;
    void on_order_acknowledged(const OE::Order *o) override final;
    void on_order_fill(const OE::Order *o, const OE::Size s, const OE::Price p, const OE::Price fee) override final;
    void on_order_cancel(const OE::Order *o, const OE::Size s) override final;
    void on_order_cancel_rejected(const OE::Order *o) override final;
    virtual void on_order_cancel_replaced(const OE::Order*) override final {}
    virtual void on_order_rejected(const OE::Order*) override final {}
    virtual void on_symbol_definition(const MD::SymbolDefEvent &) override final {}
    virtual void on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent&) override final {}
    virtual void on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent&) override final {}
    virtual void on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent&) override final {}
    virtual void on_gap(const MD::GapEvent&) override final {}
    virtual void on_feed_event(const MD::FeedEvent&) override final {}
    virtual void on_market_event(const MD::MWCBEvent&) override final {}
    virtual void on_timer(const core::Timestamp &ts, void * userdata, std::uint64_t iter) override final {}

private:
    OrderSessionPtr m_os_ptr;
    OE::EquityInstrument* m_inst;
    EventContainer m_events;
    MD::SymbolState m_state;
    Timestamp m_ts;
    std::array<bool, static_cast<size_t>(core::MICEnum::NUM_MIC)> m_mics;
    MD::FullL2Quote m_last_best;
    OE::Order * m_out_ask;
    OE::Order * m_out_bid;
    OE::Order * m_moc_bid;
    OE::Order * m_moc_ask;
};

}}
