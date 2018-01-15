#pragma once
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <i01_md/BookMuxListener.hpp>
#include <i01_md/MLBookMux.hpp>

#include <i01_md/NASDAQ/ITCH50/Messages.hpp>

#include <i01_oe/NASDAQOrder.hpp>
#include <i01_oe/SimSession.hpp>

namespace i01 { namespace OE {

class L2SimSession : public SimSession, public MD::NoopBookMuxListener {
public:
    L2SimSession(OrderManager *om_p, const std::string& name_);
    virtual ~L2SimSession() = default;

    void on_symbol_definition(const MD::SymbolDefEvent &evt) override final;
    void on_l2_update(const MD::L2BookEvent &evt) override final;
    void on_start_of_data(const MD::PacketEvent &evt) override final;
    void on_trade(const MD::TradeEvent &evt) override final;
    void on_trading_status_update(const MD::TradingStatusEvent &evt) override final;

    void on_book_added(const MD::L3AddEvent &evt) override final;
    void on_book_canceled(const MD::L3CancelEvent &) override final;
    void on_book_modified(const MD::L3ModifyEvent &) override final;
    void on_book_executed(const MD::L3ExecutionEvent &) override final;

    void on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent&) override final;

    const Timestamp & current_ts() const { return m_ts; }

    virtual std::string status() const override final;

private:
    struct SimEvent {
        struct Ordering {
            Timestamp ts;
            int index;
            Ordering(const Timestamp &t, int i = 0) : ts(t), index(i) {}
        };

        OrderState state;
        Order * order_p;
        MD::EphemeralSymbolIndex esi;
        Size size;
        Price price;
        Timestamp ts;

        SimEvent(OrderState os, Order *op, MD::EphemeralSymbolIndex idx = 0,
                 Size fs = 0, Price fp = 0, Timestamp t = Timestamp()) :
            state(os),
            order_p(op),
            esi(idx),
            size(fs),
            price(fp),
            ts(t) {
        }
        SimEvent(OrderState os, Order *op, MD::EphemeralSymbolIndex idx,
                 Timestamp t) : SimEvent(os, op, idx, 0, 0, t) {}
    };
    friend std::ostream & operator<<(std::ostream &os, const SimEvent &se);

    typedef std::pair<SimEvent::Ordering, SimEvent> TimestampedSimEvent;

    // we want the soonest timestamp first, so we have to provide a greater than ordering
    struct TSECompare {
        bool operator () (const TimestampedSimEvent &a, const TimestampedSimEvent &b) {
            if (a.first.ts > b.first.ts) {
                return true;
            } else if (a.first.ts == b.first.ts) {
                return a.first.index > b.first.index;
            }
            return false;
        }
    };

    struct SimOrder {
        Order * order_p;
        Size size_ahead;
        Size size_behind;
        Size remains;
    };
    friend std::ostream & operator<<(std::ostream &os, const SimOrder &so);

    using EventQueue = std::priority_queue<TimestampedSimEvent, std::vector<TimestampedSimEvent>, TSECompare>;

    using OrderMap = std::unordered_map<ExchangeID, Order *>;

    using AskOrderContainer = std::map<Price, SimOrder>;
    using BidOrderContainer = std::map<Price, SimOrder, std::greater<Price> > ;

    using BookIndexMap = std::unordered_map<std::string, MD::EphemeralSymbolIndex>;
    using TradeRefNumContainer = std::unordered_set<MD::TradeEvent::TradeRefNum>;

    using OrderRefNumSizeMap = std::unordered_map<MD::OrderBook::Order::RefNum, MD::OrderBook::Order::Size>;

    struct BookEntry {
        const MD::BookBase * book_p;
        BidOrderContainer bids;
        BidOrderContainer close_bids;
        BidOrderContainer open_bids;
        AskOrderContainer asks;
        AskOrderContainer close_asks;
        AskOrderContainer open_asks;
        TradeRefNumContainer trades;
        MD::SymbolState state;
        BookEntry(const MD::BookBase *p = nullptr) : book_p(p) {}
    };
    using BooksContainer = std::vector<BookEntry>;

    template<typename CompType>
    struct PriceComp {
        Price ref;
        bool operator() (const OE::Order * op) const {
            return CompType()(op->price(), ref);
        }
    };

    using PriceLE = PriceComp<std::less_equal<double> >;
    using PriceGE = PriceComp<std::greater_equal<double> >;

