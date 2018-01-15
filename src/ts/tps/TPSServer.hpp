#pragma once

#include <iosfwd>
#include <memory>
#include <vector>

#include <i01_core/Date.hpp>
#include <i01_core/Lock.hpp>

#include <i01_md/Symbol.hpp>

#include <i01_net/SocketListener.hpp>

namespace i01 {

namespace core {
struct MIC;
class Timestamp;
}

namespace MD {
class DataManager;
struct FullL2Quote;
}

namespace OE {
class OrderManager;
}

namespace TS { namespace TPS {
/*
   This defines a number of types that make up the protocol used to
   communicate with the TPS.

   Unfortunately this protocol was not well designed, so encoding and
   decoding it is kind of annoying.

   The basic idea is that the EventBuf types (EventBuf, NetEventBuf,
   ...) do their best to represent the types of events as they are
   serialized (by using packed structs).  For some events (like
   Handshake events), this is not totally possible, as there is
   variable length data in the middle of the event packet to represent
   strings.

   The EventBufWrapper types are used for decoding the data from the
   wire.  These class wrap a given data buffer with functions that
   know how to decode the buffer into the appropriate type.  The
   should have the same scope as the data in the const buffer, since
   they can contain pointers to bytes within the buffer.

   The Event types (NetEvent, HandshakeEvent) are used for encoding
   events into a buffer.  These are events constructed here and then
   the serialize function can be called to serialize the event onto
   the given buffer.
*/

class TPSServer;

using BufferPair = std::pair<const std::uint8_t *, std::size_t>;

enum class FDOExchange : std::uint8_t {
    UNKNOWN             = 0
 , XNYS                 = 1
 , XNAS                 = 3
 , ARCX                 = 36
 , XBOS                 = 6
 , XPSX                 = 51
 , BATY                 = 52
 , BATS                 = 2
 , EDGA                 = 5
 , EDGX                 = 6
 , CAES                 = 29
 };

union SerializeVersion {
    std::array<std::uint8_t,2>          arr;
    std::uint16_t                       u16;
} __attribute__((packed));
I01_ASSERT_SIZE(SerializeVersion, 2);

enum class EventType : std::uint16_t {
     MARKET_EVENT = 0
 , ORDER_STATUS_EVENT = 2
 , NASDAQ_CLOSING_CROSS_EVENT = 3
 , PARENT_ORDER_STATUS_EVENT = 4
 , ROOT_ORDER_STATUS_EVENT = 5
 , NET_EVENT = 6
 , SUBSCRIPTION_EVENT = 7
 , SNAPSHOT_REQUEST_EVENT = 8
 , SNAPSHOT_EVENT = 9
 , NET_MARKET_EVENT = 10
 , HANDSHAKE_EVENT = 11
 , EXPRET_EVENT = 12
 };
std::ostream& operator<<(std::ostream& os, const EventType& e);

union Inst {
    struct InstLayout {
        std::int32_t        inst_type;
        std::int32_t        inst_id;
    } __attribute__((packed))   inst;
    std::uint64_t               u64;
} __attribute__((packed));
I01_ASSERT_SIZE(Inst, 8);
std::ostream& operator<<(std::ostream& os, const Inst& i);

using DateType = std::uint32_t;

DateType date_from_yyyymmdd(std::uint32_t yyyy, std::uint32_t mm, std::uint32_t dd);

struct TimeType {
    std::uint64_t               tt_secs;
    std::uint64_t               tt_nsecs;

    static TimeType from_timestamp(const core::Timestamp& ts);
} __attribute__((packed));
I01_ASSERT_SIZE(TimeType, 16);
std::ostream& operator<<(std::ostream& os, const TimeType& tt);

struct DateTime {
    DateType                    date;
    TimeType                    time;
} __attribute__((packed));
I01_ASSERT_SIZE(DateTime, 20);
std::ostream& operator<<(std::ostream& os, const DateTime& d);

struct EventBufHeader {
    SerializeVersion version;
    EventType event_type;
} __attribute__((packed));
I01_ASSERT_SIZE(EventBufHeader, 4);

struct EventBuf {
    EventBufHeader header;
    Inst inst;
    DateTime date_time;

