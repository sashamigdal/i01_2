#pragma once

#include <cstdint>
#include <array>

#include <i01_core/macro.hpp>

namespace i01 { namespace OE { namespace NASDAQ { namespace SoupBinTCP30 { namespace Types {
    typedef std::uint16_t PacketLength;

    enum class DownstreamPacketType : std::uint8_t {
          DEBUG           = '+'
        , LOGIN_ACCEPT    = 'A'
        , LOGIN_REJECT    = 'J'
        , SEQUENCED_DATA  = 'S'
        , SERVER_HEARTBEAT= 'H'
        , END_OF_SESSION  = 'Z'
    };

using Session = std::array<uint8_t, 10>;
I01_ASSERT_SIZE(Session, 10);

    typedef std::array<std::uint8_t, 20> SequenceNumber;

    enum class LoginRejectReasonCode : std::uint8_t {
          NOT_AUTHORIZED  = 'A'
        , INVALID_SESSION = 'S'
    };

    enum class UpstreamPacketType : std::uint8_t {
          DEBUG                   = '+'
        , LOGIN_REQUEST           = 'L'
        , RETRANSMISSION_REQUEST  = 'T'
        , UNSEQUENCED_MESSAGE     = 'U'
        , HEARTBEAT_MESSAGE       = 'R'
        , LOGOFF_REQUEST          = 'O'
    };

    typedef std::uint8_t Username[6];
    typedef std::uint8_t Password[10];

} } } } }
