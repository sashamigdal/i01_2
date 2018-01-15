#pragma once

#include <cstdint>

#include "Types.hpp"

namespace i01 { namespace OE { namespace NASDAQ { namespace SoupBinTCP30 { namespace Messages {
    using namespace i01::OE::NASDAQ::SoupBinTCP30::Types;

    struct PacketHeader {
        PacketLength                    packet_length;
        DownstreamPacketType            packet_type;
    } __attribute__((packed));

    struct DebugPacket {
        PacketHeader                    hdr;
        //std::uint8_t                    text[];
    } __attribute__((packed));

    // DOWNSTREAM

    struct LoginAccept {
        PacketHeader                    hdr;
        Session                         session;
        SequenceNumber                  sequence_number;
    } __attribute__((packed));

    struct LoginReject {
        PacketHeader                    hdr;
        LoginRejectReasonCode           reason_code;
    } __attribute__((packed));
    
    struct SequencedDataPacket {
        PacketHeader                    hdr;
        //std::uint8_t                    message_data[];
    } __attribute__((packed));

    struct ServerHeartbeat {
        PacketHeader                    hdr;
    } __attribute__((packed));

    struct EndOfSession {
        PacketHeader                    hdr;
    } __attribute__((packed));

    // UPSTREAM
   
    struct LoginRequest {
        PacketLength                    packet_length;
        UpstreamPacketType              packet_type;
        Username                        username;
        Password                        password;
        Session                         requested_session;
        SequenceNumber                  requested_sequence_number;
    } __attribute__((packed));

    struct UnsequencedDataPacket {
        PacketLength                    packet_length;
        UpstreamPacketType              packet_type;
        //std::uint8_t                    message_data[];
    } __attribute__((packed));

    struct Heartbeat {
        PacketLength                    packet_length;
        UpstreamPacketType              packet_type;
    } __attribute__((packed));

    struct LogoutRequest {
        PacketLength                    packet_length;
        UpstreamPacketType              packet_type;
    } __attribute__((packed));

} } } } }