    const static SerializeVersion EXPECTED_VERSION;
} __attribute__((packed));
I01_ASSERT_SIZE(EventBuf, 32);
std::ostream& operator<<(std::ostream& os, const EventBuf& e);

struct NetEventBuf {
    EventBuf            event;
    struct Layout {
        SerializeVersion    version;
        TimeType            time;
    } __attribute__((packed)) net_event;
    const static SerializeVersion EXPECTED_VERSION;
} __attribute__((packed));
I01_ASSERT_SIZE(NetEventBuf, 32 + 2 + 16);
std::ostream& operator<<(std::ostream& os, const NetEventBuf& b);

enum class MarketEventType : std::uint32_t {
    QUOTE_EVENT_TYPE            = 1
 , TRADE_EVENT_TYPE             = 2
 };
std::ostream& operator<<(std::ostream& os, const MarketEventType& t);

struct MarketEventBaseBuf {
    SerializeVersion        version;
    MarketEventType         market_event_type;
    std::uint32_t           exchange;

    const static SerializeVersion EXPECTED_VERSION;
} __attribute__((packed));
I01_ASSERT_SIZE(MarketEventBaseBuf, 10);
std::ostream& operator<<(std::ostream& os, const MarketEventBaseBuf& b);

struct MarketQuoteEventBuf {
    EventBuf            event;
    struct Layout {
        MarketEventBaseBuf          market_event_base;
        float                       bid_price;
        std::uint32_t               bid_size;
        float                       ask_price;
        std::uint32_t               ask_size;
    } __attribute__((packed))   quote_event;
} __attribute__((packed));
I01_ASSERT_SIZE(MarketQuoteEventBuf, 32 + 10 + 16);
std::ostream& operator<<(std::ostream& os, const MarketQuoteEventBuf& q);

struct MarketTradeEventBuf {
    EventBuf            event;
    struct Layout {
        MarketEventBaseBuf          market_event_base;
        float                       trade_price;
        std::uint32_t               trade_size;
    } __attribute__((packed))   trade_event;
} __attribute__((packed));
I01_ASSERT_SIZE(MarketTradeEventBuf, 32 + 10 + 8);
std::ostream& operator<<(std::ostream& os, const MarketTradeEventBuf& t);

struct NetMarketQuoteEventBuf {
    MarketQuoteEventBuf         market_event;
    struct Layout {
        SerializeVersion        version;
        TimeType                time;
    } __attribute__((packed))   net_event;
    static const SerializeVersion     EXPECTED_VERSION;
} __attribute__((packed));
I01_ASSERT_SIZE(NetMarketQuoteEventBuf, 58 + 18);
std::ostream& operator<<(std::ostream& os, const NetMarketQuoteEventBuf& q);

struct NetMarketTradeEventBuf {
    MarketTradeEventBuf         market_event;
    struct Layout {
        SerializeVersion        version;
        TimeType                time;
    } __attribute__((packed))   net_event;
    static const SerializeVersion     EXPECTED_VERSION;
} __attribute__((packed));
I01_ASSERT_SIZE(NetMarketTradeEventBuf, 50 + 18);
std::ostream& operator<<(std::ostream& os, const NetMarketTradeEventBuf& e);

struct HandshakeEventBuf {
    NetEventBuf         net_event;
    struct Layout {
        SerializeVersion    version;
        std::uint16_t       version_string_len;
    } __attribute__((packed)) handshake_event;
    const static SerializeVersion EXPECTED_VERSION;
} __attribute__((packed));

struct SubscriptionEventBuf {
    NetEventBuf         net_event;
    struct Layout {
        SerializeVersion        version;
        std::uint32_t           flags;
    } __attribute__((packed)) subscription_event;

