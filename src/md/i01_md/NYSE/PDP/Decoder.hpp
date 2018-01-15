#pragma once

#include <arpa/inet.h>

#include <regex>

#include <boost/algorithm/string.hpp>

#include <i01_core/Config.hpp>

#include <i01_net/IPAddress.hpp>

#include <i01_md/ConfigReader.hpp>
#include <i01_md/Decoder.hpp>
#include <i01_md/DecoderEvents.hpp>
#include <i01_md/MsgStream.hpp>

#include <i01_md/NYSE/PDP/Messages.hpp>

namespace i01 { namespace core {
struct MIC;
}}

namespace i01 { namespace MD { namespace NYSE { namespace PDP {

using PDPMsgStream = MD::MsgStream<Messages::MsgSeqNum,core::Timestamp>;
using UnitState = PDPMsgStream;

std::unique_ptr<UnitState> create_pdp_unit_state(const core::MIC& mic, const std::string& feed_name, const std::string& unit_name, const core::Config::storage_type& cfg);

template<typename RawListenerType>
class Decoder :
        public net::UDPPktListener<Decoder<RawListenerType> >,
        public i01::MD::MultiStreamDecoder<RawListenerType, PDPMsgStream> {
private:
    static const int MAX_CHANNELS = UINT8_MAX + 1;
    using Timestamp = core::Timestamp;
    using msd_type = MultiStreamDecoder<RawListenerType, PDPMsgStream>;

public:
    Decoder(RawListenerType *rlt, const std::vector<std::string> & feed_names = {"OB", "IMB"});
    virtual ~Decoder() = default;

    virtual void init(const core::Config::storage_type &cfg) override final;

    void on_runt_packet(std::uint32_t addr, std::uint16_t port, std::uint8_t *buf, std::size_t len, std::size_t pkt_len) override final {
        std::cerr << "PDP Decoder Received runt packet from " << net::ip_addr_to_str({addr,port}) << ", packet length is " << pkt_len << std::endl;
    }

    void handle_payload(std::uint32_t ip_src_addr, std::uint16_t udp_src_port, std::uint32_t ip_dst_addr, std::uint16_t udp_dst_port, std::uint8_t *buf, std::size_t len, const Timestamp *ts);

    virtual void on_recv(const core::Timestamp & ts, void *, const std::uint8_t *buf, const ssize_t & len) override final;

    virtual void on_timer(const core::Timestamp&, void * userdata, std::uint64_t iter) override final;

    template<typename MsgType>
    void debug_print_msg(const char *prefix, const Timestamp &ts, std::uint32_t seqnum, MsgType *msg)
    {
#ifdef I01_DEBUG_MESSAGING
        std::cerr << "DBG,"
                  << this->m_mic.name() << ","
                  << prefix << ","
                  << ts << ","
                  << seqnum << ","
                  << *msg
                  << std::endl;
#endif
    }

    void debug_print_msg(const Timestamp &ts, std::uint32_t seqnum, Messages::FullUpdate *msg)
    {
#ifdef I01_DEBUG_MESSAGING
        auto npp = (msg->msg_size - sizeof(Messages::FullUpdate)) / sizeof(Messages::FullUpdatePricePoint);
        std::cerr << "DBG,"
                  << this->m_mic.name() << ","
                  << "FU,"
                  << ts << ","
                  << seqnum << ","
                  << *msg << ","
                  << npp;
        std::uint8_t *bufidx = reinterpret_cast<std::uint8_t*>(msg);
        bufidx += sizeof(Messages::FullUpdate);
        for (auto i = 0U; i < npp; i++) {
            auto fpp = reinterpret_cast<Messages::FullUpdatePricePoint *>(bufidx);
            std::cerr << "," << *fpp;
            bufidx += sizeof(Messages::FullUpdatePricePoint);
        }
        std::cerr << std::endl;
#endif
    }

    void debug_print_msg(const Timestamp &ts, std::uint32_t seqnum, Messages::DeltaUpdate *msg)
    {
#ifdef I01_DEBUG_MESSAGING
        auto npp = (msg->msg_size - sizeof(Messages::DeltaUpdate)) / sizeof(Messages::DeltaUpdatePricePoint);
        std::cerr << "DBG,"
                  << this->m_mic.name() << ","
                  << "DU,"
                  << ts << ","
                  << seqnum << ","
                  << *msg << ","
                  << npp;
        std::uint8_t * bufidx = reinterpret_cast<std::uint8_t *>(msg);
        bufidx += sizeof(Messages::DeltaUpdate);
        for (auto i = 0U; i < npp; i++) {
            auto fpp = reinterpret_cast<Messages::DeltaUpdatePricePoint *>(bufidx);
            std::cerr << "," << *fpp;
            bufidx += sizeof(Messages::DeltaUpdatePricePoint);
        }
        std::cerr << std::endl;
#endif
    }

private:
    std::size_t handle_message_(std::uint32_t ip_addr, std::uint16_t udp_port, std::uint32_t seqnum, PDPMsgStream &channel, Messages::MessageHeader *header, std::uint8_t *buf, const Timestamp &ts);

private:
    std::vector<std::string> m_feed_names;
};

template<typename RLT>
Decoder<RLT>::Decoder(RLT *rlt, const std::vector<std::string> &fn) :
    i01::MD::MultiStreamDecoder<RLT,PDPMsgStream>(rlt, core::MICEnum::XNYS),
    m_feed_names(fn)
{
}


template<typename RLT>
void Decoder<RLT>::init(const core::Config::storage_type &cfg)
{
    std::vector<std::string> stream_names;
    typename msd_type::address_name_map_type addr_name_map;

    for (const auto & fn : m_feed_names) {
        MD::ConfigReader::read_stream_names(cfg, this->m_mic.name(), fn, stream_names, addr_name_map);
    }
    this->init_streams(stream_names, addr_name_map);
}

template<typename RLT>
void Decoder<RLT>::on_timer(const core::Timestamp& ts, void * userdata, std::uint64_t iter)
{
    auto& stream = *reinterpret_cast<UnitState *>(userdata);
    auto stream_ts = stream.timestamp();
    auto diff = ts - stream_ts;

    if (diff >= stream.data_timeout()) {
        if (!stream.in_data_timeout()) {
            this->notify(ts, TimeoutMsg{}, true, stream.name(), stream.index(), stream_ts);
            stream.in_data_timeout(true);
        }
    } else {
        // turn off alarm ?
        if (stream.in_data_timeout()) {
            stream.in_data_timeout(false);
            this->notify(ts, TimeoutMsg{}, false, stream.name(), stream.index(), stream_ts);
        }
    }
}

/// this version is just an intercept to get the channel info from the deprecated decoder interface and pass to the SocketListener version
template<typename LT>
void Decoder<LT>::handle_payload(std::uint32_t ip_src_addr, std::uint16_t udp_src_port, std::uint32_t ip_dst_addr, std::uint16_t udp_dst_port, std::uint8_t *buf, std::size_t len, const Timestamp *ts)
{
    auto & channel = this->get_stream(ip_dst_addr, udp_dst_port);
    this->on_recv(*ts, &channel, buf, len);
}

template<typename LT>
void Decoder<LT>::on_recv(const core::Timestamp& ts, void *userdata, const std::uint8_t *const_buf, const ssize_t& len)
{
    auto buf = const_cast<std::uint8_t *>(const_buf);
    auto& channel = *reinterpret_cast<PDPMsgStream *>(userdata);
    auto ip_dst_addr = std::uint32_t{};
    auto udp_dst_port = std::uint16_t{};

    std::uint8_t *bufidx = buf;

    auto header = reinterpret_cast<Messages::MessageHeader *>(bufidx);
    // binary numbers are big endian here ...
    header->msg_size = ntohs(header->msg_size);
    if (header->msg_size > len-2) {
        // FIXME ...
        on_runt_packet(ip_dst_addr, udp_dst_port, buf,len, header->msg_size);
        return;
    }

    header->send_time = ntohl(header->send_time);

    header->msg_seq_num = ntohl(header->msg_seq_num);

    channel.increment_pkt_count();

    if (header->msg_seq_num < channel.expect_seqnum()) {
        return;
    } else if (header->msg_seq_num > channel.expect_seqnum()) {
        this->notify(ts, MD::GapMsg(), ip_dst_addr, udp_dst_port, static_cast<std::uint8_t>(channel.index()), channel.expect_seqnum(), header->msg_seq_num, channel.timestamp());
        channel.expect_seqnum(header->msg_seq_num);
    }

    header->msg_type = static_cast<Types::MsgType>(ntohs(static_cast<std::uint16_t>(header->msg_type)));
    bufidx += sizeof(Messages::MessageHeader);
    channel.timestamp(ts);
    std::uint32_t seqnum = channel.expect_seqnum();

    // send this before channel seqnum has been incremented, so we know the seqnum of the start pkt
    this->notify(ts, MD::StartOfPktMsg(), seqnum, channel.index());

    channel.increment_seqnum();

    // deal with multiple messages
    while (bufidx < buf + len) {

        channel.increment_msg_count();

        // this will update the pointer to the next message in the body since it does the switch on the message type
        auto n = handle_message_(ip_dst_addr, udp_dst_port, seqnum, channel, header, bufidx, ts);
        if (0 == n) {
            // got a heartbeat
            break;
        }
        bufidx += n;
    }
    this->notify(ts, MD::EndOfPktMsg(), seqnum, channel.index());
}

template<typename LT>
std::size_t Decoder<LT>::handle_message_(std::uint32_t ip_dst_addr, std::uint16_t udp_dst_port, std::uint32_t seqnum, PDPMsgStream &channel, Messages::MessageHeader *header, std::uint8_t *bufidx, const Timestamp &ts)
{
    switch (header->msg_type) {
    case Types::MsgType::NYSE_OPENING_IMBALANCE:
        {
            // most of the message is the same between the two ...
            auto msg = reinterpret_cast<Messages::OpeningImbalance *>(bufidx);

            msg->reference_price_numerator = ntohl(msg->reference_price_numerator);
            msg->imbalance_quantity = ntohl(msg->imbalance_quantity);
            msg->paired_quantity = ntohl(msg->paired_quantity);
            msg->clearing_price_numerator = ntohl(msg->clearing_price_numerator);
            msg->source_time = ntohl(msg->source_time);
            msg->ssr_filing_price_numerator = ntohl(msg->ssr_filing_price_numerator);
            debug_print_msg("OI", ts, seqnum, msg);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);
        }
        return sizeof(Messages::OpeningImbalance);

    case Types::MsgType::NYSE_CLOSING_IMBALANCE:
        {
            // most of the message is the same between the two ...
            auto msg = reinterpret_cast<Messages::ClosingImbalance *>(bufidx);
            msg->reference_price_numerator = ntohl(msg->reference_price_numerator);
            msg->imbalance_quantity = ntohl(msg->imbalance_quantity);
            msg->paired_quantity = ntohl(msg->paired_quantity);
            msg->continuous_book_clearing_price_numerator = ntohl(msg->continuous_book_clearing_price_numerator);
            msg->source_time = ntohl(msg->source_time);
            msg->closing_only_clearing_price_numerator = ntohl(msg->closing_only_clearing_price_numerator);
            debug_print_msg("CI", ts, seqnum, msg);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);
        }
        return sizeof(Messages::ClosingImbalance);

