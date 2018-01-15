#pragma once

#include <i01_core/Lock.hpp>
#include <i01_core/MIC.hpp>
#include <i01_core/util.hpp>

#include <i01_oe/Instrument.hpp>
#include <i01_oe/Types.hpp>
#include <i01_oe/OrderListener.hpp>

namespace i01 { namespace OE {

    class Instrument;
    class OrderManager;
    class OrderSession;
    namespace BATS { class BOE20Session; }
    namespace NASDAQ { class OUCH42Session; }

    class Order {
    public:
        /* DERIVED INTERFACE: */
        /// Validate the order for correctness.
        //  This answers ``Can this order be sent to the market?''
        //  This does not need to call validate_base(), OrderManager will do
        //  that on its own.  If derived order does not have anything to
        //  validate besides the members of the base class, it should simply
        //  return true.  (Remember to mark your derived order's validate()
        //  as final to help optimization.)
        virtual bool validate() const = 0;
        virtual const i01::core::MIC market() const = 0;
        virtual Order* clone() const = 0;

        /* BASE INTERFACE: */

        typedef i01::OE::Size Size;
        typedef i01::OE::Price Price;
        typedef i01::OE::Side Side;

        /// Returns the Instrument.
        Instrument* instrument() const { return m_instrument; }
        /// Returns the original price when the order was created.
        Price original_price() const { return m_original_price; }
        /// Returns the current price.
        Price price() const { return m_price; }
        /// Returns the size.
        Size size() const { return m_size; }
        /// Returns the side.
        Side side() const { return m_side; }
        /// Returns the time in force.
        TimeInForce tif() const { return m_tif; }
        /// Returns the current order type.
        OrderType type() const { return m_type; }
        /// Returns current order state. Initially OrderState::NEW_AND_UNSENT.
        OrderState state() const { return m_state; }
        /// Returns true if the order is in a terminal state.
        bool is_terminal() const {
            return open_size() == 0
                || state() == OrderState::LOCALLY_REJECTED
                || state() == OrderState::REMOTELY_REJECTED
                || state() == OrderState::CANCELLED
                || state() == OrderState::FILLED
                || state() == OrderState::NEW_AND_UNSENT
                || state() == OrderState::CANCEL_REPLACED
                || state() == OrderState::UNKNOWN; }
        /// Returns true if we believe that the order is open with the
        /// exchange (i.e. it's sent and in a nonterminal state).
        /// Open orders are counted against open exposure.
        //  NB: Open orders may be unacked.
        bool is_open() const {
            return state() != OrderState::NEW_AND_UNSENT && !is_terminal();
        }
        /// Returns a pointer to the user data associated with the order.
        UserData userdata() const { return m_userdata.ud; }
        uintptr_t userdata_integral() const { return m_userdata.ui; }
        /// Returns the broker locate code, or -1 if unset (use session's
        /// default if <0.)  TODO: Add support for per-order broker locate
        /// codes.
        std::int8_t broker_locate() const { return m_broker_locate; }
        /// Returns the creation timestamp.
        const Timestamp& creation_time() const { return m_creation_time; }


        /// Returns whether the state is OrderState::NEW_AND_UNSENT.
        bool unsent() const { return state() == OrderState::NEW_AND_UNSENT; }

        /// Returns the sent timestamp.
        const Timestamp& sent_time() const { return m_sent_time; }
        /// Returns the last request timestamp.
        const Timestamp& last_request_time() const { return m_last_request_time; }
        /// Returns the last response timestamp.
        const Timestamp& last_response_time() const { return m_last_response_time; }
        /// Returns the last exchange timestamp.
        const ExchangeTimestamp& last_exchange_time() const { return m_last_exchange_time; }

        /// Returns filled size.
        const Size& filled_size() const { return m_filled_size; }
        /// Returns open size.
        const Size& open_size() const { return m_open_size; }
        /// Returns (partially) cancelled size.
        const Size& cancelled_size() const { return m_cancelled_size; }
        /// Returns total value paid.
        const Price& total_value_paid() const { return m_total_value_paid; }
        /// Returns average fill price.
        Price mean_fill_price() const { return m_total_value_paid / (Price)filled_size(); }

        /// Sets the userdata (can be modified at any time by the user).
        void userdata(UserData userdata_) { m_userdata.ud = std::move(userdata_); }
        void userdata_integral(uintptr_t userdata_) { m_userdata.ui = std::move(userdata_); }

        const LocalAccount& local_account() const { return m_local_account; }
        void local_account(LocalAccount local_account_) { m_local_account = std::move(local_account_); }

        const ClientOrderID& client_order_id() const { return m_client_order_id; }

        /// Returns the OrderListener (i.e. Strategy that owns this order).
        /// IF THIS IS `nullptr`, THE ORDER IS ORPHANED, AND BELONGS TO THE
        /// DEFAULT STRATEGY (i.e. the manual trader strategy)
        OrderListener* listener() const { return m_listener; }
        /// Convenience function used by OrderManager.
        OrderListener* listener_or_default(OrderListener* default_listener) const { return m_listener ? m_listener : default_listener; }

