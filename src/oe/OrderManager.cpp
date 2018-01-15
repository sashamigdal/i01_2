#include <cassert>
#include <cstdlib>
#include <string>

#include <i01_core/Log.hpp>
#include <i01_md/DataManager.hpp>

#include <i01_oe/Blotter.hpp>
#include <i01_oe/FileBlotter.hpp>
#include <i01_oe/FileBlotterReader.hpp>
#include <i01_oe/Instrument.hpp>
#include <i01_oe/OrderLogFormat.hpp>
#include <i01_oe/OrderManager.hpp>
#include <i01_oe/OrderSession.hpp>

namespace i01 { namespace OE {

    OrderManager::OrderManager(MD::DataManager *dm)
        : m_localID(0),
          m_firm_risk(),
          m_portfolio_risk(),
          m_orders({nullptr}), // ... to protect against looking up localID 0.
          m_sessions(),
          m_default_listener_p(nullptr),
          m_mutex(),
          m_dm_p(dm),
          m_universe(),
          m_trading_fees(),
          m_last_sale(),
          m_blotter_p(new FileBlotter(*this)),
          m_blotter_reader_p(new FileBlotterReader())
    {
        assert(m_localID == m_orders.size() - 1);
        m_blotter_reader_p->register_listeners(this);

        // need to do this somewhere other than start() since that may not always be called
        m_blotter_p->log_start();
    }

    OrderManager::~OrderManager()
    {
        for (auto s : m_sessions) {
            s.second->disconnect(true);
        }

        if (m_blotter_p)
            delete m_blotter_p;
    }

    void OrderManager::init(const core::Config::storage_type &cfg)
    {
        auto oecfg(cfg.copy_prefix_domain("oe."));
        auto oeunivcfg(oecfg->copy_prefix_domain("universe."));

        auto mdcfg(cfg.copy_prefix_domain("md.universe."));

        m_universe.init(*mdcfg, *oeunivcfg);

        // set up the firm risk
        auto frd(oecfg->copy_prefix_domain("risk.firm."));
        m_firm_risk.init(*frd);

        if (m_dm_p) {
            m_dm_p->register_last_sale_listener(this);
            m_dm_p->register_timer(this, nullptr);
        }
    }

    void OrderManager::start(const bool replay)
    {
        if (replay) {
            OrderManagerMutex::scoped_lock lock(m_mutex);

            m_blotter_reader_p->replay();
        }

        for (auto it = m_sessions.begin()
                ; it != m_sessions.end()
                ; ++it) {
            if (it->second)
                it->second->connect(replay);
        }
    }

    bool OrderManager::send( Order* order_p
                             , OrderSession* session_p)
    {
        if (order_p == nullptr || session_p == nullptr) {
#ifdef I01_DEBUG_MESSAGING
            std::cerr << "ERROR: OrderManager::send called without order or session pointers." << std::endl;
#endif
            return false;
        }

        Order::OrderMutex::scoped_lock lock(order_p->mutex());
        assert(order_p->market() == session_p->market());

        {
            OrderManagerMutex::scoped_lock mlock(m_mutex);
            order_p->localID(++m_localID);
            m_orders.push_back(order_p);
            assert(m_localID == m_orders.size() - 1);
        }

        EquityInstrument::mutex_type::scoped_lock instlock(order_p->instrument()->mutex());
        if (order_p->validate_base() && order_p->validate()) {
            // First, do system-wide risk checks:
            if ( m_firm_risk.new_order(order_p)
               && m_portfolio_risk.new_order(order_p)
               && (session_p->risk().new_order(order_p))
               && (order_p->instrument()->validate(order_p))
                ) {
                order_p->session(session_p);

                m_firm_risk.on_order_adds(order_p, order_p->size());

                if (session_p->send(order_p)) {
                    order_p->state(OrderState::SENT);
                    m_blotter_p->log_new_order(order_p);
                    m_blotter_p->log_order_sent(order_p);
                    return true;
                } else {
                    m_firm_risk.on_order_removes(order_p, order_p->size());
                    std::cerr << "OrderManager: send fail for " << *order_p << std::endl;
                }
            }
        }
        order_p->state(OrderState::LOCALLY_REJECTED);
        m_blotter_p->log_local_reject(order_p);
        return false;
    }

