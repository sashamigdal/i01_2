#include <endian.h>
#include <arpa/inet.h>
#include <string.h>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <regex>

#include <i01_core/Config.hpp>
#include <i01_core/util.hpp>
#include <i01_oe/NASDAQOrder.hpp>

#include "SoupBinTCP30/Types.hpp"
#include "SoupBinTCP30/Messages.hpp"
#include "UFO10/Types.hpp"
#include "UFO10/Messages.hpp"
#include "OUCH42/Types.hpp"
#include "OUCH42/Messages.hpp"
#include "OUCH42Session.hpp"

namespace O42 = i01::OE::NASDAQ::OUCH42;
namespace SBT = i01::OE::NASDAQ::SoupBinTCP30;
namespace UFO = i01::OE::NASDAQ::UFO10;
typedef i01::core::Config Config;

namespace i01 { namespace OE { namespace NASDAQ {

namespace {
template<typename ArrayType, std::size_t WIDTH, bool RIGHT_PAD = true>
void copy_to_array(std::array<ArrayType,WIDTH> &arr, const std::string &str, const ArrayType &padding)
{
    for (auto i = 0U; i < arr.size(); i++) {
        if (i < str.size()) {
            arr[RIGHT_PAD ? i : arr.size() - i - 1] = str[RIGHT_PAD ? i : str.size() - i - 1];
        } else {
            arr[RIGHT_PAD ? i : arr.size() - i - 1] = padding;
        }
    }
}
}

OUCH42Session::OUCH42Session(OrderManager *om_p, const std::string& name_)
    : OrderSession(om_p, name_, "OUCH42Session")
    , m_username(), m_password(), m_req_session()
    , m_firm_identifier{.arr = {{' ', ' ', ' ', ' '}}}
    , m_heartbeat_grace_sec(10)
    , m_default_broker_locate(-1)
    , m_session_state(State::UNKNOWN)
    , m_last_message_received_ts{0,0}
    , m_last_message_sent_ts{0,0}
    , m_dangly_length(0)
    , m_dangly_bytes()
    , m_connection("OUCH42Session", this, this)
    , m_state_path{}
{
    m_dangly_bytes.fill('\0');
    auto cs = Config::instance().get_shared_state()->copy_prefix_domain("oe.sessions." + m_name + ".");

    if (boost::optional<std::string> protocol = cs->get<std::string>("protocol")) {
        if (*protocol != "SoupBinTCP")
            throw std::runtime_error("OUCH42Session " + name() + " only supports the SoupBinTCP protocol currently.");
    } else {
        throw std::runtime_error("OUCH42Session " + name() + " missing protocol.");
    }

    boost::optional<std::string> remote_addr = cs->get<std::string>("remote.addr");
    boost::optional<int> remote_port = cs->get<int>("remote.port");
    if (!remote_addr || !remote_port) {
        throw std::runtime_error("OUCH42Session " + name() + " missing remote.addr or remote.port.");
    }
    m_host = *remote_addr;
    m_port = *remote_port;

    if (!cs->get<std::string>("username", m_username))
        throw std::runtime_error("OUCH42Session " + name() + " missing username.");
    m_username.resize(sizeof(SBT::Types::Username), ' '); // right pad
    if (!cs->get<std::string>("password", m_password))
        throw std::runtime_error("OUCH42Session " + name() + " missing password.");
    m_password.resize(sizeof(SBT::Types::Password), ' '); // right pad
    m_req_session = cs->get_or_default<std::string>("session", "");
    m_req_session.resize(sizeof(SBT::Types::Session), ' '); // right pad
    if (!cs->get("heartbeat_grace_period", m_heartbeat_grace_sec)) {
        m_heartbeat_grace_sec = 5;
        std::cerr << "OUCH42Session " << name()
            << " missing heartbeat_grace_period, defaulted to 5 sec." << std::endl;
    }
    m_default_broker_locate = static_cast<std::int8_t>(cs->get_or_default<int>("default_broker_locate", -1));
    if (m_default_broker_locate < 0 || m_default_broker_locate > 3)
        throw std::runtime_error("OUCH42Session " + name() + " missing or invalid default_broker_locate");
    boost::optional<std::string> firm_identifier = cs->get<std::string>("firm_identifier");
    if (firm_identifier) {
        if (!firm_identifier->empty())
        firm_identifier->copy(
                m_firm_identifier.raw
              , std::min(firm_identifier->size(), sizeof(m_firm_identifier.raw)));
        if (firm_identifier->size() > 4)
            std::cerr << "OUCH42Session " << name()
                      << " truncated firm_identifier to 4 characters." << std::endl;
    }

    m_state_path = cs->get_or_default("state_path", "ouch42." + m_name + ".state");

    symbology_init();

    recover_state();

    m_connection.set_heartbeat_seconds(1);
}

OUCH42Session::~OUCH42Session()
{
    disconnect(true);
    m_connection.shutdown();
}

void OUCH42Session::symbology_init()
{
    auto rules = std::vector<std::pair<std::regex, std::string> >{};
    rules.emplace_back(std::regex("(.+)/U$"), "$1="); // units
    rules.emplace_back(std::regex("(.+)/([A-Z])$"), "$1.$2"); // classes
    rules.emplace_back(std::regex("(.+)r$"), "$1^"); // rights

    auto stock = O42::Types::Stock{};
    auto esi_set = m_order_manager_p->universe().valid_esi();
    for (auto esi : esi_set) {
        const auto& u = m_order_manager_p->universe()[esi];
        try {
            auto nasdaq_sym = u.cta_symbol();
            for (const auto& r: rules) {
                auto res = std::regex_replace(u.cta_symbol(), r.first, r.second);
                if (res != nasdaq_sym) {
                    nasdaq_sym = res;
                    break;
                }
            }
            copy_to_array(stock.arr, nasdaq_sym, ' ');
            m_symbol_inst_cache[stock.arr] = u.data();
            m_symbol_cache[esi] = nasdaq_sym;
        } catch (std::exception& e) {
            std::cerr << "OUCH42Session " << name() << " caught exception converting symbol " << u.cta_symbol() << " to NASDAQ: " << e.what() << std::endl;
        }
    }
}

void OUCH42Session::recover_state()
{
    if (!m_state_path.size()) {
        throw std::runtime_error(m_name + " requires a state_path");
    }

    m_persist = std::unique_ptr<PersistentStateRegion>(new PersistentStateRegion(m_state_path, false, true));
    if (!m_persist->mapped()) {
        std::cerr << m_name << ",ERR,RECOVER STATE,COULD NOT MAP STATE," << m_state_path << std::endl;
        throw std::runtime_error(m_name + " could not open persistent state ");
    }

    if (m_persist->data()->session[0] != '\0') {
        // then we have a previous session here ...
        // if we have no req_session, then set it here
        if (m_req_session.front() == ' ') {
            m_req_session = std::string(m_persist->data()->session.begin(), m_persist->data()->session.end());
            m_req_session.resize(sizeof(SoupBinTCP30::Types::Session), ' ');

            std::cerr << "OUCH42Session " << name() << " requesting session " << m_req_session << std::endl;
        }
    }
}

bool OUCH42Session::connect(const bool I01_UNUSED replay)
{
    m_session_state = State::CONNECTING;
    m_connection.connect(m_host, m_port);
    while (m_connection.state() == net::HeartBeatConnThread::State::STARTING) {
        ::usleep(1000);
    }

    if (m_connection.state() == net::HeartBeatConnThread::State::ACTIVE)
        return true;

    return false;
}

void OUCH42Session::disconnect(const bool I01_UNUSED force)
{
    m_active = false;
    bool was_logged_in = __sync_val_compare_and_swap((int*)&m_session_state, (int)State::LOGGED_IN, (int)State::DISCONNECTED);
    if (!force && was_logged_in) {
        SBT::Messages::LogoutRequest msg;
        msg.packet_length = htons(sizeof(msg) - 2);
        msg.packet_type = SBT::Types::UpstreamPacketType::LOGOFF_REQUEST;
        m_connection.send((std::uint8_t *)&msg, sizeof(msg));
    }
    m_session_state = State::DISCONNECTED;
    m_connection.disconnect();
}

bool OUCH42Session::create_order_token_hpr(Order *o_p, OUCH42::Types::OrderToken& token)
{
    // TODO: Support LocalAccount - not enough digits available in HPR
    //       MSSSSSCCCCCCCC OrderToken format....
    // TODO: get BL0 - 3 from session config instead of hardcoding it here...
    std::uint8_t hpr_broker_locate = (LIKELY(o_p->broker_locate() < 0 || o_p->broker_locate() > 3) ? m_default_broker_locate : o_p->broker_locate());

    // See section of HPR RoE on "order marking character"
    int marker;
    switch(o_p->side()) {
        case Side::UNKNOWN:
            return false;
        case Side::SHORT:
        case Side::SHORT_EXEMPT:
            marker = 'A' + hpr_broker_locate;
            break;
        case Side::SELL:
            marker = 'E' + hpr_broker_locate;
            break;
        case Side::BUY:
            marker = 'I';
            break;
        default:
            return false;
    }
    if (  o_p->tif() == TimeInForce::AUCTION_OPEN
       || o_p->tif() == TimeInForce::AUCTION_CLOSE
        )
            marker += 9;
    assert(marker >= 'A' && marker <= 'R');
    token.hprdata.hpr_marking = static_cast<uint8_t>(marker);
    // TODO: use uint64_t instead of std::string to store nasdaq symbol...
    std::string s(m_symbol_cache[o_p->instrument()->esi()]);
    s.resize(token.hprdata.symbol.size(), '0');
    for (char& c : s) {
        switch (c) {
            case '.':
                c = 'c';
                continue;
            case '-':
                c = 'p';
                continue;
            case '+':
                c = 'w';
                continue;
            default:
                continue;
        }
    }
    ::memcpy(token.hprdata.symbol.data(), s.data(), token.hprdata.symbol.size());
    static_assert(sizeof(decltype(o_p->localID())) <= 4, "Local ID must be <= 4 bytes.");
    token.hprdata.id.set(o_p->localID());
    return true;
}

bool OUCH42Session::send(Order* oo_p)
{
    if (UNLIKELY(oo_p == nullptr || oo_p->instrument() == nullptr))
        return false;

    assert(typeid(*oo_p) == typeid(NASDAQOrder));
    NASDAQOrder *o_p = static_cast<NASDAQOrder*>(oo_p);

    if (UNLIKELY(!m_active || !logged_in())) {
        std::cerr << "OUCH42Session " << name() << " not active or not logged in: " << *o_p << std::endl;
        return false;
    }

    struct {
        SBT::Messages::UnsequencedDataPacket hdr;
        OUCH42::Messages::EnterOrder order;
    } __attribute__((packed)) msg {
        .hdr = {
            .packet_length = htons(sizeof(msg) - 2),
            .packet_type = SBT::Types::UpstreamPacketType::UNSEQUENCED_MESSAGE
        } };
    msg.order.message_type = OUCH42::Types::InboundMessageType::ENTER_ORDER;
    if(!create_order_token_hpr(o_p, msg.order.order_token)) {
        std::cerr << "OUCH42Session " << name() << " could not create order token " << *o_p << std::endl;
        return false;
    }
    oo_p->client_order_id(msg.order.order_token.arr);

    switch (o_p->side()) {
        case Order::Side::BUY:
            msg.order.buy_sell_indicator = OUCH42::Types::BuySellIndicator::BUY;
            break;
        case Order::Side::SELL:
            msg.order.buy_sell_indicator = OUCH42::Types::BuySellIndicator::SELL;
            break;
        case Order::Side::SHORT:
            msg.order.buy_sell_indicator = OUCH42::Types::BuySellIndicator::SHORT;
            break;
        case Order::Side::SHORT_EXEMPT:
            msg.order.buy_sell_indicator = OUCH42::Types::BuySellIndicator::SHORT_EXEMPT;
            break;
        case Order::Side::UNKNOWN:
        default:
            std::cerr << "OUCH42Session " << name() << " unknown order side" << *o_p << std::endl;
            return false;
            break;
    }
    msg.order.shares = htonl(o_p->size());

    // TODO: make this a uint64_t assignment instead of messing around with
    // std::strings -- ideally retrieve nasdaq symbol.
    std::string sym(m_symbol_cache[o_p->instrument()->esi()]);
    sym.resize(8, ' ');
    ::strncpy((char*)&msg.order.stock.raw, sym.c_str(), 8);
    msg.order.price.set_limit(o_p->price());
    switch(o_p->type()) {
        case OrderType::MARKET:
            msg.order.display = OUCH42::Types::Display::NON_DISPLAY;
            msg.order.price.set_market();
            break;
        case OrderType::LIMIT:
            msg.order.display = OUCH42::Types::Display::ANONYMOUS_PRICE_TO_COMPLY;
            msg.order.price.set_limit(o_p->price());
            break;
        case OrderType::MIDPOINT_PEG:
            msg.order.display = OUCH42::Types::Display::MIDPOINT_PEG;
            msg.order.price.set_limit(o_p->price());
            break;
        case OrderType::STOP:
        case OrderType::UNKNOWN:
        default:
            std::cerr << "OUCH42Session " << name() << " invalid order type: " << o_p << std::endl;
            return false;
    }
    switch (o_p->tif()) {
        case TimeInForce::FILL_OR_KILL:
        case TimeInForce::IMMEDIATE_OR_CANCEL:
            msg.order.time_in_force.set_ioc();
            msg.order.cross_type = O42::Types::CrossType::NO_CROSS;
            msg.order.display = OUCH42::Types::Display::NON_DISPLAY;
            break;
        case TimeInForce::DAY:
            msg.order.time_in_force.set_market_hours();
            msg.order.cross_type = O42::Types::CrossType::NO_CROSS;
            break;
        case TimeInForce::EXT:
            msg.order.time_in_force.set_system_hours();
            msg.order.cross_type = O42::Types::CrossType::NO_CROSS;
            break;
        case TimeInForce::AUCTION_CLOSE:
            msg.order.time_in_force.set_ioc();
            msg.order.cross_type = O42::Types::CrossType::CLOSING_CROSS;
            break;
        case TimeInForce::AUCTION_OPEN:
            msg.order.time_in_force.set_ioc();
            msg.order.cross_type = O42::Types::CrossType::OPENING_CROSS;
            break;
        case TimeInForce::AUCTION_HALT:
            msg.order.time_in_force.set_ioc();
            msg.order.cross_type = O42::Types::CrossType::HALT_OR_IPO_CROSS;
            break;
        case TimeInForce::GTC:
        case TimeInForce::TIMED:
        case TimeInForce::UNKNOWN:
        default:
            std::cerr << "OUCH42Session " << name() << " invalid TIF: " << o_p << std::endl;
            return false;
            break;
    }
    msg.order.firm.integral = m_firm_identifier.integral;
    if (o_p->nasdaq_display() != NASDAQOrder::Display::UNKNOWN) {
        msg.order.display = o_p->nasdaq_display();
    }
    msg.order.capacity = o_p->nasdaq_capacity();
    msg.order.iso_eligibility = O42::Types::IntermarketSweepEligibility::NOT_ELIGIBLE;
    msg.order.minimum_quantity = htonl(o_p->nasdaq_min_size());
    if (o_p->tif() == TimeInForce::FILL_OR_KILL)
        msg.order.minimum_quantity = msg.order.shares;
    msg.order.customer_type = O42::Types::CustomerType::USE_DEFAULT;
    set_order_sent_time(o_p, Timestamp::now());
    debug_print_msg("ORD", msg.order);
    return send_order_data(o_p, (const char*)&msg, sizeof(msg));
}

bool OUCH42Session::cancel(Order* oo_p, Size newqty)
{
    if (UNLIKELY(oo_p == nullptr || oo_p->instrument() == nullptr))
        return false;

    assert(typeid(*oo_p) == typeid(NASDAQOrder));
    NASDAQOrder *o_p = static_cast<NASDAQOrder*>(oo_p);

    if (UNLIKELY(!m_active || !logged_in()))
        return false;

    struct {
        SBT::Messages::UnsequencedDataPacket hdr;
        OUCH42::Messages::CancelOrder cxl;
    } __attribute__((packed)) msg {
        .hdr = {
            .packet_length = htons(sizeof(msg) - 2),
            .packet_type = SBT::Types::UpstreamPacketType::UNSEQUENCED_MESSAGE
        } };

    msg.cxl.message_type = O42::Types::InboundMessageType::CANCEL_ORDER;
    if (!create_order_token_hpr(o_p, msg.cxl.order_token))
        return false;
    msg.cxl.shares = htonl(newqty);
    debug_print_msg("CXL",msg.cxl);
    return send_order_data(o_p, (const char*)&msg, sizeof(msg));
}

bool OUCH42Session::destroy(Order* I01_UNUSED o_p)
{
    if (o_p && o_p->is_terminal())
        return true;
    return false;
}

void OUCH42Session::on_timer(const Timestamp& ts, void * I01_UNUSED userdata, std::uint64_t I01_UNUSED iter)
{
    if (!logged_in())
        return;

    if (m_last_message_received_ts.milliseconds_since_midnight()
            < ts.milliseconds_since_midnight() - (m_heartbeat_grace_sec * 1000ULL)) {
        core::LockGuard<Mutex> l(m_mutex);
        m_session_state = State::TIMED_OUT;
        // Nothing received from the server in 5 seconds.
        std::cerr << "OUCH42Session " << name()
                  << ts << " last message: " << m_last_message_received_ts
                  << " has no data in over " << m_heartbeat_grace_sec << " seconds, reconnecting." << std::endl;
        if (active()) {
            m_active = false;
            disconnect(/* force = */ true);
            connect(/* replay = */ true);
        }
    }

    if (logged_in())
        if (ts.milliseconds_since_midnight() - m_last_message_sent_ts.milliseconds_since_midnight() >= 250ULL) {
            core::LockGuard<Mutex> l(m_mutex);
            // Send client heartbeat.
            SBT::Messages::Heartbeat msg{
                .packet_length = htons(sizeof(SBT::Messages::Heartbeat) - 2),
                .packet_type = SBT::Types::UpstreamPacketType::HEARTBEAT_MESSAGE
            };
            if (!send_order_data(nullptr, (const char*)&msg, sizeof(msg))) {
                // Failed to send heartbeat:
                std::cerr << "OUCH42Session " << name() << " failed to send heartbeat." << std::endl;
                m_active = false;
                m_session_state = State::ERROR;
                disconnect(true);
            }
        }
}

void OUCH42Session::on_connected(const core::Timestamp&, void* I01_UNUSED ud)
{
    core::LockGuard<Mutex> l(m_mutex);
    bool was_connecting = __sync_val_compare_and_swap((int*)&m_session_state, (int)State::CONNECTING, (int)State::CONNECTED);
    if (!was_connecting) {
        return;
    }
    // Send LoginRequest:
    SoupBinTCP30::Messages::LoginRequest msg {
        .packet_length = htons(sizeof(SoupBinTCP30::Messages::LoginRequest) - 2),
        .packet_type = SoupBinTCP30::Types::UpstreamPacketType::LOGIN_REQUEST,
        .username =             {' ',' ',' ',' ',' ',' '},
        .password =             {' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
        .requested_session =    {' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
        .requested_sequence_number = {{' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','0'}}
    };

    ::memcpy((char*)&msg.username, m_username.c_str(), std::min(m_username.size(), sizeof(msg.username)));
    ::memcpy((char*)&msg.password, m_password.c_str(), std::min(m_password.size(), sizeof(msg.password)));
    ::memcpy((char*)&msg.requested_session, m_req_session.c_str(), std::min(m_req_session.size(), sizeof(msg.requested_session)));

    std::string seqnum = std::to_string(m_persist->data()->last_seqnum_received + 1);
    copy_to_array<std::uint8_t, 20, false>(msg.requested_sequence_number, seqnum, ' ');

    if (!send_order_data(nullptr, (const char*)&msg, sizeof(msg)))
        std::cerr << "OUCH42Session " << name() << " failed to send LoginRequest." << std::endl;
}

void OUCH42Session::on_peer_disconnect(const core::Timestamp&, void* ud)
{
    core::LockGuard<Mutex> l(m_mutex);
    m_active = false;
    m_session_state = State::DISCONNECTED;
    std::cerr << "OUCH42Session " << name() << " peer disconnect." << std::endl;
    m_connection.shutdown(false);
}

void OUCH42Session::on_local_disconnect(const core::Timestamp&, void* ud)
{
    core::LockGuard<Mutex> l(m_mutex);
    m_active = false;
    m_session_state = State::DISCONNECTED;
    std::cerr << "OUCH42Session " << name() << " local disconnect." << std::endl;
    m_connection.shutdown(false);
}

inline
void OUCH42Session::defer_bytes(const char *buf, size_t len)
{
    if (UNLIKELY(len == 0))
        return;
    ::memcpy(m_dangly_bytes.data() + m_dangly_length, buf, len);
    m_dangly_length += len;
}

inline
size_t OUCH42Session::dispatch_packet(const Timestamp& ts, const char *buf, size_t len)
{
    if (len < sizeof(SBT::Messages::PacketHeader))
        return 0;

    const SBT::Messages::PacketHeader *hdr = reinterpret_cast<const SBT::Messages::PacketHeader*>(buf);
    size_t consumed_len = sizeof(hdr->packet_length) + ntohs(hdr->packet_length);
    if (len < consumed_len)
        return 0;

    switch (hdr->packet_type) {
        case SBT::Types::DownstreamPacketType::DEBUG:
#if defined(I01_DEBUG_MESSAGING)
            std::cerr
                << "OUCH42Session " << name()
                << " debug packet received: "
                << std::string((const char*)(buf + sizeof(*hdr)), ntohs(hdr->packet_length) - 1) << std::endl;
#endif
            break;
        case SBT::Types::DownstreamPacketType::LOGIN_ACCEPT: {
            const SBT::Messages::LoginAccept *msg = reinterpret_cast<const SBT::Messages::LoginAccept*>(buf);
            handle_packet(ts, msg);
        } break;
        case SBT::Types::DownstreamPacketType::LOGIN_REJECT: {
            const SBT::Messages::LoginReject *msg = reinterpret_cast<const SBT::Messages::LoginReject*>(buf);
            handle_packet(ts, msg);
        } break;
        case SBT::Types::DownstreamPacketType::SEQUENCED_DATA: {
            const SBT::Messages::SequencedDataPacket *msg = reinterpret_cast<const SBT::Messages::SequencedDataPacket*>(buf);
            handle_packet(ts, msg, buf, consumed_len);
        } break;
        case SBT::Types::DownstreamPacketType::SERVER_HEARTBEAT: {
            const SBT::Messages::ServerHeartbeat *msg = reinterpret_cast<const SBT::Messages::ServerHeartbeat*>(buf);
            handle_packet(ts, msg);
        } break;
        case SBT::Types::DownstreamPacketType::END_OF_SESSION: {
            const SBT::Messages::EndOfSession *msg = reinterpret_cast<const SBT::Messages::EndOfSession*>(buf);
            handle_packet(ts, msg);
        } break;
        default: {
            std::cerr << "OUCH42Session " << name() << " skipping invalid packet type ";
            if (std::isprint(static_cast<char>(hdr->packet_type))) {
                std::cerr << (char) hdr->packet_type;
            } else {
                std::cerr << ' ';
            }
            std::cerr << "(" << std::hex << (int) hdr->packet_type << std::dec << ")" << std::endl;
        } break;
    }
    return consumed_len;
}

void OUCH42Session::on_recv(const core::Timestamp& ts, void * I01_UNUSED ud, const std::uint8_t *buf, const ssize_t & len)
{
    // TODO: use ud (userdata).
    if (buf == nullptr) // || !connected())
        return;

    core::LockGuard<Mutex> l(m_mutex);
    m_last_message_received_ts = ts;
    // If we don't have enough bytes to construct a header, defer:
    if (UNLIKELY(m_dangly_length + len < (ssize_t)sizeof(SBT::Messages::PacketHeader))) {
        defer_bytes((const char*)buf, len);
        return;
    }
    const char *cur = (const char*)buf;
    size_t curlen = len;
    if (m_dangly_length > 0) {
        // Dangly bytes present, try to dispatch one packet.
        if (m_dangly_length < (ssize_t)sizeof(SBT::Messages::PacketHeader)) {
            // Defer the remainder of the header:
            ssize_t buf_off = sizeof(SBT::Messages::PacketHeader) - m_dangly_length;
            defer_bytes((const char *)buf, buf_off);
            cur += buf_off;
            curlen -= buf_off;
        }
        auto *ph = reinterpret_cast<SBT::Messages::PacketHeader *>(m_dangly_bytes.data());
        size_t premlen = ntohs(ph->packet_length) + sizeof(ph->packet_length) - sizeof(*ph);
        if (UNLIKELY(curlen < premlen)) {
            // We have the header, but not the rest of the whole packet.
            defer_bytes(cur, curlen);
            return;
        }
        // We have a dangly header and at least one full packet:
        defer_bytes(cur, premlen);
        cur += premlen;
        curlen -= premlen;
        dispatch_packet(ts, m_dangly_bytes.data(), 0);
    }
    // We have remaining bytes left over, dispatch as many packets as we
    // can, and dangle what's left:
    size_t dispatch_ret = 0;
    do {
        dispatch_ret = dispatch_packet(ts, cur, curlen);
        cur += dispatch_ret;
        curlen -= dispatch_ret;
    } while (dispatch_ret > 0);
    defer_bytes(cur, curlen);
}

void OUCH42Session::handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::LoginAccept * msg)
{
    bool was_connected = __sync_val_compare_and_swap((int*)&m_session_state, (int)State::CONNECTED, (int)State::LOGGED_IN);
    if (!was_connected)
        return;

    ::memcpy(m_persist->data()->session.data(), msg->session.data(), sizeof(msg->session));
    std::uint64_t seqnum = 0;
    for (size_t i = 0; i < sizeof(msg->sequence_number); ++i) {
        if (msg->sequence_number[i] == ' ')
            continue;
        else if (msg->sequence_number[i] >= '0' && msg->sequence_number[i] <= '9') {
            seqnum = (seqnum * 10) + (msg->sequence_number[i] - '0');
        } else {
            std::cerr << "OUCH42Session " << name() << " encountered invalid LoginAccept sequence number " << std::string((const char*)msg->sequence_number.data(), msg->sequence_number.size()) << " at " << i << std::endl;
            m_active = false;
            m_session_state = State::ERROR;
            disconnect(true);
            return;
        }
    }
    // TODO: recovery / seqnum mismatch logic.
    m_persist->data()->last_seqnum_received = seqnum - 1;
    using i01::core::operator<<;
    std::cerr << "OUCH42Session " << name() << " logged in to session " << m_persist->data()->session << " at seqnum " << m_persist->data()->last_seqnum_received + 1 << std::endl;
    m_active = true;
}

void OUCH42Session::handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::LoginReject * msg)
{
    m_active = false;
    m_session_state = State::LOGIN_REJECTED;
    std::cerr << "OUCH42Session " << name() << " login rejected: " << (char)msg->reason_code << "." << std::endl;
}

void OUCH42Session::handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::SequencedDataPacket * msg, const char *buf, size_t len)
{
    ++m_persist->data()->last_seqnum_received;
    // OUCH processing happens here:
    if (len <= sizeof(*msg)) {
        std::cerr << "OUCH42Session " << name() << " encountered empty SequencedDataPacket: " << m_persist->data()->last_seqnum_received << std::endl;
        return;
    }

    namespace M = OUCH42::Messages;
    namespace T = OUCH42::Types;
    typedef M::OutboundMessageHeader MH;
    typedef M::OutboundOrderMessageHeader OMH;
    typedef T::OutboundMessageType MT;
    if (UNLIKELY(len < sizeof(*msg) + sizeof(MH))) {
        std::cerr << "OUCH42Session " << name() << " encountered runt message with seqnum " << m_persist->data()->last_seqnum_received << " of size " << len << std::endl;
    }
    const MH *mh = reinterpret_cast<const MH*>(buf + sizeof(*msg));
    LocalID id = 0;
    NASDAQOrder * o_p = nullptr;
    size_t expected_len = sizeof(MH);
    char hpr_marking = 0;
    switch (mh->message_type) {
        // these msg types start with OrderTokens...
        case MT::ACCEPTED:
            expected_len = sizeof(M::Accepted);
            goto order_token_check;
        case MT::REPLACED:
            expected_len = sizeof(M::Rejected);
            goto order_token_check;
        case MT::CANCELED:
            expected_len = sizeof(M::Canceled);
            goto order_token_check;
        case MT::AIQ_CANCELLED:
            expected_len = sizeof(M::AIQCanceled);
            goto order_token_check;
        case MT::EXECUTED:
            expected_len = sizeof(M::Executed);
            goto order_token_check;
        case MT::BROKEN_TRADE:
            expected_len = sizeof(M::BrokenTrade);
            goto order_token_check;
        case MT::REJECTED:
            expected_len = sizeof(M::Rejected);
            goto order_token_check;
        case MT::CANCEL_PENDING:
            expected_len = sizeof(M::CancelPending);
            goto order_token_check;
        case MT::CANCEL_REJECT:
            expected_len = sizeof(M::CancelReject);
            goto order_token_check;
        case MT::ORDER_PRIORITY_UPDATE:
            expected_len = sizeof(M::OrderPriorityUpdate);
            goto order_token_check;
        case MT::ORDER_MODIFIED:
            expected_len = sizeof(M::Modified);
            goto order_token_check;
        case MT::SYSTEM_EVENT:
            goto message_size_check;
order_token_check:
        {
            if (len >= sizeof(*msg) + sizeof(OMH)) {
                const auto *omh = reinterpret_cast<const OMH*>(mh);
                hpr_marking = omh->order_token.hprdata.hpr_marking;
                id = omh->order_token.hprdata.id.get<LocalID>();
                o_p = static_cast<NASDAQOrder*>(get_order(id));
                if (id == 0) {
                    std::cerr << "OUCH42Session " << name() << " encountered message "
                              << (char)mh->message_type << " with invalid OrderToken with seqnum "
                              << m_persist->data()->last_seqnum_received << " and token "
                              << std::string((const char*)omh->order_token.raw, sizeof(omh->order_token))
                              << " and localid " << id << std::endl;

                    return;
                }
                if (o_p == nullptr) {
                    // only a problem if this is not an ACCEPTED order
                    if (mh->message_type == MT::ACCEPTED) {
                        if ('Z' == hpr_marking) {
                            // this is a riskbot reject ... OM will just update the local id and then we're done here
                            if (!m_order_manager_p->adopt_orphan_order(name(), id, nullptr)) {
                                std::cerr << "OUCH42Session " << name() << " riskbot rejected adopt orphan order failed id=" << id << " " << *omh << std::endl;
                            }
                            return;
                        } else {
                            // not rejected, but we have to create this order
                            const auto *m = reinterpret_cast<const M::Accepted*>(mh);
                            o_p = create_replay_order(ts, id, *m);

                        }
                    } else {
                        std::cerr << "OUCH42Session " << name() << " encountered message "
                                  << (char)mh->message_type << " with invalid OrderToken with seqnum "
                                  << m_persist->data()->last_seqnum_received << " and token "
                                  << std::string((const char*)omh->order_token.raw, sizeof(omh->order_token))
                                  << " and localid " << id << " (could be replay)" << std::endl;
                    }
                    std::cerr << "OUCH42Session " << name() << " encountered message which we could not create a replay order for: " << *mh << std::endl;
                    return;
                } else {
                    if (hpr_marking == 'Z') {
                        // rejected by HPR before reaching the exchange.
                        // TODO: add reject reason to order manager callback.
                        m_order_manager_p->on_rejected(o_p, ts, omh->exch_time.get());
                        return;
                    }
                }
            }
        }
message_size_check:
        {
            if (len < sizeof(*msg) + expected_len) {
                std::cerr << "OUCH42Session " << name() << " encountered runt message: " << (char)mh->message_type << " with seqnum " << m_persist->data()->last_seqnum_received << std::endl;
                return;
            }
        }
        break;
        default: {
            std::cerr << "OUCH42Session " << name() << " encountered unexpected message type " << (char)mh->message_type << " at seqnum " << m_persist->data()->last_seqnum_received << std::endl;
            return;
        } break;
    }
    // Length safety checks complete, OrderToken converted to LocalID where
    // relevant...  Continuing:
    switch (mh->message_type) {
        case MT::SYSTEM_EVENT: {
            const auto *m = reinterpret_cast<const M::SystemEvent*>(mh);
            std::cerr << "OUCH42Session " << name() << " SystemEvent: " << (char)m->system_event_code << std::endl;
            // TODO: keep track of market hours, system hours, etc.
        } break;
        case MT::ACCEPTED: {
            const auto *m = reinterpret_cast<const M::Accepted*>(mh);
            debug_print_msg("ACC",*m);
            o_p->m_price = m->price.get_limit();
            switch(be32toh(m->time_in_force.raw)) {
                case (T::TimeInForceRaw)T::TimeInForceSpecial::IMMEDIATE_OR_CANCEL:
                    if (m->cross_type == T::CrossType::CLOSING_CROSS)
                        o_p->m_tif = OE::TimeInForce::AUCTION_CLOSE;
                    else if (m->cross_type == T::CrossType::OPENING_CROSS)
                        o_p->m_tif = OE::TimeInForce::AUCTION_OPEN;
                    else if (m->cross_type == T::CrossType::HALT_OR_IPO_CROSS)
                        o_p->m_tif = OE::TimeInForce::AUCTION_HALT;
                    else if (m->minimum_quantity == m->shares)
                        o_p->m_tif = OE::TimeInForce::FILL_OR_KILL;
                    else if (m->minimum_quantity != m->shares)
                        o_p->m_tif = OE::TimeInForce::IMMEDIATE_OR_CANCEL;
                    break;
                case (T::TimeInForceRaw)T::TimeInForceSpecial::MARKET_HOURS:
                    o_p->m_tif = OE::TimeInForce::DAY;
                    break;
                case (T::TimeInForceRaw)T::TimeInForceSpecial::SYSTEM_HOURS:
                    o_p->m_tif = OE::TimeInForce::EXT;
                    break;
                default:
                    o_p->m_tif = OE::TimeInForce::TIMED;
                    o_p->m_nasdaq_tif_timed = be32toh(m->time_in_force.raw);
                    break;
            }
            o_p->m_nasdaq_display = m->display;
            o_p->m_nasdaq_capacity = m->capacity;
            o_p->m_iso_eligible = m->iso_eligibility == T::IntermarketSweepEligibility::ELIGIBLE;
            o_p->m_nasdaq_min_size = be32toh(m->minimum_quantity);
            o_p->m_bbo_weight_indicator = (char)m->bbo_weight_indicator;
            switch (m->order_state) {
                case T::OrderState::LIVE:
                    m_order_manager_p->on_acknowledged(
                                                       o_p,
                                                       be64toh(m->order_reference_number),
                                                       be32toh(m->shares),
                                                       ts,
                                                       m->hdr.exch_time.get()
                                                       );
                    break;
                case T::OrderState::DEAD:
                    m_order_manager_p->on_acknowledged(
                                                       o_p,
                                                       be64toh(m->order_reference_number),
                                                       be32toh(m->shares),
                                                       ts,
                                                       m->hdr.exch_time.get());
                    m_order_manager_p->on_cancel(o_p, o_p->size(), ts, m->hdr.exch_time.get());
                    o_p->m_state = OrderState::CANCELLED;
                    break;
                default:
                    std::cerr << "OUCH42Session " << name() << " encountered Accepted with zombie OrderState on seqnum " << m_persist->data()->last_seqnum_received << " and token " << id << " and refnum " << be64toh(m->order_reference_number) << ": " << (char)m->order_state << std::endl;
                    return;
            }
        } break;
        case MT::REPLACED: {
            const auto *m = reinterpret_cast<const M::Replaced*>(mh);
            debug_print_msg("REP",*m);
            LocalID previd = m->previous_order_token.hprdata.id.get<LocalID>();
            NASDAQOrder * po_p = static_cast<NASDAQOrder*>(get_order(previd));
            if (previd == 0 || po_p == nullptr) {
                std::cerr << "OUCH42Session " << name() << " encountered Replaced for invalid previous OrderToken with seqnum " << m_persist->data()->last_seqnum_received << " and previous token " << std::string((const char*)m->previous_order_token.raw, sizeof(m->previous_order_token))  << " and localid " << id << std::endl;
                return;
            }
            std::cerr << "OUCH42Session " << name() << " encountered Replaced on seqnum " << m_persist->data()->last_seqnum_received << " and prevtoken " << previd << " newtoken " << id << " and refnum " << be64toh(m->order_reference_number) << ": " << (char)m->order_state << std::endl;

            // TODO: support on_cxlreplace in order manager.
            po_p->m_state = OrderState::CANCEL_REPLACED;
            m_order_manager_p->on_cancel(po_p, po_p->open_size(), ts, m->hdr.exch_time.get());
            po_p->m_state = OrderState::CANCEL_REPLACED;
            if (m->order_state == T::OrderState::DEAD && !o_p->is_terminal()) {
                m_order_manager_p->on_cancel(o_p, o_p->open_size(), ts, m->hdr.exch_time.get());
            } else if (m->order_state == T::OrderState::LIVE) {
                m_order_manager_p->on_acknowledged(
                                                   o_p,
                                                   be64toh(m->order_reference_number),
                                                   be32toh(m->shares),
                                                   ts,
                                                   m->hdr.exch_time.get()
                                                   );
            }
        } break;
        case MT::AIQ_CANCELLED:
            // TODO: Add any separate AIQ cancellation logic (e.g. STP alerts).
        case MT::CANCELED: {
            const auto *m = reinterpret_cast<const M::Canceled*>(mh);
            debug_print_msg("CXL",*m);
            o_p->m_cancel_reason = (char)m->reason;
            m_order_manager_p->on_cancel(o_p, be32toh(m->decremented_shares), ts,
                                         m->hdr.exch_time.get());
        } break;
        case MT::EXECUTED: {
            const auto *m = reinterpret_cast<const M::Executed*>(mh);
            debug_print_msg("EXE",*m);
            // TODO: make FillFeeCode less of a hack...
            auto fc = static_cast<OE::FillFeeCode>(0x000000FF & (std::uint8_t)(m->liquidity_flag));
            o_p->m_liquidity_flag = m->liquidity_flag;
            o_p->m_match_number = be64toh(m->match_number);
            m_order_manager_p->on_fill(o_p,
                                       be32toh(m->executed_shares),
                                       m->execution_price.get_limit(),
                                       ts,
                                       m->hdr.exch_time.get(),
                                       fc);
        } break;
        case MT::BROKEN_TRADE: {
            const auto *m = reinterpret_cast<const M::BrokenTrade*>(mh);
            debug_print_msg("BRK",*m);
            std::cerr << "OUCH42Session " << name() << " encountered BrokenTrade for " << std::string((const char*)m->hdr.order_token.raw, sizeof(m->hdr.order_token)) << " id " << id << " matchnumber " << be64toh(m->match_number) << " reason " << (char)m->reason <<  std::endl;
        } break;
        case MT::REJECTED: {
            const auto *m = reinterpret_cast<const M::Rejected*>(mh);
            debug_print_msg("REJ",*m);
            std::cerr << "OUCH42Session " << name() << " encountered Reject for " << id << " reason " << (char)m->reason << std::endl;
            m_order_manager_p->on_rejected(o_p, ts, m->hdr.exch_time.get());
        } break;
        case MT::CANCEL_PENDING: {
            const auto *m = reinterpret_cast<const M::CancelPending*>(mh);
            debug_print_msg("CXLPND",*m);
            // TODO: CancelPending: sent in response to a cancel request for a
            // cross order during a pre-cross late period signifying that it
            // cannot be canceled at this time, but any unexecuted portion of
            // the order will be cancelled after the cross.  we do not have
            // any callbacks for this case in order manager.
            // TODO: add a parameter to cancelrejected, e.g. "soft" = true -->
            // partial success of cancel.
            std::cerr << "OUCH42Session " << name() << " cancel is pending for " << id << std::endl;
            m_order_manager_p->on_cancel_rejected(o_p, ts, m->hdr.exch_time.get());
        } break;
        case MT::CANCEL_REJECT: {
            const auto *m = reinterpret_cast<const M::CancelReject*>(mh);
            debug_print_msg("CXLREJ",*m);
            (void)m; // TODO: handle cancel rejects better.
            m_order_manager_p->on_cancel_rejected(o_p, ts, m->hdr.exch_time.get());
        } break;
        case MT::ORDER_PRIORITY_UPDATE: {
            const auto *m = reinterpret_cast<const M::OrderPriorityUpdate*>(mh);
            debug_print_msg("OPU",*m);
            o_p->m_price = m->execution_price.get_limit();
            o_p->m_nasdaq_display = m->display;
            o_p->m_exchangeID = be64toh(m->order_reference_number);
            // TODO: create an order manager callback for order priority
            // updates!
        } break;
        case MT::ORDER_MODIFIED: {
            const auto *m = reinterpret_cast<const M::Modified*>(mh);
            debug_print_msg("MOD",*m);
            // TODO: create m_order_manager_p->on_modified...
            switch (m->buy_sell_indicator) {
                case T::BuySellIndicator::BUY:
                    o_p->m_side = OE::Side::BUY;
                    break;
                case T::BuySellIndicator::SELL:
                    o_p->m_side = OE::Side::SELL;
                    break;
                case T::BuySellIndicator::SHORT:
                    o_p->m_side = OE::Side::SHORT;
                    break;
                case T::BuySellIndicator::SHORT_EXEMPT:
                    o_p->m_side = OE::Side::SHORT_EXEMPT;
                    break;
                default:
                    std::cerr << "OUCH42Session " << name() << " encountered unknown buy sell indicator modifying " << id << ": " << (char) m->buy_sell_indicator << std::endl;
                    return;
            }
            o_p->m_open_size = be32toh(m->shares);
            o_p->m_last_response_time = ts;
            o_p->m_last_exchange_time = m->hdr.exch_time.get();
        } break;
        default: {
            std::cerr << "OUCH42Session " << name() << " encountered invalid OUCH message type: " << (char)(mh->message_type) << " at " << m_persist->data()->last_seqnum_received << std::endl;
        } break;
    }
}

void OUCH42Session::handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::ServerHeartbeat * msg)
{
    // nothing to see here...
}

void OUCH42Session::handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::EndOfSession * msg)
{
    std::cerr << "OUCH42Session " << name() << " received end of session." << std::endl;
    m_active = false;
    m_session_state = State::DISCONNECTED;
    disconnect(false);
}