    case Types::MsgType::OPENBOOK_FULL_UPDATE:
        {
            auto msg = reinterpret_cast<Messages::FullUpdate *>(bufidx);

            msg->msg_size = ntohs(msg->msg_size);
            msg->security_index = ntohs(msg->security_index);
            msg->source_time = ntohl(msg->source_time);
            msg->source_time_microseconds = ntohs(msg->source_time_microseconds);
            msg->symbol_seqnum = ntohl(msg->symbol_seqnum);
            msg->mpv = ntohs(msg->mpv);

            auto npp = (msg->msg_size - sizeof(Messages::FullUpdate)) / sizeof(Messages::FullUpdatePricePoint);
            bufidx += sizeof(Messages::FullUpdate);
            for (auto i = 0U; i < npp; i++) {
                auto fpp = reinterpret_cast<Messages::FullUpdatePricePoint *>(bufidx);

                fpp->price_numerator = ntohl(fpp->price_numerator);
                fpp->volume = ntohl(fpp->volume);
                fpp->num_orders = ntohs(fpp->num_orders);

                bufidx += sizeof(Messages::FullUpdatePricePoint);
            }
            debug_print_msg(ts, seqnum, msg);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);

            return msg->msg_size;
        }
        break;

    case Types::MsgType::OPENBOOK_DELTA_UPDATE:
        {
            auto msg = reinterpret_cast<Messages::DeltaUpdate *>(bufidx);

            msg->msg_size = ntohs(msg->msg_size);
            msg->security_index = ntohs(msg->security_index);
            msg->source_time = ntohl(msg->source_time);
            msg->source_time_microseconds = ntohs(msg->source_time_microseconds);
            msg->symbol_seqnum = ntohl(msg->symbol_seqnum);
            auto npp = (msg->msg_size - sizeof(Messages::DeltaUpdate)) / sizeof(Messages::DeltaUpdatePricePoint);
            bufidx += sizeof(Messages::DeltaUpdate);
            for (auto i = 0U; i < npp; i++) {
                auto dupp = reinterpret_cast<Messages::DeltaUpdatePricePoint *>(bufidx);

                dupp->price_numerator = ntohl(dupp->price_numerator);
                dupp->volume = ntohl(dupp->volume);
                dupp->chg_qty = ntohl(dupp->chg_qty);
                dupp->num_orders = ntohs(dupp->num_orders);
                bufidx += sizeof(Messages::DeltaUpdatePricePoint);
            }

            debug_print_msg(ts, seqnum, msg);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);

