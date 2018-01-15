#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <map>
#include <type_traits>

#include <boost/noncopyable.hpp>

#include <i01_core/Config.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/TimerListener.hpp>

#include <i01_md/LastSale.hpp>
#include <i01_md/LastSaleListener.hpp>

#include <i01_oe/BlotterReader.hpp>
#include <i01_oe/BlotterReaderListener.hpp>
#include <i01_oe/EqInstUniverse.hpp>
#include <i01_oe/OrderSessionPtr.hpp>
#include <i01_oe/TradingFees.hpp>
#include <i01_oe/Types.hpp>

#include <i01_oe/Risk/FirmRiskCheck.hpp>
#include <i01_oe/Risk/PortfolioRiskCheck.hpp>
#include <i01_oe/OrderSessionPtr.hpp>
#include <i01_oe/TradingFees.hpp>
#include <i01_oe/Blotter.hpp>

// forward declare the DataManager
namespace i01 { namespace MD {
class DataManager;
}}

namespace i01 { namespace OE {

class Order;
class OrderListener;
class OrderSession;

    class OrderManager
        : public MD::LastSaleListener
        , public BlotterReaderListener
        , public core::TimerListener
        , private boost::noncopyable {
    public:
        using OrderPtrContainer = std::vector<Order *>;
        using OrderFilterFunc = std::function<bool(const Order*)>;
    public:
        OrderManager(MD::DataManager * dm_p = nullptr);
        virtual ~OrderManager();

        void init(const core::Config::storage_type &cfg);

        void start(const bool replay = false);

        OrderListener* default_listener() const { return m_default_listener_p; }
        void default_listener(OrderListener* l) { m_default_listener_p = l; }

        /// Creates an order of type T, e.g. NYSEOrder.
        template <typename T, typename... Args>
        T* create_order(Args&&... args);

        /// Sends an order through the session.
        bool send(Order* order_p, OrderSession* session);

        /// Cancels an order, partially cancels if newqty > 0.
        bool cancel(const Order* order_p, Size newqty = 0);

        /// Removes an order from existence, without sending anything to the
        /// market.  USE WITH EXTREME CAUTION!
        bool destroy(Order* order_p, bool only_if_terminal = true);

        void on_acknowledged(Order* order_p,
                             const ExchangeID exchangeID,
                             const Size size,
                             const Timestamp& timestamp,
                             const ExchangeTimestamp& exchange_timestamp);
        void on_rejected(Order* order_p,
                         const Timestamp& timestamp,
                         const ExchangeTimestamp& exchange_timestamp);
        void on_fill(Order* order_p,
                     const Size fillSize,
                     const Price fillPrice,
                     const Timestamp& timestamp,
                     const ExchangeTimestamp& exchange_timestamp,
                     const FillFeeCode fill_fee_code = FillFeeCode::UNKNOWN);
        // Additional parameters: executionID, liquidityFlag
        void on_cancel_rejected(Order* order_p,
                                const Timestamp& timestamp,
                                const ExchangeTimestamp& exchange_timestamp);
        void on_cancel(Order* order_p,
                       const Size cancelSize,
                       const Timestamp& timestamp,
                       const ExchangeTimestamp& exchange_timestamp);

        /// Update remote/global position.
        void on_position_update(const EngineID source, EquityInstrument* ins, Quantity qty, Price avg_px, Price mark_px);

        /// Adds a session to the session name mapping, or returns one if it
        /// existed already.  The bool returned will be true if a new session
        /// was created, false otherwise.
        std::pair<OrderSessionPtr, bool> add_session(const std::string& session_name, const std::string& session_type);
        /// Return a shared_ptr to an OrderSession, retrieved by name from the
        /// session mapping.  Cache the return value, as this is slow.
        OrderSessionPtr get_session(const std::string& session_name) const;

        /// Return all sessions that have the given MIC.  Also slow.
        std::vector<OrderSessionPtr> get_sessions(const core::MIC &m = core::MICEnum::UNKNOWN) const;

        const EqInstUniverse & universe() const { return m_universe; }

        FirmRiskSnapshot firm_risk() const { return m_firm_risk.snapshot(); }
        FirmRiskLimits firm_risk_limits() const { return m_firm_risk.limits(); }

        virtual void on_last_sale_change(const MD::EphemeralSymbolIndex esi, const MD::LastSale& ls) override final;

        /// Cancel all non-terminal orders
        size_t cancel_all();

        /// Cancel all orders given a local account
        size_t cancel_all_local_account(LocalAccount la);

        /// Cancel all orders for a session
        size_t cancel_all(OrderSessionPtr osp);

        /// Cancel all orders for an OrderListener
        size_t cancel_all(OrderListener* olp);

        /// Cancels all orders that the predicate returns true for.
        /// Returns the number of orders for which a cancel was sent.
        size_t find_and_cancel(std::function<bool(OE::Order *)> pred_func);

        // BlotterReaderListener
        virtual void on_log_start(const Timestamp&) override final;
        virtual void on_log_end(const Timestamp&) override final;
        virtual void on_log_order_sent(const Timestamp&, const OrderLogFormat::OrderSentBody&) override final;
        virtual void on_log_local_reject(const Timestamp&, const OrderLogFormat::OrderLocalRejectBody&) override final;
        virtual void on_log_pending_cancel(const Timestamp&, const OrderLogFormat::OrderCxlReqBody&) override final;
        virtual void on_log_destroy(const Timestamp&, const OrderLogFormat::OrderDestroyBody&) override final;
        virtual void on_log_acknowledged(const Timestamp&, const OrderLogFormat::OrderAckBody&) override final;
        virtual void on_log_rejected(const Timestamp&, const OrderLogFormat::OrderRejectBody&) override final;
        virtual void on_log_filled(const Timestamp&, bool partial_fill, const OrderLogFormat::OrderFillBody&) override final;
        virtual void on_log_cancel_rejected(const Timestamp&, const OrderLogFormat::OrderCxlRejectBody&) override final;
        virtual void on_log_cancelled(const Timestamp&, const OrderLogFormat::OrderCxlBody&) override final;
        virtual void on_log_add_session(const Timestamp&, const OrderLogFormat::NewSessionBody&) override final;
        virtual void on_log_add_strategy(const Timestamp&, const OrderLogFormat::NewAccountBody&) override final;
        virtual void on_log_manual_position_adj(const Timestamp&, const OrderLogFormat::ManualPositionAdjBody&) override final;

        // TimerListener
        virtual void on_timer(const Timestamp&, void* userdata, std::uint64_t iter) override final;

        void load_and_update_locates(std::string locates_update_file);

        bool adopt_orphan_order(const std::string& session_name, LocalID, Order *);

        std::string status() const;

        OrderPtrContainer open_orders() const;
        OrderPtrContainer all_orders() const;

        void enable_stock(MD::EphemeralSymbolIndex esi);
        void disable_stock(MD::EphemeralSymbolIndex esi);

        void enable_all_trading();
        void disable_all_trading();

        FirmRiskCheck::InstPermissionBits get_firm_risk_status(MD::EphemeralSymbolIndex esi) const;
        FirmRiskCheck::Status get_firm_risk_status() const;

        void manual_position_adj(const Timestamp&, const MD::EphemeralSymbolIndex esi, const int64_t delta_start_qty, const int64_t delta_trading_qty);
    private:

        /// \internal Helper function to update order on ack.
        void order_update_on_ack(Order *, const ExchangeID, const Size, const Timestamp&,
                                 const ExchangeTimestamp&);

        /// \internal Helper function to update order state on fill.  Used on new fills and when replaying the log.
        void order_update_on_fill(Order *, const Size, const Price, const Timestamp&,
                                  const ExchangeTimestamp& exchange_timestamp, const FillFeeCode);

        /// \internal Helper function to update order on cancel.
        void order_update_on_cancel(Order *, const Size, const Timestamp&, const ExchangeTimestamp&);

        /// \internal Helper function to update order on reject.
        void order_update_on_reject(Order *, const Timestamp&, const ExchangeTimestamp&);

        bool extend_local_id_helper(const std::string& func_name, const OrderLogFormat::OrderIdentifier&, bool);
        void extend_local_id(LocalID id);
        void adopt_order_helper(Order *op, LocalID, bool do_log);
        // bool replay_update_local_id_helper(const std::string& name, const OrderLogFormat::OrderIdentifier& oid, bool expect_new = false);

        /// Gets an order by localID.
        Order* get_order_unsafe(LocalID id) {
            if (id < m_orders.size() && id > 0)
                return m_orders[id];
            return nullptr;
        }

        Order* get_order_safe(LocalID id) {
            //OrderManagerMutex::scoped_lock lock(m_mutex);
            return get_order_unsafe(id);
        }

        OrderPtrContainer unsafe_order_filter(const OrderFilterFunc& f) const;

    private:
        typedef i01::core::SpinMutex OrderManagerMutex;
        typedef std::vector<Order*> OrderVecT;
        typedef std::map<std::string, OrderSessionPtr> OrderSessionMapT;
        using LastSaleArray = std::array<MD::LastSale, MD::NUM_SYMBOL_INDEX>;

        LocalID               m_localID;
        FirmRiskCheck         m_firm_risk;
        PortfolioRiskCheck    m_portfolio_risk;
        OrderVecT             m_orders;
        OrderSessionMapT      m_sessions;
        OrderListener*        m_default_listener_p;

        mutable OrderManagerMutex     m_mutex;

        MD::DataManager*      m_dm_p;

        EqInstUniverse        m_universe;

        TradingFees           m_trading_fees;

        LastSaleArray         m_last_sale;

        Blotter*              m_blotter_p;
        BlotterReader*        m_blotter_reader_p;

        friend OrderSession;
    };

    template <typename T, typename ... Args>
    T* OrderManager::create_order(Args&&... args)
    {
        static_assert(std::is_base_of<Order, T>::value,
            "OrderManager::create_order return type must inherit from Order.");

        // TODO: replace with placement new with memory_pool for storage
        return new T(std::forward<Args>(args)...);
    }

}}
