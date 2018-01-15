#pragma once

#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

#include <i01_oe/Risk/SessionRiskCheck.hpp>
#include <i01_oe/Types.hpp>

namespace i01 { namespace OE {

    class OrderManager;
    class Order;
    class RiskCheck;

    typedef std::shared_ptr<OrderSession> OrderSessionPtr;
    class OrderSession
        : public std::enable_shared_from_this<OrderSession>
        , private boost::noncopyable {
    public:
        /// This session's unique name (identifying it in the session
        /// registry and configuration database).
        const std::string& name() const { return m_name; }
        /// Is the session supposed to be active?
        /// Is the session currently connected?  Can orders be sent?
        bool active() const { return m_active; }

        /// Manually try to connect (as opposed to the Order Manager handling it)
        bool manual_connect() { if (!active()) { return connect(false); } return true; }
        /// Manually attempt to disconnect
        bool manual_disconnect() { if (active()) { disconnect(true); return true; } return false; }
        /// Which market is this session supporting
        virtual const core::MIC & market() const { return m_mic; }

        virtual std::string status() const { return std::string{}; }

    protected:
        using std::enable_shared_from_this<OrderSession>::shared_from_this;
        /// Create a new session with a unique name.
        OrderSession(OrderManager*, const std::string& name_, const std::string& type_name_ = "OrderSession", bool simulated = false);
        /// Destructor.
        virtual ~OrderSession() = default;
        /// Risk check
        virtual RiskCheck& risk() { return m_session_risk; }
        /// Establish the connection.  Set m_active to true when ready to send
        /// orders!
        virtual bool connect(const bool replay) = 0;
        /// Tear down the connection
        virtual void disconnect(const bool I01_UNUSED force = false) { m_active = false; }
        /// Send the order to the market via this session.
        virtual bool send(Order* order_p) = 0;
        /// Cancel the order via this session, partially cancels if newqty > 0.
        virtual bool cancel(Order* order_p, Size newqty = 0) = 0;
        /// Cancel all orders (if cancel all is supported by session).
        /// Return < 0 if unsupported, return >= 0 if supported.  This affects
        /// `OrderManager::cancel_all(OrderSessionPtr)` behavior.
        virtual ssize_t cancel_all()
        { return -1; }
        /// Removes an order from existence in this session, without sending
        /// anything to the market.  USE WITH EXTREME CAUTION!
        virtual bool destroy(Order* I01_UNUSED order_p)
        { return false; }

        void set_order_sent_time(Order *op, const Timestamp&ts) { op->sent_time(ts); }
        void set_order_last_request_time(Order *op, const Timestamp& ts) { op->last_request_time(ts); }

        /// A callback used by the OM to pass info about potential order when doing recovery/replay.
        virtual Order * on_create_replay_order(const Timestamp&, Instrument *, const core::MIC&,
                                               const LocalAccount, const LocalID,
                                               const OE::Price, const OE::Size, const OE::Side, const OE::TimeInForce,
                                               const OrderType, const UserData);

        Order* get_order(LocalID id);

        std::string m_name;
        core::MIC m_mic;
        bool m_active;
        OrderManager* m_order_manager_p;
        SessionRiskCheck m_session_risk;

        /// MOC/LOC add order deadline since midnight in nanoseconds
        std::uint64_t m_mocloc_add_deadline_ns;
        /// MOC/LOC modify/cancel deadline since midnight in nanoseconds
        std::uint64_t m_mocloc_mod_deadline_ns;
        /// Is this session a simulation?
        bool m_simulated;
    private:
        static OrderSessionPtr factory(OrderManager* om, const std::string& session_name, const std::string& session_type);

        friend class OrderManager;
    };

} }