    enum class Flags : std::uint32_t {
        REQUEST         = 0x01
    , RESP_OK           = 0x02
    , RESP_NOK          = 0x04
    };
    const static SerializeVersion EXPECTED_VERSION;
} __attribute__((packed));
I01_ASSERT_SIZE(SubscriptionEventBuf, sizeof(NetEventBuf)+6);

enum class SnapshotType : std::uint8_t {
    SNAPSHOT_BY_PRICE=0,
    SNAPSHOT_BY_LEVEL=1,
    SNAPSHOT_BY_ABS_PRICE=2
};
std::ostream& operator<<(std::ostream& os, const SnapshotType& s);

enum class SnapshotSide : std::uint8_t {
    BID=1,
    ASK=2,
};
std::ostream& operator<<(std::ostream& os, const SnapshotSide& s);

using SnapshotID = std::uint64_t;
using SnapshotPriceOrLevel = std::uint32_t;

struct SnapshotRequestEventBuf {
    NetEventBuf         net_event;
    struct Layout {
        SerializeVersion        version;
        SnapshotType            type;
        std::uint8_t            which;
        SnapshotPriceOrLevel    price_or_level;
        SnapshotID              id;
    } __attribute__((packed)) snapshot_req_event;
    const static SerializeVersion EXPECTED_VERSION;
} __attribute__((packed));
I01_ASSERT_SIZE(SnapshotRequestEventBuf, sizeof(NetEventBuf) + 16);
std::ostream& operator<<(std::ostream& os, const SnapshotRequestEventBuf& e);

struct SnapshotEventBuf {
    NetEventBuf         net_event;
    struct Layout {
        SerializeVersion        version;
        SnapshotID              id;
        std::uint32_t           num_bid_prices;
        std::uint32_t           num_ask_prices;
        std::uint32_t           max_venues;
    } __attribute__((packed)) snapshot_event;
    const static SerializeVersion EXPECTED_VERSION;
} __attribute__((packed));
I01_ASSERT_SIZE(SnapshotEventBuf, sizeof(NetEventBuf) + 22);
std::ostream& operator<<(std::ostream& os, const SnapshotEventBuf& e);

class Event {
public:
    Event(const EventType& et, EventBuf*);
    Event(const EventType& et, EventBuf*, const DateType& d, const Inst& inst, const core::Timestamp& ts);
    // Event(const EventType& et, const Inst& inst, const DateTime& dt);
    EventType event_type() const { return m_event_buf->header.event_type; }
    // returns 0 if buf is not big enough
    // virtual std::size_t serialize(std::uint8_t* buf, std::size_t len) const;
    // virtual BufferPair get_serialize_buffer() const = 0;
    static const char * EXPECTED_VERSION_STRING;

protected:
    EventBuf * m_event_buf;
};

class EventBufWrapper {
public:
    EventBufWrapper() : m_event_buf{nullptr}, m_valid{false}, m_bytes_read{0} {}
    EventBufWrapper(const std::uint8_t* buf, const std::size_t len);
    bool valid() const { return m_valid; }

    EventType event_type() const { return m_event_buf->header.event_type; }

    std::size_t bytes_read() const { return m_bytes_read; }

    Inst inst() const { return m_event_buf->inst; }
    friend std::ostream& operator<<(std::ostream& os, const EventBufWrapper& e);
protected:
    const EventBuf* m_event_buf;
    bool m_valid;
    std::size_t m_bytes_read;
};


class NetEvent : public Event {
public:
    NetEvent(NetEventBuf *p, EventType et = EventType::NET_EVENT);

    // virtual std::size_t serialize(std::uint8_t* buf, std::size_t len) const override;
protected:
    NetEventBuf *m_net_event;
};

class NetEventBufWrapper : public EventBufWrapper {
public:
    NetEventBufWrapper(const std::uint8_t* buf, const std::size_t len);
    friend std::ostream& operator<<(std::ostream& os, const NetEventBufWrapper& w);

protected:
    const NetEventBuf* m_net_event_buf;
};


class MarketQuoteEvent : public Event {
public:
    MarketQuoteEvent(const core::Timestamp& ts,
                     const DateType& d,
                     std::uint32_t fdoid,
                     const MD::FullL2Quote& q, std::uint32_t exchange = 0);

    // virtual std::size_t serialize(std::uint8_t* buf, std::size_t len) const override final;

    BufferPair get_serialize_buffer() const;

private:
    MarketQuoteEventBuf m_quote_event;
};

class MarketTradeEvent : public Event {
public:
    MarketTradeEvent(const core::Timestamp& ts,
                     const DateType& d,
                     std::uint32_t fdoid,
                     const MD::Price price,
                     const MD::Size size,
                     std::uint32_t exchange = 0);
    // virtual std::size_t serialize(std::uint8_t* buf, std::size_t len) const override final;
    BufferPair get_serialize_buffer() const;
private:
    MarketTradeEventBuf m_trade_event;
};

class NetMarketQuoteEvent : public Event {
public:
    NetMarketQuoteEvent(const core::Timestamp& ts,
                        const DateType& d,
                        std::uint32_t fdoid,
                        const MD::FullL2Quote& q,
                        const core::Timestamp& nts,
                        std::uint32_t exchange = 0);

