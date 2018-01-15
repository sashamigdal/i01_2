#pragma once

#include <cstdint>

namespace i01 { namespace OE { namespace NASDAQ { namespace UFO10 { namespace Types {

    enum class DownstreamPacketType : std::uint8_t {
          LOGIN_ACCEPT    = 'A'
        , LOGIN_REJECT    = 'J'
        , SEQUENCED_DATA  = 'S'
        , END_OF_SESSION  = 'E'
    };

    typedef std::uint8_t Session[10];
    typedef std::uint32_t SequenceNumber;

    enum class LoginRejectReasonCode : std::uint8_t {
          NOT_AUTHORIZED  = 'A'
        , INVALID_SESSION = 'S'
    };

    typedef std::uint16_t MessageCount;
    typedef std::uint16_t MessageLength;

    enum class UpstreamMessageType : std::uint8_t {
          LOGIN_REQUEST           = 'L'
        , RETRANSMISSION_REQUEST  = 'T'
        , UNSEQUENCED_MESSAGE     = 'U'
        , HEARTBEAT_MESSAGE       = 'R'
        , LOGOFF_REQUEST          = 'O'
    };

    typedef std::uint8_t Username[6];
    typedef std::uint8_t Password[10];

} } } } }

