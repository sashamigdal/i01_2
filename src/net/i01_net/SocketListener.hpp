#pragma once

namespace i01 { namespace core {
class Timestamp;
}}

namespace i01 { namespace net {

class SocketListener {
public:
    virtual void on_connected(const core::Timestamp&, void *) = 0;
    virtual void on_peer_disconnect(const core::Timestamp&, void *) = 0;
    virtual void on_local_disconnect(const core::Timestamp&, void *) = 0;
    virtual void on_recv(const core::Timestamp & ts, void *, const std::uint8_t *buf, const ssize_t & len) = 0;
};

}}
