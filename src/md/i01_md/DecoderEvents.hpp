#pragma once

namespace i01 { namespace MD {

enum class DecoderMsgType : std::uint32_t {
    GAP_MSG = 10000,
    HEARTBEAT_MSG = 10001,
    UNHANDLED_MSG = 10002,
    END_OF_PKT_MSG = 10003,
    START_OF_PKT_MSG = 10004,
    TIMEOUT_MSG = 10005,
 };

struct GapMsg {
    static const std::uint32_t i01_msg_type = (std::uint32_t)DecoderMsgType::GAP_MSG;
};
struct HeartbeatMsg {
    static const std::uint32_t i01_msg_type = (std::uint32_t)DecoderMsgType::HEARTBEAT_MSG;
};
struct UnhandledMsg {
    static const std::uint32_t i01_msg_type = (std::uint32_t)DecoderMsgType::UNHANDLED_MSG;
};
struct EndOfPktMsg {
    static const std::uint32_t i01_msg_type = (std::uint32_t)DecoderMsgType::END_OF_PKT_MSG;
};
struct StartOfPktMsg {
    static const std::uint32_t i01_msg_type = (std::uint32_t)DecoderMsgType::START_OF_PKT_MSG;
};
struct TimeoutMsg {
    static const std::uint32_t i01_msg_type = (std::uint32_t)DecoderMsgType::TIMEOUT_MSG;
};
}}