    template<typename CompTypeT>
    struct NasdaqImbCross {
        Price ref;
        MD::NASDAQ::ITCH50::Types::ImbalanceDirection direction;
        bool operator()(OE::Order * op) const {
            namespace tt = MD::NASDAQ::ITCH50::Types;
            auto * nop = static_cast<OE::NASDAQOrder *>(op);
            auto comp = CompTypeT()(op->price(), ref);
            if (nop->nasdaq_display() == OE::NASDAQOrder::Display::IMBALANCE_ONLY) {
                switch (direction) {
                case tt::ImbalanceDirection::BUY_IMBALANCE:
                    return (int) nop->side() >= (int) OE::Side::SELL && comp;
                case tt::ImbalanceDirection::SELL_IMBALANCE:
                    return nop->side() == OE::Side::BUY && comp;
                case tt::ImbalanceDirection::NO_IMBALANCE:
                case tt::ImbalanceDirection::INSUFFICIENT_ORDERS_TO_CALCULATE:
                default:
                    return false;
                }
            }

            // just a regular auction order....
            return comp;
        }
    };

    using NasdaqCrossPriceGE = NasdaqImbCross<std::greater_equal<double> >;
    using NasdaqCrossPriceLE = NasdaqImbCross<std::less_equal<double> >;

    struct AvailSize {
        Size avail;
        Size get(const OE::Order *op) const { return avail; }
        void remove(const OE::Order *op, Size shares) { avail -= shares; }
    };

    struct NasdaqCrossAvailSize {
        Size avail;
        Size imbalance_avail;
        Size get(const OE::Order *op) const {
            auto * nop = static_cast<const OE::NASDAQOrder *>(op);
            if (nop->nasdaq_display() == OE::NASDAQOrder::Display::IMBALANCE_ONLY) {
                return imbalance_avail;
            }
            return avail;
        }
        void remove(const OE::Order *op, Size shares) {
            auto * nop = static_cast<const OE::NASDAQOrder *>(op);
            if (nop->nasdaq_display() == OE::NASDAQOrder::Display::IMBALANCE_ONLY) {
                imbalance_avail -= shares;
            } else {
                avail -= shares;
            }
        }
    };

    struct NOIIEntry {
        using NOII = MD::NASDAQ::ITCH50::Messages::NetOrderImbalanceIndicator;
        using NOIIDirection = MD::NASDAQ::ITCH50::Types::ImbalanceDirection;
        core::Timestamp timestamp;
        NOII noii;
        bool valid;
        NOIIEntry() : timestamp(), noii{}, valid{false} {}
        void update(const core::Timestamp& ts, const NOII& ii) {
            timestamp = ts;
            noii = ii;
            valid = true;
        }
        NOIIDirection direction() const {
            if (valid) {
                return noii.imbalance_direction;
            } else {
                return NOIIDirection::NO_IMBALANCE;
            }
        }
        Size imbalance_shares() const {
            if (valid) {
                return static_cast<Size>(noii.imbalance_shares);
            }
            return 0;
        }
    };

    using NOIIArray = std::array<NOIIEntry, MD::NUM_SYMBOL_INDEX>;

protected:

    virtual bool send(Order *o_p) override;
    virtual bool cancel(Order *o_p, Size newqty = 0) override;

    void update_ts(const Timestamp &ts);

    BookEntry * find_book_entry(const MD::BookBase & book);

    void on_order_sent(const Timestamp &ts, const SimEvent &se);
    void on_order_acknowledged(const Timestamp &ts, const SimEvent &se);
    void on_order_filled(const Timestamp &ts, const SimEvent &se);
    void on_order_pending_cancel(const Timestamp &ts, const SimEvent &se);
    void on_order_cancelled(const Timestamp &ts, const SimEvent &se);
    void on_order_cancel_rejected(const Timestamp &ts, const SimEvent &se);

    Size reduce_size_from_front(SimOrder &o, Size amt);
    void reduce_size_from_back(SimOrder &o, Size amt);

    template<typename ContainerT>
    void add_order_helper(ContainerT &orders, const MD::L3AddEvent &evt);

    template<typename ContainerT>
    void cancel_order_helper(ContainerT &orders, const MD::L3CancelEvent &evt, MD::Size delta_size);

    template<typename ContainerT>
    void modify_order_helper(ContainerT &orders, const MD::L3ModifyEvent &evt, MD::Size delta_if_reduced);

    template<typename C, typename Key>
    Size reduce_helper(C &cont, const Key & k, Order *v, Size newqty);

    template<typename ContainerT, typename PriceComp>
    void exec_cross_helper(ContainerT &orders, MD::EphemeralSymbolIndex idx, const PriceComp &check_price, Size &avail_size, const Timestamp &ts);

    template<typename ContainerT>
    void exec_event_size_helper(ContainerT &orders, const MD::L3ExecutionEvent &evt);

