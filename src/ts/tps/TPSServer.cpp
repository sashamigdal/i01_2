#include <string.h>

#include <iostream>
#include <unordered_map>

#include <boost/lexical_cast.hpp>

#include <i01_core/Time.hpp>
#include <i01_core/MIC.hpp>

#include <i01_md/DataManager.hpp>
#include <i01_md/OrderData.hpp>

#include <i01_oe/OrderManager.hpp>

#include "TPSServer.hpp"

namespace i01 { namespace TS { namespace TPS {

const char * Event::EXPECTED_VERSION_STRING = "EVS111111221";
const SerializeVersion EventBuf::EXPECTED_VERSION = {0, 1};
const SerializeVersion NetEventBuf::EXPECTED_VERSION = {0, 1};
const SerializeVersion HandshakeEventBuf::EXPECTED_VERSION = {0,1};
const SerializeVersion SubscriptionEventBuf::EXPECTED_VERSION = {0,1};
const SerializeVersion MarketEventBaseBuf::EXPECTED_VERSION = {0,1};
const SerializeVersion NetMarketQuoteEventBuf::EXPECTED_VERSION = {0,1};
const SerializeVersion NetMarketTradeEventBuf::EXPECTED_VERSION = {0,1};
const SerializeVersion SnapshotRequestEventBuf::EXPECTED_VERSION = {0,2};

const char * ClientSession::RESPONSE_HANDSHAKE_OK = "OK";
const char * ClientSession::RESPONSE_HANDSHAKE_DENIED = "DENIED";

const ClientSession::FDOExchangeMap ClientSession::m_fdo_exchange = {
    FDOExchange::UNKNOWN
    , FDOExchange::XNYS
    , FDOExchange::XNAS
    , FDOExchange::ARCX
    , FDOExchange::XBOS
    , FDOExchange::XPSX
    , FDOExchange::BATY
    , FDOExchange::BATS
    , FDOExchange::EDGA
    , FDOExchange::EDGX
    , FDOExchange::CAES
};

TimeType TimeType::from_timestamp(const core::Timestamp& ts)
{
    auto ns = ts.nanoseconds_since_midnight();
    return {ns/1000000000ULL, ns % 1000000000ULL};
}

TPSServer::TPSServer(OE::OrderManager* omp, MD::DataManager* dmp) :
    m_omp(omp),
    m_dmp(dmp)
{
    m_esi_fdoid_map.fill(0);

    for (const auto& e : m_omp->universe().valid_esi()) {
        try {
            auto fdoid = boost::lexical_cast<std::uint32_t>(m_omp->universe()[e].fdo_symbol_string());
            m_fdoid_esi_map[fdoid] = e;
            m_esi_fdoid_map[e] = fdoid;
        } catch (boost::bad_lexical_cast&) {
            // we ignore this symbol
        }
    }
}

core::Date TPSServer::date() const
{
    return m_dmp->date();
}

void * TPSServer::on_new_client(int fd)
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ true);

    m_clients.emplace_back(ClientUniquePtr{new ClientSession(this, m_clients.size(), fd)});

    std::cout << "TPSServer new client " << m_clients.size() << std::endl;
    return reinterpret_cast<void*>(m_clients.back().get());
}

void TPSServer::on_peer_disconnect(const core::Timestamp& ts, void * ud)
{
    auto * cp = reinterpret_cast<ClientSession*>(ud);

    std::cerr << "TPSServer: on_peer_disconnect " << ts << " " << *cp << std::endl;
    cp->disconnect();
}


void TPSServer::on_recv(const core::Timestamp & ts, void * ud, const std::uint8_t *buf, const ssize_t & len)
{
    auto* cp = reinterpret_cast<ClientSession*>(ud);
    std::cerr << "TPSServer: on_recv: " << ts << " client: " <<  *cp << " len: " << len << std::endl;

    cp->handle_raw_bytes(ts, buf, static_cast<const std::size_t>(len));
}


void TPSServer::on_quote_update(const core::Timestamp& ts,
                                const core::MIC& mic,
                                MD::EphemeralSymbolIndex esi,
                                const MD::FullL2Quote& q)
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ false);

    // see if a client is subscribed to this
    if (m_instrument_clients[esi].size()) {
        for (const auto& cid : m_instrument_clients[esi]) {
            m_clients[cid]->write_quote(ts, mic, esi, m_esi_fdoid_map[esi], q);
        }
    }
}

void TPSServer::on_trade_update(const core::Timestamp& ts,
                                const core::MIC& mic,
                                MD::EphemeralSymbolIndex esi,
                                const MD::TradePair& tp)
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ false);

    // see if a client is subscribed to this
    if (m_instrument_clients[esi].size()) {
        for (const auto& cid : m_instrument_clients[esi]) {
            m_clients[cid]->write_trade(ts, mic, esi, m_esi_fdoid_map[esi], tp);
        }
    }
}