            return msg->msg_size;
        }
        break;

    case Types::MsgType::HEARTBEAT:
        {
            this->notify(ts, MD::HeartbeatMsg(), ip_dst_addr, udp_dst_port, static_cast<std::uint8_t>(1), seqnum);
        }
        return 0;

    case Types::MsgType::SEQUENCE_NUMBER_RESET:
        {
            auto msg = reinterpret_cast<Messages::SequenceNumberReset *>(bufidx);
            msg->next_seqnum = ntohl(msg->next_seqnum);
            channel.expect_seqnum(msg->next_seqnum);
            debug_print_msg("SNR", ts, seqnum, msg);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);
        }
        return sizeof(Messages::SequenceNumberReset);
    case Types::MsgType::MESSAGE_UNAVAILABLE:
        {
            auto msg = reinterpret_cast<Messages::MessageUnavailable *>(bufidx);
            msg->begin_seqnum = ntohl(msg->begin_seqnum);
            msg->end_seqnum = ntohl(msg->end_seqnum);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);
        }
        return sizeof(Messages::MessageUnavailable);

    case Types::MsgType::RETRANSMISSION_RESPONSE:
        {
            auto msg = reinterpret_cast<Messages::RetransmissionResponse *>(bufidx);
            msg->source_seqnum = ntohl(msg->source_seqnum);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);
        }
        return sizeof(Messages::RetransmissionResponse);

    case Types::MsgType::RETRANSMISSION_REQUEST:
        {
            auto msg = reinterpret_cast<Messages::RetransmissionRequest *>(bufidx);
            msg->begin_seqnum = ntohl(msg->begin_seqnum);
            msg->end_seqnum = ntohl(msg->end_seqnum);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);
        }
        return sizeof(Messages::RetransmissionRequest);

    case Types::MsgType::REFRESH_REQUEST:
        {
            auto msg = reinterpret_cast<Messages::BookRefreshRequest *>(bufidx);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);
        }
        return sizeof(Messages::BookRefreshRequest);

    case Types::MsgType::HEARTBEAT_RESPONSE:
        {
            auto msg = reinterpret_cast<Messages::HeartbeatResponse *>(bufidx);
            this->notify(ts, msg, ip_dst_addr, udp_dst_port, seqnum);
        }
        return sizeof(Messages::HeartbeatResponse);
    default:
        break;
    }
    // not reached
    return 0;
}

}}}}
