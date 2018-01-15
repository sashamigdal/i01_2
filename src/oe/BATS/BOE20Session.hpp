#pragma once

#include <array>
#include <functional>
#include <list>
#include <regex>
#include <unordered_map>

#include <i01_core/Alphanumeric.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/MappedRegion.hpp>
#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/TimerListener.hpp>
#include <i01_core/util.hpp>

#include <i01_net/HeartBeatConnThread.hpp>
#include <i01_net/TCPSocket.hpp>

#include <i01_oe/OrderManager.hpp>
#include <i01_oe/OrderSession.hpp>

#include "BATS/BOE20/Messages.hpp"

namespace i01 { namespace OE {

class BOE20Order;

namespace BATS {

namespace msgs = BOE20::Messages;

std::ostream& debug_print_bytes(std::ostream& os, const core::Timestamp& ts,
                                const std::uint8_t *buf, const ssize_t& len);

class BOE20Session : public OrderSession,
                     public net::SocketListener,
                     public core::TimerListener {
public:
    using Timestamp = core::Timestamp;
    using Connection = net::HeartBeatConnThread;
    using DataBuffer = std::vector<std::uint8_t>;
    using Mutex = core::SpinMutex;
    using Price = BOE20::Types::Price;
    using LocalIDToClOrdIDMap = std::unordered_map<LocalID, msgs::ClOrdID>;
    using SymbolInstCache = std::unordered_map<std::array<std::uint8_t, sizeof(msgs::Symbol)>, Instrument *, core::array_hasher<std::uint8_t,sizeof(msgs::Symbol)> >;
    using BATSSymbolArray = std::array<BOE20::Types::Symbol, MD::NUM_SYMBOL_INDEX>;

    enum class State {
        UNINITIALIZED=0,
            UNCONNECTED,
            WAITING_FOR_LOGIN,
            REPLAY,
            ACTIVE,
            LOGOUT_SENT,
            USER_LOGOUT_REQUEST,
            ERROR_DISCONNECT,
            DISCONNECT,
            };
    friend std::ostream& operator<<(std::ostream&, const State& s);

    static const int MSG_BUFFER_SIZE = 2048;
    static const int NUM_MSG_BUFFERS = 128;
    static const std::uint32_t DEFAULT_RECONNECT_INTERVAL_MS = 5000;

protected:
    struct Credentials {
        msgs::SessionSubID session_sub_id;
        msgs::Username username;
        msgs::Password password;
    };

    struct PersistentState {
        msgs::SequenceNumber outbound_seqnum;
        std::array<msgs::SequenceNumber,msgs::MAX_NUMBER_OF_MATCHING_UNITS> inbound_seqnum;
    } __attribute__((packed));
    using PersistentStateRegion = core::MappedTypedRegion<PersistentState>;

    template<typename MsgType>
    struct OutboundMsgBuffer {
        union {
            MsgType msg;
            std::uint8_t bytes[MSG_BUFFER_SIZE];
        } data;
        int index;
        size_t msglen;
        std::uint8_t * eom;
        using OptionalFields = typename MsgType::OptionalFields;
        OptionalFields *opt;
    };

    using NewOrderBuffer = OutboundMsgBuffer<msgs::NewOrder>;
    using CancelOrderBuffer = OutboundMsgBuffer<msgs::CancelOrder>;
    using ModifyOrderBuffer = OutboundMsgBuffer<msgs::ModifyOrder>;

    template<typename BufType, int NUM = NUM_MSG_BUFFERS>
    class BufferStore {
    public:
        void init(std::function<void(BufType &)> f);

        bool get(BufType *& ptr);
        void release(BufType *ptr);
        // FIXME should ensure that BufType is an OutboundMsgBuffer instance
    private:
        std::array<BufType, NUM> m_array;
        std::list<int> m_free_list;
        Mutex m_mutex;
    };

    template<typename MsgType>
    struct InboundMsgBuffer {
        MsgType msg;
        std::uint8_t return_bitfields[8];
        typename MsgType::OptionalFields opt;
    } __attribute__ ((packed));

    using OrderAcknowledgmentBuffer = InboundMsgBuffer<msgs::OrderAcknowledgment>;
    I01_ASSERT_SIZE(OrderAcknowledgmentBuffer, sizeof(msgs::OrderAcknowledgment) + 8 + sizeof(msgs::OrderAcknowledgment::OptionalFields));

    using OrderCancelledBuffer = InboundMsgBuffer<msgs::OrderCancelled>;
    I01_ASSERT_SIZE(OrderCancelledBuffer, sizeof(msgs::OrderCancelled) + 8 + sizeof(msgs::OrderCancelled::OptionalFields));

    using OrderExecutionBuffer = InboundMsgBuffer<msgs::OrderExecution>;
    I01_ASSERT_SIZE(OrderExecutionBuffer, sizeof(msgs::OrderExecution) + 8 + sizeof(msgs::OrderExecution::OptionalFields));

    using OrderModifiedBuffer = InboundMsgBuffer<msgs::OrderModified>;
    I01_ASSERT_SIZE(OrderModifiedBuffer, sizeof(msgs::OrderModified) + 8 + sizeof(msgs::OrderModified::OptionalFields));

    using OrderRestatedBuffer = InboundMsgBuffer<msgs::OrderRestated>;
    I01_ASSERT_SIZE(OrderRestatedBuffer, sizeof(msgs::OrderRestated) + 8 + sizeof(msgs::OrderRestated::OptionalFields));

public:
    BOE20Session(OrderManager *omp, const std::string &name);
    virtual ~BOE20Session();

    virtual void on_timer(const core::Timestamp& ts, void *ud, std::uint64_t iter) override final;

    virtual void on_connected(const core::Timestamp& ts, void *ud) override final;
    virtual void on_peer_disconnect(const core::Timestamp& ts, void *ud) override final;
    virtual void on_local_disconnect(const core::Timestamp& ts, void *ud) override final;
    virtual void on_recv(const Timestamp &ts, void *, const std::uint8_t *buf, const ssize_t &len) override final;

    virtual std::string status() const override final { return to_string(); }

protected:
    void init_conf();
    void recover_state();

    std::string to_string() const;

    template<typename ResType>
    void get_from_conf_or_throw(const std::shared_ptr<core::ConfigState> &cs, const std::string &name, ResType &res);

    virtual bool connect(const bool replay) override final;
    virtual bool send(Order* o_p) override final;
    virtual bool cancel(Order* o_p, Size newqty = 0) override final;
    virtual void disconnect(const bool force) override final;
    virtual bool destroy(Order* o_p) override final;

    void disconnect_and_quit();
    void disconnect_connection(bool requested = false);

    // handle different message types here ...

    void handle_raw_bytes(const Timestamp &ts, const std::uint8_t *buf, const ssize_t &len);
    void handle_raw_msg(const Timestamp &ts, const std::uint8_t *buf, const size_t &len);
    // the socket/connection thing handles message framing

    void handle_msg(const Timestamp &ts, const msgs::LoginResponse &msg);
    void handle_msg(const Timestamp &ts, const msgs::Logout &msg);
    void handle_msg(const Timestamp &ts, const msgs::ServerHeartbeat &msg);
    void handle_msg(const Timestamp &ts, const msgs::ReplayComplete &msg);
    void handle_msg(const Timestamp &ts, const msgs::OrderAcknowledgment &msg);
    void handle_msg(const Timestamp &ts, const msgs::OrderRejected &msg);
    void handle_msg(const Timestamp &ts, const msgs::OrderModified &msg);
    void handle_msg(const Timestamp &ts, const msgs::OrderRestated &msg);
    void handle_msg(const Timestamp &ts, const msgs::UserModifyRejected &msg);
    void handle_msg(const Timestamp &ts, const msgs::OrderCancelled &msg);
    void handle_msg(const Timestamp &ts, const msgs::CancelRejected &msg);
    void handle_msg(const Timestamp &ts, const msgs::OrderExecution &msg);
    void handle_msg(const Timestamp &ts, const msgs::TradeCancelOrCorrect &msg);

    void handle_riskbot_reject(const Timestamp& ts, const msgs::OrderAcknowledgment& msg,
                               Order* op, std::uint64_t local_id);

    template<typename MsgType>
    void debug_print_msg(const Timestamp& ts, const char *prefix, const MsgType &msg);

    template<typename MsgType>
    void debug_print_msg(const char *prefix, const MsgType &msg);

    void init_seqnum_overrides(const core::Config::storage_type&);

    void symbology_init();
    std::string cta_to_bats_symbology(const std::string&);

    void send_login_request();
    void send_logout_request();
    void send_heartbeat();

    void do_logout(bool requested = false);

    bool send_new_order(Order *op);
    bool send_cancel(Order *op);
    bool send_modify(Order *op, Size newqty);

    void send_helper_nolock(const std::uint8_t *buf, const size_t & len);

    template<typename MsgType>
    void send_helper(const MsgType &msg);

    static size_t get_sizeof_unit_sequences_group(size_t num);
    static size_t get_sizeof_return_bitfields_group();
    static void fill_msg_header(msgs::MessageHeader *hdr, const msgs::MessageType & mt, const msgs::SequenceNumber & sn = 0);

    template<typename ArrayType, std::size_t WIDTH>
    static void copy_to_array(std::array<ArrayType,WIDTH> &arr, const std::string &str, const ArrayType &padding = '\0');

    static Price price_to_fixed(const OE::Price & p);
    static OE::Price price_to_double(const Price &p);

    static bool side_from_order(Order *op, msgs::Side &side);
    bool exec_inst_from_order(BOE20Order *op, msgs::ExecInst &ei) const;
    static bool ord_type_from_order(Order *op, msgs::OrdType &ord_type);
    static bool tif_from_order(Order *op, msgs::TimeInForce &tif);

    bool display_indicator_from_order(BOE20Order* bop, msgs::DisplayIndicator& di) const;

    bool routing_inst_from_order(BOE20Order* bop, msgs::RoutingInst& ri) const;

    static OE::Side to_side(const msgs::Side &s);
    static OE::TimeInForce to_tif(const msgs::TimeInForce &s);
    static OE::OrderType to_ord_type(const msgs::OrdType &ot);

    Instrument * find_instrument(const msgs::Symbol &s) const;

    void confirm_unit_sequence(const msgs::UnitNumberSequencePair &unsp);

    msgs::ClOrdID create_cl_ord_id(Order *op) const ;

    bool get_buffer(NewOrderBuffer *&nob);
    void release_buffer(NewOrderBuffer *nob);

    void prepare_new_order_buffer(NewOrderBuffer &nob);

    inline void prepare_new_order_bitfield1(std::uint8_t * bufidx);
    inline void prepare_new_order_bitfield2(std::uint8_t * bufidx);
    inline void prepare_new_order_bitfield3(std::uint8_t * bufidx);
    inline void prepare_new_order_bitfield4(std::uint8_t * bufidx);
    inline void prepare_new_order_bitfield5(std::uint8_t * bufidx);
    inline void prepare_new_order_bitfield6(std::uint8_t * bufidx);

    void process_new_order_bitfields1(NewOrderBuffer::OptionalFields *opt);
    void process_new_order_bitfields2(NewOrderBuffer::OptionalFields *opt);
    void process_new_order_bitfields3(NewOrderBuffer::OptionalFields *opt);
    void process_new_order_bitfields4(NewOrderBuffer::OptionalFields *opt);
    void process_new_order_bitfields5(NewOrderBuffer::OptionalFields *opt);
    void process_new_order_bitfields6(NewOrderBuffer::OptionalFields *opt);

    void prepare_cancel_order_buffer(CancelOrderBuffer &cob);
    void prepare_cancel_order_bitfield1(std::uint8_t * bufidx);
    void process_cancel_order_bitfields1(CancelOrderBuffer::OptionalFields *opt);

    void prepare_modify_order_buffer(ModifyOrderBuffer &mob);
    void prepare_modify_order_bitfield1(std::uint8_t *bufidx);
    void prepare_modify_order_bitfield2(std::uint8_t *bufidx);
    void process_modify_order_bitfields1(ModifyOrderBuffer::OptionalFields *opt);
    void process_modify_order_bitfields2(ModifyOrderBuffer::OptionalFields *opt);

    virtual Order * on_create_replay_order(const Timestamp&, Instrument *, const core::MIC&,
                                           const LocalAccount, const LocalID,
                                           const OE::Price, const OE::Size,
                                           const OE::Side, const OE::TimeInForce,
                                           const OrderType, const UserData) override final;

protected:
    State m_state;
    Connection m_connection;
    std::string m_host;
    int m_port;

    DataBuffer m_dangly_bytes;

    uint64_t m_heartbeat_ms;

    uint64_t m_missed_hb_before_logout;

    Timestamp m_last_message_recv_ts;
    Timestamp m_last_message_sent_ts;

    Credentials m_credentials;

    mutable Mutex m_mutex;

    std::uint8_t m_hpr_broker_loc;

    LocalIDToClOrdIDMap m_id_map;

    bool m_auto_reconnect;
    Timestamp m_last_reconnect_attempt_ts;
    std::uint32_t m_reconnect_interval_ms;

    std::string m_clearing_firm;
    std::string m_clearing_account;
    bool m_cancel_orig_on_reject;

    BufferStore<NewOrderBuffer> m_no_bufs;
    BufferStore<CancelOrderBuffer> m_cxl_bufs;
    BufferStore<ModifyOrderBuffer> m_mod_bufs;

    SymbolInstCache m_symbol_inst_cache;
    BATSSymbolArray m_bats_symbol_array;
    std::regex m_unit_r;
    std::regex m_class_r;
    std::regex m_rights_r;

    std::string m_state_path;
    std::unique_ptr<PersistentStateRegion> m_persist_state;

    std::int64_t m_outbound_seqnum_init_override;
    std::vector<std::int64_t> m_inbound_seqnum_init_override;

    msgs::MessageType m_last_message_type;

protected:
    friend class OrderManager;
};

template<typename BT, int NUM>
void BOE20Session::BufferStore<BT,NUM>::init(std::function<void(BT &)> f)
{
    for (int i = 0; i < NUM; i++) {
        f(m_array[i]);
        m_array[i].index = i;
        m_free_list.push_back(i);
    }
}

template<typename BT, int NUM>
bool BOE20Session::BufferStore<BT,NUM>::get(BT *& ptr)
{
    Mutex::scoped_lock lock(m_mutex);

    if (m_free_list.empty()) {
        ptr = nullptr;
        return false;
    }
    ptr = &m_array[m_free_list.front()];
    m_free_list.pop_front();
    return true;
}

template<typename BT, int NUM>
void BOE20Session::BufferStore<BT,NUM>::release(BT * ptr)
{
    Mutex::scoped_lock lock(m_mutex);

    m_free_list.push_front(ptr->index);
}

template<typename ResType>
void BOE20Session::get_from_conf_or_throw(const std::shared_ptr<core::ConfigState> &cs, const std::string &nm, ResType &res)
{
    if (!cs->get(nm, res)) {
        throw std::runtime_error("BOE20Session: " + name() + ": could not find " + nm + " in config");
    }
}

template<typename MsgType>
void BOE20Session::debug_print_msg(const Timestamp& ts, const char *prefix, const MsgType &msg)
{
#ifdef I01_DEBUG_MESSAGING
    std::cerr << m_mic << "," << name() << "," << ts << "," << prefix << "," << msg << std::endl;
#endif
}

template<typename MsgType>
void BOE20Session::debug_print_msg(const char *prefix, const MsgType &msg)
{
#ifdef I01_DEBUG_MESSAGING
    debug_print_msg(Timestamp::now(), prefix, msg);
#endif
}

template<typename MsgType>
void BOE20Session::send_helper(const MsgType &msg) {
#ifdef I01_DEBUG_MESSAGING
    std::cerr << m_name << ",SNDHLPRT," << msg << std::endl;
#endif

    // since this should be used for unsequence messages, we can just lock in here
    Mutex::scoped_lock lock(m_mutex);
    m_connection.send(reinterpret_cast<const std::uint8_t *>(&msg), sizeof(msg));
    m_last_message_sent_ts = Timestamp::now();
}

template<typename ArrayType, std::size_t WIDTH>
void BOE20Session::copy_to_array(std::array<ArrayType,WIDTH> &arr, const std::string &str, const ArrayType &padding)
{
    // right fill with '\0'
    for (auto i = 0U; i < arr.size(); i++) {
        if (i < str.size()) {
            arr[i] = str[i];
        } else {
            arr[i] = padding;
        }
    }
}



}}}
