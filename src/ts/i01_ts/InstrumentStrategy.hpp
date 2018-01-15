#pragma once

#include <set>
#include <memory>
#include <limits>

#include <i01_core/Version.hpp>
#include <i01_md/DataManager.hpp>
#include <i01_md/Symbol.hpp>
#include <i01_md/BookMuxEvents.hpp>
#include <i01_md/BookBase.hpp>
#include <i01_md/OrderBook.hpp>
#include <i01_md/L2Book.hpp>
#include <i01_oe/EquityInstrument.hpp>
#include <i01_oe/OrderManager.hpp>
#include <i01_ts/EquitiesStrategy.hpp>

namespace i01 { namespace TS {

template <typename InstStratT>
class InstrumentStrategy;
template <typename InstStratT>
class PerInstStrat : protected std::enable_shared_from_this<PerInstStrat<InstStratT> > {
    protected:
        InstrumentStrategy<InstStratT>& m_container;
        MD::EphemeralSymbolIndex m_esi;
        const OE::EquityInstrument* m_eqinst_p;

    public:
        PerInstStrat(InstrumentStrategy<InstStratT>& container)
            : PerInstStrat(container, nullptr) {}
        PerInstStrat(InstrumentStrategy<InstStratT>& container, const OE::EquityInstrument* eqinst_p)
            : m_container(container), m_esi(eqinst_p != nullptr ? eqinst_p->esi() : 0), m_eqinst_p(eqinst_p) {}
        virtual ~PerInstStrat() {}

        /// Override this to get a normalized callback when an order is
        /// updated.  This normalized callback is useful, e.g., if your
        /// strategy code does not care about state transitions, merely the
        /// current state, e.g. if you only care about open_size() and
        /// filled_size()...
        virtual void on_order_update(const OE::Order* op) = 0;

        virtual void on_book_update(const MD::BookBase& book) = 0;

        virtual void on_l3_book_update(const MD::OrderBook& book) { on_book_update(book); }
        virtual void on_l2_book_update(const MD::L2Book& book) { on_book_update(book); }

        /* OrderListener */
        /// Override if you don't want to use the normalized callback for ACKs:
        virtual void on_order_acknowledged(const OE::Order* op) { on_order_update(op); }
        /// Override if you don't want to use the normalized callback for FILLs:
        virtual void on_order_fill(const OE::Order* op, const OE::Size, const OE::Price fillpx, const OE::Price fee) { on_order_update(op); }
        /// Override if you don't want to use the normalized callback for CXLs:
        virtual void on_order_cancel(const OE::Order* op, const OE::Size) { on_order_update(op); }
        /// Override if you don't want to use the normalized callback for CXLREJs:
        virtual void on_order_cancel_rejected(const OE::Order* op) { on_order_update(op); }
        /// Override if you don't want to use the normalized callback for CXLREPLs:
        virtual void on_order_cancel_replaced(const OE::Order* op) { on_order_update(op); }
        /// Override if you don't want to use the normalized callback for REJs:
        virtual void on_order_rejected(const OE::Order* op) { on_order_update(op); }

        /* BookMuxListener */
        virtual void on_book_added(const MD::L3AddEvent& new_o) { on_l3_book_update(new_o.m_book); }
        virtual void on_book_canceled(const MD::L3CancelEvent& cxl_o) { on_l3_book_update(cxl_o.m_book); }
        virtual void on_book_modified(const MD::L3ModifyEvent& mod_e) { on_l3_book_update(mod_e.m_book); }
        virtual void on_book_executed(const MD::L3ExecutionEvent& exe_e) { on_l3_book_update(exe_e.m_book); }
        virtual void on_book_crossed(const MD::L3CrossedEvent& xev) { on_l3_book_update(xev.book); }
        /// NYSE is a precious snowflake.
        virtual void on_l2_update(const MD::L2BookEvent& e) { on_l2_book_update(e.m_book); }

