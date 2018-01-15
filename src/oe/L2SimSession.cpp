#include <i01_core/Config.hpp>
#include <i01_oe/L2SimSession.hpp>

#include <i01_md/L2Book.hpp>
#include <i01_md/ARCA/XDP/Messages.hpp>
#include <i01_md/NASDAQ/ITCH50/Messages.hpp>

namespace i01 { namespace OE {

L2SimSession::L2SimSession(OrderManager *om_p, const std::string& name_)
    : SimSession(om_p, name_, "L2SimSession")
    , m_ignore_trading_state(true)
{

    // set ack/cxl/fill latency here if available in Config ....
    auto cfg = core::Config::instance().get_shared_state();
    auto cs(cfg->copy_prefix_domain("oe.sessions." + name_ + "."));

    std::uint32_t num;
    if (!(cs->get<std::uint32_t>("ack_latency_ns", num))) {
        std::cerr << "L2SimSession: " << name_ << ": warning no ack_latency_ns given, using 2e5 ns" << std::endl;
        num = 200*1000;
    }
    ack_latency(Timestamp{0,num});

    if (!(cs->get<std::uint32_t>("cxl_latency_ns", num))) {
        std::cerr << "L2SimSession: " << name_ << ": warning no cxl_latency_ns given, using ack latency value" << std::endl;
        cxl_latency(ack_latency());
    } else {
        cxl_latency(Timestamp{0,num});
    }
    m_active = true;
}

bool L2SimSession::send(Order *o_p)
{
    // make sure we have a valid time first before we accept orders
    if (m_ts == Timestamp()) {
        std::cerr << "ERR,L2SIM," << market() << "," << name()
                  << ",SEND,NO TIMESTAMP," << *o_p << std::endl;
        return false;
    }

    switch(o_p->type()) {
    case OrderType::MARKET:
    case OrderType::LIMIT:
        break;
    case OrderType::UNKNOWN:
    case OrderType::STOP:
    case OrderType::MIDPOINT_PEG:
    default:
        std::cerr << "ERR,L2SIM," << market() << "," << name()
                  << ",SEND,TYPE_UNSUPPORTED," << *o_p << std::endl;
        return false;
    }

    switch (o_p->tif()) {
    case TimeInForce::DAY:
    case TimeInForce::EXT:
    case TimeInForce::IMMEDIATE_OR_CANCEL:
    case TimeInForce::AUCTION_OPEN:
    case TimeInForce::AUCTION_CLOSE:
        break;
    case TimeInForce::UNKNOWN:
    case TimeInForce::GTC:
    case TimeInForce::AUCTION_HALT:
    case TimeInForce::TIMED:
    case TimeInForce::FILL_OR_KILL:
    default:
        std::cerr << "ERR,L2SIM," << market() << "," << name()
                  << ",SEND,TIF_UNSUPPORTED," << *o_p << std::endl;
        return false;
    }

    // here we make sure we know about this symbol
    auto it = m_index_map.find(o_p->instrument()->symbol());
    if (it == m_index_map.end()) {
        std::cerr << "ERR,L2SIM," << market() << "," << name()
                  << ",SEND,NO_ESI," << *o_p << std::endl;
        return false;
    }
    auto idx = it->second;


    auto ts = m_ts + m_ack_latency;

    set_order_sent_time(o_p, m_ts);
    m_eventq.emplace(ts,SimEvent{OrderState::SENT, o_p, idx});
    return true;
}

bool L2SimSession::cancel(Order *o_p, Size newqty)
{
    auto ts = m_ts + m_cxl_latency;

    auto it = m_index_map.find(o_p->instrument()->symbol());
    if (it == m_index_map.end()) {
        std::cerr << "ERR,L2SIM," << market() << "," << name()
                  << ",CXL,NO_ESI," << *o_p << std::endl;
        return false;
    }
    auto idx = it->second;

    if (newqty >= o_p->size()) {
        std::cerr << "ERR,L2SIM," << market() << "," << name()
                  << "CXL,NEWQTY INCREASES,"
                  << newqty << ","
                  << *o_p << std::endl;
        return false;
    }

    set_order_last_request_time(o_p, m_ts);
    m_eventq.emplace(ts,SimEvent{OrderState::PENDING_CANCEL, o_p, idx, newqty});

    return true;
}

void L2SimSession::on_symbol_definition(const MD::SymbolDefEvent &evt)
{
    if (evt.mic != m_mic) {
        return;
    }

    m_index_map[evt.ticker] = evt.esi;

    if (evt.esi >= m_books.size()) {
        m_books.resize(evt.esi+1);
    }
}

void L2SimSession::on_trading_status_update(const MD::TradingStatusEvent &evt)
{
    if (evt.m_book.mic() != m_mic) {
        return;
    }

    auto idx = evt.m_book.symbol_index();
    if (idx >= m_books.size()) {
        return;
    }

    m_books[idx].state.update(evt.m_event);
}

auto L2SimSession::find_book_entry(const MD::BookBase &book) -> BookEntry *
{
    auto idx = book.symbol_index();

    if (idx >= m_books.size()) {
        return nullptr;
    }

    if (nullptr == m_books[idx].book_p) {
        m_books[idx].book_p = &book;
    }

    return &m_books[idx];
}

void L2SimSession::on_book_added(const MD::L3AddEvent &evt)
{
    if (evt.m_book.mic() != m_mic) {
        return;
    }

    auto * be = find_book_entry(evt.m_book);
    if (!be) {
        return;
    }

    if (evt.m_order.side == MD::OrderBook::Order::Side::BUY) {
        add_order_helper(be->bids, evt);
    } else {
        add_order_helper(be->asks, evt);
    }
}

void L2SimSession::on_book_canceled(const MD::L3CancelEvent &evt)
{
    if (evt.m_book.mic() != m_mic) {
        return;
    }

    auto * be = find_book_entry(evt.m_book);
    if (!be) {
        return;
    }

    auto delta_size = evt.m_old_size - evt.m_order.size;
    auto it = m_exec_this_pkt.find(evt.m_order.refnum);
    if (it != m_exec_this_pkt.end()) {
        delta_size = it->second;
    }

    if (evt.m_order.side == MD::OrderBook::Order::Side::BUY) {
        cancel_order_helper(be->bids, evt, delta_size);
    } else {
        cancel_order_helper(be->asks, evt, delta_size);
    }
}

void L2SimSession::on_book_modified(const MD::L3ModifyEvent &evt)
{
    if (evt.m_book.mic() != m_mic) {
        return;
    }

    auto * be = find_book_entry(evt.m_book);
    if (!be) {
        return;
    }

    auto delta_size = evt.m_old_size - evt.m_new_order.size;
    auto it = m_exec_this_pkt.find(evt.m_old_refnum);
    if (it != m_exec_this_pkt.end()) {
        // then it's a reduce of the old size I hope
        // SEE FIXME ON L2SimSession.hpp:232
        //assert(evt.m_new_order.size < evt.m_old_size);
        delta_size = it->second;
    }

    if (evt.m_new_order.side == MD::OrderBook::Order::Side::BUY) {
        modify_order_helper(be->bids, evt, delta_size);
    } else {
        modify_order_helper(be->asks, evt, delta_size);
    }
}

void L2SimSession::on_book_executed(const MD::L3ExecutionEvent &evt)
{
    if (evt.m_book.mic() != m_mic) {
        return;
    }

    auto * be = find_book_entry(evt.m_book);
    if (!be) {
        return;
    }

    // since we found our book entry, then we know we're using the esi to index
    auto idx = evt.m_book.symbol_index();

    Price ref_price = MD::to_double(evt.m_exec_price);
    auto avail_size = evt.m_exec_size;

    // so that we only try and cross once (looking at you arca, where
    // we get execution message and a trade message)
    auto it = be->trades.find(evt.m_trade_id);
    if (it != be->trades.end()) {
        // we've already seen this trade
        return;
    }

    // add this for ARCX: assume that we get Exec and Cancel/Modify
    // message in same packet ...  if we have any sim orders at this
    // price, then we should denote the size of the execution, because
    // that is how much we have modified any sim orders for
    // subsequent Cancel/Modify messages

    if (evt.m_order.side == MD::OrderBook::Order::Side::BUY) {
        // this will remove from front/trade at or better priced orders
        exec_cross_helper(be->bids, idx, PriceGE{ref_price}, avail_size, evt.m_timestamp);

        // now we may need to reduce size
        exec_event_size_helper(be->bids, evt);
    } else {
        exec_cross_helper(be->asks, idx, PriceLE{ref_price}, avail_size, evt.m_timestamp);

        exec_event_size_helper(be->asks, evt);
    }

    m_exec_this_pkt.insert({evt.m_order.refnum, avail_size});
    be->trades.insert(evt.m_trade_id);
}

void L2SimSession::on_l2_update(const MD::L2BookEvent &evt)
{
    if (evt.m_book.mic() != m_mic) {
        return;
    }

    auto idx = evt.m_book.symbol_index();
    if (idx >= m_books.size()) {
        // don't know about this instrument ... error?
        return;
    }

    auto &be = m_books[idx];

    if (nullptr == be.book_p) {
        be.book_p = &evt.m_book;
    }

    // ignore full updates here ...
    if (0 == evt.m_delta_size) {
        return;
    }

    // based on side of quote, we will either try and cross or update size
    if (evt.m_is_buy) {
        size_update_helper(be.bids, evt);
        if (evt.m_is_bbo) {
            // we need to get the actual BBO, since this update could be to set a price level to 0 size ...
            MD::L2Quote q = evt.m_book.best_bid();
            auto ref_price = MD::to_double(q.price);
            cross_helper(be.asks, idx, PriceLE{ref_price}, AvailSize{q.size}, evt.m_timestamp);
        }
    } else {
        size_update_helper(be.asks, evt);
        if (evt.m_is_bbo) {
            MD::L2Quote q = evt.m_book.best_ask();
            auto ref_price = MD::to_double(q.price);
            cross_helper(be.bids, idx, PriceGE{ref_price}, AvailSize{q.size}, evt.m_timestamp);
        }
    }
}

void L2SimSession::on_trade(const MD::TradeEvent &evt)
{
    if (evt.m_book.mic() != m_mic) {
        return;
    }

    auto * be = find_book_entry(evt.m_book);
    if (!be) {
        return;
    }
    auto idx = evt.m_book.symbol_index();

    auto it = be->trades.find(evt.m_trade_id);
    if (it != be->trades.end()) {
        // already seen this trade, so dont need to check for crosses
        return;
    }

    if (evt.m_cross) {
        // normalize the cross types?
        if (core::MICEnum::XNYS == m_mic) {
            handle_nyse_cross(be, evt);
        } else if (core::MICEnum::XNAS == m_mic) {
            handle_nasdaq_cross(be, evt);
        }
        // we shouldn't cross anything other than auction orders if this is a cross trade
        return;
    }

    if (be->bids.size() || be->asks.size()) {
        auto avail_size = AvailSize{static_cast<OE::Size>(evt.m_size)};
        int sum =0;
        auto trade_price = MD::to_double(evt.m_price);
        sum += cross_helper(be->bids, idx, PriceGE{trade_price}, avail_size, evt.m_timestamp);

        // can't fill both bids and asks on same trade
        if (sum) {
            return;
        }
        cross_helper(be->asks, idx, PriceLE{trade_price}, avail_size, evt.m_timestamp);
    }

    be->trades.insert(evt.m_trade_id);
}

void L2SimSession::on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent& evt)
{
    // I don't think we even need to do this check??
    if (UNLIKELY(evt.m_book.mic() != m_mic)) {
        return;
    }

    m_last_noii[evt.m_book.symbol_index()].update(evt.m_timestamp, *evt.msg);
}

void L2SimSession::on_start_of_data(const MD::PacketEvent &evt)
{
    if (evt.mic != m_mic) {
        return;
    }

    m_exec_this_pkt.clear();
    update_ts(evt.timestamp);
}


void L2SimSession::update_ts(const Timestamp &ts)
{
    // this should check for events to service
    while (!m_eventq.empty() && ts > m_eventq.top().first.ts) {
        // then we have an event we need to deal with here ...
        // std::cerr << "L2SIM,UPD_TS," << ts << ","
        //           << m_eventq.top().first << ","
        //           << m_eventq.top().second << std::endl;
        auto evt = m_eventq.top();
        // set the sim time here b/c these call backs could generate more events
        m_ts = evt.first.ts;
        // we have to pop here, b/c in processing the event we could have another event added
        m_eventq.pop();
        if (OrderState::SENT == evt.second.state) {
            on_order_sent(evt.first.ts, evt.second);
        } else if (OrderState::ACKNOWLEDGED == evt.second.state) {
            on_order_acknowledged(evt.first.ts, evt.second);
        } else if (OrderState::PENDING_CANCEL == evt.second.state) {
            on_order_pending_cancel(evt.first.ts, evt.second);
        } else if (OrderState::CANCELLED == evt.second.state) {
            on_order_cancelled(evt.first.ts, evt.second);
        } else if (OrderState::CANCEL_REJECTED == evt.second.state) {
            on_order_cancel_rejected(evt.first.ts, evt.second);
        } else if (OrderState::FILLED == evt.second.state) {
            on_order_filled(evt.first.ts, evt.second);
        } else {
            std::cerr << "ERR,L2SIM,"
                      << market() << "," << name() << ","
                      << "UNSUPPORTED ORDER STATE,"
                      << evt.first.ts << ","
                      << evt.first.index << ","
                      << evt.second
                      << std::endl;
        }

        // TODO: change Order memory model to allow deletion
        // delete evt.second.order_p;
    }
    m_ts = ts;
}

void L2SimSession::on_order_sent(const Timestamp &ts, const SimEvent &se)
{
    // we are now the exchange and have received this order ... let's
    // try and match and send back ack after another wait ...

    // we are getting this callback before the current packet has been
    // integrated into the bookmux (since time is from start_of_data)

    // for a new order: 1. if it crosses, then we need to generate a
    // fill.  2. if it's an IOC, then we need to cancel the unfilled
    // remainder.  3. for non-IOC, we need to "add" the order to the
    // book, which is just a map based on price

    auto * op = se.order_p;
    m_orders[op->localID()] = op;

    auto idx = se.esi;
    auto &be = m_books[idx];
    Price fill_price = 0;
    Size fill_size = 0;
    auto remains = op->size();

    // check the trading status
    if (!m_ignore_trading_state) {
        switch(be.state.trading_state) {
        case MD::SymbolState::TradingState::TRADING:
        case MD::SymbolState::TradingState::QUOTATION_ONLY:
            // TODO: fix this to deal with quote only
            break;
        case MD::SymbolState::TradingState::UNKNOWN:
        case MD::SymbolState::TradingState::HALTED:
        default:
            std::cerr << "ERR,L2SIM,"
                      << market() << "," << name()
                      << ",SEND,TRADING_STATE,"
                      << be.state << ","
                      << *op << std::endl;
            goto cancel;
        }
    }

    // TODO: check for SSR condition

    // TODO: check session validity

    if (TimeInForce::AUCTION_OPEN == op->tif() || TimeInForce::AUCTION_CLOSE == op->tif()) {
        if (TimeInForce::AUCTION_OPEN == op->tif()) {
            switch (be.state.session) {
            case MD::SymbolState::Session::UNKNOWN:
            case MD::SymbolState::Session::PREMARKET:
                break;
            case MD::SymbolState::Session::MARKET:
            case MD::SymbolState::Session::POSTMARKET:
            default:
                goto cancel;
            }
        }

        if (TimeInForce::AUCTION_CLOSE == op->tif()) {
            switch (be.state.session) {
            case MD::SymbolState::Session::UNKNOWN:
            case MD::SymbolState::Session::PREMARKET:
            case MD::SymbolState::Session::MARKET:
                break;
            case MD::SymbolState::Session::POSTMARKET:
            default:
                goto cancel;
            }
        }

        auto price = op->price();
        auto so = SimOrder{op, 0, 0, op->size()};
        if (Side::BUY == op->side()) {
            if (OrderType::MARKET == op->type()) {
                price = 5e5; // TODO: find a better way to handle MOO/MOC orders?
            }
            if (TimeInForce::AUCTION_CLOSE == op->tif()) {
                be.close_bids.emplace(price, so);
            } else {
                be.open_bids.emplace(price, so);
            }
        } else {
            if (OrderType::MARKET == op->type()) {
                price = 0;
            }
            if (TimeInForce::AUCTION_CLOSE == op->tif()) {
                be.close_asks.emplace(price, so);
            } else {
                be.open_asks.emplace(price, so);
            }
        }
        m_eventq.emplace(ts + m_ack_latency, SimEvent{OrderState::ACKNOWLEDGED, op, idx});
        return;
    }

    if (nullptr != be.book_p) {
        const auto & q= be.book_p->best();
        MD::FullL2Quote l2q(q);
        if (op->side() == Side::BUY) {
            // only support one order per price right now
            if (be.bids.find(op->price()) != be.bids.end()) {
                goto cancel;
            }

            if ((OrderType::MARKET == op->type() || op->price() >= l2q.ask.price_as_double())) {
                fill_size = std::min(op->size(), l2q.ask.size);
                fill_price = l2q.ask.price_as_double();
                remains -= fill_size;
                // we generate the ack lower down
                m_eventq.emplace(ts + m_ack_latency, SimEvent{OrderState::ACKNOWLEDGED, op, idx});
                m_eventq.emplace(SimEvent::Ordering{ts + m_ack_latency, 1}, SimEvent(OrderState::FILLED, op, idx, fill_size, fill_price, ts));

                // if we are a non-market, non-IOC, then we send a fill... otherwise we need to partially cancel
                if (remains) {
                    if ((OrderType::MARKET == op->type()) || TimeInForce::IMMEDIATE_OR_CANCEL == op->tif()) {
                        m_eventq.emplace(SimEvent::Ordering{ts + m_cxl_latency, 1}, SimEvent(OrderState::CANCELLED, op, idx, remains, 0, ts));
                        return;
                    }
                }
            }
        } else if (op->side() > Side::BUY) {
            if (be.asks.find(op->price()) != be.asks.end()) {
                goto cancel;
            }

            if ((OrderType::MARKET == op->type() || op->price() <= l2q.bid.price_as_double())) {
                fill_size = std::min(op->size(), l2q.bid.size);
                fill_price = l2q.bid.price_as_double();
                remains -= fill_size;
                m_eventq.emplace(ts + m_ack_latency, SimEvent{OrderState::ACKNOWLEDGED, op, idx});
                m_eventq.emplace(SimEvent::Ordering{ts + m_ack_latency, 1}, SimEvent(OrderState::FILLED, op, idx, fill_size, fill_price, ts));

                if (remains) {
                    if ((OrderType::MARKET == op->type()) || TimeInForce::IMMEDIATE_OR_CANCEL == op->tif()) {
                        m_eventq.emplace(SimEvent::Ordering{ts + m_cxl_latency, 1}, SimEvent(OrderState::CANCELLED, op, idx, remains, 0, ts));
                        return;
                    }
                }
            }
        }

        if (0 != remains) {
            // we should be non-MKT, non-IOC here
            if (OrderType::MARKET == op->type() || (OrderType::LIMIT == op->type() && TimeInForce::IMMEDIATE_OR_CANCEL == op->tif())) {
                goto cancel;
            }
            assert(OrderType::MARKET != op->type() && !(OrderType::LIMIT == op->type() && TimeInForce::IMMEDIATE_OR_CANCEL == op->tif()));
            // means we have shares remaining ... maybe we post it
            // since this is an L1 sim, market orders are treated like IOC

            // if we haven't had any fills, then just send an ACK
            if (remains == op->size()) {
                m_eventq.emplace(ts + m_ack_latency, SimEvent{OrderState::ACKNOWLEDGED, op, idx});
            }

            Size size_ahead = 0;
            Size size_behind = 0;
            if (Side::BUY == op->side()) {
                auto qq = be.book_p->l2_bid(MD::to_fixed(op->price()));
                size_ahead += qq.size;
                be.bids.emplace(op->price(), SimOrder{op, size_ahead, size_behind, remains});
            } else {
                auto qq = be.book_p->l2_ask(MD::to_fixed(op->price()));
                size_ahead += qq.size;
                be.asks.emplace(op->price(), SimOrder{op, size_ahead, size_behind, remains});
            }
        }

        return;
    }
cancel:
    m_eventq.emplace(ts+m_cxl_latency, SimEvent(OrderState::CANCELLED, op, idx, remains, 0, ts));
}

void L2SimSession::on_order_acknowledged(const Timestamp &ts, const SimEvent &se)
{
    assert(se.order_p->state() == OrderState::NEW_AND_UNSENT
           || se.order_p->state() == OrderState::SENT
           || se.order_p->state() == OrderState::PENDING_CANCEL);
    m_order_manager_p->on_acknowledged(se.order_p, se.order_p->localID(), se.order_p->size(), ts, ts.nanoseconds_since_midnight());
}

void L2SimSession::on_order_filled(const Timestamp &ts, const SimEvent &se)
{
    // dont fill un-acked orders
    assert(se.order_p->state() >= OrderState::ACKNOWLEDGED);
    // make sure this happens at same time or after ack
    assert(se.order_p->state() != OrderState::ACKNOWLEDGED || ts >= se.order_p->last_response_time());
    m_order_manager_p->on_fill(se.order_p, se.size, se.price, ts, ts.nanoseconds_since_midnight());

    // remove this order from our records
    if (!se.order_p->open_size()) {
        auto it = m_orders.find(se.order_p->localID());
        if (it == m_orders.end()) {
            // this is strange ... have no record of this order
            std::cerr << "ERR,L2SIM," << market() << "," << name() << ",OOF,NO_ORDER," << *se.order_p << std::endl;
        } else {
            m_orders.erase(it);
        }
    }
}



void L2SimSession::on_order_pending_cancel(const Timestamp &ts, const SimEvent &se)
{
    auto it = m_orders.find(se.order_p->localID());
    if (it == m_orders.end()) {
        m_eventq.emplace(ts+m_cxl_latency, SimEvent(OrderState::CANCEL_REJECTED, se.order_p));
        return;
    }

    switch (se.order_p->state()) {
    case OrderState::SENT:
    case OrderState::ACKNOWLEDGED:
    case OrderState::PARTIALLY_FILLED:
    case OrderState::CANCEL_REPLACED:
    case OrderState::PENDING_CANCEL:
    case OrderState::PENDING_CANCEL_REPLACE:
        break;
    case OrderState::UNKNOWN:
    case OrderState::NEW_AND_UNSENT:
    case OrderState::LOCALLY_REJECTED:
    case OrderState::FILLED:
    case OrderState::PARTIALLY_CANCELLED:
    case OrderState::CANCELLED:
    case OrderState::REMOTELY_REJECTED:
    case OrderState::CANCEL_REJECTED:
    default:
        m_eventq.emplace(ts+m_cxl_latency, SimEvent(OrderState::CANCEL_REJECTED, se.order_p));
        return;
    }

    auto * op = it->second;

    Size amt_cancelled = 0;
    if (Side::BUY == op->side()) {
        amt_cancelled = reduce_helper(m_books[se.esi].bids, op->price(), op, se.size);
    } else {
        amt_cancelled = reduce_helper(m_books[se.esi].asks, op->price(), op, se.size);
    }
    if (!amt_cancelled) {
        // this could happen if the order is filled
        m_eventq.emplace(ts+m_cxl_latency, SimEvent(OrderState::CANCEL_REJECTED, se.order_p));
    } else {
        m_eventq.emplace(ts+m_cxl_latency, SimEvent(OrderState::CANCELLED, se.order_p, se.esi, amt_cancelled));
    }
}

void L2SimSession::on_order_cancel_rejected(const Timestamp &ts, const SimEvent &se)
{
    m_order_manager_p->on_cancel_rejected(se.order_p, ts, ts.nanoseconds_since_midnight());
}

void L2SimSession::on_order_cancelled(const Timestamp &ts, const SimEvent &se)
{
    assert(se.order_p->state() == OrderState::ACKNOWLEDGED
           || se.order_p->state() == OrderState::SENT
           || se.order_p->state() == OrderState::PARTIALLY_FILLED
           || se.order_p->state() == OrderState::PENDING_CANCEL);
    m_order_manager_p->on_cancel(se.order_p, se.size, ts, ts.nanoseconds_since_midnight());

    assert(se.size <= se.order_p->size());

    auto remains = se.order_p->size() - se.size;

    if (0 == remains) {
        auto it = m_orders.find(se.order_p->localID());
        assert(it != m_orders.end());
        m_orders.erase(it);
    }
}

void L2SimSession::reduce_size_from_back(SimOrder &o, Size delta_size)
{
    auto amt = std::min(o.size_behind, delta_size);
    o.size_behind -= amt;
    delta_size -= amt;
    if (0 == o.size_behind && delta_size) {
        amt = std::min(o.size_ahead, delta_size);
        o.size_ahead -= amt;
        // if size ahead is 0, then shouldn't be any delta size left
        assert(o.size_ahead || (delta_size == amt));
    }
}

Size L2SimSession::reduce_size_from_front(SimOrder &o, Size delta_size)
{
    auto amt = std::min(o.size_ahead, delta_size);
    o.size_ahead -= amt;
    return amt;
}


std::string L2SimSession::status() const
{
    std::ostringstream os;
    os << m_ts;
    return os.str();
}

void L2SimSession::handle_nyse_cross(BookEntry *be, const MD::TradeEvent& evt)
{
    namespace xdp = MD::ARCA::XDP;
    auto idx = be->book_p->symbol_index();
    const auto& msg = *reinterpret_cast<const xdp::Messages::Trade *>(evt.m_msg);
    auto trade_price = MD::to_double(evt.m_price);
    auto avail_size = AvailSize{static_cast<Size>(evt.m_size)};
    if (xdp::Types::TradeCondition2::CLOSING_TRADE == msg.trade_condition2) {
        cross_helper(be->close_bids, idx, PriceGE{trade_price}, avail_size, evt.m_timestamp, true);
        cross_helper(be->close_asks, idx, PriceLE{trade_price}, avail_size, evt.m_timestamp, true);
    } else if (xdp::Types::TradeCondition2::OPENING_TRADE == msg.trade_condition2) {
        cross_helper(be->close_bids, idx, PriceGE{trade_price}, avail_size, evt.m_timestamp, true);
        cross_helper(be->close_asks, idx, PriceLE{trade_price}, avail_size, evt.m_timestamp, true);

    }
}

void L2SimSession::handle_nasdaq_cross(BookEntry *be, const MD::TradeEvent& evt)
{
    auto idx = be->book_p->symbol_index();
    const auto& msg = *reinterpret_cast<const MD::NASDAQ::ITCH50::Messages::TradeCross *>(evt.m_msg);
    auto trade_price = MD::to_double(evt.m_price);

    auto bid_comp = NasdaqCrossPriceGE{trade_price, m_last_noii[idx].direction()};
    auto ask_comp = NasdaqCrossPriceLE{trade_price, m_last_noii[idx].direction()};
    auto avail_size = NasdaqCrossAvailSize{static_cast<Size>(evt.m_size),
                                           m_last_noii[idx].imbalance_shares()};

    if (msg.cross_type == decltype(msg.cross_type)::NASDAQ_CLOSING_CROSS) {
        cross_helper(be->close_bids, idx, bid_comp, avail_size, evt.m_timestamp, true);
        cross_helper(be->close_asks, idx, ask_comp, avail_size, evt.m_timestamp, true);
    } else if (msg.cross_type == decltype(msg.cross_type)::NASDAQ_OPENING_CROSS) {
        cross_helper(be->open_bids, idx, bid_comp, avail_size, evt.m_timestamp, true);
        cross_helper(be->open_asks, idx, ask_comp, avail_size, evt.m_timestamp, true);
    }

}

std::ostream & operator<<(std::ostream &os, const L2SimSession::SimEvent &se)
{
    return os << se.state << ","
              << se.esi << ","
              << *se.order_p;
}

std::ostream & operator<<(std::ostream &os, const L2SimSession::SimOrder &so)
{
    return os << so.order_p->instrument()->symbol() << ","
              << so.order_p->side() << ","
              << so.order_p->open_size() << ","
              << so.order_p->price() << ","
              << so.remains << ","
              << so.size_ahead << ","
              << so.size_behind;
}


}}
