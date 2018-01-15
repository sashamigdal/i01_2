#pragma once

#include "Messages.hpp"

namespace i01 { namespace OE { namespace NASDAQ { namespace UFO10 {

class Listener {
public:
    virtual void handle_packet(const UFO10::Messages::LoginAccept *) = 0;
    virtual void handle_packet(const UFO10::Messages::LoginReject *) = 0;
    virtual void handle_packet(const UFO10::Messages::SequencedPacketHeader *, const char * buf, std::size_t buf_len) = 0;
    virtual void handle_packet(const UFO10::Messages::EndOfSession *) = 0;
};

} } } }