MD::EphemeralSymbolIndex TPSServer::add_subscription(const ClientID& cid, std::uint32_t fdo_id)
{
    // look up the instrument
    auto it = m_fdoid_esi_map.find(fdo_id);
    if (it != m_fdoid_esi_map.end()) {
        Mutex::scoped_lock lock(m_mutex, /*write=*/ true);
        m_instrument_clients[it->second].push_back(cid);
        return it->second;
    }
    return 0;
}

bool TPSServer::is_subscribed(const ClientID &cid, std::uint32_t fdo_id)
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ false);
    auto it = m_fdoid_esi_map.find(fdo_id);
    if (it != m_fdoid_esi_map.end()) {
        for (const auto& c : m_instrument_clients[it->second]) {
            if (c == cid) {
                return true;
            }
        }
    }
    return false;
}

void TPSServer::remove_subscription(const ClientID& cid, MD::EphemeralSymbolIndex esi)
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ true);

    // for a given ESI, remove this client ... order of clients
    // doesn't matter, so just put the last client in this spot and
    // pop the back
    auto& v = m_instrument_clients[esi];
    for (auto i = 0U; i < v.size(); i++) {
        if (v[i] == cid) {
            v[i] = v.back();
            v.pop_back();
            break;
        }
    }
}

ClientSession::ClientSession(TPSServer* server, const ClientID& iid, int fd) :
    m_server(server),
    m_id(iid),
    m_fd(fd),
    m_state(State::PRE_HANDSHAKE),
    m_date{0},
    m_tz_offset_secs{0}
{
    m_date = date_from_yyyymmdd(static_cast<std::uint32_t>(m_server->date().yyyy()),
                                static_cast<std::uint32_t>(m_server->date().mm()),
                                static_cast<std::uint32_t>(m_server->date().dd()));

    // TODO: need to compute a time zone offset to adjust the core::Timestamps to localtime
    struct tm tmt;
    ::memset(&tmt, 0, sizeof(tmt));
    tmt.tm_year = static_cast<std::uint32_t>(m_server->date().yyyy() - 1900ULL);
    tmt.tm_mon = static_cast<std::uint32_t>(m_server->date().mm()-1ULL);
    tmt.tm_mday = static_cast<std::uint32_t>(m_server->date().dd());
    tmt.tm_sec = 0;
    tmt.tm_min = 0;
    tmt.tm_hour = 0;
    tmt.tm_isdst = -1; // mktime should figure it out

    auto tt_local = mktime(&tmt);
    auto tt_utc = timegm(&tmt);

    m_tz_offset_secs = tt_local - tt_utc;
}

void ClientSession::disconnect()
{
    std::cerr << "ClientSession," << *this << ",disconnect" << std::endl;

    m_state = State::DISCONNECTED;
    m_fd = -1;

    for (auto e : m_subscribed_esi) {
        m_server->remove_subscription(m_id, e);
    }
}

void ClientSession::write_quote(const core::Timestamp& ts, const core::MIC& m,
                                MD::EphemeralSymbolIndex esi, const std::uint32_t fdoid,
                                const MD::FullL2Quote& q)
{
    MarketQuoteEvent me(ts - core::Timestamp(m_tz_offset_secs,0), m_date, fdoid, q);

    send_or_disconnect(me);
}

void ClientSession::write_trade(const core::Timestamp& ts, const core::MIC& m,
                                MD::EphemeralSymbolIndex esi, const std::uint32_t fdoid,
                                const MD::TradePair& tp)
{
    MarketTradeEvent te(ts - core::Timestamp(m_tz_offset_secs, 0), m_date, fdoid, tp.first, tp.second,
                           static_cast<std::uint32_t>(m_fdo_exchange[m.index()]));
    send_or_disconnect(te);
}

int ClientSession::write_buffer(const std::uint8_t* buf, std::size_t len, bool blocking)
{
    auto err = int{0};
    auto end_p = buf + len;
    auto bufidx = buf;

    do {
        auto num = ::write(m_fd, bufidx, end_p - bufidx);
        if (num >= 0) {
            bufidx += num;
        } else {
            err = errno;
            if (EAGAIN == err || EWOULDBLOCK == err) {
                if (!blocking) {
                    return -err;
                }
            } else {
                return -err;
            }
        }
    } while (bufidx < end_p);

    return 0;
}