    bool OrderManager::cancel(const Order* order_p, Size newqty)
    {
        if (UNLIKELY(order_p == nullptr))
            return false;

        auto* nonconst_order_p = const_cast<Order*>(order_p);
        if (UNLIKELY(nonconst_order_p == nullptr))
            return false;

        Order::OrderMutex::scoped_lock lock(nonconst_order_p->mutex());
        if (UNLIKELY(nonconst_order_p->session() == nullptr))
            return false;

        if (UNLIKELY(nonconst_order_p->is_terminal())) {
            return false;
        }

        // if already requested a cancel/reduce to <= newqty:
        if (UNLIKELY((nonconst_order_p->state() == OE::OrderState::PENDING_CANCEL) && (nonconst_order_p->m_pendingcancel_newqty <= newqty)))
            return false;

        EquityInstrument::mutex_type::scoped_lock instlock(order_p->instrument()->mutex());
        if (nonconst_order_p->session()->cancel(nonconst_order_p, newqty)) {
            nonconst_order_p->m_pendingcancel_newqty = newqty;
            nonconst_order_p->state(OrderState::PENDING_CANCEL);
            m_blotter_p->log_pending_cancel(order_p, newqty);
            return true;
        }

        return false;
    }

    bool OrderManager::destroy(Order* order_p, bool only_if_terminal)
    {
        if (UNLIKELY(order_p == nullptr)) {
            std::cerr << "OrderManager::destroy called without an order." << std::endl;
            return false;
        }

        Order::OrderMutex::scoped_lock lock(order_p->mutex());

        // TODO: consider allowing destruction of NEW_AND_UNSENT orders too.
        if (UNLIKELY(only_if_terminal && !(order_p->is_terminal())))
            return false;

        if (UNLIKELY(order_p->localID() == 0)) {
            goto exterminate;
        }

        if (order_p->session() == nullptr) {
            goto forget;
        }

        if (order_p->session()->destroy(order_p)) {
            goto forget;
        }

        std::cerr << "OrderManager::destroy failed on order " << *order_p << std::endl;
        return false;

forget:
        m_orders[order_p->localID()] = nullptr;
exterminate:
        m_blotter_p->log_destroy(order_p);
        delete order_p;
        return true;
    }

void OrderManager::order_update_on_ack(Order *order_p, const ExchangeID exchangeID,
                                       const Size size, const Timestamp& timestamp,
                                       const ExchangeTimestamp& exchange_timestamp)
{
    order_p->exchangeID(exchangeID);
    order_p->open_size(size);
    order_p->state(OrderState::ACKNOWLEDGED);
    order_p->last_response_time(timestamp);
    order_p->last_exchange_time(exchange_timestamp);
}

    void OrderManager::on_acknowledged(Order* order_p,
                                       const ExchangeID exchangeID,
                                       const Size size,
                                       const Timestamp& timestamp,
                                       const ExchangeTimestamp& exchange_timestamp)
    {
        if (UNLIKELY(!order_p)) {
            std::cerr << "OrderManager::on_acknowledged called without an order." << std::endl;
            return;
        }
        Order::OrderMutex::scoped_lock lock(order_p->mutex());
        order_update_on_ack(order_p, exchangeID, size, timestamp, exchange_timestamp);
        m_blotter_p->log_acknowledged(order_p);

        if (auto* l = order_p->listener_or_default(m_default_listener_p))
            l->on_order_acknowledged(order_p);
    }

void OrderManager::order_update_on_reject(Order *order_p, const Timestamp& timestamp,
                                          const ExchangeTimestamp& exchange_timestamp)
{
    order_p->open_size(0);
    order_p->state(OrderState::REMOTELY_REJECTED);
    order_p->last_response_time(timestamp);
    order_p->last_exchange_time(exchange_timestamp);
}

