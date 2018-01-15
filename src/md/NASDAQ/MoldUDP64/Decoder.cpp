#include <i01_md/NASDAQ/MoldUDP64/Decoder.hpp>

namespace i01 { namespace MD { namespace NASDAQ { namespace MoldUDP64 { namespace Decoder {

void MUDecoder::debug_print_msg(const char * prefix, const Timestamp &ts, DownstreamPacketHeader *msg)
{
#ifdef I01_DEBUG_MESSAGING
    std::cerr << "DBG," << prefix << ","
              << ts << ","
              << *msg
              << std::endl;
#endif
}

void MUDecoder::on_recv(const core::Timestamp & ts, void * userdata, const std::uint8_t *const_buf, const ssize_t & len)
{
    auto* buf = const_cast<std::uint8_t *>(const_buf);
    decode_downstream_packet(ts, (char *)buf, static_cast<std::size_t>(len));
}

void MUDecoder::on_timer(const core::Timestamp& ts, void * userdata, std::uint64_t iter)
{
    auto diff = ts - m_state.timestamp;

    if (diff >= m_state.data_timeout) {
        if (!m_state.in_data_timeout) {
            on_data_timeout(ts, true, m_state.timestamp);
            m_state.in_data_timeout = true;
        }
    } else {
        if (m_state.in_data_timeout) {
            m_state.in_data_timeout = false;
            on_data_timeout(ts, false, m_state.timestamp);
        }
    }
}
}}}}}