std::ostream& operator<<(std::ostream& os, const ClientSession::State& cs)
{
    switch (cs) {
    case ClientSession::State::UNKNOWN:
        os << "UNKNOWN";
        break;
    case ClientSession::State::PRE_HANDSHAKE:
        os << "PRE_HANDSHAKE";
        break;
    case ClientSession::State::HANDSHAKE_OK:
        os << "HANDSHAKE_OK";
        break;
    case ClientSession::State::HANDSHAKE_DENIED:
        os << "HANDSHAKE_DENIED";
        break;
    case ClientSession::State::DISCONNECTED:
        os << "DISCONNECTED";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const ClientSession& c)
{
    return os << c.m_id << "," << c.m_state;
}

void ClientSession::handle_raw_bytes(const core::Timestamp& ts,
                                     const std::uint8_t * buf,
                                     const std::size_t len)
{
    std::size_t bytes_read;

    auto use_buf = buf;
    auto use_len = len;
    if (m_dangly_bytes.size()) {
        // we have bytes from previous read...
        // dangly bytes will always start on a message boundary...

        // copy this buffer onto the dangly bytes
        for (auto i = 0U; i < len; i++) {
            m_dangly_bytes.push_back(buf[i]);
        }
        use_buf = m_dangly_bytes.data();
        use_len = m_dangly_bytes.size();
    }

    auto start_buf = use_buf;
    while (1) {
        // we know that the event is always going to start from the
        // beginning of the buffer .. so this tells us whether there
        // are enough bytes there for the event type, and how many
        // bytes that is .... we will cast the buffer to determine which type of event it is

        // ep is only valid within the scope of handle_raw_bytes, b/c
        // it is referencing the input buffer

        auto ep = next_event_from_buffer(use_buf, use_len);
        if (ep && ep->valid()) {
            // ep points to an event
            handle_event(ts, ep.get());
            bytes_read = ep->bytes_read();
            use_buf += bytes_read;
            use_len -= bytes_read;
        } else {
            break;
        }
    }

    // use_len is now the number of bytes left over
    if (use_len) {
        // we have dangly bytes to deal with
        bytes_read = use_buf - start_buf;
        if (m_dangly_bytes.size()) {
            // shift the unread bytes to the beginning
            for (auto i = 0U; i < use_len; i++) {
                m_dangly_bytes[i] = m_dangly_bytes[bytes_read + i];
            }

            // clear out the vector except the bytes we just saved
            m_dangly_bytes.erase(m_dangly_bytes.begin()+use_len, m_dangly_bytes.end());
        } else {
            for (auto i = 0U; i < use_len; i++) {
                m_dangly_bytes.push_back(use_buf[i]);
            }
        }
    }
}

std::unique_ptr<EventBufWrapper> next_event_from_buffer(const std::uint8_t* buf, const std::size_t len)
{
    if (sizeof(EventBufHeader) <= len) {
        auto evth = reinterpret_cast<const EventBufHeader*>(buf);

        // now we have the EventType, use it to figure out the rest
        switch (evth->event_type) {
        case EventType::MARKET_EVENT:
            break;
        case EventType::ORDER_STATUS_EVENT:
            break;
        case EventType::NASDAQ_CLOSING_CROSS_EVENT:
            break;
        case EventType::PARENT_ORDER_STATUS_EVENT:
            break;
        case EventType::ROOT_ORDER_STATUS_EVENT:
            break;
        case EventType::NET_EVENT:
            break;
        case EventType::SUBSCRIPTION_EVENT:
            return std::unique_ptr<SubscriptionEventBufWrapper>(new SubscriptionEventBufWrapper(buf,len));
            break;
        case EventType::SNAPSHOT_REQUEST_EVENT:
            return std::unique_ptr<SnapshotRequestEventBufWrapper>(new SnapshotRequestEventBufWrapper(buf, len));
           break;
        case EventType::SNAPSHOT_EVENT:
            break;
        case EventType::NET_MARKET_EVENT:
            break;
        case EventType::HANDSHAKE_EVENT:
            return std::unique_ptr<HandshakeEventBufWrapper>(new HandshakeEventBufWrapper(buf, len));
            break;
        case EventType::EXPRET_EVENT:
            break;
        default:
            break;
        }
    }
    return nullptr;
}

void ClientSession::handle_event(const core::Timestamp& ts, const EventBufWrapper* ep)
{
    switch(ep->event_type()) {
    case EventType::MARKET_EVENT:
        break;
    case EventType::ORDER_STATUS_EVENT:
        break;
    case EventType::NASDAQ_CLOSING_CROSS_EVENT:
        break;
    case EventType::PARENT_ORDER_STATUS_EVENT:
        break;
    case EventType::ROOT_ORDER_STATUS_EVENT:
        break;
    case EventType::NET_EVENT:
        break;
    case EventType::SUBSCRIPTION_EVENT:
        handle_subscription_event(ts, static_cast<const SubscriptionEventBufWrapper*>(ep));
        break;
    case EventType::SNAPSHOT_REQUEST_EVENT:
        handle_snapshot_request_event(ts, static_cast<const SnapshotRequestEventBufWrapper*>(ep));
        break;
    case EventType::SNAPSHOT_EVENT:
        break;
    case EventType::NET_MARKET_EVENT:
        break;
    case EventType::HANDSHAKE_EVENT:
        handle_handshake_event(ts, static_cast<const HandshakeEventBufWrapper*>(ep));
        break;
    case EventType::EXPRET_EVENT:
        break;
    default:
        break;
    }
}

std::ostream& operator<<(std::ostream& os, const EventType& e)
{
    switch(e) {
    case EventType::MARKET_EVENT:
        os << "MARKET_EVENT";
        break;
    case EventType::ORDER_STATUS_EVENT:
        os << "ORDER_STATUS_EVENT";
        break;
    case EventType::NASDAQ_CLOSING_CROSS_EVENT:
        os << "NASDAQ_CLOSING_CROSS_EVENT";
        break;
    case EventType::PARENT_ORDER_STATUS_EVENT:
        os << "PARENT_ORDER_STATUS_EVENT";
        break;
    case EventType::ROOT_ORDER_STATUS_EVENT:
        os << "ROOT_ORDER_STATUS_EVENT";
        break;
    case EventType::NET_EVENT:
        os << "NET_EVENT";
        break;
    case EventType::SUBSCRIPTION_EVENT:
        os << "SUBSCRIPTION_EVENT";
        break;
    case EventType::SNAPSHOT_REQUEST_EVENT:
        os << "SNAPSHOT_REQUEST_EVENT";
        break;
    case EventType::SNAPSHOT_EVENT:
        os << "SNAPSHOT_EVENT";
        break;
    case EventType::NET_MARKET_EVENT:
        os << "NET_MARKET_EVENT";
        break;
    case EventType::HANDSHAKE_EVENT:
        os << "HANDSHAKE_EVENT";
        break;
    case EventType::EXPRET_EVENT:
        os << "EXPRET_EVENT";
        break;
    default:
        break;
    }
    return os;
}

void ClientSession::handle_handshake_event(const core::Timestamp& ts,
                                           const HandshakeEventBufWrapper* evt)
{
    const char* reply;
    // check that the serialize version in the event is what we expect
    if (Event::EXPECTED_VERSION_STRING == evt->version()) {
        m_state = State::HANDSHAKE_OK;
        reply = RESPONSE_HANDSHAKE_OK;
    } else {
        m_state = State::HANDSHAKE_DENIED;
        reply = RESPONSE_HANDSHAKE_DENIED;
    }

    // send back a handshake event...
    HandshakeEvent he(Event::EXPECTED_VERSION_STRING, reply);

    send_or_disconnect(he);
}

void ClientSession::handle_subscription_event(const core::Timestamp& ts,
                                              const SubscriptionEventBufWrapper* evt)
{
    if ((evt->flags() & static_cast<std::uint32_t>(SubscriptionEventBuf::Flags::REQUEST)) == 0) {
        // we expect the REQUEST flag to be set
        std::cerr << "ClientSession," << *this << ",got subscription request without request flag," << *evt << std::endl;
    } else {
        if (!m_server->is_subscribed(m_id, evt->inst().inst.inst_id)) {
            std::cerr << "ClientSession," << *this << ",sub event," << *evt << std::endl;
            auto esi = m_server->add_subscription(m_id, evt->inst().inst.inst_id);
            if (0 != esi) {
                m_subscribed_esi.push_back(esi);
            }
        }
    }
}

void ClientSession::handle_snapshot_request_event(const core::Timestamp& ts,
                                                  const SnapshotRequestEventBufWrapper* evt)
{
    std::cerr << "ClientSession," << *this << ",snap shot event," << *evt << std::endl;
}

EventBufWrapper::EventBufWrapper(const std::uint8_t* buf, const std::size_t len) :
    EventBufWrapper()
{
    if (sizeof(EventBuf) <= len) {
        m_event_buf = reinterpret_cast<decltype(m_event_buf)>(buf);
        if (m_event_buf->header.version.u16 == EventBuf::EXPECTED_VERSION.u16) {
            m_bytes_read += sizeof(EventBuf);
            m_valid = true;
        }
    }
}

NetEventBufWrapper::NetEventBufWrapper(const std::uint8_t* buf, const std::size_t len) :
    EventBufWrapper(buf, len),
    m_net_event_buf{nullptr}
{
    if (valid()) {
        if (sizeof(NetEventBuf) <= len) {
            m_net_event_buf = reinterpret_cast<const NetEventBuf *>(buf);
            if (m_net_event_buf->net_event.version.u16 == NetEventBuf::EXPECTED_VERSION.u16) {
                m_bytes_read = sizeof(NetEventBuf);
            }
        } else {
            m_valid = false;
            m_bytes_read = 0;
        }
    }
}

std::ostream& operator<<(std::ostream& os, const EventBufWrapper& e)
{
    return os << e.event_type() << "," << e.inst().inst.inst_id;
}

std::ostream& operator<<(std::ostream& os, const NetEventBufWrapper& w)
{
    return os << static_cast<const EventBufWrapper&>(w) << ","
              << w.m_net_event_buf->net_event.time.tt_secs << ","
              << w.m_net_event_buf->net_event.time.tt_nsecs;
}


std::ostream& operator<<(std::ostream& os, const SnapshotRequestEventBufWrapper& e)
{
    return os << *e.m_snapshot_req_buf;
}

HandshakeEventBufWrapper::HandshakeEventBufWrapper(const std::uint8_t* buf, const std::size_t len) :
    NetEventBufWrapper(buf, len),
    m_handshake_event_buf{nullptr},
    m_version_buf{nullptr},
    m_version_buf_len{0},
    m_alias_buf{nullptr},
    m_alias_buf_len{0}
{
    if (valid()) {
        // m_bytes_read has been updated by subclasses....
        if (len - m_bytes_read < sizeof(HandshakeEventBuf)) {
            // not enough bytes
            goto invalid;
        }
        m_handshake_event_buf = reinterpret_cast<const HandshakeEventBuf*>(buf);
        m_bytes_read = sizeof(HandshakeEventBuf);

        if (m_handshake_event_buf->handshake_event.version.u16 != HandshakeEventBuf::EXPECTED_VERSION.u16) {
            goto invalid;
        }

        m_version_buf_len = &m_handshake_event_buf->handshake_event.version_string_len;

        auto bufidx = reinterpret_cast<const std::uint8_t*>(m_version_buf_len);
        if (m_bytes_read + *m_version_buf_len <= len) {
            bufidx += sizeof(HandshakeEventBuf::Layout::version_string_len);
            m_version_buf = bufidx;
            m_bytes_read += *m_version_buf_len;
            bufidx += *m_version_buf_len;
       } else {
            goto invalid;
        }

        if (m_bytes_read + sizeof(*m_alias_buf_len) <= len) {
            m_alias_buf_len = reinterpret_cast<const std::uint16_t*>(bufidx);
            m_bytes_read += sizeof(*m_alias_buf_len);
            bufidx += sizeof(*m_alias_buf_len);
        } else {
            goto invalid;
        }

        if (m_bytes_read + *m_alias_buf_len <= len) {
            m_alias_buf = bufidx;
            m_bytes_read += *m_alias_buf_len;
        }
        return;
    }
    // here means invalid read of this buffer to this event type

 invalid:
    m_valid = false;
    m_bytes_read = 0;
}

std::ostream& operator<<(std::ostream& os, const HandshakeEventBufWrapper& h)
{
    return os << h.valid() << " br: " << h.bytes_read() << " "
              << std::hex << h.m_handshake_event_buf << std::dec;
}

std::ostream& operator<<(std::ostream& os, const HandshakeEvent& h)
{
    return os << h.event_type() << "," << h.m_version << "," << h.m_alias;
}

Event::Event(const EventType& et, EventBuf *p) :
    m_event_buf(p)
{
    m_event_buf->header.version = EventBuf::EXPECTED_VERSION;
    m_event_buf->header.event_type = et;
    m_event_buf->inst = Inst{0,-1};
    m_event_buf->date_time = DateTime{0,0};
}

Event::Event(const EventType& et, EventBuf *p, const DateType& d,
             const Inst& inst, const core::Timestamp& ts) :
    Event(et, p)
{
    m_event_buf->inst = inst;
    m_event_buf->date_time.date = d;
    m_event_buf->date_time.time = TimeType::from_timestamp(ts);
}

NetEvent::NetEvent(NetEventBuf *ne, EventType et) :
    Event(et, &ne->event)
    , m_net_event(ne)
{
    m_net_event->net_event.version = NetEventBuf::EXPECTED_VERSION;
    m_net_event->net_event.time = {0,0};
}

HandshakeEvent::HandshakeEvent(const std::string& version, const std::string& alias) :
    NetEvent(&m_tmp_handshake.net_event, EventType::HANDSHAKE_EVENT),
    m_data{nullptr},
    m_version(version),
    m_alias(alias)
{
    m_data = new std::uint8_t[num_bytes_needed()];
    m_handshake = reinterpret_cast<HandshakeEventBuf *>(m_data);
    m_handshake->net_event = m_tmp_handshake.net_event;

    m_handshake->handshake_event.version = HandshakeEventBuf::EXPECTED_VERSION;
    m_handshake->handshake_event.version_string_len = static_cast<std::uint16_t>(m_version.size());

    auto bufidx = reinterpret_cast<std::uint8_t*>(&m_handshake->handshake_event);
    bufidx += sizeof(HandshakeEventBuf::Layout);

    // now we write the version string data
    for (const auto& c : m_version) {
        *bufidx++ = c;
    }

    auto alias_len = reinterpret_cast<std::uint16_t*>(bufidx);
    *alias_len = static_cast<std::uint16_t>(m_alias.size());
    bufidx += sizeof(std::uint16_t);

    for (const auto& c : m_alias) {
        *bufidx++ = c;
    }
}

HandshakeEvent::~HandshakeEvent()
{
    delete[] m_data;
}

std::size_t HandshakeEvent::num_bytes_needed() const
{
    return sizeof(NetEventBuf)
        + sizeof(HandshakeEventBuf::Layout)
        + /* alias string size */ sizeof(std::uint16_t)
        + m_version.size()
        + m_alias.size();
}

BufferPair HandshakeEvent::get_serialize_buffer() const
{
    return std::make_pair(m_data, num_bytes_needed());
}

SubscriptionEventBufWrapper::SubscriptionEventBufWrapper(const std::uint8_t* buf,
                                                         const std::size_t len) :
    NetEventBufWrapper(buf, len),
    m_sub_event_buf{nullptr}
{
    if (valid()) {
        if (sizeof(SubscriptionEventBuf) <= len) {
            m_sub_event_buf = reinterpret_cast<const SubscriptionEventBuf*>(buf);
            if (m_sub_event_buf->subscription_event.version.u16 ==
                SubscriptionEventBuf::EXPECTED_VERSION.u16) {
                m_bytes_read = sizeof(SubscriptionEventBuf);
            }
        } else {
            m_valid = false;
            m_bytes_read = 0;
        }
    }
}

std::ostream& operator<<(std::ostream& os, const SubscriptionEventBufWrapper& e)
{
    return os << static_cast<const NetEventBufWrapper&>(e) << "," << e.flags();
}

SnapshotRequestEventBufWrapper::SnapshotRequestEventBufWrapper(const std::uint8_t* buf,
                                                               const std::size_t len) :
    NetEventBufWrapper(buf, len),
    m_snapshot_req_buf{nullptr}
{
    if (valid()) {
        if (sizeof(SnapshotRequestEventBuf) <= len) {
            m_snapshot_req_buf = reinterpret_cast<const SnapshotRequestEventBuf*>(buf);
            if (m_snapshot_req_buf->snapshot_req_event.version.u16 ==
                SnapshotRequestEventBuf::EXPECTED_VERSION.u16) {
                m_bytes_read = sizeof(SnapshotRequestEventBuf);
            }
        } else {
            m_valid = false;
            m_bytes_read = 0;
        }
    }
}


MarketQuoteEvent::MarketQuoteEvent(const core::Timestamp& ts,
                                   const DateType& d,
                                   std::uint32_t fdoid,
                                   const MD::FullL2Quote& q, std::uint32_t exchange) :
    Event(EventType::MARKET_EVENT, &m_quote_event.event, d,
          {0,static_cast<decltype(Inst::InstLayout::inst_id)>(fdoid)}, ts)
{
    auto& qe = m_quote_event.quote_event;
    qe.market_event_base.version = MarketEventBaseBuf::EXPECTED_VERSION;
    qe.market_event_base.market_event_type = MarketEventType::QUOTE_EVENT_TYPE;
    qe.market_event_base.exchange = exchange;

    qe.bid_price = static_cast<float>(MD::to_double(q.bid.price));
    qe.bid_size = q.bid.size;
    qe.ask_price = static_cast<float>(MD::to_double(q.ask.price));
    qe.ask_size = q.ask.size;
}

BufferPair MarketQuoteEvent::get_serialize_buffer() const
{
    return std::make_pair(reinterpret_cast<const std::uint8_t *>(&m_quote_event), sizeof(MarketQuoteEventBuf));
}

NetMarketQuoteEvent::NetMarketQuoteEvent(const core::Timestamp& ts,
                                         const DateType& d,
                                         std::uint32_t fdoid,
                                         const MD::FullL2Quote& q,
                                         const core::Timestamp& nts,
                                         std::uint32_t exchange) :
    Event(EventType::NET_MARKET_EVENT, &m_quote_event.market_event.event, d,
          {0,static_cast<decltype(Inst::InstLayout::inst_id)>(fdoid)}, ts)
{
    auto& qe = m_quote_event.market_event.quote_event;
    qe.market_event_base.version = NetMarketQuoteEventBuf::EXPECTED_VERSION;
    qe.market_event_base.market_event_type = MarketEventType::QUOTE_EVENT_TYPE;
    qe.market_event_base.exchange = exchange;

    qe.bid_price = static_cast<float>(MD::to_double(q.bid.price));
    qe.bid_size = q.bid.size;
    qe.ask_price = static_cast<float>(MD::to_double(q.ask.price));
    qe.ask_size = q.ask.size;

    m_quote_event.net_event.version = NetMarketQuoteEventBuf::EXPECTED_VERSION;
    m_quote_event.net_event.time = TimeType::from_timestamp(nts);
}

BufferPair NetMarketQuoteEvent::get_serialize_buffer() const
{
    return std::make_pair(reinterpret_cast<const std::uint8_t *>(&m_quote_event),
                          sizeof(NetMarketQuoteEventBuf));
}

MarketTradeEvent::MarketTradeEvent(const core::Timestamp& ts,
                                   const DateType& d,
                                   std::uint32_t fdoid,
                                   const MD::Price price,
                                   const MD::Size size,
                                   std::uint32_t exchange) :
    Event(EventType::MARKET_EVENT, &m_trade_event.event, d,
          {0, static_cast<decltype(Inst::InstLayout::inst_id)>(fdoid)}, ts)
{
    auto& te = m_trade_event.trade_event;
    te.market_event_base.version = MarketEventBaseBuf::EXPECTED_VERSION;
    te.market_event_base.market_event_type = MarketEventType::TRADE_EVENT_TYPE;
    te.market_event_base.exchange = exchange;

    te.trade_price = static_cast<float>(MD::to_double(price));
    te.trade_size = size;
}

BufferPair MarketTradeEvent::get_serialize_buffer() const
{
    return std::make_pair(reinterpret_cast<const std::uint8_t *>(&m_trade_event), sizeof(MarketTradeEventBuf));
}

NetMarketTradeEvent::NetMarketTradeEvent(const core::Timestamp& ts,
                                         const DateType& d,
                                         std::uint32_t fdoid,
                                         const MD::Price price,
                                         const MD::Size size,
                                         std::uint32_t exchange,
                                         const core::Timestamp& nts) :
    Event(EventType::NET_MARKET_EVENT, &m_trade_event.market_event.event, d,
          {0, static_cast<decltype(Inst::InstLayout::inst_id)>(fdoid)}, ts)
{
    auto& te = m_trade_event.market_event.trade_event;
    te.market_event_base.version = NetMarketTradeEventBuf::EXPECTED_VERSION;
    te.market_event_base.market_event_type = MarketEventType::TRADE_EVENT_TYPE;
    te.market_event_base.exchange = exchange;

    te.trade_price = static_cast<float>(MD::to_double(price));
    te.trade_size = size;

    m_trade_event.net_event.version = NetMarketTradeEventBuf::EXPECTED_VERSION;
    m_trade_event.net_event.time = TimeType::from_timestamp(nts);
}

BufferPair NetMarketTradeEvent::get_serialize_buffer() const
{
    return std::make_pair(reinterpret_cast<const std::uint8_t *>(&m_trade_event), sizeof(NetMarketTradeEventBuf));
}

std::ostream& operator<<(std::ostream& os, const NetMarketQuoteEvent& e)
{
    return os << e.m_quote_event;
}

std::ostream& operator<<(std::ostream& os, const Inst& i)
{
    return os << i.inst.inst_type << ","
              << i.inst.inst_id;
}

std::ostream& operator<<(std::ostream& os, const DateTime& d)
{
    return os << d.date << ","
              << d.time;
}

std::ostream& operator<<(std::ostream& os, const TimeType& tt)
{
    return os << tt.tt_secs << "," << tt.tt_nsecs;
}

std::ostream& operator<<(std::ostream& os, const EventBuf& e)
{
    return os << e.header.event_type << ","
              << e.inst << ","
              << e.date_time;
}

std::ostream& operator<<(std::ostream& os, const NetEventBuf& e)
{
    return os << e.event << ","
              << e.net_event.time;
}

std::ostream& operator<<(std::ostream& os, const MarketEventType& t)
{
    switch(t) {
    case MarketEventType::QUOTE_EVENT_TYPE:
        os << "QUOTE_EVENT_TYPE";
        break;
    case MarketEventType::TRADE_EVENT_TYPE:
        os << "TRADE_EVENT_TYPE";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const MarketEventBaseBuf& b)
{
    return os << b.market_event_type << ","
              << b.exchange;
}

std::ostream& operator<<(std::ostream& os, const MarketQuoteEventBuf& q)
{
    return os << q.event << ","
              << q.quote_event.market_event_base << ","
              << q.quote_event.bid_size << ","
              << q.quote_event.bid_price << ","
              << q.quote_event.ask_price << ","
              << q.quote_event.ask_size;
}

std::ostream& operator<<(std::ostream& os, const MarketTradeEventBuf& t)
{
    return os << t.event << ","
              << t.trade_event.market_event_base << ","
              << t.trade_event.trade_price << ","
              << t.trade_event.trade_size;
}


std::ostream& operator<<(std::ostream& os, const NetMarketTradeEvent& e)
{
    return os << e.m_trade_event;
}

std::ostream& operator<<(std::ostream& os, const NetMarketQuoteEventBuf& e)
{
    return os << e.market_event << ","
              << e.net_event.time;
}


std::ostream& operator<<(std::ostream& os, const NetMarketTradeEventBuf& e)
{
    return os << e.market_event << ","
              << e.net_event.time;
}



std::ostream& operator<<(std::ostream& os, const SnapshotRequestEventBuf& e)
{
    return os << e.net_event << ","
              << e.snapshot_req_event.type << ","
              << (int) e.snapshot_req_event.which << ","
              << e.snapshot_req_event.price_or_level << ","
              << e.snapshot_req_event.id;
}

std::ostream& operator<<(std::ostream& os, const SnapshotType& s)
{
    switch (s) {
    case SnapshotType::SNAPSHOT_BY_PRICE:
        os << "SNAPSHOT_BY_PRICE";
        break;
    case SnapshotType::SNAPSHOT_BY_LEVEL:
        os << "SNAPSHOT_BY_LEVEL";
        break;
    case SnapshotType::SNAPSHOT_BY_ABS_PRICE:
        os << "SNAPSHOT_BY_ABS_PRICE";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const SnapshotEventBuf& e)
{
    return os << e.net_event << ","
              << e.snapshot_event.id << ","
              << e.snapshot_event.num_bid_prices << ","
              << e.snapshot_event.num_ask_prices << ","
              << e.snapshot_event.max_venues;
}

DateType date_from_yyyymmdd(std::uint32_t yyyy, std::uint32_t mm, std::uint32_t dd) {
    DateType ret=0;

    auto is_leap_year = [](std::uint32_t yy) {
        if (yy == 1900) // just copying an excel bug
            return true;
        else if (yy % 400 == 0)
            return true;
        else if (yy % 100 == 0)
            return false;
        else if (yy % 4 == 0)
            return true;
        else
            return false;
    };

    // check for valid year (>=1900)
    if(yyyy<1900) {
        throw std::runtime_error("Invalid year (must be greater than 1899)");
    }

    // check for valid month
    if(mm<1 || mm>12) {
        throw std::runtime_error(" Invalid month (must be between 1 and 12)");
    }

    // check for valid day, given month (and year, in case mm=feb)
    if(dd<1) {
        throw std::runtime_error(" Invalid day (must be between 1 and number of days in month)");
    }
    switch(mm) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        if(dd>31) { throw std::runtime_error(" Invalid day (must be between 1 and number of days in month)"); }
        break;
    case 2:
        if(dd==29) { // is it a leap year?
            if(!is_leap_year(yyyy)) {
                throw std::runtime_error(" Invalid day (must be between 1 and number of days in month)");
            }
            else {
                mm=2;
                dd=1;
                ret += 28;
            }
        }
        else if(dd>28) { throw std::runtime_error(" Invalid day (must be between 1 and number of days in month)") ;}
        break;
    case 4: case 6: case 9: case 11:
        if(dd>30) { throw std::runtime_error(" Invalid day (must be between 1 and number of days in month)") ; }
        break;
    default:
        throw std::runtime_error(" Shouldn't be here -- very serious");
        break;
    }

    // Jan 1 1900 == 1.
    ret += dd;
    dd=1;

    while(yyyy > 1900) {
        if ((mm>2 && is_leap_year(yyyy)) || (mm <=2 && is_leap_year(yyyy-1))) {
            yyyy--;
            ret += 366;
        }
        else {
            yyyy--;
            ret += 365;
        }
    }

    // break intentionally omitted; note that we add the number of days in the PREVIOUS month when decrementing mm
    switch(mm) {
    case 12: mm--; ret += 30;
    case 11: mm--; ret += 31;
    case 10: mm--; ret += 30;
    case 9: mm--; ret += 31;
    case 8: mm--; ret += 31;
    case 7: mm--; ret += 30;
    case 6: mm--; ret += 31;
    case 5: mm--; ret += 30;
    case 4: mm--; ret += 31;
    case 3: mm--; ret += 28 + is_leap_year(1900); // copies leap year bug from excel/lotus
    case 2: mm--; ret += 31;
    case 1: break;
    default:  throw std::runtime_error(" Invalid month, but shouldn't be here");
    }

    if (yyyy != 1900 || mm != 1 || dd != 1) {
        throw std::runtime_error(" Serious problem, didn't get to 1/1/00 at end of func");
    }

    return ret;
}

}}}