    void OrderManager::on_rejected(Order* order_p,
                                   const Timestamp& timestamp,
                                   const ExchangeTimestamp& exchange_timestamp)
    {
        if (UNLIKELY(!order_p)) {
            std::cerr << "OrderManager::on_rejected called without an order." << std::endl;
            return;
        }

        Order::OrderMutex::scoped_lock lock(order_p->mutex());
        order_update_on_reject(order_p, timestamp, exchange_timestamp);
        EquityInstrument::mutex_type::scoped_lock instlock(order_p->instrument()->mutex());
        m_firm_risk.on_order_removes(order_p, order_p->size());
        m_blotter_p->log_rejected(order_p);

        if (auto* l = order_p->listener_or_default(m_default_listener_p))
            l->on_order_rejected(order_p);
    }

void OrderManager::order_update_on_fill(Order *order_p, const Size fillSize,
                                        const Price fillPrice, const Timestamp& timestamp,
                                        const ExchangeTimestamp& exchange_timestamp,
                                        const FillFeeCode fee_code)
{
    // TODO check for overfills here? (fillSize + filled > size)
    if (fillSize + order_p->filled_size() == order_p->size())
        order_p->state(OrderState::FILLED);
    else
        order_p->state(OrderState::PARTIALLY_FILLED);
    order_p->filled_size(order_p->filled_size() + fillSize);
    order_p->open_size(order_p->open_size() - fillSize);
    order_p->total_value_paid(order_p->total_value_paid() + (fillPrice * fillSize));
    order_p->last_response_time(timestamp);
    order_p->last_exchange_time(exchange_timestamp);
    order_p->last_fill_fee_code(fee_code);
}

void OrderManager::on_fill(Order* order_p,
                           const Size fillSize,
                           const Price fillPrice,
                           const Timestamp& timestamp,
                           const ExchangeTimestamp& exchange_timestamp,
                           const FillFeeCode fee_code)
    {
        if (UNLIKELY(!order_p)) {
            std::cerr << "OrderManager::on_fill called without an order." << std::endl;
            return;
        }

        Order::OrderMutex::scoped_lock lock(order_p->mutex());
        order_update_on_fill(order_p, fillSize, fillPrice, timestamp, exchange_timestamp, fee_code);

        auto fee = m_trading_fees.fee_from_fill(order_p, fillSize, fillPrice);

        // TODO FIXME: this assumes all Instruments are EquityInstruments.
        // this must run before the position is updated.
        auto* ei = static_cast<EquityInstrument*>(order_p->instrument());
        EquityInstrument::mutex_type::scoped_lock instlock(order_p->instrument()->mutex());
        if (   ei
            && order_p->side() == Side::BUY
            && ei->position().quantity() + ei->position().remote_quantity() < 0
            && (!(ei->position().m_norecycle))
            && (ei->position().m_locates_remaining != Position::LOCATES_REMAINING_SENTINEL) // if not configured, don't do anything.
            )
        {
            auto& pos = ei->position();
            auto lsz = std::min((Quantity)fillSize, -(pos.quantity() + pos.remote_quantity())); // TODO FIXME: this needs to change once we do distributed locates.
            pos.m_locates_remaining += lsz;
            pos.m_locates_used -= lsz;
            if (UNLIKELY(pos.m_locates_remaining > ei->locate_size())) {
                std::cerr << "OrderManager::on_fill added back more locates than we started with." << std::endl;
            }
        }

        // portfolio will take care of updating the instrument position
        m_firm_risk.on_order_fill(order_p, fillSize, fillPrice, fee);

        m_blotter_p->log_filled(order_p, fillSize, fillPrice, timestamp, fee_code);

        if (auto* l = order_p->listener_or_default(m_default_listener_p))
            l->on_order_fill(order_p, fillSize, fillPrice, fee);
    }

    void OrderManager::on_cancel_rejected(Order* order_p,
                                          const Timestamp& timestamp,
                                          const ExchangeTimestamp& exchange_timestamp)
    {
        if (order_p)
        {
            Order::OrderMutex::scoped_lock lock(order_p->mutex());
            order_p->state(OrderState::CANCEL_REJECTED);
            order_p->m_pendingcancel_newqty = std::numeric_limits<decltype(order_p->m_pendingcancel_newqty)>::max();
            order_p->last_response_time(timestamp);
            order_p->last_exchange_time(exchange_timestamp);
            m_blotter_p->log_cancel_rejected(order_p);

            if (auto* l = order_p->listener_or_default(m_default_listener_p))
                l->on_order_cancel_rejected(order_p);
        }
    }

void OrderManager::order_update_on_cancel(Order *order_p, const Size cancelSize,
                                          const Timestamp& timestamp,
                                          const ExchangeTimestamp& exchange_timestamp)
{
    if (cancelSize >= order_p->open_size())
    {
        order_p->state(OrderState::CANCELLED);
        order_p->cancelled_size(order_p->cancelled_size() + cancelSize);
        order_p->open_size(0);
    }
    else
    {
        order_p->state(OrderState::PARTIALLY_CANCELLED);
        order_p->cancelled_size(order_p->cancelled_size() + cancelSize);
        order_p->open_size(order_p->open_size() - cancelSize);
    }
    order_p->last_response_time(timestamp);
    order_p->last_exchange_time(exchange_timestamp);
}