    // virtual std::size_t serialize(std::uint8_t* buf, std::size_t len) const override final;

    BufferPair get_serialize_buffer() const;

    friend std::ostream& operator<<(std::ostream& os, const NetMarketQuoteEvent& e);
private:
    NetMarketQuoteEventBuf m_quote_event;
};

class NetMarketTradeEvent : public Event {
public:
    NetMarketTradeEvent(const core::Timestamp& ts,
                        const DateType& d,
                        std::uint32_t fdoid,
                        const MD::Price price,
                        const MD::Size size,
                        std::uint32_t exchange,
                        const core::Timestamp& nts);
    // virtual std::size_t serialize(std::uint8_t* buf, std::size_t len) const override final;
    BufferPair get_serialize_buffer() const;
    friend std::ostream& operator<<(std::ostream& os, const NetMarketTradeEvent& e);
private:
    NetMarketTradeEventBuf m_trade_event;
};



class HandshakeEvent : public NetEvent {
public:
    HandshakeEvent(const std::string& version, const std::string& alias);
    ~HandshakeEvent();
    // virtual std::size_t serialize(std::uint8_t* buf, std::size_t len) const override final;
    BufferPair get_serialize_buffer() const;

    friend std::ostream& operator<<(std::ostream& os, const HandshakeEvent& h);

private:
    size_t num_bytes_needed() const;

private:
    std::uint8_t*       m_data;
    HandshakeEventBuf   m_tmp_handshake;
    HandshakeEventBuf*  m_handshake;
    std::string         m_version;
    std::string         m_alias;
};

class HandshakeEventBufWrapper : public NetEventBufWrapper {
public:
    HandshakeEventBufWrapper(const std::uint8_t* buf, const std::size_t len);

    std::string version() const { return std::string(reinterpret_cast<const char*>(m_version_buf), *m_version_buf_len); }
    std::string alias() const { return std::string(reinterpret_cast<const char *>(m_alias_buf), *m_alias_buf_len); }
    friend std::ostream& operator<<(std::ostream& os, const HandshakeEventBufWrapper& h);
private:
    const HandshakeEventBuf* m_handshake_event_buf;
    const std::uint8_t* m_version_buf;
    const std::uint16_t* m_version_buf_len;
    const std::uint8_t* m_alias_buf;
    const std::uint16_t* m_alias_buf_len;
};


class SubscriptionEventBufWrapper : public NetEventBufWrapper {
public:
    SubscriptionEventBufWrapper(const std::uint8_t* buf, const std::size_t len);

    std::uint32_t flags() const { return m_sub_event_buf->subscription_event.flags; }
    friend std::ostream& operator<<(std::ostream& os, const SubscriptionEventBufWrapper& e);

private:
    const SubscriptionEventBuf* m_sub_event_buf;
};

class SnapshotRequestEventBufWrapper : public NetEventBufWrapper {
public:
    SnapshotRequestEventBufWrapper(const std::uint8_t* buf, const std::size_t len);
    friend std::ostream& operator<<(std::ostream& os, const SnapshotRequestEventBufWrapper& e);
private:
    const SnapshotRequestEventBuf* m_snapshot_req_buf;
};

std::unique_ptr<EventBufWrapper> next_event_from_buffer(const std::uint8_t* buf, const std::size_t len);

using ClientID = std::size_t;

class ClientSession {
public:
    enum class State : std::uint8_t {
        UNKNOWN                 = 0
    , PRE_HANDSHAKE             = 1
    , HANDSHAKE_OK              = 2
    , HANDSHAKE_DENIED          = 3
    , DISCONNECTED              = 4
    };
    friend std::ostream& operator<<(std::ostream& os, const State& cs);

    static const char* RESPONSE_HANDSHAKE_OK;
    static const char* RESPONSE_HANDSHAKE_DENIED;

    using DataBuffer = std::vector<std::uint8_t>;

    using ESIContainer = std::vector<MD::EphemeralSymbolIndex>;

    using FDOExchangeMap = std::array<FDOExchange, (int)core::MICEnum::NUM_MIC>;

public:
    ClientSession(TPSServer* server, const ClientID& iid, int fd);