Instrument * OUCH42Session::find_instrument(const OUCH42::Types::StockArray& stock)
{
    auto it = m_symbol_inst_cache.find(stock);
    if (it == m_symbol_inst_cache.end()) {
        return nullptr;
    }
    return it->second;
}

NASDAQOrder * OUCH42Session::create_replay_order(const Timestamp& ts, LocalID id, const O42::Messages::Accepted& msg)
{
    NASDAQOrder *op = nullptr;

    // look up the symbol ...
    auto *inst = find_instrument(msg.stock.arr);

    if (nullptr == inst) {
        std::cerr << "OUCH42Session " << name() << " could not find instrument for replay order " << msg << std::endl;
    } else {
        op = static_cast<NASDAQOrder*>(on_create_replay_order(ts, inst, market(), 0 /*local account*/, id,
                                                              msg.price.get_limit(),
                                                              be32toh(msg.shares),
                                                              O42::Types::to_i01_side(msg.buy_sell_indicator),
                                                              msg.time_in_force.to_i01(),
                                                              OE::OrderType::LIMIT,
                                                              nullptr));
        if (!m_order_manager_p->adopt_orphan_order(name(), id, op)) {
            std::cerr << "OUCH42Session " << name() << " order manager returned error trying to adopt orphan order " << msg << std::endl;
        }
    }

    return op;
}