        virtual void on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent&) {}
        virtual void on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent&) {}
        virtual void on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent&) {}
        virtual void on_trade(const MD::TradeEvent&) {}
        virtual void on_trading_status_update(const MD::TradingStatusEvent&) {}
        /* TimerListener */
        virtual void on_timer(const core::Timestamp&, void *, std::uint64_t) = 0;
};

template <class InstStratT>
class InstrumentStrategy : public EquitiesStrategy {
public:
    static_assert(std::is_base_of<PerInstStrat<InstStratT>, InstStratT>::value, "InstStratT must have PerInstStrat<InstStratT> as a base class.");
    typedef MD::ESIArray<std::shared_ptr<InstStratT> > InstStratsArray;

    InstrumentStrategy(OE::OrderManager* omp, MD::DataManager * dmp, const std::string& n);
    virtual ~InstrumentStrategy();

    /* OrderListener */
    virtual void on_order_acknowledged(const OE::Order*) override;
    virtual void on_order_fill(const OE::Order*, const OE::Size, const OE::Price, const OE::Price fee) override;
    virtual void on_order_cancel(const OE::Order*, const OE::Size) override;
    virtual void on_order_cancel_rejected(const OE::Order*) override;
    virtual void on_order_cancel_replaced(const OE::Order*) override;
    virtual void on_order_rejected(const OE::Order*) override;

    /* BookMuxListener */
    virtual void on_symbol_definition(const MD::SymbolDefEvent &) override;
    virtual void on_book_added(const MD::L3AddEvent& new_o) override;
    virtual void on_book_canceled(const MD::L3CancelEvent& cxl_o) override;
    virtual void on_book_modified(const MD::L3ModifyEvent& mod_e) override;
    virtual void on_book_executed(const MD::L3ExecutionEvent& exe_e) override;
    virtual void on_book_crossed(const MD::L3CrossedEvent&) override;
    /// NYSE is a precious snowflake.
    virtual void on_l2_update(const MD::L2BookEvent&) override;
    virtual void on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent&) override;
    virtual void on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent&) override;
    virtual void on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent&) override;
    virtual void on_trade(const MD::TradeEvent&) override;
    virtual void on_gap(const MD::GapEvent&) override;
    virtual void on_trading_status_update(const MD::TradingStatusEvent&) override;
    virtual void on_feed_event(const MD::FeedEvent&) override;
    virtual void on_market_event(const MD::MWCBEvent&) override;
    virtual void on_start_of_data(const MD::PacketEvent&) override;
    virtual void on_end_of_data(const MD::PacketEvent&) override;
    virtual void on_timeout_event(const MD::TimeoutEvent&) override;
    /* TimerListener */
    virtual void on_timer(const core::Timestamp&, void *, std::uint64_t) override;

    /* Instrument-specific strategies. */
    const InstStratsArray& all() { return m_inststrats; }

protected:
    InstStratsArray& all_mutable() { return m_inststrats; }

private:
    InstStratsArray m_inststrats;
    std::set<MD::EphemeralSymbolIndex> m_active_esis;

    friend class PerInstStrat<InstStratT>;
    friend InstStratT;
};

template <class T>
InstrumentStrategy<T>::InstrumentStrategy(OE::OrderManager * omp, MD::DataManager *dmp, const std::string& n)
    : i01::TS::EquitiesStrategy(omp, dmp, n)
{
    // Initialize per-instrument instances:
    for (auto& i : omp->universe()) {
        if (i.data()->esi() > 0 && i.data()->esi() < sizeof(m_inststrats)) {
            m_inststrats[i.data()->esi()] = std::shared_ptr<PerInstStrat<T> >(
                    new PerInstStrat<T>(*this, i.data())
                );
            m_active_esis.insert(i.data()->esi());
        } else {
            std::cerr << "ERROR: InstrumentStrategy encountered out of bounds ESI." << std::endl;
        }
    }
}

template <class T>
InstrumentStrategy<T>::~InstrumentStrategy()
{
}

template <class T>
void InstrumentStrategy<T>::on_order_acknowledged(const OE::Order* op)
{
    if (!op || !(op->instrument()))
        return;
    m_inststrats[op->instrument()->esi()]->on_order_acknowledged(op);
}

