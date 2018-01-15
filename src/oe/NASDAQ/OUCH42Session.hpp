#pragma once

#include <unordered_map>

#include <i01_core/MappedRegion.hpp>
#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/Lock.hpp>
#include <i01_net/SocketListener.hpp>
#include <i01_core/TimerListener.hpp>

#include <i01_core/util.hpp>

#include <i01_net/HeartBeatConnThread.hpp>

#include <i01_oe/OrderManager.hpp>
#include <i01_oe/OrderSession.hpp>

#include "SoupBinTCP30/Listener.hpp"
#include "UFO10/Listener.hpp"
#include "OUCH42/Messages.hpp"

namespace i01 { namespace OE { namespace NASDAQ {

class OUCH42Session
    : public OrderSession
    , public net::SocketListener
    , public core::TimerListener
    , protected SoupBinTCP30::Listener {
public:
    typedef core::Timestamp Timestamp;
    typedef core::SpinMutex Mutex;
    using SymbolInstCache = std::unordered_map<OUCH42::Types::StockArray, Instrument *, core::array_hasher<char, sizeof(OUCH42::Types::StockArray)> >;
    using NASDAQSymbolCache = std::array<std::string, MD::NUM_SYMBOL_INDEX>;

public:
    OUCH42Session(OrderManager *om_p, const std::string& name_);
    virtual ~OUCH42Session() override;

protected:
    bool connect(const bool replay) override final;
    void disconnect(bool force = false) override final;

    void symbology_init();
    Instrument * find_instrument(const OUCH42::Types::StockArray& stock);
    NASDAQOrder * create_replay_order(const Timestamp& ts, LocalID id, const OUCH42::Messages::Accepted& msg);

    bool timed_out(const core::Timestamp& ts) const { return (m_session_state == State::TIMED_OUT || (m_last_message_received_ts.milliseconds_since_midnight() > (ts.milliseconds_since_midnight() - m_heartbeat_grace_sec * 1000ULL))); }
    bool connected() const { return ( m_session_state == State::CONNECTED || m_session_state == State::LOGGED_IN ); }
    bool logged_in() const { return m_session_state == State::LOGGED_IN; }

    bool send(Order* o_p) override final;
    bool cancel(Order* o_p, Size newqty = 0) override final;
    bool destroy(Order* o_p) override final;

    void on_timer(const core::Timestamp& ts, void * userdata, std::uint64_t iter) override final;

    void on_connected(const core::Timestamp&, void*) override final;
    void on_peer_disconnect(const core::Timestamp&, void*) override final;
    void on_local_disconnect(const core::Timestamp&, void*) override final;
    void on_recv(const core::Timestamp& ts, void * ud, const std::uint8_t *buf, const ssize_t & len) override final;

    void handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::LoginAccept *) override final;
    void handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::LoginReject *) override final;
    void handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::SequencedDataPacket *, const char *buf, size_t len) override final;
    void handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::ServerHeartbeat *) override final;
    void handle_packet(const Timestamp& ts, const SoupBinTCP30::Messages::EndOfSession *) override final;

    virtual Order * on_create_replay_order(const Timestamp& ts, Instrument *inst, const core::MIC& mic,
                                           const LocalAccount local_account, const LocalID local_id,
                                           const OE::Price price, const OE::Size size, const OE::Side side,
                                           const OE::TimeInForce tif, const OrderType type,
                                           const UserData ud) override final;

    virtual std::string status() const override final { return to_string(); }

    OE::Side to_i01_side(const OUCH42::Types::BuySellIndicator& bs);
    OE::TimeInForce to_i01_tif(const OUCH42::Types::TimeInForce& tif);

    inline bool create_order_token_hpr(Order *o_p, OUCH42::Types::OrderToken& token);

private:
    struct PersistentState {
        std::uint64_t last_seqnum_received;
        SoupBinTCP30::Types::Session session;
    } __attribute__((packed));
    using PersistentStateRegion = core::MappedTypedRegion<PersistentState>;

private:
    template<typename MsgType>
    void debug_print_msg(const char *prefix, const MsgType& msg);

    /// \internal
    bool send_order_data(NASDAQOrder* o_p, const char * buf, size_t len);
    /// \internal
    void defer_bytes(const char *buf, size_t len);
    /// \internal
    size_t dispatch_packet(const Timestamp& ts, const char *buf, size_t len);
    /// \internal
    void recover_state();

    std::string to_string() const;

    // Configuration state:
    std::string m_host;
    int m_port;
    std::string m_username;
    std::string m_password;
    std::string m_req_session;
    OUCH42::Types::Firm m_firm_identifier;
    std::uint64_t m_heartbeat_grace_sec; // seconds
    std::int8_t m_default_broker_locate; // 0 - 3

    // Session state (for reconnection):
    enum class State : int {
        TIMED_OUT = -4
      , LOGIN_REJECTED = -3
      , DISCONNECTED = -2
      , ERROR = -1
      , UNKNOWN = 0
      , INITIALIZED = 1
      , CONNECTING = 2
      , CONNECTED = 3
      , LOGGED_IN = 4
    } m_session_state;
    friend std::ostream& operator<<(std::ostream& os, const State& s);

    mutable Mutex m_mutex;
    core::Timestamp m_last_message_received_ts;
    core::Timestamp m_last_message_sent_ts;
    ssize_t m_dangly_length;
    std::array<char, 4096> m_dangly_bytes;
    net::HeartBeatConnThread m_connection;
    std::string m_state_path;
    std::unique_ptr<PersistentStateRegion> m_persist;

    SymbolInstCache m_symbol_inst_cache;
    NASDAQSymbolCache m_symbol_cache;

    friend class OrderManager;

};

template<typename MsgType>
void OUCH42Session::debug_print_msg(const char *prefix, const MsgType& msg)
{
#ifdef I01_DEBUG_MESSAGING
    std::cerr << market() << "," << name() << "," << prefix << "," << msg << std::endl;
#endif
}

} } }