    void disconnect();

    void handle_raw_bytes(const core::Timestamp& ts, const std::uint8_t * buf, const std::size_t len);

    void write_quote(const core::Timestamp& ts, const core::MIC& mic,
                     MD::EphemeralSymbolIndex esi, const std::uint32_t fdoid,
                     const MD::FullL2Quote& q);

    void write_trade(const core::Timestamp& ts, const core::MIC& mic,
                     MD::EphemeralSymbolIndex esi, const std::uint32_t fdoid,
                     const MD::TradePair& tp);

    State state() const { return m_state; }

    friend std::ostream& operator<<(std::ostream& os, const ClientSession& c);

private:
    void handle_event(const core::Timestamp& ts, const EventBufWrapper* ep);
    void handle_handshake_event(const core::Timestamp& ts, const HandshakeEventBufWrapper* evt);
    void handle_subscription_event(const core::Timestamp& ts, const SubscriptionEventBufWrapper* evt);
    void handle_snapshot_request_event(const core::Timestamp& ts, const SnapshotRequestEventBufWrapper* evt);

    template<typename EventType>
    bool send_or_disconnect(const EventType& e, bool blocking = true);

    int write_buffer(const std::uint8_t* buf, std::size_t len, bool blocking);

private:
    TPSServer* m_server;
    ClientID m_id;
    int m_fd;
    State m_state;
    DataBuffer m_dangly_bytes;
    ESIContainer m_subscribed_esi;
    DateType m_date;
    static const FDOExchangeMap m_fdo_exchange;
    time_t m_tz_offset_secs;
};


// this knows how to talk to TPS clients
class TPSServer : public net::SocketListener {
private:

    using ClientIDsForInstrument = std::array<std::vector<ClientID>, MD::NUM_SYMBOL_INDEX>;

    using Mutex = core::SpinRWMutex;

    using ClientUniquePtr = std::unique_ptr<ClientSession>;

    using ClientSessionVector = std::vector<ClientUniquePtr>;

    using FDOIDToESIMap = std::unordered_map<std::uint32_t, MD::EphemeralSymbolIndex>;
    using ESIToFDOIDMap = std::array<std::uint32_t, MD::NUM_SYMBOL_INDEX>;

public:

    TPSServer(OE::OrderManager* omp, MD::DataManager* dmp);
    virtual ~TPSServer() = default;

    void * on_new_client(int fd);
    void on_quote_update(const core::Timestamp& ts, const core::MIC& mic, MD::EphemeralSymbolIndex esi,
                         const MD::FullL2Quote& q);
    void on_trade_update(const core::Timestamp& ts, const core::MIC& mic, MD::EphemeralSymbolIndex esi,
                         const MD::TradePair& tp);
    virtual void on_connected(const core::Timestamp&, void *) override {}
    virtual void on_peer_disconnect(const core::Timestamp& ts, void * ud) override;
    virtual void on_local_disconnect(const core::Timestamp&, void *) override {}
    // if the userdata says so, then this is coming on the listening socket,
    virtual void on_recv(const core::Timestamp & ts, void * ud, const std::uint8_t *buf, const ssize_t & len) override;

    MD::EphemeralSymbolIndex add_subscription(const ClientID& cid, std::uint32_t fdo_id);
    bool is_subscribed(const ClientID& cid, std::uint32_t fdo_id);
    void remove_subscription(const ClientID& cid, MD::EphemeralSymbolIndex esi);

    core::Date date() const;

private:
    OE::OrderManager* m_omp;
    MD::DataManager* m_dmp;
    Mutex m_mutex;

    ClientSessionVector m_clients;

    ClientIDsForInstrument m_instrument_clients;

    FDOIDToESIMap m_fdoid_esi_map;
    ESIToFDOIDMap m_esi_fdoid_map;
};

template<typename EventType>
bool ClientSession::send_or_disconnect(const EventType& e, bool blocking)
{
    int err = 0;
    auto p = e.get_serialize_buffer();

    if ((err = ClientSession::write_buffer(p.first, p.second, blocking)) < 0) {
        std::cerr << "ClientSession," << *this << ",error on send," << -err << "," << ::strerror(-err)
                  << ",disconnecting..." << std::endl;
        disconnect();
        return false;
    }
    return true;
}

}}}
