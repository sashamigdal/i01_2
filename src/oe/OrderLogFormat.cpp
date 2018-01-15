#include <cctype>
#include <sstream>

#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>

#include <i01_oe/OrderLogFormat.hpp>
#include <i01_oe/Types.hpp>

namespace i01 { namespace OE { namespace OrderLogFormat {

inline namespace v1 {

void array_render_helper(std::ostream& os, const std::uint8_t* arr, const size_t len, char padding = ' ')
{
    bool end = false;
    bool padding_isprint = std::isprint(padding);
    for (auto i = 0U; i < len; i++) {
        if ('\0' == arr[i]) {
            end = true;
        }
        if (!end) {
            if (std::isprint(arr[i]) && arr[i] != '\n') {
                os << (char) arr[i];
            } else if (padding_isprint) {
                os << padding;
            }
        } else {
            if (padding_isprint) {
                os << padding;
            }
        }
    }
}

std::string SessionName::to_string() const
{
    std::ostringstream os;
    array_render_helper(os, arr.data(), arr.size(), '\0');
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const TimestampBody& body)
{
    return os << core::Timestamp{body.ts.tv_sec, body.ts.tv_nsec};
}

std::ostream& operator<<(std::ostream& os, const NewInstrumentBody& body)
{
    os << body.esi << ","
       << core::MIC(body.listing_market) << ",";
    array_render_helper(os, body.symbol, sizeof(body.symbol));
    os << ",";
    array_render_helper(os, body.bloomberg_gid_figi, sizeof(body.bloomberg_gid_figi));
    return os;
}

std::ostream& operator<<(std::ostream& os, const PositionBody& body)
{
    return os << (int) body.source << ","
              << body.instrument << ","
              << body.qty << ","
              << body.avg_price;
}

std::ostream& operator<<(std::ostream& os, const ManualPositionAdjBody& body)
{
    return os << body.esi << ","
              << body.delta_trading_qty << ","
              << body.delta_start_qty;
}

std::ostream& operator<<(std::ostream& os, const NewSessionBody& body)
{
    array_render_helper(os, body.name.arr.data(), body.name.arr.size());
    return os << ","
              << core::MIC(body.market_mic);
}

std::ostream& operator<<(std::ostream& os, const NewOrderBody& b)
{
    using OE::operator<<;
    os << b.instrument_esi << ","
       << core::MIC(b.market_mic) << ","
       << b.oid << ","
       << b.client_order_id << ","
       << b.orig_price << ","
       << b.size << ","
       << b.side << ","
       << b.tif << ","
       << b.type << ","
       << b.user_data << ",";

    return os;
}

bool OrderIdentifier::operator==(const OrderIdentifier& a) const
{
    return session_name.u64 == a.session_name.u64 && local_account == a.local_account && local_id == a.local_id;
}

std::ostream& operator<<(std::ostream& os, const OrderIdentifier& oid)
{
    os << oid.local_account << ","
       << oid.local_id << ",";
    array_render_helper(os, oid.session_name.arr.data(), oid.session_name.arr.size());
    return os;
}

std::ostream& operator<<(std::ostream& os, const OrderSessionDataHeader& osdh)
{
    os << osdh.oid;
    return os;
}

std::ostream& operator<<(std::ostream& os, const OrderLocalRejectBody& body)
{
    return os << body.order;
}

std::ostream& operator<<(std::ostream& os, const OrderSentBody& body)
{
    return os << body.order;
}

bool OrderSentBody::compare::operator()(const OrderSentBody& a, const OrderSentBody& b) const
{
    return a.order.oid.local_id < b.order.oid.local_id;
}

std::ostream& operator<<(std::ostream& os, const OrderAckBody& body)
{
    return os << body.oid << ","
              << body.open_size << ","
              << body.exchange_ts;
}

std::ostream& operator<<(std::ostream& os, const OrderFillBody& body)
{
    return os << body.oid << ","
              << body.filled_qty << ","
              << body.filled_price << ","
              << body.filled_fee_code << ","
              << body.exchange_ts;
}

std::ostream& operator<<(std::ostream& os, const OrderCxlReqBody& body)
{
    return os << body.oid << ","
              << body.new_qty;
}

std::ostream& operator<<(std::ostream& os, const OrderCxlBody& body)
{
    return os << body.oid << ","
              << body.cxled_qty << ","
              << body.exchange_ts;
}

std::ostream& operator<<(std::ostream& os, const OrderCxlReplaceReqBody& body)
{
    return os << body.old_oid << ","
              << body.new_order;
}

std::ostream& operator<<(std::ostream& os, const OrderCxlReplaceBody& body)
{
    return os << body.old_oid << ","
              << body.new_order;
}

std::ostream& operator<<(std::ostream& os, const OrderRejectBody& body)
{
    return os << body.oid << ","
              << body.exchange_ts;
}

std::ostream& operator<<(std::ostream& os, const OrderCxlRejectBody& body)
{
    return os << body.oid << ","
              << body.exchange_ts;
}

std::ostream& operator<<(std::ostream& os, const OrderDestroyBody& body)
{
    return os << body.oid;
}

std::ostream& operator<<(std::ostream& os, const NewAccountBody& body)
{
    return os << body.la << "," << body.name;;
}

}}}}