        LocalID localID() const { return m_localID; }
        void localID(LocalID localID_) { m_localID = std::move(localID_); }

        ExchangeID exchangeID() const { return m_exchangeID; }
        void exchangeID(ExchangeID e) { m_exchangeID = std::move(e); }

        FillFeeCode last_fill_fee_code() const { return m_last_fill_fee_code; }

        friend std::ostream & operator<<(std::ostream &os, const Order &o);

    protected:
        /// \internal Create a new order, initialized for sending.
        Order( Instrument* const instrument_
             , const Price orig_price_
             , const Size size_
             , const Side side_
             , const TimeInForce tif_
             , const OrderType type_
             , OrderListener* listener_
             , const UserData userdata_ = nullptr);

        // Implement a copy constructor, as we don't want to copy the mutex
        Order(const Order&);

        /// \internal Destructor.
        virtual ~Order();

        /// Returns the session this order was sent through.
        OrderSession* session() const { return m_session_p; }
        void session(OrderSession* session_) { m_session_p = std::move(session_); }

        /// Sets the OrderListener (i.e. Strategy that owns this order).
        void listener(OrderListener* listener_) { m_listener = std::move(listener_); }

        /// \internal Sets the state.
        void state(OrderState state_) { m_state = std::move(state_); }

        void side(Side side_) { m_side = std::move(side_); }

        /// \internal Sets the timestamp of the last request.
        void last_request_time(Timestamp timestamp_) { m_last_request_time = std::move(timestamp_); }
        /// \internal Sets the timestamp of the last response.
        void last_response_time(Timestamp timestamp_) { m_last_response_time = std::move(timestamp_); }
        /// \internal Sets the last exchange timestamp response.
        void last_exchange_time(ExchangeTimestamp timestamp_) { m_last_exchange_time = std::move(timestamp_); }
        /// \internal Sets the sent time.
        void sent_time(Timestamp timestamp_) { m_sent_time = std::move(timestamp_); }
        /// \internal Sets the filled size.
        void filled_size(Size filled_size_) { m_filled_size = std::move(filled_size_); }
        /// \internal Sets the open size.
        void open_size(Size open_size_) { m_open_size = std::move(open_size_); }
        /// \internal Sets the total value paid.
        void total_value_paid(Price total_value_paid_) { m_total_value_paid = std::move(total_value_paid_); }
        /// \internal Sets the cancelled size.
        void cancelled_size(Size cancelled_size_) { m_cancelled_size = std::move(cancelled_size_); }
        /// \internal Sets the last fill fee code.
        void last_fill_fee_code(FillFeeCode ffc) { m_last_fill_fee_code = std::move(ffc); }

        template<std::size_t N>
        void client_order_id(const std::array<std::uint8_t, N>& clordid) {
            auto m = core::cmin(N, CLIENT_ORDER_ID_LENGTH);
            for (auto i = 0U; i < m; i++) {
                m_client_order_id[i] = clordid[i];
            }
        }

        /// \internal Validates the base Order portion of the order.
        bool validate_base() const;

        typedef i01::core::RecursiveMutex OrderMutex;

        OrderMutex& mutex() const { return m_mutex; }

        /* INITIAL DATA */

        Instrument* const   m_instrument;
        const Price         m_original_price;
        Price               m_price;
        const Size          m_size;
        Side                m_side;
        TimeInForce         m_tif;
        const OrderType     m_type;
        union UserDataStorage {
            UserData ud;
            uintptr_t ui;

            UserDataStorage() : ud(nullptr) {}
            UserDataStorage(const UserData& u) : ud(u) {}
            UserDataStorage(const uintptr_t& i) : ui(i) {}
        }                   m_userdata;
        I01_ASSERT_SIZE(UserDataStorage, sizeof(UserData));
        I01_ASSERT_SIZE(UserDataStorage, sizeof(uintptr_t));
        const std::int8_t   m_broker_locate;
        const Timestamp     m_creation_time;

        /* STATE DATA */

        OrderState          m_state;

        /* SEND TIME DATA */

        OrderSession*       m_session_p;
        Timestamp           m_sent_time;
        Timestamp           m_last_request_time;
        LocalAccount        m_local_account;
        LocalID             m_localID;
        ClientOrderID       m_client_order_id;
        Size                m_pendingcancel_newqty;

        /* RESPONSE TIME DATA */

        OrderListener*      m_listener; //< Strategy associated with this Order.
        ExchangeID          m_exchangeID;
        Timestamp           m_last_response_time;
        Size                m_filled_size;
        Size                m_open_size;
        Size                m_cancelled_size;
        Price               m_total_value_paid;
        FillFeeCode         m_last_fill_fee_code;
        ExchangeTimestamp   m_last_exchange_time;

    private:
        // Don't allow assignment
        Order& operator=(const Order&) = delete;

        friend class OrderSession;
        friend class OrderManager;
        friend class EquityInstrument;
        friend class FileBlotter;
        friend class OrderIdentifier;
        friend class NASDAQ::OUCH42Session;
        friend class BATS::BOE20Session;

        mutable OrderMutex m_mutex;
    };

} }
