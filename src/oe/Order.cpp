#include <i01_core/Time.hpp>
#include <i01_oe/Order.hpp>

namespace i01 { namespace OE {

Order::Order( Instrument* const instrument_
            , const Price orig_price_
            , const Size size_
            , const Side side_
            , const TimeInForce tif_
            , const OrderType type_
            , OrderListener* listener_
            , const UserData userdata_)
  : m_instrument(instrument_)
  , m_original_price(orig_price_)
  , m_price(orig_price_)
  , m_size(size_)
  , m_side(side_)
  , m_tif(tif_)
  , m_type(type_)
  , m_userdata( userdata_ )
  , m_broker_locate(-1) // TODO: add support for per-order broker locate codes.
  , m_creation_time(i01::core::Timestamp::now())
  , m_state(OrderState::NEW_AND_UNSENT)
  , m_session_p(nullptr)
  , m_sent_time({0, 0})
  , m_local_account(0)
  , m_localID(0)
  , m_pendingcancel_newqty(std::numeric_limits<decltype(m_pendingcancel_newqty)>::max())
  , m_listener(listener_)
  , m_exchangeID(0)
  , m_last_response_time({0, 0})
  , m_filled_size(0)
  , m_open_size(0)
  , m_cancelled_size(0)
  , m_total_value_paid(0.0)
  , m_last_fill_fee_code(FillFeeCode::UNKNOWN)
  , m_last_exchange_time{0}
{
    m_client_order_id.fill(' ');
}

Order::Order(const Order& o)
  : m_instrument(o.instrument())
  , m_original_price(o.original_price())
  , m_price(o.price())
  , m_size(o.size())
  , m_side(o.side())
  , m_tif(o.tif())
  , m_type(o.type())
  , m_userdata( o.userdata() )
  , m_broker_locate(o.broker_locate())
  , m_creation_time(o.creation_time())
  , m_state(o.state())
  , m_session_p(o.session())
  , m_sent_time(o.sent_time())
  , m_local_account(o.local_account())
  , m_localID(o.localID())
  , m_pendingcancel_newqty(std::numeric_limits<decltype(m_pendingcancel_newqty)>::max())
  , m_exchangeID(o.exchangeID())
  , m_last_response_time(o.last_response_time())
  , m_filled_size(o.filled_size())
  , m_open_size(o.open_size())
  , m_cancelled_size(o.cancelled_size())
  , m_total_value_paid(o.total_value_paid())
  , m_last_fill_fee_code(o.last_fill_fee_code())
  , m_last_exchange_time(o.last_exchange_time())
  , m_mutex() // don't copy the mutex
{
    m_client_order_id = o.client_order_id();
}

Order::~Order()
{
}

bool Order::validate_base() const
{
    return unsent()
        && (price() > 0.0 + std::numeric_limits<Price>::epsilon())
        && (size() > 0)
        && (side() != Side::UNKNOWN)
        && (tif() != TimeInForce::UNKNOWN)
        && (type() != OrderType::UNKNOWN)
           ;
}

std::ostream & operator<<(std::ostream &os, const Order &o)
{
    return os << o.m_instrument->symbol() << ","
              << o.market() << ","
              << o.m_price << ","
              << o.m_size << ","
              << o.m_side << ","
              << o.m_tif << ","
              << o.m_type << ","
              << o.m_userdata.ud << ","
              // << o.m_creation_time << ","
              << o.m_state << ","
              // << o.m_session_p << ","
              << o.m_sent_time << ","
              << o.m_local_account << ","
              << o.m_localID << ","
              << o.m_exchangeID << ","
              << o.m_last_response_time << ","
              << o.m_filled_size << ","
              << o.m_open_size << ","
              << o.m_cancelled_size << ","
              << o.m_total_value_paid << ","
              << o.m_last_fill_fee_code << ","
              << o.m_last_exchange_time;
}

}}
