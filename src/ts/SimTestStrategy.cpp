#include <i01_md/L2Book.hpp>

#include <i01_oe/OrderManager.hpp>

#include <i01_ts/SimTestStrategy.hpp>

namespace i01 { namespace TS {


SimTestStrategy::SimTestStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string &n) :
    EquitiesStrategy(omp, dmp, n),
    m_inst{nullptr},
    m_out_ask(nullptr),
    m_out_bid(nullptr),
    m_moc_bid(nullptr),
    m_moc_ask(nullptr) {

        m_os_ptr = m_om_p->get_session("UAT3");

        for (auto i = 0U; i < m_mics.size(); i++) {
            m_mics[i] = (i == (unsigned) core::MICEnum::BATS) ? true : false;
        }

        if (m_om_p->universe().size()) {
            m_inst = (*m_om_p->universe().begin()).data();
        }
        if (!m_inst) {
            std::cerr << "SimTestStrategy: no instrument defined" << std::endl;
        }
    }

void SimTestStrategy::add_event(const Timestamp &ts, const Event &evt) {
    m_events.emplace(ts, evt);
}

void SimTestStrategy::on_start_of_data(const MD::PacketEvent &evt)   {
    if (!m_mics[evt.mic.index()]) {
        return;
    }
    m_ts = evt.timestamp;
    // std::cerr << "SOD," << evt << "," << m_ts << std::endl;
}

void SimTestStrategy::on_trading_status_update(const MD::TradingStatusEvent &evt)   {
    if (!m_mics[evt.m_book.mic().index()]) {
        return;
    }

    m_state.update(evt.m_event);
}

void SimTestStrategy::book_added_helper(const MD::FullL2Quote &q, OE::Order*& o, const MD::L3AddEvent &evt) {
    // if (MD::SymbolState::TradingState::TRADING != m_state.trading_state) {
    //     return;
    // }

    if (nullptr == o && m_inst) {
        auto size = q.bid.size;
        auto price = MD::to_double(q.bid.price);
        if (evt.m_order.side == OB::Order::Side::SELL) {
            size = q.ask.size;
            price = MD::to_double(q.ask.price);
        }

        o = m_om_p->create_order<OrderType>(m_inst, price, size,
                                            evt.m_order.side == OB::Order::Side::BUY ? OE::Side::BUY : OE::Side::SELL,
                                            OE::TimeInForce::DAY,
                                            OE::OrderType::LIMIT, this);
        if (!m_om_p->send(o, m_os_ptr.get())) {
            std::cerr << "TST,ERR,SEND_ON_NULL," << *o << std::endl;
            o = nullptr;
        }
    }
}

void SimTestStrategy::on_book_added(const MD::L3AddEvent &evt)   {
    if (!m_mics[evt.m_book.mic().index()]) {
        return;
    }

    MD::FullL2Quote q = evt.m_book.best();
    if (q != m_last_best) {

        std::cerr << "TST,OBA," << evt.m_timestamp << "," << q << "," << evt << std::endl;
        book_added_helper(q, evt.m_order.side == OB::Order::Side::BUY ? m_out_bid : m_out_ask, evt);
        check_for_cancel(evt.m_order.side, q, evt.m_book);
    }
}

void SimTestStrategy::check_for_cancel(OB::Order::Side order_side,
                      const MD::FullL2Quote & q,
                      const BookBase & book) {
    if (OB::Order::Side::BUY == order_side) {
        if (m_out_bid) {
            if (0 == q.bid.size) {
                if (m_out_bid->state() == OrderState::ACKNOWLEDGED
                    || m_out_bid->state() == OrderState::PARTIALLY_FILLED) {
                    m_om_p->cancel(m_out_bid);
                }
                m_last_best = q;
                const auto & b = static_cast<const OB &>(book);
                auto it = b.bids().begin();
                if (++it != b.bids().end()) {
                    m_last_best.bid = it->second.l2quote();
                } else {
                    m_last_best.bid = MD::FullL2Quote().bid;
                }
            } else if (q.bid.price != m_last_best.bid.price) {
                m_last_best = q;
                if (m_out_bid->state() == OrderState::ACKNOWLEDGED
                    || m_out_bid->state() == OrderState::PARTIALLY_FILLED) {
                    m_om_p->cancel(m_out_bid);
                }
            }
        }
    } else {
        if (m_out_ask) {
            if (0 == q.ask.size) {
                if (m_out_ask->state() == OrderState::ACKNOWLEDGED
                    || m_out_ask->state() == OrderState::PARTIALLY_FILLED) {
                    m_om_p->cancel(m_out_ask);
                }
                m_last_best = q;
                const auto & b = static_cast<const OB &>(book);
                auto it = b.asks().begin();
                if (++it != b.asks().end()) {
                    m_last_best.ask = it->second.l2quote();
                } else {
                    m_last_best.ask = MD::FullL2Quote().ask;
                }
            } else if (q.ask.price != m_last_best.ask.price) {
                m_last_best = q;
                if (m_out_ask->state() == OrderState::ACKNOWLEDGED
                    || m_out_ask->state() == OrderState::PARTIALLY_FILLED) {
                    m_om_p->cancel(m_out_ask);
                }
            }
        }
    }
}

void SimTestStrategy::on_book_canceled(const MD::L3CancelEvent &evt)   {
    if (!m_mics[evt.m_book.mic().index()]) {
        return;
    }

    MD::FullL2Quote q = evt.m_book.best();
    if (q != m_last_best) {
        std::cerr << "TST,OBC," << evt.m_timestamp << "," << q << "," << evt << std::endl;
        check_for_cancel(evt.m_order.side, q, evt.m_book);
    }
}

void SimTestStrategy::on_book_modified(const MD::L3ModifyEvent &evt)   {
    if (!m_mics[evt.m_book.mic().index()]) {
        return;
    }
    MD::FullL2Quote q = evt.m_book.best();
    if (q != m_last_best) {
        std::cerr << "TST,OBM," << evt.m_timestamp << "," << q << "," << evt << std::endl;
        check_for_cancel(evt.m_new_order.side, q, evt.m_book);
    }
}

void SimTestStrategy::on_book_executed(const MD::L3ExecutionEvent &evt)   {
    if (!m_mics[evt.m_book.mic().index()]) {
        return;
    }
    MD::FullL2Quote q = evt.m_book.best();
    std::cerr << "TST,OBE," << evt.m_timestamp << "," << q << "," << evt << std::endl;
    if (q != m_last_best) {
        check_for_cancel(evt.m_order.side, q, evt.m_book);
    }
}

void SimTestStrategy::moc_helper(OE::Order *& order_p, OE::Size sz, OE::Side s) {
    OE::Price p = 70.80;
    if (OE::Side::BUY == s) {
        p = 70.80;
    }
    if (!m_inst) {
        return;
    }
    order_p = m_om_p->create_order<OrderType>(m_inst, p, sz,
                                              s,
                                              OE::TimeInForce::AUCTION_CLOSE,
                                              OE::OrderType::LIMIT, this);
    if (!m_om_p->send(order_p, m_os_ptr.get())) {
        std::cerr << "TST,ERR,SEND_MOC," << *order_p << std::endl;
        order_p = nullptr;
    }
}

void SimTestStrategy::l2_update_helper(OE::Order *& order_p, const MD::L2BookEvent &evt) {
    if (MD::SymbolState::TradingState::TRADING != m_state.trading_state) {
        return;
    }

    if (nullptr == order_p && m_inst) {
        OE::Price price = 0.01;
        OE::Size size = 0;
        if (evt.m_is_buy) {
            price = MD::to_double(m_last_best.bid.price);
            size = m_last_best.bid.size;
        } else {
            price = MD::to_double(m_last_best.ask.price);
            size = m_last_best.ask.size;
        }

        order_p = m_om_p->create_order<OrderType>(m_inst,
                                                  price,
                                                  size,
                                                  evt.m_is_buy ? OE::Side::BUY : OE::Side::SELL,
                                                  OE::TimeInForce::DAY,
                                                  OE::OrderType::LIMIT, this);
        if (!m_om_p->send(order_p, m_os_ptr.get())) {
            std::cerr << "TST,ERR,SEND_ON_NULL," << *order_p << std::endl;
            order_p = nullptr;
        }
    } else {
        auto ref_price = evt.m_is_buy ? m_last_best.bid.price : m_last_best.ask.price;
        if (MD::to_fixed(order_p->price()) != ref_price
            && order_p->state() != OrderState::PENDING_CANCEL) {
            // cancel outstanding order
            m_om_p->cancel(order_p);
        }
    }
}

void SimTestStrategy::on_l2_update(const MD::L2BookEvent &evt)   {
    if (!m_mics[evt.m_book.mic().index()]) {
        return;
    }
    // just try and peg the inside quote...
    if (!evt.m_is_bbo) {
        return;
    }

    if (nullptr == m_moc_bid) {
        moc_helper(m_moc_bid, 100, OE::Side::BUY);
    }
    if (nullptr == m_moc_ask) {
        moc_helper(m_moc_ask, 100, OE::Side::SELL);
    }


    m_last_best = evt.m_book.best();
    // need to use the best quote b/c this could be an update with size ==0
    l2_update_helper(evt.m_is_buy ? m_out_bid : m_out_ask, evt);
    if (evt.m_is_bbo) {
        std::cerr << "TST,L2Q,"
                  << evt.m_timestamp << ","
                  << m_last_best << std::endl;
    }
}

void SimTestStrategy::on_trade(const MD::TradeEvent &evt)   {
    if (!m_mics[evt.m_book.mic().index()]) {
        return;
    }
    std::cerr << "TST,TRD," << evt << std::endl;
}

void SimTestStrategy::on_end_of_data(const MD::PacketEvent &evt)   {
    if (!m_mics[evt.mic.index()]) {
        return;
    }
    if (!m_inst) {
        return;
    }
    if (m_events.size()) {
        auto e = m_events.begin();
        while (e != m_events.end() && evt.timestamp > e->first) {
            if (e->second.is_order) {
                auto * op = m_om_p->create_order<OrderType>(m_inst, e->second.price,
                                                            e->second.size, e->second.side,
                                                            e->second.tif,
                                                            e->second.type, this);
                if (op) {
                    if (m_om_p->send(op, m_os_ptr.get())) {

                        add_event(e->first + Timestamp{1,0}, {false, 0, 0, OE::Side::UNKNOWN, OE::TimeInForce::UNKNOWN, OE::OrderType::UNKNOWN, op});
                    } else {
                        std::cerr << "TST,ERR,EOD_SEND," << *op << std::endl;
                    }
                }
            } else {
                // cancel
                m_om_p->cancel(e->second.order_p);
            }
            e = m_events.erase(e);
        }
    }
}

void SimTestStrategy::on_order_acknowledged(const OE::Order *o)  {
    std::cerr << "TST,ACK," << o->last_response_time() << "," << m_ts << "," << *o << std::endl;
}

void SimTestStrategy::on_order_fill(const OE::Order *o, const OE::Size s, const OE::Price p, const OE::Price fee)   {
    std::cerr << "TST,FILL,"
              << o->last_response_time() << ","
              << m_ts << ","
              << s << ","
              << p << ","
              << *o
              << std::endl;

    if (o->state() == OrderState::FILLED) {
        if (OE::Side::BUY == o->side()) {
            assert(o->localID() == m_out_bid->localID() || o->localID() == m_moc_bid->localID());
            m_out_bid = nullptr;
        } else {
            assert(o->localID() == m_out_ask->localID() || o->localID() == m_moc_ask->localID());
            m_out_ask = nullptr;
        }
    }
}


void SimTestStrategy::on_order_cancel(const OE::Order *o, const OE::Size s)  {
    std::cerr << "TST,CXL,"
              << o->last_response_time() << ","
              << m_ts << ","
              << s << ","
              << *o << std::endl;

    if (m_out_ask && o->localID() == m_out_ask->localID()) {
        if (m_last_best.ask.price != std::numeric_limits<MD::Price>::max() && m_inst) {
            m_out_ask = m_om_p->create_order<OrderType>(m_inst, MD::to_double(m_last_best.ask.price), m_last_best.ask.size, OE::Side::SELL, OE::TimeInForce::DAY, OE::OrderType::LIMIT, this);
            if (!m_om_p->send(m_out_ask, m_os_ptr.get())) {
                std::cerr << "TST,ERR,CXL_SEND," << *m_out_ask <<std::endl;
            }
        } else {
            m_out_ask = nullptr;
        }
    } else if (m_out_bid && o->localID() == m_out_bid->localID()) {
        if (m_last_best.bid.price != 0 && m_inst) {
            m_out_bid = m_om_p->create_order<OrderType>(m_inst, MD::to_double(m_last_best.bid.price), m_last_best.bid.size, OE::Side::BUY, OE::TimeInForce::DAY, OE::OrderType::LIMIT, this);
            if (!m_om_p->send(m_out_bid, m_os_ptr.get())) {
                std::cerr << "TST,ERR,CXL_SEND," << *m_out_bid << std::endl;
            }
        } else {
            m_out_bid = nullptr;
        }
    } else {
        assert(true);
    }
}

void SimTestStrategy::on_order_cancel_rejected(const OE::Order *o)  {
    std::cerr << "TST,CXLREJ,"
              << o->last_response_time() << ","
              << m_ts << ","
              << *o
              << std::endl;
}


}}