inline
bool OUCH42Session::send_order_data(NASDAQOrder* o_p, const char*buf, size_t len)
{
    ssize_t slen = m_connection.send((const uint8_t*) buf, len);
    if (slen < 0)
        perror("OUCH42Session");
    if ((ssize_t)len == slen) {
        Timestamp t = Timestamp::now();
        m_last_message_sent_ts = t;
        if (o_p)
            o_p->last_request_time(t);
        return true;
    }
    std::cerr << "OUCH42Session " << name() << " failed to send " << len << " bytes." << std::endl;
    return false;
}

Order * OUCH42Session::on_create_replay_order(const Timestamp& ts, Instrument *inst, const core::MIC& mic,
                                              const LocalAccount local_account, const LocalID local_id,
                                              const OE::Price price, const OE::Size size, const OE::Side side,
                                              const OE::TimeInForce tif, const OrderType type,
                                              const UserData ud)
{
    // TODO: check the MIC
    if (mic != core::MICEnum::XNAS) {
        std::cerr << m_name << ",ERR,CREATE REPLAY ORDER,MIC IS NOT XNAS," << mic << std::endl;
        return nullptr;
    }
    auto op = m_order_manager_p->create_order<NASDAQOrder>(inst, price, size, side, tif, type, nullptr, nullptr);
    op->session(this);
    op->sent_time(ts);
    op->localID(local_id);
    O42::Messages::OrderToken order_token;
    if(create_order_token_hpr(op, order_token)) {
        op->client_order_id(order_token.arr);
    }
    return op;
}

std::string OUCH42Session::to_string() const
{
    std::ostringstream ss;
    {
        core::LockGuard<Mutex> l(m_mutex);
        ss << m_session_state << ","
           << "INSEQ," << m_persist->data()->last_seqnum_received;
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const OUCH42Session::State& s)
{
    switch (s) {
    case OUCH42Session::State::TIMED_OUT:
        return os << "TIMED_OUT";

    case OUCH42Session::State::LOGIN_REJECTED:
        return os << "LOGIN_REJECTED";

    case OUCH42Session::State::DISCONNECTED:
        return os << "DISCONNECTED";

    case OUCH42Session::State::ERROR:
        return os << "ERROR";

    case OUCH42Session::State::UNKNOWN:
        return os << "UNKNOWN";

    case OUCH42Session::State::INITIALIZED:
        return os << "INITIALIZED";

    case OUCH42Session::State::CONNECTING:
        return os << "CONNECTING";

    case OUCH42Session::State::CONNECTED:
        return os << "CONNECTED";

    case OUCH42Session::State::LOGGED_IN:
        return os << "LOGGED_IN";
    default:
        return os;
    }
}

} } }
