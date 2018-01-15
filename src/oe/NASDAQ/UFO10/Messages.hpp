#pragma once

#include <cstdint>

#include "Types.hpp"

namespace i01 { namespace OE { namespace NASDAQ { namespace UFO10 { namespace Messages {
    using namespace i01::OE::NASDAQ::UFO10::Types;

    // DOWNSTREAM

    struct LoginAccept {
        DownstreamPacketType            packet_type;
        Session                         session;
        SequenceNumber                  sequence_number;
    } __attribute__((packed));

    struct LoginReject {
        DownstreamPacketType            packet_type;
        LoginRejectReasonCode           reason_code;
    } __attribute__((packed));
    
    struct SequencedPacketHeader {
        DownstreamPacketType            packet_type;
        SequenceNumber                  sequence_number;
        MessageCount                    count;
    } __attribute__((packed));

    struct SequencedMessageHeader {
        MessageLength                   message_length;
        std::uint8_t                    message_data[];
    } __attribute__((packed));

    struct EndOfSession {
        DownstreamPacketType            packet_type;
        SequenceNumber                  sequenced_message_count;
    } __attribute__((packed));

    // UPSTREAM
   
    struct UpstreamPacketHeader {
        MessageLength                   message_length;
    } __attribute__((packed));

    struct LoginRequest {
        UpstreamMessageType             message_type;
        Username                        username;
        Password                        password;
        Session                         requested_session;
    } __attribute__((packed));

    struct RetransmissionRequest {
        UpstreamMessageType             message_type;
        SequenceNumber                  sequence_number;
        MessageCount                    message_count;
    } __attribute__((packed));
    
    struct UnsequencedMessage {
        UpstreamMessageType             message_type;
    } __attribute__((packed));

    struct Heartbeat {
        UpstreamMessageType             message_type;
    } __attribute__((packed));

    struct LogoffRequest {
        UpstreamMessageType             message_type;
    } __attribute__((packed));

} } } } }

