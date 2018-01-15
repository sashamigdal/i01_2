#pragma once

#include "Messages.hpp"

namespace i01 { namespace OE { namespace NASDAQ { namespace SoupBinTCP30 {

class Listener {
public:
    virtual void handle_packet(const Timestamp&, const SoupBinTCP30::Messages::LoginAccept *) = 0;
    virtual void handle_packet(const Timestamp&, const SoupBinTCP30::Messages::LoginReject *) = 0;
    virtual void handle_packet(const Timestamp&, const SoupBinTCP30::Messages::SequencedDataPacket *, const char*, size_t) = 0;
    virtual void handle_packet(const Timestamp&, const SoupBinTCP30::Messages::ServerHeartbeat *) = 0;
    virtual void handle_packet(const Timestamp&, const SoupBinTCP30::Messages::EndOfSession *) = 0;
};

} } } }
