#include <i01_oe/OrderLogFormat.hpp>
#include <i01_oe/BlotterReader.hpp>
#include <i01_oe/BlotterReaderListener.hpp>


namespace olf = i01::OE::OrderLogFormat;

namespace i01 { namespace OE {

void BlotterReader::rewind()
{
    do_rewind();
}

void BlotterReader::next()
{
    if (!at_end()) {
        do_next();
    }
}

bool BlotterReader::at_end()
{
    return do_at_end();
}

void BlotterReader::replay()
{
    while (!at_end()) {
        next();
    }
}

std::int32_t BlotterReader::decode(const char *buf, std::uint64_t len)
{
    auto bufidx = buf;
    auto end_of_buffer = buf + len;
    if (sizeof(olf::MessageHeader) > len) {
        return -static_cast<int>(Errors::NO_HEADER_SIZE);
    }

    auto header = reinterpret_cast<const olf::MessageHeader *>(bufidx);
    bufidx += sizeof(olf::MessageHeader);
    const auto& timestamp = Timestamp{header->timestamp.tv_sec, header->timestamp.tv_nsec};
    if (bufidx + header->length > end_of_buffer) {
        // this is a malformed log
        return -static_cast<int>(Errors::NO_MSG_SIZE);
    }

    auto olfmt = static_cast<olf::MessageType>(header->type);

    switch (olfmt) {
    case olf::MessageType::TIMESTAMP:
        break;
    case olf::MessageType::START_OF_LOG:
        notify(&BlotterReaderListener::on_log_start, timestamp);
        break;
    case olf::MessageType::END_OF_LOG:
        notify(&BlotterReaderListener::on_log_end, timestamp);
        break;
    case olf::MessageType::NEW_INSTRUMENT:
        notify(&BlotterReaderListener::on_log_new_instrument, timestamp,
                *reinterpret_cast<const olf::NewInstrumentBody *>(bufidx));
        break;
    case olf::MessageType::NEW_ORDER:
        notify(&BlotterReaderListener::on_log_new_order, timestamp,
                *reinterpret_cast<const olf::NewOrderBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_LOCAL_REJECT:
        notify(&BlotterReaderListener::on_log_local_reject, timestamp,
               *reinterpret_cast<const olf::OrderLocalRejectBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_SENT:
        notify(&BlotterReaderListener::on_log_order_sent, timestamp,
               *reinterpret_cast<const olf::OrderSentBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_ACKNOWLEDGEMENT:
        notify(&BlotterReaderListener::on_log_acknowledged, timestamp,
               *reinterpret_cast<const olf::OrderAckBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_PARTIAL_FILL:
        // intentionally fall through to ORDER_FILL:
    case olf::MessageType::ORDER_FILL:
        notify(&BlotterReaderListener::on_log_filled, timestamp,
               olfmt == olf::MessageType::ORDER_PARTIAL_FILL ? true : false,
               *reinterpret_cast<const olf::OrderFillBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_CANCEL_REQUEST:
        notify(&BlotterReaderListener::on_log_pending_cancel, timestamp,
               *reinterpret_cast<const olf::OrderCxlReqBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_PARTIAL_CANCEL:
        notify(&BlotterReaderListener::on_log_partial_cancel, timestamp,
               *reinterpret_cast<const olf::OrderCxlBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_CANCEL:
        notify(&BlotterReaderListener::on_log_cancelled, timestamp,
               *reinterpret_cast<const olf::OrderCxlBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_CXLREPL_REQUEST:
        notify(&BlotterReaderListener::on_log_cxlreplace_request, timestamp,
               *reinterpret_cast<const olf::OrderCxlReplaceReqBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_CANCEL_REPLACE:
        notify(&BlotterReaderListener::on_log_cxlreplaced, timestamp,
               *reinterpret_cast<const olf::OrderCxlReplaceBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_REMOTE_REJECT:
        notify(&BlotterReaderListener::on_log_rejected, timestamp,
               *reinterpret_cast<const olf::OrderRejectBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_CANCEL_REJECT:
        notify(&BlotterReaderListener::on_log_cancel_rejected, timestamp,
               *reinterpret_cast<const olf::OrderCxlRejectBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_DESTROY:
        notify(&BlotterReaderListener::on_log_destroy, timestamp,
               *reinterpret_cast<const olf::OrderDestroyBody *>(bufidx));
        break;
    case olf::MessageType::POSITION:
        notify(&BlotterReaderListener::on_log_position, timestamp,
               *reinterpret_cast<const olf::PositionBody*>(bufidx));
        break;
    case olf::MessageType::MANUAL_POSITION_ADJ:
        notify(&BlotterReaderListener::on_log_manual_position_adj, timestamp,
               *reinterpret_cast<const olf::ManualPositionAdjBody*>(bufidx));
        break;
    case olf::MessageType::NEW_SESSION:
        notify(&BlotterReaderListener::on_log_add_session, timestamp,
               *reinterpret_cast<const olf::NewSessionBody *>(bufidx));
        break;
    case olf::MessageType::ORDER_SESSION_DATA:
        notify(&BlotterReaderListener::on_log_order_session_data, timestamp,
               *reinterpret_cast<const olf::OrderSessionDataHeader *>(bufidx));
        break;
    case olf::MessageType::NEW_ACCOUNT:
        notify(&BlotterReaderListener::on_log_add_strategy, timestamp,
               *reinterpret_cast<const olf::NewAccountBody *>(bufidx));
        break;
    case olf::MessageType::MAX_MESSAGE_TYPE:
    case olf::MessageType::UNKNOWN:
    default:
        break;
    }
    return static_cast<std::int32_t>(sizeof(olf::MessageHeader)) + static_cast<std::int32_t>(header->length);
}

}}