    template<typename ContainerT>
    void size_update_helper(ContainerT &orders, const MD::L2BookEvent &evt);

    template<typename ContainerT, typename PriceCompT, typename AvailSizeT>
    int cross_helper(ContainerT &orders, MD::EphemeralSymbolIndex esi, const PriceCompT &check_price,
                     AvailSizeT avail_size, const Timestamp &ts, bool cxl_remainder = false);

    void handle_nyse_cross(BookEntry *be, const MD::TradeEvent& evt);
    void handle_nasdaq_cross(BookEntry *be, const MD::TradeEvent& evt);

private:
    Timestamp m_ts;
    EventQueue m_eventq;
    OrderMap m_orders;

    BooksContainer m_books;
    BookIndexMap m_index_map;

    // keep track of executions within this data pkt (for ARCA, where
    // we will also get a cancel/modify in the same pkt)
    OrderRefNumSizeMap m_exec_this_pkt;

    bool m_ignore_trading_state;

    NOIIArray m_last_noii;
};



template<typename ContainerT>
void L2SimSession::add_order_helper(ContainerT &orders, const MD::L3AddEvent &evt)
{
    const auto & qprice = MD::to_double(evt.m_order.price);
    auto it = orders.find(qprice);
    if (it == orders.end()) {
        return;
    }
    it->second.size_behind += evt.m_order.size;
}

template<typename ContainerT>
void L2SimSession::cancel_order_helper(ContainerT &orders, const MD::L3CancelEvent &evt, MD::Size delta_size)
{
    const auto & qprice = MD::to_double(evt.m_order.price);
    auto it = orders.find(qprice);
    if (it == orders.end()) {
        return;
    }
    reduce_size_from_back(it->second, delta_size);
}

template<typename ContainerT>
void L2SimSession::modify_order_helper(ContainerT &orders, const MD::L3ModifyEvent &evt, MD::Size delta_if_reduced)
{
    if (evt.m_new_order.price != evt.m_old_price) {
        // old size should be removed from old price level
        auto qprice = MD::to_double(evt.m_old_price);
        auto it = orders.find(qprice);
        if (it != orders.end()) {
            reduce_size_from_back(it->second, evt.m_old_size);
        }

        // add size behind any orders at the new price
        qprice = MD::to_double(evt.m_new_order.price);
        it = orders.find(qprice);
        if (it != orders.end()) {
            it->second.size_behind += evt.m_new_order.size;
        }
    } else {
        // same price, but now different size ...
        auto qprice = MD::to_double(evt.m_old_price);
        auto it = orders.find(qprice);
        if (it == orders.end()) {
            return;
        }
        if (evt.m_new_order.size >= evt.m_old_size) {
            // order has increased size.. i think it loses prio? FIXME
            reduce_size_from_back(it->second, evt.m_old_size);
            it->second.size_behind += evt.m_new_order.size;
        } else {
            // order has decreased
            reduce_size_from_back(it->second, delta_if_reduced);
        }
    }
}

template<typename CT, typename KeyT>
Size L2SimSession::reduce_helper(CT &cont, const KeyT & k, Order *v, Size newqty)
{
    auto it = cont.find(k);
    if (it != cont.end() && it->second.order_p == v) {
        if (newqty == 0 || it->second.remains == newqty) {
            cont.erase(it);
            return it->second.remains;
        } else if (newqty < it->second.remains) {
            it->second.remains -= newqty;
            return newqty;
        } else {
            // newqty > it->second.remains .. found it but still return false so the cancel is rejected
            return 0;
        }
    }

    return 0;
}

template<typename ContainerT, typename PriceCompT>
void L2SimSession::exec_cross_helper(ContainerT &orders, MD::EphemeralSymbolIndex idx,
                                     const PriceCompT &check_price, Size &avail_size,
const Timestamp &ts)
{
    auto it = orders.begin();
    while (it != orders.end() && check_price(it->second.order_p)) {
        // this order is at or better priced ...
        auto & simo = it->second;
        avail_size -= reduce_size_from_front(simo, avail_size);
        if (avail_size) {
            // then we can generate a trade for this order
            auto fill_size = std::min(simo.remains, avail_size);
            m_eventq.emplace(ts+m_ack_latency, SimEvent(OrderState::FILLED, simo.order_p, idx, fill_size, check_price.ref));
            simo.remains -= fill_size;

            if (!simo.remains) {
                it = orders.erase(it);
            }
        } else {
            ++it;
        }
    }
}

template<typename ContainerT>
void L2SimSession::exec_event_size_helper(ContainerT &orders, const MD::L3ExecutionEvent &evt)
{
    const auto & qprice = MD::to_double(evt.m_order.price);
    auto it = orders.find(qprice);
    if (it == orders.end()) {
        return;
    }

    // only worry about new size showing up, this gets placed behind ...
    if (evt.m_order.size > evt.m_old_size) {
        auto delta_size = evt.m_order.size - evt.m_old_size;
        it->second.size_behind += delta_size;
    } else {
        // we've already either eaten up the size in front or executed
        // some (or all) of our order.
    }
}

template<typename ContainerT>
void L2SimSession::size_update_helper(ContainerT &orders, const MD::L2BookEvent &evt)
{
    const auto & qprice = MD::to_double(evt.m_price);
    auto delta_size = evt.m_delta_size;
    const auto & reason = evt.m_reason;

    auto it = orders.find(qprice);
    if (it == orders.end()) {
        return;
    }
    switch (reason) {
    case MD::L2BookEvent::DeltaReasonCode::NEW_ORDER:
        it->second.size_behind += delta_size;
        break;
    case MD::L2BookEvent::DeltaReasonCode::CANCEL:
        reduce_size_from_back(it->second, delta_size);
        break;
    case MD::L2BookEvent::DeltaReasonCode::EXECUTION:
        // data seems to suggest that we will see a trade on the
        // trades feed after we get this message (and not the
        // other way around...)



        // TODO: fix potential double counting if we choose to reduce size here

        // we can see execution changes without a corresponding
        // trade ... but more common is to see an execution
        // followed by a trade.  ideally we should reduce size
        // based on the execution and then conditionally reduce
        // size on a trade


        reduce_size_from_front(it->second, delta_size);
        break;
    case MD::L2BookEvent::DeltaReasonCode::MULTIPLE_EVENTS:
        if (evt.m_is_reduced) {
            reduce_size_from_back(it->second, delta_size);
        } else {
            it->second.size_behind += delta_size;
        }
        break;
    case MD::L2BookEvent::DeltaReasonCode::NONE:
    default:
        break;
    }
}


template<typename ContainerT, typename PriceCompT, typename AvailSizeT>
int L2SimSession::cross_helper(ContainerT& orders, MD::EphemeralSymbolIndex esi,
                               const PriceCompT& check_price, AvailSizeT avail_size,
                               const Timestamp& ts, bool cxl_remainder)
{
    int shares_crossed = 0;
    // while we have orders to fill and size available, check
    // whether the order's spread is better than the hurdle.  if
    // so, we have a fill
    auto it = orders.begin();
    while (it != orders.end()) {
        auto & simo = it->second;
        auto open_size = simo.remains;
        if (simo.order_p->state() < OE::OrderState::ACKNOWLEDGED) {
            // then open_size is not yet set but it should be the same as the size
            open_size = simo.order_p->size();
        }

        if (check_price(simo.order_p) && avail_size.get(simo.order_p)) {
            // then we have a trade at our price or better
            Size fill_size = 0;

            // means we have a trade, so we should fill orders ahead of us

            // if we assume that all hidden liquidity is ahead of us,
            // then we need size_ahead==0 in order to trade ... since
            // we'll be seeing execution messages for the non-hidden
            // liquidity

            // fill_size = std::min(simo.size_ahead, avail_size);
            // avail_size -= fill_size;

            if (0 == simo.size_ahead) {
                // we have shares avail to cross our sim order
                assert(simo.remains <= open_size);
                fill_size = std::min(simo.remains, avail_size.get(simo.order_p));
                avail_size.remove(simo.order_p, fill_size);
                shares_crossed += fill_size;
                m_eventq.emplace(ts+m_ack_latency, SimEvent(OrderState::FILLED, simo.order_p, esi, fill_size, check_price.ref));
                simo.remains -= fill_size;
            }

            if (!simo.remains) {
                // if we have nothing remaining, we need to erase this order
                it = orders.erase(it);
            } else {
                if (cxl_remainder) {
                    m_eventq.emplace(ts+m_ack_latency, SimEvent(OrderState::CANCELLED, simo.order_p, esi, open_size - fill_size, 0, ts));
                }

                ++it;
            }
        } else {
            // then this trade price is not of interest to us, so any
            // worse priced orders will also not get filled...
            if (cxl_remainder) {
                    m_eventq.emplace(ts+m_ack_latency, SimEvent(OrderState::CANCELLED,
                                                                simo.order_p, esi,
                                                                open_size, 0, ts));
            }

            // we continue iterating because we could have differences
            // in how price/size is available
            ++it;
        }
    }
    return shares_crossed;
}


}}