template <class T>
void InstrumentStrategy<T>::on_order_fill(const OE::Order* op, const OE::Size fillsz, const OE::Price fillpx, const OE::Price fee)
{
    if (!op || !(op->instrument()))
        return;
    m_inststrats[op->instrument()->esi()]->on_order_fill(op, fillsz, fillpx, fee);
}

template <class T>
void InstrumentStrategy<T>::on_order_cancel(const OE::Order* op, const OE::Size cxlsz)
{
    if (!op || !(op->instrument()))
        return;
    m_inststrats[op->instrument()->esi()]->on_order_cancel(op, cxlsz);
}

template <class T>
void InstrumentStrategy<T>::on_order_cancel_rejected(const OE::Order* op)
{
    if (!op || !(op->instrument()))
        return;
    m_inststrats[op->instrument()->esi()]->on_order_cancel_rejected(op);
}

template <class T>
void InstrumentStrategy<T>::on_order_cancel_replaced(const OE::Order* op)
{
    if (!op || !(op->instrument()))
        return;
    m_inststrats[op->instrument()->esi()]->on_order_cancel_replaced(op);
}

template <class T>
void InstrumentStrategy<T>::on_order_rejected(const OE::Order* op)
{
    if (!op || !(op->instrument()))
        return;
    m_inststrats[op->instrument()->esi()]->on_order_rejected(op);
}

template <class T>
void InstrumentStrategy<T>::on_symbol_definition(const MD::SymbolDefEvent & e)
{
}

template <class T>
void InstrumentStrategy<T>::on_book_added(const MD::L3AddEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_book_added(e);
}

template <class T>
void InstrumentStrategy<T>::on_book_canceled(const MD::L3CancelEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_book_canceled(e);
}

template <class T>
void InstrumentStrategy<T>::on_book_modified(const MD::L3ModifyEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_book_modified(e);
}

template <class T>
void InstrumentStrategy<T>::on_book_executed(const MD::L3ExecutionEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_book_executed(e);
}

template <class T>
void InstrumentStrategy<T>::on_book_crossed(const MD::L3CrossedEvent& e)
{
    m_inststrats[e.book.symbol_index()]->on_book_crossed(e);
}

template <class T>
void InstrumentStrategy<T>::on_timeout_event(const MD::TimeoutEvent& e)
{
}

/// NYSE is a precious snowflake.
template <class T>
void InstrumentStrategy<T>::on_l2_update(const MD::L2BookEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_l2_update(e);
}

template <class T>
void InstrumentStrategy<T>::on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_nyse_opening_imbalance(e);
}

template <class T>
void InstrumentStrategy<T>::on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_nyse_closing_imbalance(e);
}

template <class T>
void InstrumentStrategy<T>::on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_nasdaq_imbalance(e);
}

template <class T>
void InstrumentStrategy<T>::on_trade(const MD::TradeEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_trade(e);
}

template <class T>
void InstrumentStrategy<T>::on_gap(const MD::GapEvent& e)
{
}

template <class T>
void InstrumentStrategy<T>::on_trading_status_update(const MD::TradingStatusEvent& e)
{
    m_inststrats[e.m_book.symbol_index()]->on_trading_status_update(e);
}

template <class T>
void InstrumentStrategy<T>::on_feed_event(const MD::FeedEvent& e)
{
}

template <class T>
void InstrumentStrategy<T>::on_market_event(const MD::MWCBEvent& e)
{
}

template <class T>
void InstrumentStrategy<T>::on_start_of_data(const MD::PacketEvent& e)
{
}

template <class T>
void InstrumentStrategy<T>::on_end_of_data(const MD::PacketEvent& e)
{
}

template <class T>
void InstrumentStrategy<T>::on_timer(const core::Timestamp& I01_UNUSED ts, void * I01_UNUSED userdata, std::uint64_t I01_UNUSED iter)
{
    for (auto& i : m_inststrats) {
        i->on_timer(ts, userdata, iter);
    }
}

} } // namespace i01::TS
