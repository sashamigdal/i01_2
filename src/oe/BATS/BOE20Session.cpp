#include <math.h>

#include <algorithm>
#include <cstdlib>
#include <iomanip>

#include <boost/lexical_cast.hpp>

#include <i01_core/Config.hpp>
#include <i01_core/util.hpp>

#include <i01_oe/BATSOrder.hpp>

#include "BOE20/Types.hpp"
#include "BOE20/Messages.hpp"

#include "BOE20Session.hpp"

using namespace i01::core;

namespace i01 { namespace OE { namespace BATS {

using namespace BOE20::Messages;
using i01::core::operator<<;

std::ostream& debug_print_bytes(std::ostream& os,
                                const core::Timestamp& ts,
                                const MessageType& mt,
                                const std::uint8_t * buf, const ssize_t& len)
{
    os << "[" << ts << "] [LAST MSG: " << mt << "] Buffer size: " << len << "\n"
       << std::hex;
    for (auto i = 0; i < len; i++) {
        os << std::setfill('0') << std::setw(2) << (int) buf[i] << " ";
    }
    os << std::dec << "\n---\n";
    for (auto i = 0; i < len; i++) {
        if (std::isprint(buf[i])) {
            os << " " << (char) buf[i];
        } else {
            os << " .";
        }
        os << " ";
    }
    os << std::endl;
    return os;
}

BOE20Session::BOE20Session(OrderManager *omp, const std::string &n) :
    OrderSession(omp, n, "BOE20Session"),
    m_state(State::UNCONNECTED),
    m_connection("BOE20Session", this, this),
    m_port(0),
    m_heartbeat_ms(1000),
    m_missed_hb_before_logout(5),
    m_auto_reconnect(true),
    m_reconnect_interval_ms(DEFAULT_RECONNECT_INTERVAL_MS),
    m_outbound_seqnum_init_override(-1),
    m_inbound_seqnum_init_override(msgs::MAX_NUMBER_OF_MATCHING_UNITS,-1)
{
    m_connection.set_heartbeat_seconds(1);

    // set up instrument name cache
    symbology_init();

    init_conf();

    recover_state();
}

BOE20Session::~BOE20Session()
{
    disconnect_and_quit();
}

void BOE20Session::init_conf()
{
    auto cfg = Config::instance().get_shared_state();

    auto cs(cfg->copy_prefix_domain("oe.sessions." + m_name + "."));

    std::string str;

    get_from_conf_or_throw(cs, "remote.addr", m_host);
    get_from_conf_or_throw(cs, "remote.port", m_port);

    get_from_conf_or_throw(cs, "session_sub_id", str);
    copy_to_array(m_credentials.session_sub_id.arr, str);

    get_from_conf_or_throw(cs, "username", str);
    copy_to_array(m_credentials.username.arr, str);

    get_from_conf_or_throw(cs, "password", str);
    copy_to_array(m_credentials.password.arr, str);

    m_auto_reconnect = cs->get_or_default("auto_reconnect", 1);
    m_reconnect_interval_ms = cs->get_or_default("reconnect_interval_ms", DEFAULT_RECONNECT_INTERVAL_MS);

    get_from_conf_or_throw(cs, "clearing_firm", m_clearing_firm);
    get_from_conf_or_throw(cs, "clearing_account", m_clearing_account);

    get_from_conf_or_throw(cs, "hpr_broker_loc", str);
    m_hpr_broker_loc = str[0];

    if (!cs->get("cancel_orig_on_reject", m_cancel_orig_on_reject)) {
        m_cancel_orig_on_reject = false;
    }

    m_state_path = cs->get_or_default("state_path", "boe20." + m_name + ".state");

    init_seqnum_overrides(*cs);

    m_no_bufs.init([this] (NewOrderBuffer &nob) { this->prepare_new_order_buffer(nob); });
    m_cxl_bufs.init([this] (CancelOrderBuffer &cob) {this->prepare_cancel_order_buffer(cob);});
    m_mod_bufs.init([this] (ModifyOrderBuffer &mob) { this->prepare_modify_order_buffer(mob);});
}

void BOE20Session::init_seqnum_overrides(const core::Config::storage_type& cfg)
{
    m_outbound_seqnum_init_override = cfg.get_or_default("outbound_seqnum_override",-1);

    // check for a single inbound seqnum override
    const auto str = std::string{"inbound_seqnum_override"};
    auto all = cfg.get<std::int64_t>(str);
    if (all) {
        for (auto i = 0U; i < msgs::MAX_NUMBER_OF_MATCHING_UNITS; i++) {
            m_inbound_seqnum_init_override[i] = *all;
        }
        return;
    }

    // look for indexed value instead
    auto ibseq = cfg.copy_prefix_domain(str + ".");
    auto keys(ibseq->get_key_prefix_set());
    for (const auto& k : keys) {
        try {
            auto index = boost::lexical_cast<int>(k);
            if (index < 1 || index > msgs::NUMBER_OF_MATCHING_UNITS) {
                std::cerr << m_name << ",ERR,inbound_seqnum_override,BAD INDEX," << index << std::endl;
                continue;
            }

            m_inbound_seqnum_init_override[index] = ibseq->get_or_default<std::int64_t>(k,-1);
        } catch (boost::bad_lexical_cast& e) {
            std::cerr << m_name << ",ERR,inbound_seqnum_override,BAD INDEX," << k << std::endl;
        }
    }
}

void BOE20Session::recover_state()
{

    if (!m_state_path.size()) {
        throw std::runtime_error(m_name + " requires a state_path");
    }

    m_persist_state = std::unique_ptr<PersistentStateRegion>(new PersistentStateRegion(m_state_path, false, true));
    if (!m_persist_state->mapped()) {
        std::cerr << m_name << ",ERR,RECOVER STATE,COULD NOT MAP STATE," << m_state_path << std::endl;
        throw std::runtime_error(m_name + " could not open persistent state ");
    }

    if (m_persist_state->data()->outbound_seqnum) {
        // we decrement here since this state was the *next* outbound seqnum, which will not match the server when we reconnect
        m_persist_state->data()->outbound_seqnum--;
    }

    if (m_outbound_seqnum_init_override >= 0) {
        m_persist_state->data()->outbound_seqnum = static_cast<msgs::SequenceNumber>(m_outbound_seqnum_init_override);
    }

    for (auto i = 0U; i < msgs::MAX_NUMBER_OF_MATCHING_UNITS; i++) {
        if (m_inbound_seqnum_init_override[i] >= 0) {
            m_persist_state->data()->inbound_seqnum[i] = static_cast<msgs::SequenceNumber>(m_inbound_seqnum_init_override[i]);
        }
    }
}

void BOE20Session::on_timer(const core::Timestamp& ts, void * ud, std::uint64_t iter)
{
     auto now = ts.milliseconds_since_midnight();

    // check for stale server
    auto diff = now - m_last_message_recv_ts.milliseconds_since_midnight();
    if (((State::ACTIVE == m_state || State::REPLAY == m_state)
         && diff >= m_missed_hb_before_logout * m_heartbeat_ms)
        || State::USER_LOGOUT_REQUEST == m_state) {
        std::cerr << name() << " " << ts << " no data received in too long, last message: "
                  << m_last_message_recv_ts << " > "
                  << m_missed_hb_before_logout * m_heartbeat_ms << std::endl;
        do_logout();
        return;
    }

    diff = now - m_last_message_sent_ts.milliseconds_since_midnight();
    if ((State::ACTIVE == m_state || State::REPLAY == m_state)
        && diff >= m_heartbeat_ms) {
        // there is a lock happening within the templated send_helper
        send_heartbeat();
    }

    // check if we are in an ERROR_DISCONNECTED state and it has been long enough since last reconnection attempt to try again...
    if ((State::ERROR_DISCONNECT == m_state)
        && m_auto_reconnect
        && (now - m_last_reconnect_attempt_ts.milliseconds_since_midnight()) >= m_reconnect_interval_ms) {
        std::cerr << name() << "," << ts << ",attempting reconnect..." << std::endl;

        m_last_reconnect_attempt_ts = ts;
        connect(false);
    }
}

void BOE20Session::on_connected(const core::Timestamp&, void *)
{
    if (State::UNCONNECTED == m_state
        || State::DISCONNECT == m_state
        || State::ERROR_DISCONNECT == m_state) {
        send_login_request();

        m_state = State::WAITING_FOR_LOGIN;
    } else {
        std::cerr << name() << ",ERR,ON_CONNECTED WITH NON-DISCONNECTED STATE" << std::endl;
    }
}

void BOE20Session::on_peer_disconnect(const core::Timestamp& ts, void *)
{
    std::cerr << m_name << ",ERR,PEER DISCONNECT " << std::endl;
    m_last_reconnect_attempt_ts = ts;
    disconnect_connection();
}

void BOE20Session::on_local_disconnect(const core::Timestamp& ts, void *)
{
    std::cerr << m_name << ",ERR,LOCAL DISCONNECT " << std::endl;
    m_last_reconnect_attempt_ts = ts;
    disconnect_connection();
}

void BOE20Session::on_recv(const Timestamp &ts, void *, const std::uint8_t *buf, const ssize_t &len)
{
#ifdef I01_DEBUG_MESSAGING
    std::cerr << m_name << ",on_recv," << ts << "," << len << ",";
    for (auto i = 0U; i < len; i++) {
        std::cerr << std::hex << (int) buf[i] << std::dec << " ";
    }
    std::cerr << std::endl;
#endif

    handle_raw_bytes(ts, buf, len);
}

void BOE20Session::symbology_init()
{
    m_unit_r = std::regex("(.+)/U$");
    m_class_r = std::regex("(.+)/([A-Z])$");
    m_rights_r = std::regex("(.+)r$");

    auto symbol_arr = BOE20::Types::Symbol{};
    for (const auto& u : m_order_manager_p->universe()) {
        try {
            auto bats_sym = cta_to_bats_symbology(u.cta_symbol());
            copy_to_array(symbol_arr.arr, bats_sym);
            m_bats_symbol_array[u.data()->esi()].arr = symbol_arr.arr;
            m_symbol_inst_cache[symbol_arr.arr] = u.data();
        } catch (std::exception& e) {
            std::cerr << m_name << " caught exception converting symbol " << u.cta_symbol() << " to BATS: " << e.what() << std::endl;
        }
    }
}

std::string BOE20Session::cta_to_bats_symbology(const std::string &cta)
{
    auto res = std::regex_replace(cta, m_unit_r, "$1=");
    if (res != cta) {
        return res;
    }

    res = std::regex_replace(cta, m_class_r, "$1.$2");
    if (res != cta) {
        return res;
    }

    res = std::regex_replace(cta, m_rights_r, "$1^");
    if (res != cta) {
        return res;
    }

    return cta;
}

void BOE20Session::handle_raw_bytes(const Timestamp &ts, const std::uint8_t *buf, const ssize_t &len)
{
    // debug_print_bytes(std::cerr, ts, m_last_message_type, buf, len);

    auto dbs = m_dangly_bytes.size();
    if (dbs) {
        ssize_t idx = 0;
        // we have bytes from previous call hanging around
        ssize_t remains = 0;

        if (dbs < sizeof(StartOfMessage)+sizeof(MessageLength)) {
            // we need to get enough bytes to at least read the message length
            remains = sizeof(StartOfMessage) + sizeof(MessageLength) - dbs;
            for (idx = 0; idx < std::min(remains, len); idx++) {
                m_dangly_bytes.push_back(buf[idx]);
                dbs++;
            }

            if (len < remains) {
                // still don't have enough
                return;
            }
        }
        // we have enough for start of message and message length...

        auto som = *reinterpret_cast<const StartOfMessage *>(m_dangly_bytes.data());
        if (UNLIKELY(START_OF_MESSAGE_SENTINEL != som)) {
            std::cerr << name() << ",ERR,RAWBYTES,DB SENTINEL MISMATCH," << len << ",DISCONNECTING" << std::endl;
            debug_print_bytes(std::cerr, ts, m_last_message_type, m_dangly_bytes.data(), m_dangly_bytes.size());
            debug_print_bytes(std::cerr, ts, m_last_message_type, buf, len);
            m_dangly_bytes.clear();
            disconnect_connection();
            return;
        }

        auto mlen = *reinterpret_cast<const MessageLength *>(m_dangly_bytes.data()+(ssize_t)sizeof(StartOfMessage)) + sizeof(StartOfMessage);
        assert(mlen > dbs);

        if (LIKELY(mlen >= dbs)) {
            // bytes left in the whole msg
            remains = mlen - dbs;
        } else {
            // means that message length is less than the bytes we
            // have in dangly bytes....  so should have the message
            // within dangly_bytes ...
            // should not get here....
            remains = 0;

            std::cerr << name() << ",ERR,RAWBYTES,DB REMAINS IS ZERO," << len << ",DISCONNECTING" << std::endl;
            debug_print_bytes(std::cerr, ts, m_last_message_type, m_dangly_bytes.data(), m_dangly_bytes.size());
            debug_print_bytes(std::cerr, ts, m_last_message_type, buf, len);
            disconnect_connection();
            return;
        }

        for (ssize_t i = 0; i < std::min(remains, len); i++, idx++, dbs++) {
            m_dangly_bytes.push_back(buf[idx]);
        }

        if (remains <= len) {
            // then we have a whole message here
            handle_raw_msg(ts, m_dangly_bytes.data(), mlen);
            assert(mlen == m_dangly_bytes.size());
            m_dangly_bytes.clear();

            // we still have more data to parse....
            if (idx < len) {
                handle_raw_bytes(ts, buf+idx, len-idx);
            }
            return;
        } else {
            // remains > len ... means we still don't have enough on
            // dangly_bytes for this message ... and we've pushed this
            // buffer onto dangly so try again later...
            return;
        }
    } else {
        // we have no prior data
        if (len < (ssize_t) (sizeof(StartOfMessage) + sizeof(MessageLength))) {
            // not enough to even know message size ...
            for (auto i = 0; i < len; i++) {
                m_dangly_bytes.push_back(buf[i]);
            }
            return;
        }

        auto som = *reinterpret_cast<const StartOfMessage *>(buf);
        if (UNLIKELY(START_OF_MESSAGE_SENTINEL != som)) {
            std::cerr << name() << ",ERR,RAWBYTES,SENTINEL MISMATCH," << len << ",DISCONNECTING" << std::endl;
            debug_print_bytes(std::cerr, ts, m_last_message_type, buf, len);
            disconnect_connection();
            return;
        }

        // we can infer message length .. need to add StartOfMessage for total length
        auto mlen = *reinterpret_cast<const MessageLength *>(buf+sizeof(StartOfMessage)) + (ssize_t)sizeof(StartOfMessage);
        if (mlen <= len) {
            // we have a full message here...
            handle_raw_msg(ts, buf, mlen);

            if (len > mlen) {
                // we have bytes remaining ...
                handle_raw_bytes(ts, buf+mlen, len-mlen);
            }
        } else {
            // we don't have enough for the full message ... so make dangly bytes
            for (ssize_t i = 0; i < len; i++) {
                m_dangly_bytes.push_back(buf[i]);
            }
        }
    }
}

void BOE20Session::handle_raw_msg(const Timestamp &ts, const std::uint8_t *buf, const size_t &len)
{
    auto hdr = reinterpret_cast<const MessageHeader *>(buf);
    assert(START_OF_MESSAGE_SENTINEL == hdr->start_of_message);

    switch(hdr->message_type) {
    case MessageType::CLIENT_HEARTBEAT:
        std::cerr << "client heartbeat" << std::endl;
        break;
    case MessageType::LOGIN_RESPONSE_V2:
        handle_msg(ts, *reinterpret_cast<const LoginResponse *>(buf));
        break;
    case MessageType::LOGOUT:
        handle_msg(ts, *reinterpret_cast<const Logout *>(buf));
        break;
    case MessageType::SERVER_HEARTBEAT:
        handle_msg(ts, *reinterpret_cast<const ServerHeartbeat *>(buf));
        break;
    case MessageType::REPLAY_COMPLETE:
        handle_msg(ts, *reinterpret_cast<const ReplayComplete *>(buf));
        break;
    case MessageType::ORDER_ACKNOWLEDGMENT_V2:
        handle_msg(ts, *reinterpret_cast<const OrderAcknowledgment *>(buf));
        break;
    case MessageType::ORDER_REJECTED_V2:
        handle_msg(ts, *reinterpret_cast<const OrderRejected *>(buf));
        break;
    case MessageType::ORDER_MODIFIED_V2:
        handle_msg(ts, *reinterpret_cast<const OrderModified *>(buf));
        break;
    case MessageType::ORDER_RESTATED_V2:
        handle_msg(ts, *reinterpret_cast<const OrderRestated *>(buf));
        break;
    case MessageType::USER_MODIFY_REJECTED_V2:
        handle_msg(ts, *reinterpret_cast<const UserModifyRejected *>(buf));
        break;
    case MessageType::ORDER_CANCELLED_V2:
        handle_msg(ts, *reinterpret_cast<const OrderCancelled *>(buf));
        break;
    case MessageType::CANCEL_REJECTED_V2:
        handle_msg(ts, *reinterpret_cast<const CancelRejected *>(buf));
        break;
    case MessageType::ORDER_EXECUTION_V2:
        handle_msg(ts, *reinterpret_cast<const OrderExecution *>(buf));
        break;
    case MessageType::TRADE_CANCEL_OR_CORRECT_V2:
        handle_msg(ts, *reinterpret_cast<const TradeCancelOrCorrect *>(buf));
        break;
    case MessageType::LOGIN_REQUEST_V2:
    case MessageType::NEW_ORDER_V2:
    case MessageType::CANCEL_ORDER_V2:
    case MessageType::MODIFY_ORDER_V2:
    case MessageType::LOGOUT_REQUEST:
    default:
        std::cerr << name() << ",ERR,RAWMSG,UNSUPPORTED MESSAGE TYPE," << (int) hdr->message_type << ",DISCONNECTING" << std::endl;
        debug_print_bytes(std::cerr, ts, m_last_message_type, buf, len);
        disconnect_connection();
        break;
    }
    m_last_message_recv_ts = ts;
    m_last_message_type = hdr->message_type;
}

void BOE20Session::confirm_unit_sequence(const UnitNumberSequencePair &unsp)
{
    if (unsp.unit_sequence != m_persist_state->data()->inbound_seqnum[unsp.unit_number]) {
        std::cerr << m_name << ",ERR,SEQ MISMATCH,"
                  << unsp << ","
                  << m_persist_state->data()->inbound_seqnum[unsp.unit_number]
                  << std::endl;
    }
}

void BOE20Session::handle_msg(const Timestamp &ts, const LoginResponse &msg)
{
    debug_print_msg(ts, "LGR", msg);

    if (LoginResponseStatus::LOGIN_ACCEPTED != msg.login_response_status) {
        std::cerr << name() << " " << ts
                  << " login not accepted! " << (char) msg.login_response_status << " "
                  << msg.login_response_text.arr << std::endl;
        m_state = State::UNCONNECTED;
        return;
    }

    // got login accepted ... next is replay ..
    m_state = State::REPLAY;

    // FIXME tell the order manager we are in replay mode now
    if (0 != m_persist_state->data()->outbound_seqnum) {
        // we have some state of our outbound seqnums...
        if (msg.last_received_sequence_number > m_persist_state->data()->outbound_seqnum) {
            // they have more messages than we know about ...
            std::cerr << name() << "," << ts << ",ERR,OUTBOUND SEQ MISMATCH SERVER GREATER,"
                      << msg.last_received_sequence_number << ","
                      << m_persist_state->data()->outbound_seqnum << std::endl;
        } else if (msg.last_received_sequence_number < m_persist_state->data()->outbound_seqnum) {
            std::cerr << name() << "," << ts << ",ERR,OUTBOUND SEQ MISMATCH SERVER LESS,"
                      << msg.last_received_sequence_number << ","
                      << m_persist_state->data()->outbound_seqnum << std::endl;
            // we could choose to the messages the server missed
        }
    }
    m_persist_state->data()->outbound_seqnum = msg.last_received_sequence_number + 1;

    // look at unit sequence numbers
    for (auto i = 0U; i < msg.unit_sequence_number.number_of_units; i++) {
        const auto *unsp = reinterpret_cast<const UnitNumberSequencePair *>((const std::uint8_t*)&msg.unit_sequence_number.units[0] + i*sizeof(UnitNumberSequencePair));
        confirm_unit_sequence(*unsp);
    }

    // we need to verify that the return bits are the same as we sent .... since we rely on messages being basially fixed size based on them.
}

void BOE20Session::handle_msg(const Timestamp &ts, const Logout &msg)
{
    debug_print_msg(ts, "LGO", msg);
    std::cerr << name() << "," << ts << ",LOGOUT," << msg << std::endl;

    for (auto i = 0U; i < msg.unit_sequence_number.number_of_units; i++) {
        const auto *unsp = reinterpret_cast<const UnitNumberSequencePair *>((const std::uint8_t*)&msg.unit_sequence_number.units[0] + i*sizeof(UnitNumberSequencePair));

        confirm_unit_sequence(*unsp);
    }

    disconnect_connection();
}

void BOE20Session::handle_msg(const Timestamp &ts, const ServerHeartbeat &msg)
{
    debug_print_msg("SHB", msg);
}

void BOE20Session::handle_msg(const Timestamp &ts, const ReplayComplete &msg)
{
    m_state = State::ACTIVE;
    m_active = true;

#ifdef I01_DEBUG_MESSAGING
    std::cerr << m_name << ",REPLAY COMPLETE" << std::endl;
#endif
    // FIXME ORder manager replay complete call back
    debug_print_msg(ts, "RPC", msg);
}

void BOE20Session::handle_riskbot_reject(const Timestamp& ts, const msgs::OrderAcknowledgment& msg, Order* op, std::uint64_t local_id)
{
    debug_print_msg(ts, "RREJ",msg);

    std::cerr << m_name << ",ERR,ORDACK,";
    if (State::REPLAY == m_state) {
        std::cerr << "REPLAY ";
    }
    std::cerr << "RISKBOT REJECT," << msg << std::endl;

    if (State::REPLAY == m_state) {
        // then this is new to us, so we need to callback the OM
        if (!m_order_manager_p->adopt_orphan_order(name(), static_cast<OE::LocalID>(local_id), nullptr)) {
            std::cerr << m_name << ",ERR,RISKBOTREJ,ADOPT ORPHAN FAILED," << msg << std::endl;
            return;
        }
    } else {
        if (UNLIKELY(nullptr == op)) {
            std::cerr << m_name << ",ERR,RISKBOTREJ,CLORDID NOT FOUND," << msg << std::endl;
        } else {
            m_order_manager_p->on_rejected(op, ts, msg.transaction_time);
        }
    }
}

void BOE20Session::handle_msg(const Timestamp &ts, const OrderAcknowledgment &msg)
{
    m_persist_state->data()->inbound_seqnum[msg.message_header.matching_unit] = msg.message_header.sequence_number;
    const auto & opt = reinterpret_cast<const OrderAcknowledgmentBuffer *>(&msg)->opt;

    auto local_id = msg.cl_ord_id.get_local_id<std::uint64_t>();
    auto* op = get_order(static_cast<OE::LocalID>(local_id));

    if (msg.cl_ord_id.fields.broker_loc == HPR_REJECT_BROKER_LOC
        && HPR_REJECT_ORDER_SIZE == opt.order_qty
        && HPR_REJECT_SYMBOL.arr == opt.symbol.arr) {
        // they will also change price to $0.01 and symbol to ZVZZT
        // HPR rejected us
        handle_riskbot_reject(ts, msg, op, local_id);
        return;
    }

    debug_print_msg(ts, "ACK",msg);

    if (State::REPLAY == m_state) {
        // this is a new replay message, so we need to tell the OM about it...
        // need to reverse map from symbol to Instrument*, etc
        if (nullptr == op) {
            // this is an order the OM doesn't know about
            std::cerr << m_name << ",ERR,ORDACK,REPLAY LOCAL ID NOT FOUND," << static_cast<OE::LocalID>(local_id) << "," << msg << std::endl;

            auto * inst = find_instrument(opt.symbol);

            if (nullptr == inst) {
                std::cerr << m_name << ",ERR,ORDACK,REPLAY INST NOT FOUND," << msg << std::endl;
                return;
            }

            // TODO: use different time stamp
            op = on_create_replay_order(ts, inst, market(), 0, static_cast<OE::LocalID>(local_id),
                                        price_to_double(opt.price), opt.leaves_qty, to_side(opt.side),
                                        to_tif(opt.time_in_force), to_ord_type(opt.ord_type), nullptr);

            // TODO: need to assign local_id to it if able...
            if (!m_order_manager_p->adopt_orphan_order(name(), static_cast<OE::LocalID>(local_id), op)) {
                std::cerr << m_name << ",ERR,ORDACK,ADOPT ORPHAN FAILED," << msg << std::endl;
                return;
            }
        }
    }
    m_order_manager_p->on_acknowledged(op, msg.order_id, opt.leaves_qty, ts, msg.transaction_time);
}

void BOE20Session::handle_msg(const Timestamp &ts, const OrderRejected &msg)
{
    debug_print_msg(ts, "REJ",msg);

    // unsequenced, so not sent during replay

    // find the Order * for this clordid
    auto* op = get_order(static_cast<OE::LocalID>(msg.cl_ord_id.get_local_id<std::uint64_t>()));
    if (nullptr == op) {
        std::cerr << m_name << ",ERR,ORDREJ,CLORDID NOT FOUND," << msg << std::endl;
        return;
    }

    std::cerr << m_name << ",ORDREJ," << msg << std::endl;

    m_order_manager_p->on_rejected(op,ts, msg.transaction_time);
}

void BOE20Session::handle_msg(const Timestamp &ts, const OrderModified &msg)
{
    debug_print_msg(ts, "MOD",msg);

    m_persist_state->data()->inbound_seqnum[msg.message_header.matching_unit] = msg.message_header.sequence_number;

    auto local_id = msg.cl_ord_id.get_local_id<std::uint64_t>();
    auto* op = get_order(static_cast<OE::LocalID>(local_id));

    // it's an error in replay or active if we can't find this order
    if (UNLIKELY(nullptr == op)) {
        std::cerr << m_name << ",ERR,ORDMOD,CLORDID NOT FOUND," << msg << std::endl;
        return;
    }
    // we are either ACTIVE or REPLAYing an unseen message here...
    const auto & opt = reinterpret_cast<const OrderModifiedBuffer *>(&msg)->opt;
    if (opt.leaves_qty != op->open_size()) {
        m_order_manager_p->on_cancel(op, op->open_size() - opt.leaves_qty, ts, msg.transaction_time);
    } else {
        std::cerr << m_name << ",ERR,ORDMOD,LEAVES==OPEN SIZE," << msg << std::endl;
    }
}

void BOE20Session::handle_msg(const Timestamp &ts, const OrderRestated &msg)
{
    debug_print_msg(ts, "RST",msg);

    m_persist_state->data()->inbound_seqnum[msg.message_header.matching_unit] = msg.message_header.sequence_number;

    auto local_id = msg.cl_ord_id.get_local_id<std::uint64_t>();
    auto* op = get_order(static_cast<OE::LocalID>(local_id));

    if (UNLIKELY(nullptr == op)) {
        std::cerr << m_name << ",ERR,ORDRST,CLORDID NOT FOUND," << msg << std::endl;
        return;
    }

    // this could happen in ACTIVE or REPLAY
    std::cerr << name() << "," << ts << ",ORDRST," << msg << std::endl;

    const auto & opt = reinterpret_cast<const OrderRestatedBuffer *>(&msg)->opt;
    if (opt.leaves_qty == 0) {
        m_order_manager_p->on_cancel(op, opt.order_qty, ts, msg.transaction_time);
    }
}
void BOE20Session::handle_msg(const Timestamp &ts, const UserModifyRejected &msg)
{
    debug_print_msg(ts, "UMR",msg);
    auto local_id = msg.cl_ord_id.get_local_id<std::uint64_t>();

    // unsequenced, not sent during replay
    auto* op = get_order(static_cast<OE::LocalID>(local_id));

    if (UNLIKELY(nullptr == op)) {
        std::cerr << name() << "," << ts << ",ERR,UMR,CLORDID NOT FOUND," << msg << std::endl;
        return;
    }

    std::cerr << name() << "," << ts << ",UMR," << msg << std::endl;

    m_order_manager_p->on_cancel_rejected(op, ts, msg.transaction_time);
}

void BOE20Session::handle_msg(const Timestamp &ts, const OrderCancelled &msg)
{
    debug_print_msg(ts, "CXL",msg);

    m_persist_state->data()->inbound_seqnum[msg.message_header.matching_unit] = msg.message_header.sequence_number;

    auto local_id = msg.cl_ord_id.fields.order_id.get<std::uint64_t>();
    auto* op = get_order(static_cast<OE::LocalID>(local_id));
    if (UNLIKELY(nullptr == op)) {
        std::cerr << name() << "," << ts << ",ERR,ORDCXL,CLORDID NOT FOUND," << msg << std::endl;
        return;
    }

    // could be ACTIVE or REPLAY here
    const auto *cxl = reinterpret_cast<const OrderCancelledBuffer *>(&msg);
    // get the leavesQty as the amount cancelled...

    // it's possible for an IOC order to be rejected by the riskbot, and still get an order cancel...
    // so if the order is in the rejected state then don't call the cancel callback
    if (OrderState::REMOTELY_REJECTED != op->state()) {
        m_order_manager_p->on_cancel(op, op->open_size() - cxl->opt.leaves_qty, ts, msg.transaction_time);
    }
}

void BOE20Session::handle_msg(const Timestamp &ts, const CancelRejected &msg)
{
    debug_print_msg(ts, "CRJ",msg);

    // unsequenced
    auto local_id = msg.cl_ord_id.fields.order_id.get<std::uint64_t>();
    auto* op = get_order(static_cast<OE::LocalID>(local_id));
    if (UNLIKELY(nullptr == op)) {
        std::cerr << name() << "," << ts << ",ERR,CXLREJ,CLORDID NOT FOUND," << msg << std::endl;
        return;
    }

    std::cerr << name() << "," << ts << ",CXLREJ," << msg << std::endl;

    m_order_manager_p->on_cancel_rejected(op, ts, msg.transaction_time);
}

void BOE20Session::handle_msg(const Timestamp &ts, const OrderExecution &msg)
{
    debug_print_msg(ts, "EXE",msg);

    m_persist_state->data()->inbound_seqnum[msg.message_header.matching_unit] = msg.message_header.sequence_number;

    auto local_id = msg.cl_ord_id.get_local_id<std::uint64_t>();
    auto* op = get_order(static_cast<OE::LocalID>(local_id));
    if (UNLIKELY(nullptr == op)) {
        std::cerr << name() << "," << ts << ",ERR,ORDEXE,CLORDID NOT FOUND," << msg << std::endl;
        return;
    }

    // FIXME should have ExecutionID here too??
    m_order_manager_p->on_fill(op, msg.last_shares, price_to_double(msg.last_px), ts, msg.transaction_time);
}

void BOE20Session::handle_msg(const Timestamp &ts, const TradeCancelOrCorrect &msg)
{
    debug_print_msg(ts, "TCC",msg);

    m_persist_state->data()->inbound_seqnum[msg.message_header.matching_unit] = msg.message_header.sequence_number;

    auto local_id = msg.cl_ord_id.get_local_id<std::uint64_t>();
    auto* op = get_order(static_cast<OE::LocalID>(local_id));
    if (UNLIKELY(nullptr == op)) {
        std::cerr << name() << "," << ts << ",ERR,TCC,CLORDID NOT FOUND," << msg << std::endl;
        return;
    }

    std::cerr << name() << "," << ts << ",TCC," << msg << std::endl;
}


bool BOE20Session::connect(const bool I01_UNUSED replay)
{
    if (!m_host.size() || !m_port) {
        std::cerr << name() << ",ERR,CONNECT,NO HOST OR PORT" << std::endl;
        return false;
    }

    {
        Mutex::scoped_lock lock(m_mutex);
        if (State::UNCONNECTED != m_state
            && State::DISCONNECT != m_state
            && State::ERROR_DISCONNECT != m_state) {
            std::cerr << name() << ",ERR,CONNECT,MUST BE IN UNCONNECTED STATE" << std::endl;
            return false;
        }
    }

    if (!m_connection.connect(m_host, m_port)) {
        std::cerr << name() << ",ERR,HBCT CONNECT FAILED" << std::endl;
    }

    while (m_connection.state() == Connection::State::STARTING) {
        ::usleep(1000);
    }

    if (m_connection.state() == Connection::State::ACTIVE) {
        return true;
    }

    std::cerr << name() << ",ERR,CONNECT,CONNECTION STATE," << m_connection.state() << std::endl;
    return false;
}

void BOE20Session::disconnect_and_quit()
{
    disconnect(true);
    m_connection.shutdown();
}

void BOE20Session::disconnect(const bool force)
{
    OrderSession::disconnect(force);
    // if force, then we do it here ...

    if (force) {
        do_logout(true);
    } else {
        m_state = State::USER_LOGOUT_REQUEST;
    }
}

void BOE20Session::do_logout(bool requested)
{
    send_logout_request();
    ::usleep(1000);
    disconnect_connection(requested);
}

void BOE20Session::disconnect_connection(bool requested)
{
    Mutex::scoped_lock lock(m_mutex);
    m_active = false;
    m_state = requested ? State::DISCONNECT : State::ERROR_DISCONNECT;
    m_connection.disconnect();
}

bool BOE20Session::send(Order* op)
{
    if (UNLIKELY(State::ACTIVE != m_state)) {
        return false;
    }

    return send_new_order(op);
}

bool BOE20Session::cancel(Order* op, Size newqty)
{
    if (UNLIKELY(State::ACTIVE != m_state)) {
        return false;
    }

    if (newqty) {
        // only allow reduces
        if (UNLIKELY(newqty >= op->size())) {
            std::cerr << m_name << ",ERR,CXL,NEWQTY INCREASES," << newqty << "," << *op << std::endl;
            return false;
        }
        return send_modify(op, newqty);
    } else {
        return send_cancel(op);
    }
}

bool BOE20Session::destroy(Order* I01_UNUSED o_p)
{
    return false;
}

void BOE20Session::fill_msg_header(MessageHeader *msg, const MessageType &mt, const SequenceNumber &sn)
{
    msg->start_of_message = START_OF_MESSAGE_SENTINEL;
    msg->message_length = sizeof(MessageHeader)-2;
    msg->message_type = mt;
    msg->matching_unit = 0;
    msg->sequence_number = sn;
}

size_t BOE20Session::get_sizeof_unit_sequences_group(size_t num)
{
    if (num) {
        return sizeof(UnitSequencesParamGroup) + num * sizeof(UnitNumberSequencePair);
    }
    return 0;
}

size_t BOE20Session::get_sizeof_return_bitfields_group()
{
    return sizeof(ReturnBitfieldsParamGroup)*9;
}

void BOE20Session::send_login_request()
{
    std::vector<UnitNumberSequencePair> unitseqs;
    for (std::uint8_t i = 1; i <= msgs::NUMBER_OF_MATCHING_UNITS; i++) {
        if (m_persist_state->data()->inbound_seqnum[i]) {
            unitseqs.push_back({i,m_persist_state->data()->inbound_seqnum[i]});
        }
    }

    NumberOfUnits numunits = static_cast<NumberOfUnits>(unitseqs.size());

    // we have to determine the size of the message ahead of time .. then allocate those bytes and fill them in

    size_t msglen = sizeof(LoginRequest) + get_sizeof_unit_sequences_group(numunits) + get_sizeof_return_bitfields_group();

    std::uint8_t * bytes(new std::uint8_t[msglen]); // TODO: clean up
    std::uint8_t * buf(bytes);
    auto * msg = reinterpret_cast<LoginRequest *>(buf);

    fill_msg_header(&msg->header, MessageType::LOGIN_REQUEST_V2);

    msg->session_sub_id = m_credentials.session_sub_id;
    msg->username = m_credentials.username;
    msg->password = m_credentials.password;
    msg->header.message_length = static_cast<MessageLength>(msglen - 2U);
    msg->num_param_groups = 0;

    buf = &msg->param_groups[0];
    if (numunits) {
        // we have unit sequence numbers
        auto * usg = reinterpret_cast<UnitSequencesParamGroup *>(buf);
        usg->header.type = ParamGroupType::UNIT_SEQUENCES;
        usg->header.length = static_cast<ParamGroupLength>(numunits * sizeof(UnitNumberSequencePair) + sizeof(UnitSequencesParamGroup));
        usg->no_unspecified_unit_replay = BooleanFlag::TRUE;
        usg->unit_sequence_number.number_of_units = numunits;
        for (auto i = 0U; i < numunits; i++) {
            usg->unit_sequence_number.units[i].unit_number = unitseqs[i].unit_number;
            usg->unit_sequence_number.units[i].unit_sequence = unitseqs[i].unit_sequence;
        }

        buf += usg->header.length;
        msg->num_param_groups++;
    }

    // now let's write out the return bitfields for different message types...
    // ORDER ACKNOWLEDGMENT
    auto *rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::ORDER_ACKNOWLEDGMENT_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u8[0] = 0xFF;
    rbf->bitfields.bitfields.u8[1] = ((std::uint8_t) OrderAcknowledgmentBitfield2::SYMBOL
                                      | (std::uint8_t) OrderAcknowledgmentBitfield2::CAPACITY);
    rbf->bitfields.bitfields.u8[2] = ((std::uint8_t) OrderAcknowledgmentBitfield3::ACCOUNT
                                      | (std::uint8_t) OrderAcknowledgmentBitfield3::CLEARING_FIRM
                                      | (std::uint8_t) OrderAcknowledgmentBitfield3::CLEARING_ACCOUNT
                                      | (std::uint8_t) OrderAcknowledgmentBitfield3::DISPLAY_INDICATOR
                                      | (std::uint8_t) OrderAcknowledgmentBitfield3::MAX_FLOOR
                                      | (std::uint8_t) OrderAcknowledgmentBitfield3::DISCRETION_AMOUNT
                                      | (std::uint8_t) OrderAcknowledgmentBitfield3::ORDER_QTY);
    rbf->bitfields.bitfields.u8[3] = 0x00;
    rbf->bitfields.bitfields.u8[4] = ((std::uint8_t) OrderAcknowledgmentBitfield5::LEAVES_QTY
                                      | (std::uint8_t) OrderAcknowledgmentBitfield5::DISPLAY_PRICE
                                      | (std::uint8_t) OrderAcknowledgmentBitfield5::WORKING_PRICE);
    rbf->bitfields.bitfields.u8[5] = ((std::uint8_t) OrderAcknowledgmentBitfield6::ATTRIBUTED_QUOTE
                                      | (std::uint8_t) OrderAcknowledgmentBitfield6::EXT_EXEC_INST);
    rbf->bitfields.bitfields.u8[6] = 0x00;
    rbf->bitfields.bitfields.u8[7] = (std::uint8_t) OrderAcknowledgmentBitfield8::ROUTING_INST;

    msg->num_param_groups++;

    buf += rbf->header.length;

    // ORDER REJECTED
    rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::ORDER_REJECTED_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u8[0] = 0xFF;
    rbf->bitfields.bitfields.u8[1] = ((std::uint8_t)OrderRejectedBitfield2::SYMBOL
                                      | (std::uint8_t) OrderRejectedBitfield2::CAPACITY);
    rbf->bitfields.bitfields.u8[2] = ((std::uint8_t) OrderRejectedBitfield3::ACCOUNT
                                      | (std::uint8_t) OrderRejectedBitfield3::CLEARING_FIRM
                                      | (std::uint8_t) OrderRejectedBitfield3::CLEARING_ACCOUNT
                                      | (std::uint8_t) OrderRejectedBitfield3::DISPLAY_INDICATOR
                                      | (std::uint8_t) OrderRejectedBitfield3::MAX_FLOOR
                                      | (std::uint8_t) OrderRejectedBitfield3::DISCRETION_AMOUNT
                                      | (std::uint8_t) OrderRejectedBitfield3::ORDER_QTY);
    rbf->bitfields.bitfields.u8[3] = 0x00;
    rbf->bitfields.bitfields.u8[4] = 0x00;
    rbf->bitfields.bitfields.u8[5] = ((std::uint8_t) OrderRejectedBitfield6::ATTRIBUTED_QUOTE
                                      | (std::uint8_t) OrderRejectedBitfield6::EXT_EXEC_INST);
    rbf->bitfields.bitfields.u8[6] = 0x00;
    rbf->bitfields.bitfields.u8[7] = (std::uint8_t) OrderRejectedBitfield8::ROUTING_INST;

    msg->num_param_groups++;
    buf += rbf->header.length;

    // ORDER MODIFIED
    rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::ORDER_MODIFIED_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u8[0] = 0xFF;
    rbf->bitfields.bitfields.u8[1] = 0x00;
    rbf->bitfields.bitfields.u8[2] = ((std::uint8_t) OrderModifiedBitfield3::ACCOUNT
                                      | (std::uint8_t) OrderModifiedBitfield3::CLEARING_FIRM
                                      | (std::uint8_t) OrderModifiedBitfield3::CLEARING_ACCOUNT
                                      | (std::uint8_t) OrderModifiedBitfield3::DISPLAY_INDICATOR
                                      | (std::uint8_t) OrderModifiedBitfield3::MAX_FLOOR
                                      | (std::uint8_t) OrderModifiedBitfield3::DISCRETION_AMOUNT
                                      | (std::uint8_t) OrderModifiedBitfield3::ORDER_QTY);

    rbf->bitfields.bitfields.u8[3] = 0x00;
    rbf->bitfields.bitfields.u8[4] = ((std::uint8_t) OrderModifiedBitfield5::ORIG_CL_ORD_ID
                                      | (std::uint8_t) OrderModifiedBitfield5::LEAVES_QTY
                                      | (std::uint8_t) OrderModifiedBitfield5::DISPLAY_PRICE
                                      | (std::uint8_t) OrderModifiedBitfield5::WORKING_PRICE);
    rbf->bitfields.bitfields.u8[5] = ((std::uint8_t) OrderModifiedBitfield6::ATTRIBUTED_QUOTE
                                      | (std::uint8_t) OrderModifiedBitfield6::EXT_EXEC_INST);
    rbf->bitfields.bitfields.u8[6] = 0x00;
    rbf->bitfields.bitfields.u8[7] = ((std::uint8_t) OrderModifiedBitfield8::ROUTING_INST);
    msg->num_param_groups++;
    buf += rbf->header.length;

    // ORDER RESTATED
    rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::ORDER_RESTATED_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u8[0] = 0xFF;
    rbf->bitfields.bitfields.u8[1] = ((std::uint8_t)OrderRestatedBitfield2::SYMBOL
                                      | (std::uint8_t) OrderRestatedBitfield2::CAPACITY);
    rbf->bitfields.bitfields.u8[2] = ((std::uint8_t) OrderRestatedBitfield3::ACCOUNT
                                      | (std::uint8_t) OrderRestatedBitfield3::CLEARING_FIRM
                                      | (std::uint8_t) OrderRestatedBitfield3::CLEARING_ACCOUNT
                                      | (std::uint8_t) OrderRestatedBitfield3::DISPLAY_INDICATOR
                                      | (std::uint8_t) OrderRestatedBitfield3::MAX_FLOOR
                                      | (std::uint8_t) OrderRestatedBitfield3::DISCRETION_AMOUNT
                                      | (std::uint8_t) OrderRestatedBitfield3::ORDER_QTY);
    rbf->bitfields.bitfields.u8[3] = 0x00;
    rbf->bitfields.bitfields.u8[4] = ((std::uint8_t) OrderModifiedBitfield5::ORIG_CL_ORD_ID
                                      | (std::uint8_t) OrderModifiedBitfield5::LEAVES_QTY
                                      | (std::uint8_t) OrderModifiedBitfield5::DISPLAY_PRICE
                                      | (std::uint8_t) OrderModifiedBitfield5::WORKING_PRICE);

    rbf->bitfields.bitfields.u8[5] = ((std::uint8_t) OrderRestatedBitfield6::ATTRIBUTED_QUOTE
                                      | (std::uint8_t) OrderRestatedBitfield6::EXT_EXEC_INST);
    rbf->bitfields.bitfields.u8[6] = 0x00;
    rbf->bitfields.bitfields.u8[7] = ((std::uint8_t) OrderRestatedBitfield8::ROUTING_INST);
    msg->num_param_groups++;

    buf += rbf->header.length;

    // USER MODIFY REJECTED
    rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::USER_MODIFY_REJECTED_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u64 = 0x00;
    msg->num_param_groups++;

    buf += rbf->header.length;

    // ORDER CANCELLED
    rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::ORDER_CANCELLED_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u8[0] = 0xFF;
    rbf->bitfields.bitfields.u8[1] = ((std::uint8_t)OrderCancelledBitfield2::SYMBOL
                                      | (std::uint8_t) OrderCancelledBitfield2::CAPACITY);
    rbf->bitfields.bitfields.u8[2] = ((std::uint8_t) OrderRejectedBitfield3::ACCOUNT
                                  | (std::uint8_t) OrderRejectedBitfield3::CLEARING_FIRM
                                  | (std::uint8_t) OrderRejectedBitfield3::CLEARING_ACCOUNT
                                  | (std::uint8_t) OrderRejectedBitfield3::DISPLAY_INDICATOR
                                  | (std::uint8_t) OrderRejectedBitfield3::MAX_FLOOR
                                  | (std::uint8_t) OrderRejectedBitfield3::DISCRETION_AMOUNT
                                  | (std::uint8_t) OrderRejectedBitfield3::ORDER_QTY);

    rbf->bitfields.bitfields.u8[3] = 0x00;
    rbf->bitfields.bitfields.u8[4] = ((std::uint8_t) OrderCancelledBitfield5::LEAVES_QTY);
    rbf->bitfields.bitfields.u8[5] = ((std::uint8_t) OrderCancelledBitfield6::ATTRIBUTED_QUOTE
                                      | (std::uint8_t) OrderCancelledBitfield6::EXT_EXEC_INST);
    rbf->bitfields.bitfields.u8[6] = 0x00;
    rbf->bitfields.bitfields.u8[7] = ((std::uint8_t) OrderCancelledBitfield8::ROUTING_INST);

    msg->num_param_groups++;

    buf += rbf->header.length;

    // CANCEL REJECTED
    rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::CANCEL_REJECTED_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u8[0] = 0xFF;
    rbf->bitfields.bitfields.u8[1] = ((std::uint8_t) CancelRejectedBitfield2::SYMBOL
                                      | (std::uint8_t) CancelRejectedBitfield2::CAPACITY);
    rbf->bitfields.bitfields.u8[2] = 0x00;
    rbf->bitfields.bitfields.u8[3] = 0x00;
    rbf->bitfields.bitfields.u8[4] = 0x00;
    rbf->bitfields.bitfields.u8[5] = 0x00;
    rbf->bitfields.bitfields.u8[6] = 0x00;
    rbf->bitfields.bitfields.u8[7] = 0x00;
    msg->num_param_groups++;

    buf += rbf->header.length;

    // ORDER EXECUTION
    rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::ORDER_EXECUTION_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u8[0] = 0xFF;
    rbf->bitfields.bitfields.u8[1] = ((std::uint8_t) OrderExecutionBitfield2::SYMBOL
                                      | (std::uint8_t) OrderExecutionBitfield2::CAPACITY);
    rbf->bitfields.bitfields.u8[2] = ((std::uint8_t) OrderRejectedBitfield3::ACCOUNT
                                      | (std::uint8_t) OrderRejectedBitfield3::CLEARING_FIRM
                                      | (std::uint8_t) OrderRejectedBitfield3::CLEARING_ACCOUNT
                                      | (std::uint8_t) OrderRejectedBitfield3::DISPLAY_INDICATOR
                                      | (std::uint8_t) OrderRejectedBitfield3::MAX_FLOOR
                                      | (std::uint8_t) OrderRejectedBitfield3::DISCRETION_AMOUNT
                                      | (std::uint8_t) OrderRejectedBitfield3::ORDER_QTY);
    rbf->bitfields.bitfields.u8[3] = 0x00;
    rbf->bitfields.bitfields.u8[4] = 0x00;
    rbf->bitfields.bitfields.u8[5] = ((std::uint8_t) OrderExecutionBitfield6::ATTRIBUTED_QUOTE
                                      | (std::uint8_t) OrderExecutionBitfield6::EXT_EXEC_INST);
    rbf->bitfields.bitfields.u8[6] = 0x00;
    rbf->bitfields.bitfields.u8[7] = ((std::uint8_t) OrderExecutionBitfield8::FEE_CODE
                                      | (std::uint8_t) OrderExecutionBitfield8::ROUTING_INST);
    msg->num_param_groups++;
    buf += rbf->header.length;

    // TRADE CANCEL OR CORRECT
    rbf= reinterpret_cast<ReturnBitfieldsParamGroup *>(buf);
    rbf->header.type = ParamGroupType::RETURN_BITFIELDS;
    rbf->header.length = sizeof(ReturnBitfieldsParamGroup);
    rbf->message_type = MessageType::TRADE_CANCEL_OR_CORRECT_V2;
    rbf->bitfields.num_bitfields = 8;
    rbf->bitfields.bitfields.u64 = 0x00;
    rbf->bitfields.bitfields.u8[1] = ((std::uint8_t) TradeCancelOrCorrectBitfield2::SYMBOL
                                      | (std::uint8_t) TradeCancelOrCorrectBitfield2::CAPACITY);
    msg->num_param_groups++;


#ifdef I01_DEBUG_MESSAGING
    std::cerr << m_name << ",SLR," << *reinterpret_cast<LoginRequest *>(bytes) << std::endl;
#endif

    send_helper_nolock(bytes, msglen);
    delete[] bytes;
}

void BOE20Session::send_logout_request()
{
    {
        // send helper locks, so this must not be in scope when we call it
        Mutex::scoped_lock lock(m_mutex);
        m_state = State::LOGOUT_SENT;
        m_active = false;
    }

    send_helper(LogoutRequest::create());
}

void BOE20Session::send_heartbeat()
{
    send_helper(ClientHeartbeat::create());
}

bool BOE20Session::send_new_order(Order *op)
{
    auto * bop = static_cast<BOE20Order*>(op);

    NewOrderBuffer *buf;
    if (!m_no_bufs.get(buf)) {
        std::cerr << m_name << ",ERR,NEWORDER,NO BUFFER," << *op << std::endl;
        return false;
    }

    auto *msg = &buf->data.msg;

    // set ClOrdID
    msg->cl_ord_id = create_cl_ord_id(op);
    op->client_order_id(msg->cl_ord_id.arr);
    // we store our portion of the ID
    m_id_map.insert({op->localID(), msg->cl_ord_id});

    if (UNLIKELY(!side_from_order(op, msg->side))) {
        std::cerr << m_name << ",ERR,NEWORDER,UNKNOWN SIDE," << *op << std::endl;
        m_no_bufs.release(buf);
        return false;
    }

    if (op->size() > MAX_ORDER_QTY) {
        std::cerr << m_name << ",ERR,NEWORDER,EXCEEDS ORDER QTY," << *op << std::endl;
        m_no_bufs.release(buf);
        return false;
    }

    msg->order_qty = op->size();

    // We know that m_no_buf is big enough for the largest possible message

    buf->opt->price = price_to_fixed(op->price());

    if (UNLIKELY(!exec_inst_from_order(bop, buf->opt->exec_inst))) {
        std::cerr << m_name << ",ERR,NOBF1,UNSUPPORTED EXEC INST," << *op << std::endl;
        m_no_bufs.release(buf);
        return false;
    }

    if (UNLIKELY(!ord_type_from_order(op, buf->opt->ord_type))) {
        std::cerr << m_name << ",ERR,NOBF1,UNSUPPORTED ORDER TYPE," << *op << std::endl;
        m_no_bufs.release(buf);
        return false;
    }

    if (UNLIKELY(!tif_from_order(op, buf->opt->time_in_force))) {
        std::cerr << m_name << ",ERR,NOBF1,UNSUPPORTED TIF," << *op << std::endl;
        m_no_bufs.release(buf);
        return false;
    }

    if (UNLIKELY(!display_indicator_from_order(bop, buf->opt->display_indicator))) {
        std::cerr << m_name << ",ERR,NOBF1,UNSUPPORTED DISPLAY INDICATOR" << *bop << std::endl;
        m_no_bufs.release(buf);
        return false;
    }

    if (UNLIKELY(!routing_inst_from_order(bop, buf->opt->routing_inst))) {
        std::cerr << m_name << ",ERR,NOBF,UNSUPPORTED ROUTING INST" << *bop << std::endl;
        m_no_bufs.release(buf);
        return false;
    }

    buf->opt->symbol.arr = m_bats_symbol_array[op->instrument()->esi()].arr;

    // FIXME log state for this order ...
    // message size is static...
    {
        Mutex::scoped_lock lock(m_mutex);
        msg->message_header.sequence_number = m_persist_state->data()->outbound_seqnum++;
        set_order_sent_time(op, Timestamp::now());
        send_helper_nolock(buf->data.bytes, buf->msglen);
    }
    debug_print_msg("NO", buf->data.msg);

    m_no_bufs.release(buf);
    return true;
}

Instrument * BOE20Session::find_instrument(const Symbol &symbol) const
{
    // this should probably happen somewhere else? ... I need a canonical place for instruments
    auto it = m_symbol_inst_cache.find(symbol.arr);
    if (it == m_symbol_inst_cache.end()) {
        return nullptr;
    }

    return it->second;
}

auto BOE20Session::price_to_fixed(const OE::Price &p) -> Price
{
    return static_cast<Price>(p*PRICE_DIVISOR + 0.5);
}

OE::Price BOE20Session::price_to_double(const Price &p)
{
    return static_cast<double>(p) * PRICE_DIVISOR_INV;
}

bool BOE20Session::exec_inst_from_order(BOE20Order *op, BOE20Order::ExecInst &ei) const
{
    if (BOE20Order::ExecInst::UNKNOWN != op->boe_exec_inst()) {
        // then the user has set a BOE20 specific ExecInst value
        // ... let's honor it as long as it obeys the protocol...
        switch (op->boe_exec_inst()) {
        case BOE20Order::ExecInst::UNKNOWN:
            break;
        case BOE20Order::ExecInst::DEFAULT:
            break;
        case BOE20Order::ExecInst::INTERMARKET_SWEEP:
            break;
        case BOE20Order::ExecInst::MARKET_PEG:
            break;
        case BOE20Order::ExecInst::MARKET_MAKER_PEG:
            break;
        case BOE20Order::ExecInst::PRIMARY_PEG:
            break;
        case BOE20Order::ExecInst::SUPPLEMENTAL_PEG:
            break;
        case BOE20Order::ExecInst::LISTING_MARKET_OPENING:
        case BOE20Order::ExecInst::LISTING_MARKET_CLOSING:
        case BOE20Order::ExecInst::BOTH_LISTING_MARKET_OPEN_N_CLOSE:
            if (core::MICEnum::BATS != m_mic
                && core::MICEnum::EDGX != m_mic) {
                return false;
            }
            break;
        case BOE20Order::ExecInst::MIDPOINT_DISCRETIONARY:
            if (core::MICEnum::BATY != m_mic) {
                return false;
            }
            break;
        case BOE20Order::ExecInst::MIDPOINT_PEG_TO_NBBO_MID:
            break;
        case BOE20Order::ExecInst::MIDPOINT_PEG_TO_NBBO_MID_NO_LOCK:
            break;
        case BOE20Order::ExecInst::ALTERNATIVE_MIDPOINT:
            break;
        case BOE20Order::ExecInst::LATE:
            if (core::MICEnum::BATS != m_mic) {
                return false;
            }
            break;
        case BOE20Order::ExecInst::MIDPOINT_MATCH_DEPRECATED:
            break;
        default:
            break;
        }
        ei = op->boe_exec_inst();
    } else {
        // no specific ExecInst set, so go with what we know about
        // this order...
        ei = BOE20Order::ExecInst::DEFAULT;
        if (op->type() == OrderType::MIDPOINT_PEG) {
            ei = BOE20Order::ExecInst::MIDPOINT_PEG_TO_NBBO_MID;
        }
        op->boe_exec_inst(ei);
    }

    return true;
}

bool BOE20Session::ord_type_from_order(Order *op, OrdType &ord_type)
{
    switch(op->type()) {
    case OE::OrderType::MARKET:
        ord_type = OrdType::MARKET;
        break;
    case OE::OrderType::STOP:
        ord_type = OrdType::STOP_LIMIT;
        break;
    case OE::OrderType::MIDPOINT_PEG:
        ord_type = OrdType::PEGGED;
        break;
    case OE::OrderType::LIMIT:
        ord_type = OrdType::LIMIT;
        break;
    case OE::OrderType::UNKNOWN:
    default:
        return false;
    }
    return true;
}

OE::OrderType BOE20Session::to_ord_type(const OrdType &ot)
{
    switch (ot) {
    case OrdType::MARKET:
        return OE::OrderType::MARKET;
    case OrdType::LIMIT:
        return OE::OrderType::LIMIT;
    case OrdType::STOP:
    case OrdType::STOP_LIMIT:
        return OE::OrderType::STOP;
    case OrdType::PEGGED:
        return OE::OrderType::MIDPOINT_PEG;
    default:
        return OE::OrderType::UNKNOWN;
    }
}

bool BOE20Session::tif_from_order(Order *op, TimeInForce &tif)
{
    switch(op->tif()) {
    case OE::TimeInForce::DAY:
        tif = TimeInForce::DAY;
        break;
    case OE::TimeInForce::GTC:
        tif = TimeInForce::GTC;
        break;
    case OE::TimeInForce::EXT:
        tif = TimeInForce::GTX;
        break;
    case OE::TimeInForce::AUCTION_OPEN:
        tif = TimeInForce::AT_THE_OPEN;
        break;
    case OE::TimeInForce::AUCTION_CLOSE:
        tif = TimeInForce::AT_THE_CLOSE;
        break;
    case OE::TimeInForce::IMMEDIATE_OR_CANCEL:
        tif = TimeInForce::IOC;
        break;
    case OE::TimeInForce::FILL_OR_KILL:
        tif = TimeInForce::FOK;
        break;
    case OE::TimeInForce::TIMED:
    case OE::TimeInForce::AUCTION_HALT:
    case OE::TimeInForce::UNKNOWN:
    default:
        return false;
    }
    return true;
}

OE::TimeInForce BOE20Session::to_tif(const TimeInForce &tif)
{
    switch (tif) {
    case TimeInForce::DAY:
        return OE::TimeInForce::DAY;
    case TimeInForce::GTC:
    case TimeInForce::GTD:
        return OE::TimeInForce::GTC;
    case TimeInForce::AT_THE_OPEN:
        return OE::TimeInForce::AUCTION_OPEN;
    case TimeInForce::IOC:
        return OE::TimeInForce::IMMEDIATE_OR_CANCEL;
    case TimeInForce::FOK:
        return OE::TimeInForce::FILL_OR_KILL;
    case TimeInForce::GTX:
        return OE::TimeInForce::EXT;
    case TimeInForce::AT_THE_CLOSE:
        return OE::TimeInForce::AUCTION_CLOSE;
    case TimeInForce::REG_HOURS_ONLY:
        return OE::TimeInForce::DAY;
    default:
        return OE::TimeInForce::UNKNOWN;
    }
}

bool BOE20Session::side_from_order(Order *op, Side &side)
{
    switch(op->side()) {
    case OE::Side::BUY:
        side = Side::BUY;
        break;
    case OE::Side::SELL:
        side = Side::SELL;
        break;
    case OE::Side::SHORT:
        side = Side::SELL_SHORT;
        break;
    case OE::Side::SHORT_EXEMPT:
        side = Side::SELL_SHORT_EXEMPT;
        break;
    case OE::Side::UNKNOWN:
    default:
        return false;
    }
    return true;
}

OE::Side BOE20Session::to_side(const Side &s) {
    switch (s) {
    case Side::BUY:
        return OE::Side::BUY;
    case Side::SELL:
        return OE::Side::SELL;
    case Side::SELL_SHORT:
        return OE::Side::SHORT;
    case Side::SELL_SHORT_EXEMPT:
        return OE::Side::SHORT_EXEMPT;
    default:
        return OE::Side::UNKNOWN;
    }
}

bool BOE20Session::display_indicator_from_order(BOE20Order* op, DisplayIndicator& di) const
{
    if (BOE20Order::DisplayIndicator::UNKNOWN == op->boe_display_indicator()) {
        di = BOE20Order::DisplayIndicator::DEFAULT;
        op->boe_display_indicator(di);
    } else {
        di = op->boe_display_indicator();
    }
    return true;
}

bool BOE20Session::routing_inst_from_order(BOE20Order* op, RoutingInst& ri) const
{
    if (BOE20Order::RoutingInstFirstChar::UNKNOWN == op->boe_routing_inst().layout.first) {
        ri.set(BOE20Order::RoutingInstFirstChar::BOOK_ONLY);
        op->boe_routing_inst(ri);
    } else {
        ri = op->boe_routing_inst();
    }
    return true;
}

auto BOE20Session::create_cl_ord_id(i01::OE::Order *op) const -> msgs::ClOrdID
{
    // we have 20 characters in ascii range 33-126 except comma,
    // semicolon, and pipe but HPR uses a format LSSSSSSCCCCCCCCCCCCC
    // .... first char for locate broker ... next 6 for symbol (right
    // padded with '0'), and remaining 13 are ours ... so we only have
    // 13 chars

    msgs::ClOrdID id;
    ClOrdID::HPRSymbol symbol;
    for (auto i = 0U; i < symbol.size(); i++) {
        if (i < m_bats_symbol_array[op->instrument()->esi()].arr.size()
            && '\0' != m_bats_symbol_array[op->instrument()->esi()].arr[i]) {
            symbol[i] = m_bats_symbol_array[op->instrument()->esi()].arr[i];
        } else {
            // the bats symbol here is null terminated but we want to fill with '0'
            symbol[i] = (std::uint8_t) '0';
        }
    }
    id.set(m_hpr_broker_loc, op->local_account(), op->localID(), symbol);
    return id;
}


void BOE20Session::prepare_new_order_buffer(NewOrderBuffer &nob)
{
    auto *msg = &nob.data.msg;
    fill_msg_header(&msg->message_header, MessageType::NEW_ORDER_V2);

    msg->num_bitfields = 0;

    // end of message
    nob.eom = nob.data.bytes + sizeof(NewOrder);

    prepare_new_order_bitfield1(nob.eom);
    msg->num_bitfields++;
    nob.eom++;
    prepare_new_order_bitfield2(nob.eom);
    msg->num_bitfields++;
    nob.eom++;
    prepare_new_order_bitfield3(nob.eom);
    msg->num_bitfields++;
    nob.eom++;
    prepare_new_order_bitfield4(nob.eom);
    msg->num_bitfields++;
    nob.eom++;
    prepare_new_order_bitfield5(nob.eom);
    msg->num_bitfields++;
    nob.eom++;
    prepare_new_order_bitfield6(nob.eom);
    msg->num_bitfields++;
    nob.eom++;

    // overlay the optional fields in the order we will append them
    nob.opt = reinterpret_cast<NewOrderBuffer::OptionalFields *>(nob.eom);
    nob.eom += sizeof(NewOrderBuffer::OptionalFields);

    process_new_order_bitfields1(nob.opt);
    process_new_order_bitfields2(nob.opt);
    process_new_order_bitfields3(nob.opt);
    process_new_order_bitfields4(nob.opt);
    process_new_order_bitfields5(nob.opt);

    nob.msglen = nob.eom - nob.data.bytes;
    // we don't count the first 0xBABA
    msg->message_header.message_length = static_cast<MessageLength>(nob.msglen-2U);
}

inline
void BOE20Session::prepare_new_order_bitfield1(std::uint8_t * bufidx)
{
    *bufidx = 0;
    *bufidx |= (std::uint8_t) NewOrderBitfield1::CLEARING_FIRM;
    *bufidx |= (std::uint8_t) NewOrderBitfield1::CLEARING_ACCOUNT;
    *bufidx |= (std::uint8_t) NewOrderBitfield1::PRICE;
    *bufidx |= (std::uint8_t) NewOrderBitfield1::EXEC_INST;
    *bufidx |= (std::uint8_t) NewOrderBitfield1::ORD_TYPE;
    *bufidx |= (std::uint8_t) NewOrderBitfield1::TIME_IN_FORCE;
    *bufidx |= (std::uint8_t) NewOrderBitfield1::MIN_QTY;
    *bufidx |= (std::uint8_t) NewOrderBitfield1::MAX_FLOOR;
}

inline
void BOE20Session::prepare_new_order_bitfield2(std::uint8_t * bufidx)
{
    *bufidx = 0;
    *bufidx |= (std::uint8_t) NewOrderBitfield2::SYMBOL;
    *bufidx |= (std::uint8_t) NewOrderBitfield2::CAPACITY;
    *bufidx |= (std::uint8_t) NewOrderBitfield2::ROUTING_INST;
}

inline
void BOE20Session::prepare_new_order_bitfield3(std::uint8_t * bufidx)
{
    *bufidx = 0;
    *bufidx |= (std::uint8_t) NewOrderBitfield3::ACCOUNT;
    *bufidx |= (std::uint8_t) NewOrderBitfield3::DISPLAY_INDICATOR;
    *bufidx |= (std::uint8_t) NewOrderBitfield3::MAX_REMOVE_PCT;
    *bufidx |= (std::uint8_t) NewOrderBitfield3::DISCRETION_AMOUNT;
    *bufidx |= (std::uint8_t) NewOrderBitfield3::PEG_DIFFERENCE;
}

inline
void BOE20Session::prepare_new_order_bitfield4(std::uint8_t * bufidx)
{
    *bufidx = 0;
}

inline
void BOE20Session::prepare_new_order_bitfield5(std::uint8_t * bufidx)
{
    *bufidx = 0;
    *bufidx |= (std::uint8_t) NewOrderBitfield5::ATTRIBUTED_QUOTE;
    *bufidx |= (std::uint8_t) NewOrderBitfield5::EXT_EXEC_INST;
}

inline
void BOE20Session::prepare_new_order_bitfield6(std::uint8_t * bufidx)
{
    *bufidx = 0;
}

void BOE20Session::process_new_order_bitfields1(NewOrderBuffer::OptionalFields *opt)
{
    copy_to_array(opt->clearing_firm.arr, m_clearing_firm);
    copy_to_array(opt->clearing_account.arr, m_clearing_account);
    opt->min_qty = 0;
    opt->max_floor = 0;
}

void BOE20Session::process_new_order_bitfields2(NewOrderBuffer::OptionalFields *opt)
{
    opt->capacity = Capacity::AGENCY;
}

void BOE20Session::process_new_order_bitfields3(NewOrderBuffer::OptionalFields *opt)
{
    copy_to_array(opt->account.arr, "DEFG");
    opt->max_remove_pct = 0;
    opt->discretion_amount =0;
    opt->peg_difference = 0;
}

void BOE20Session::process_new_order_bitfields4(NewOrderBuffer::OptionalFields *opt)
{
}

void BOE20Session::process_new_order_bitfields5(NewOrderBuffer::OptionalFields *opt)
{
    opt->attributed_quote = 'N';
    opt->ext_exec_inst = 'N';
}

void BOE20Session::process_new_order_bitfields6(NewOrderBuffer::OptionalFields *opt)
{
}

void BOE20Session::prepare_cancel_order_buffer(CancelOrderBuffer &cob)
{
    auto * msg = &cob.data.msg;
    fill_msg_header(&msg->message_header, MessageType::CANCEL_ORDER_V2);

    msg->num_bitfields = 1;

    cob.eom = cob.data.bytes + sizeof(CancelOrder);

    prepare_cancel_order_bitfield1(cob.eom);
    cob.eom++;

    cob.opt = reinterpret_cast<CancelOrderBuffer::OptionalFields *>(cob.eom);
    cob.eom += sizeof(CancelOrderBuffer::OptionalFields);

    process_cancel_order_bitfields1(cob.opt);

    cob.msglen = cob.eom - cob.data.bytes;
    msg->message_header.message_length = static_cast<MessageLength>(cob.msglen-2U);
}

void BOE20Session::prepare_cancel_order_bitfield1(std::uint8_t *bufidx)
{
    *bufidx = 0;
    *bufidx |= (std::uint8_t) CancelOrderBitfield1::CLEARING_FIRM;
}

void BOE20Session::process_cancel_order_bitfields1(CancelOrderBuffer::OptionalFields *opt)
{
    copy_to_array(opt->clearing_firm.arr, m_clearing_firm);
}

void BOE20Session::prepare_modify_order_buffer(ModifyOrderBuffer &mob)
{
    auto * msg = &mob.data.msg;

    fill_msg_header(&msg->message_header, MessageType::MODIFY_ORDER_V2);

    msg->num_bitfields = 2;

    mob.eom = mob.data.bytes + sizeof(ModifyOrder);

    prepare_modify_order_bitfield1(mob.eom);
    mob.eom++;
    prepare_modify_order_bitfield2(mob.eom);
    mob.eom++;

    mob.opt = reinterpret_cast<ModifyOrderBuffer::OptionalFields *>(mob.eom);
    mob.eom += sizeof(ModifyOrderBuffer::OptionalFields);

    process_modify_order_bitfields1(mob.opt);
    process_modify_order_bitfields2(mob.opt);

    mob.msglen = mob.eom - mob.data.bytes;
    msg->message_header.message_length = static_cast<MessageLength>(mob.msglen-2U);
}

void BOE20Session::prepare_modify_order_bitfield1(std::uint8_t *bufidx)
{
    *bufidx = 0;
    *bufidx |= (std::uint8_t) ModifyOrderBitfield1::CLEARING_FIRM;
    *bufidx |= (std::uint8_t) ModifyOrderBitfield1::ORDER_QTY;
    *bufidx |= (std::uint8_t) ModifyOrderBitfield1::PRICE;
    *bufidx |= (std::uint8_t) ModifyOrderBitfield1::ORD_TYPE;
    *bufidx |= (std::uint8_t) ModifyOrderBitfield1::CANCEL_ORIG_ON_REJECT;
    *bufidx |= (std::uint8_t) ModifyOrderBitfield1::EXEC_INST;
    *bufidx |= (std::uint8_t) ModifyOrderBitfield1::SIDE;
}

void BOE20Session::prepare_modify_order_bitfield2(std::uint8_t *bufidx)
{
    *bufidx = 0;
    // HPR wants this present but with no bits set
}

void BOE20Session::process_modify_order_bitfields1(ModifyOrderBuffer::OptionalFields *opt)
{
    copy_to_array(opt->clearing_firm.arr, m_clearing_firm);
    opt->cancel_orig_on_reject = (m_cancel_orig_on_reject ? 'Y' : 'N');
}

void BOE20Session::process_modify_order_bitfields2(ModifyOrderBuffer::OptionalFields *opt)
{
    opt->max_floor = 0;
}

bool BOE20Session::send_cancel(Order *op)
{
    auto it = m_id_map.find(op->localID());
    if (it == m_id_map.end()) {
        // not in our cache ... could be because we had a recovery ... let's try and remake it...
        std::cerr << m_name << ",WARN,CXL,UNKNOWN LOCALID," << *op << std::endl;
        auto clordid = create_cl_ord_id(op);
        auto ret = m_id_map.insert({op->localID(), clordid});
        it = ret.first;
    }

    CancelOrderBuffer *buf;
    if (!m_cxl_bufs.get(buf)) {
        std::cerr << m_name << ",ERR,CXL,NO BUFFER," << *op << std::endl;
        return false;
    }
    auto *msg = &buf->data.msg;

    msg->orig_cl_ord_id = it->second;
    {
        Mutex::scoped_lock lock(m_mutex);
        msg->message_header.sequence_number = m_persist_state->data()->outbound_seqnum++;
        set_order_last_request_time(op, Timestamp::now());
        send_helper_nolock(buf->data.bytes, buf->msglen);
    }

    debug_print_msg("CXLO", buf->data.msg);
    m_cxl_bufs.release(buf);
    return true;
}

bool BOE20Session::send_modify(Order *op, Size newqty)
{
    auto * bop = static_cast<BOE20Order *>(op);

    auto it = m_id_map.find(op->localID());
    if (it == m_id_map.end()) {
        std::cerr << m_name << ",ERR,MOD,UNKNOWN LOCALID," << *op << std::endl;
        return false;
    }

    ModifyOrderBuffer *buf;
    if (!m_mod_bufs.get(buf)) {
        std::cerr << m_name << ",ERR,MOD,NO BUFFER," << *op << std::endl;
        return false;
    }

    auto *msg = &buf->data.msg;

    msg->new_cl_ord_id = it->second;
    msg->orig_cl_ord_id = it->second;

    // BATS used the delta from existing order qty less newqty and applies to leaves qty
    buf->opt->order_qty = newqty + (op->size() - op->cancelled_size() ) - op->open_size();
    buf->opt->price = price_to_fixed(op->price());
    if (UNLIKELY(!ord_type_from_order(op, buf->opt->ord_type))) {
        std::cerr << m_name << ",ERR,MODO,UNSUPPORTED ORDER TYPE," << *op << std::endl;
        m_mod_bufs.release(buf);
        return false;
    }

    if (UNLIKELY(!exec_inst_from_order(bop, buf->opt->exec_inst))) {
        std::cerr << m_name << ",ERR,MODO,UNSUPPORTED EXEC INST," << *op << std::endl;
        m_mod_bufs.release(buf);
        return false;
    }

    if (UNLIKELY(!side_from_order(op, buf->opt->side))) {
        std::cerr << m_name << ",ERR,MODO,UNKNOWN SIDE," << *op << std::endl;
        m_mod_bufs.release(buf);
        return false;
    }

    {
        Mutex::scoped_lock lock(m_mutex);
        msg->message_header.sequence_number = m_persist_state->data()->outbound_seqnum++;
        set_order_last_request_time(op, Timestamp::now());
        send_helper_nolock(buf->data.bytes, buf->msglen);
    }

    debug_print_msg("MODO", buf->data.msg);
    m_mod_bufs.release(buf);
    return true;
}

void BOE20Session::send_helper_nolock(const std::uint8_t * buf, const size_t &len)
{
#ifdef I01_DEBUG_MESSAGING
    std::cerr << m_name << ",SNDHLPR," << len << "," << std::hex;
    for (auto i = 0U; i < len; i++) {
        std::cerr << (int) buf[i] << " ";
    }
    std::cerr << std::dec << std::endl;
#endif
    // we dont lock here, we assume the caller is locking around us,
    // as they need to update the seqnum too
    if (m_connection.send(buf, len) < 0) {
        int errn = m_connection.socket_errno();
        std::cerr << m_name << ",ERR,SNDHLPR," << strerror(errn) << std::endl;
    } else {
        m_last_message_sent_ts = Timestamp::now();
    }
}

Order * BOE20Session::on_create_replay_order(const Timestamp& ts, Instrument *inst, const MIC& mic,
                                             const LocalAccount local_account, const LocalID local_id,
                                             const OE::Price price, const OE::Size size, const OE::Side side,
                                             const OE::TimeInForce tif, const OrderType type,
                                             const UserData ud)
{
    // TODO: check the MIC is correct ... need to add support for other BOE targets
    if (mic != core::MICEnum::BATS) {
        std::cerr << m_name << ",ERR,CREATE REPLAY ORDER,MIC IS NOT BATS," << mic << std::endl;
        return nullptr;
    }
    auto op = m_order_manager_p->create_order<BATSOrder>(inst, price, size, side, tif, type, nullptr, nullptr);
    op->session(this);
    op->sent_time(ts);
    op->localID(local_id);
    op->client_order_id(create_cl_ord_id(op).arr);
    return op;
}

std::string BOE20Session::to_string() const
{
    std::ostringstream ss;
    {
        Mutex::scoped_lock lock(m_mutex);

        ss << "HBCT," << m_connection.state()
           << ",SESSION_STATE," << m_state
           << ",OUTSEQ," << m_persist_state->data()->outbound_seqnum;
        for (auto i = 0U; i < msgs::MAX_NUMBER_OF_MATCHING_UNITS; i++) {
            auto seq = m_persist_state->data()->inbound_seqnum[i];
            if (seq) {
                ss << ",INSEQ," << i << "," << seq;
            }
        }
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const BOE20Session::State& s)
{
    switch (s) {
    case BOE20Session::State::UNINITIALIZED:
        return os << "UNINITIALIZED";

    case BOE20Session::State::UNCONNECTED:
        return os << "UNCONNECTED";

    case BOE20Session::State::WAITING_FOR_LOGIN:
        return os << "WAITING_FOR_LOGIN";

    case BOE20Session::State::REPLAY:
        return os << "REPLAY";

    case BOE20Session::State::ACTIVE:
        return os << "ACTIVE";

    case BOE20Session::State::LOGOUT_SENT:
        return os << "LOGOUT_SENT";

    case BOE20Session::State::USER_LOGOUT_REQUEST:
        return os << "USER_LOGOUT_REQUEST";

    case BOE20Session::State::DISCONNECT:
        return os << "DISCONNECT";

    case BOE20Session::State::ERROR_DISCONNECT:
        return os << "ERROR_DISCONNECT";

    default:
        return os;
    }
}
}}}