    void OrderManager::on_cancel(Order* order_p,
                                 const Size cancelSize,
                                 const Timestamp& timestamp,
                                 const ExchangeTimestamp& exchange_timestamp)
    {
        if (order_p)
        {
            Order::OrderMutex::scoped_lock lock(order_p->mutex());
            order_update_on_cancel(order_p, cancelSize, timestamp, exchange_timestamp);
            auto* ei = static_cast<EquityInstrument*>(order_p->instrument());
            EquityInstrument::mutex_type::scoped_lock instlock(order_p->instrument()->mutex());
            if (   ei
                && ei->position().quantity() + ei->position().remote_quantity() < 0
                && (order_p->side() == Side::SHORT || order_p->side() == Side::SHORT_EXEMPT || order_p->side() == Side::SELL)
                && (ei->position().m_locates_remaining != Position::LOCATES_REMAINING_SENTINEL)
                )
            {
                auto& pos = ei->position();
                pos.m_locates_remaining += cancelSize;
                pos.m_locates_used -= cancelSize;
                if (UNLIKELY(pos.m_locates_remaining > ei->locate_size())) {
                    std::cerr << "OrderManager::on_cancel added back more locates than we started with." << std::endl;
                }
            }
            m_firm_risk.on_order_removes(order_p, cancelSize);
            m_blotter_p->log_cancelled(order_p, cancelSize);

            if (auto* l = order_p->listener_or_default(m_default_listener_p))
                l->on_order_cancel(order_p, cancelSize);
        }
    }

    void OrderManager::on_position_update(const EngineID source, EquityInstrument* ins, Quantity qty, Price avg_px, Price mark_px)
    {
        EquityInstrument::mutex_type::scoped_lock instlock(ins->mutex());
        if ((source != 0) && (ins != nullptr))
        {
            m_blotter_p->log_position(source, ins, qty, avg_px, mark_px);
        } else {
            std::cerr << "OrderManager::on_position_update called with invalid source or instrument." << std::endl;
        }
    }

    std::pair<OrderSessionPtr, bool> OrderManager::add_session(const std::string& session_name, const std::string& session_type)
    {
        // NOTE: Session names are limited to 8 characters here due to
        //       the order log format NewOrderBody session_name size.
        if (session_name.empty() || session_type.empty() || (session_name.length() > sizeof(OrderLogFormat::SessionName)))
            return std::make_pair(nullptr, false);
        auto it = m_sessions.find(session_name);
        if (it != m_sessions.end()) {
            std::cerr << "OrderManager::add_session called on preexisting session " << session_name << std::endl;
            return std::make_pair(it->second, false);
        }

        OrderSessionPtr osp(OrderSession::factory(this, session_name, session_type));
        if (osp) {
            m_sessions.emplace(std::make_pair(session_name, osp));
            m_blotter_p->log_add_session(osp.get());
            return std::make_pair(osp, true);
        } else {
            std::cerr << "OrderManager::add_session failed to create session " << session_name << std::endl;
            return std::make_pair(osp, false);
        }
    }

    auto OrderManager::get_session(const std::string &session_name) const -> OrderSessionPtr
    {
        auto it = m_sessions.find(session_name);
        if (it == m_sessions.end()) {
            return nullptr;
        }
        return it->second;
    }

auto OrderManager::get_sessions(const core::MIC &m) const -> std::vector<OrderSessionPtr>
{
    std::vector<OrderSessionPtr> ret;
    for (const auto &s : m_sessions) {
        if (m == core::MICEnum::UNKNOWN || m == s.second->market()) {
            ret.emplace_back(s.second);
        }
    }
    return ret;
}

void OrderManager::on_last_sale_change(const MD::EphemeralSymbolIndex esi, const MD::LastSale &ls)
{
    m_last_sale[esi] = ls;

    if (m_universe[esi].data()) {
        EquityInstrument::mutex_type::scoped_lock instlock(m_universe[esi].data()->mutex());
        // FIXME: FirmRiskCheck immediately locks the same lock
        m_firm_risk.on_last_sale_change(universe()[esi].data(), ls);
    } else {
        std::cerr << "OrderManager::on_last_sale_change invalid ESI " << esi << std::endl;
    }
}

size_t OrderManager::find_and_cancel(std::function<bool(OE::Order *)> pred_func)
{
    size_t count=0;
    for (auto* o : m_orders) {
        if (o != nullptr && !o->is_terminal() && pred_func(o)) {
            if (cancel(o)) {
                count++;
            }
        }
    }
    return count;
}

size_t OrderManager::cancel_all()
{
    return find_and_cancel([](OE::Order*) { return true; });
}

size_t OrderManager::cancel_all_local_account(LocalAccount la)
{
    return find_and_cancel([la](OE::Order *o) { return la == o->local_account();});
}

size_t OrderManager::cancel_all(OrderSessionPtr osp)
{
    if (!osp)
        return 0;
    ssize_t cxled = osp->cancel_all();
    if (cxled >= 0)
        return cxled;
    else
        return find_and_cancel([osp](OE::Order *o) { return osp.get() == o->session();});
}

size_t OrderManager::cancel_all(OrderListener *olp)
{
    return find_and_cancel([olp](OE::Order *o) { return olp == o->listener();});
}

void OrderManager::on_log_start(const Timestamp& ts)
{
#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OrderManager::on_log_start called." << std::endl;
#endif
}

void OrderManager::on_log_end(const Timestamp& ts)
{
#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OrderManager::on_log_end called." << std::endl;
#endif
}

bool OrderManager::extend_local_id_helper(const std::string& func_name,
                                          const OrderLogFormat::OrderIdentifier& oid,
                                          bool expect_new)
{
    if (oid.local_id <= m_localID) {
        if (expect_new) {
            std::cerr << "OrderManager::" << func_name << ": replay localID " << oid.local_id << " overlaps existing localID " << m_localID << " for sent order: " << oid << std::endl;
        }
        // did not extend
        return false;
    } else {
        extend_local_id(oid.local_id);
    }
    return true;
}

void OrderManager::extend_local_id(LocalID lid)
{
    assert(m_orders.size() == m_localID+1);
    if (lid+1 > m_orders.size()) {
        // grow the vector with nullptrs ... we will fill them in if necessary...
        m_orders.resize(lid+1, nullptr);
        m_localID = lid;
    }
}

void OrderManager::adopt_order_helper(Order *op, LocalID local_id, bool do_log)
{
    assert(local_id < m_orders.size());
    // TODO: we should log this as a new order here
    if (op) {
        op->localID(local_id);

        m_firm_risk.on_order_adds(op, op->size());
        op->state(OrderState::SENT);
        if (do_log) {
            m_blotter_p->log_new_order(op);
            m_blotter_p->log_order_sent(op);
        }
    }
    m_orders[local_id] = op;
}


bool OrderManager::adopt_orphan_order(const std::string& session_name, LocalID local_id,
                                      Order *op)
{
    OrderManagerMutex::scoped_lock lock(m_mutex);

    // valid for op to be nullptr ... this will just increment m_localID
    auto oid = OrderLogFormat::OrderIdentifier{0, local_id, OrderLogFormat::string_to_session_name(session_name)};

    // we may or may not be creating a new order here...
    extend_local_id_helper("adopt_orphan_order", oid, false);

    adopt_order_helper(op, local_id, true);
    return true;
}


void OrderManager::on_log_order_sent(const Timestamp& ts, const OrderLogFormat::OrderSentBody& b)
{
    Order *op = nullptr;
    // find the instrument for this order...
    auto& inst = m_universe[b.order.instrument_esi];

    auto it = m_sessions.find(b.order.oid.session_name.to_string());

    if (inst.data() == nullptr) {
        // we don't have this ESI as a valid instrument...
        std::cerr << "OrderManager::on_log_order_sent: could not find valid instrument for ESI in order: " << b << std::endl;
        goto update_local_id;
    }

    if (it == m_sessions.end()) {
        std::cerr << "OrderManager::on_log_order_sent: could not find session for replay order: " << b << std::endl;
    } else {
        // ask the session to create a stub for us

        // if we are not ignoring this session on replay, then ask the session to create the order for us...
        op = it->second->on_create_replay_order(ts, inst.data(), b.order.market_mic, b.order.oid.local_account,
                                                b.order.oid.local_id, b.order.orig_price,
                                                b.order.size, b.order.side, b.order.tif,
                                                b.order.type, b.order.user_data);
    }

    if (!extend_local_id_helper("on_log_order_sent", b.order.oid, true)) {
        // expected to extend local id but did not
        std::cerr << "OrderManager::on_log_order_sent: expected to create a new local_id for order but did not, so ignoring: " << b << ", current m_localID: " << m_localID << std::endl;
        return;
    }

    update_local_id:
    adopt_order_helper(op, b.order.oid.local_id, false);
}


void OrderManager::on_log_local_reject(const Timestamp& ts, const OrderLogFormat::OrderLocalRejectBody& b)
{
    // just update local_ID and m_orders
    extend_local_id_helper("on_log_local_reject", b.order.oid,true);
}

void OrderManager::on_log_pending_cancel(const Timestamp& ts, const OrderLogFormat::OrderCxlReqBody& b)
{
    extend_local_id_helper("on_log_pending_cancel", b.oid, false);
    auto op = m_orders[b.oid.local_id];
    if (op) {
        op->state(OrderState::PENDING_CANCEL);
    }
}

void OrderManager::on_log_destroy(const Timestamp& ts, const OrderLogFormat::OrderDestroyBody& b)
{
    extend_local_id_helper("on_log_destroy", b.oid, false);
    auto op = m_orders[b.oid.local_id];
    if (op) {
        m_orders[b.oid.local_id] = nullptr;
        delete op;
    }
}

void OrderManager::on_log_acknowledged(const Timestamp& ts, const OrderLogFormat::OrderAckBody& b)
{
    extend_local_id_helper("on_log_acknowledged", b.oid, false);
    auto op = m_orders[b.oid.local_id];
    if (op) {
        order_update_on_ack(op, ExchangeID{}, b.open_size, ts, b.exchange_ts);
    }
}

void OrderManager::on_log_rejected(const Timestamp& ts, const OrderLogFormat::OrderRejectBody& b)
{
    extend_local_id_helper("on_log_rejected", b.oid, false);
    auto op = m_orders[b.oid.local_id];
    if (op) {
        order_update_on_reject(op, ts, b.exchange_ts);
        m_firm_risk.on_order_removes(op, op->size());
    }
}

void OrderManager::on_log_filled(const Timestamp& ts, bool partial_fill, const OrderLogFormat::OrderFillBody& b)
{
    extend_local_id_helper("on_log_filled", b.oid, false);
    auto op = m_orders[b.oid.local_id];
    if (op) {
        order_update_on_fill(op, b.filled_qty, b.filled_price, ts, b.exchange_ts, b.filled_fee_code);

        auto fee = m_trading_fees.fee_from_fill(op, b.filled_qty, b.filled_price);

        // portfolio will take care of updating the instrument position
        m_firm_risk.on_order_fill(op, b.filled_qty, b.filled_price, fee);
    }
}

void OrderManager::on_log_cancel_rejected(const Timestamp& ts, const OrderLogFormat::OrderCxlRejectBody& b)
{
    extend_local_id_helper("on_log_cancel_rejected", b.oid, false);
    auto op = m_orders[b.oid.local_id];
    if (op) {
        op->state(OrderState::CANCEL_REJECTED);
        op->last_response_time(ts);
    }
}

void OrderManager::on_log_cancelled(const Timestamp& ts, const OrderLogFormat::OrderCxlBody& b)
{
    extend_local_id_helper("on_log_cancelled", b.oid, false);
    auto op = m_orders[b.oid.local_id];
    if (op) {
        order_update_on_cancel(op, b.cxled_qty, ts, b.exchange_ts);
        m_firm_risk.on_order_removes(op, b.cxled_qty);
    }
}

void OrderManager::on_log_add_session(const Timestamp& ts, const OrderLogFormat::NewSessionBody& b)
{
    // TODO: check if there is a session with the same name, and output an
    // error if we are missing the session.
}

void OrderManager::on_log_add_strategy(const Timestamp& ts, const OrderLogFormat::NewAccountBody& b)
{
    // TODO: check if there is a strategy with the same name, and output an
    // error if we are missing the session.
}

void OrderManager::manual_position_adj(const Timestamp& ts, const MD::EphemeralSymbolIndex esi, const int64_t delta_start_qty, const int64_t delta_trading_qty)
{
    if (UNLIKELY(esi == 0)) {
        std::cerr 
            << "OrderManager::manual_position_adj invalid ESI."
            << std::endl;
        return;
    }
    OrderManagerMutex::scoped_lock lock(m_mutex);
    auto* inst = m_universe[esi].data();
    if (UNLIKELY(!inst)) {
        std::cerr << "OrderManager::manual_position_adj invalid inst pointer." << std::endl;
        return;
    }
    Instrument::mutex_type::scoped_lock instlock(inst->mutex());
    auto& pos = m_universe[esi].data()->m_position;
    auto prevspos = pos.start_quantity();
    auto prevpos = pos.trading_quantity();
    pos.m_start_quantity += delta_start_qty;
    pos.m_trading_quantity += delta_trading_qty;
    std::cerr << "OrderManager::manual_position_adj " << esi << " " << inst->symbol() << " position manually adjusted from " << prevspos << ":" << prevpos<< " to " << pos.m_start_quantity << ":" << pos.m_trading_quantity << " at time " << ts << std::endl;
}

void OrderManager::on_log_manual_position_adj(const Timestamp& ts, const OrderLogFormat::ManualPositionAdjBody& b)
{
    manual_position_adj(ts, b.esi, b.delta_start_qty, b.delta_trading_qty);
}

void OrderManager::load_and_update_locates(std::string locates_update_file)
{
    if (locates_update_file.empty()) {
        auto cfg = core::Config::instance().get_shared_state();
        locates_update_file = cfg->get_or_default<std::string>("oe.locates_update_file", "");
    }
    if (!(locates_update_file.empty())) {
        core::Config::instance().load_lua_file(locates_update_file);
        auto ucfg = core::Config::instance().get_shared_state();
        auto eiucfg(ucfg->copy_prefix_domain("oe.universe.eqinst."));
        for (auto& ei : m_universe) {
            if (!ei.data())
                continue;
            Quantity locqty = eiucfg->get_or_default<Quantity>(
                    ei.cta_symbol() + ".locate_size"
                  , Position::LOCATES_REMAINING_SENTINEL
                );
            ei.data()->update_locates_live(locqty, true);
        }
    }

}

void OrderManager::on_timer(const Timestamp& ts, void* userdata, std::uint64_t iter)
{
    // Check if locates have changed in the Config once every 5 minutes...
    if ((ts.seconds_since_midnight() % 300) == 0) {
        load_and_update_locates("");
    }
}

std::string OrderManager::status() const
{
    std::ostringstream ss;
    ss << core::Timestamp::now() << ","
       << m_firm_risk << ","
       << "localID," << m_localID;
    for (const auto& s : m_sessions) {
        ss << "\n" << s.first << ","
           << s.second->market() << ","
           << std::string(s.second->active() ? "ACTIVE" : "INACTIVE") << ","
           << s.second->status();
    }
    return ss.str();
}

OrderManager::OrderPtrContainer OrderManager::open_orders() const
{
    auto f = [](const Order* p) { return !p->is_terminal(); };
    OrderManagerMutex::scoped_lock lock(m_mutex);
    return unsafe_order_filter(f);
}

OrderManager::OrderPtrContainer OrderManager::all_orders() const
{
    OrderManagerMutex::scoped_lock lock(m_mutex);
    return unsafe_order_filter([](const Order*) {return true;});
}

OrderManager::OrderPtrContainer OrderManager::unsafe_order_filter(const OrderFilterFunc& f) const
{
    OrderPtrContainer res;
    for (const auto& p : m_orders) {
        if (nullptr != p && f(p)) {
            res.push_back(p);
        }
    }
    return res;
}

void OrderManager::enable_stock(MD::EphemeralSymbolIndex esi)
{
    m_firm_risk.user_enable(esi);
}

void OrderManager::disable_stock(MD::EphemeralSymbolIndex esi)
{
    m_firm_risk.user_disable(esi);
}

void OrderManager::enable_all_trading()
{
    m_firm_risk.user_enable();
}

void OrderManager::disable_all_trading()
{
    m_firm_risk.user_disable();
}

FirmRiskCheck::Status OrderManager::get_firm_risk_status() const
{
    return m_firm_risk.status();
}

FirmRiskCheck::InstPermissionBits OrderManager::get_firm_risk_status(MD::EphemeralSymbolIndex esi) const
{
    return m_firm_risk.get_inst_permissions(esi);
}

}}
