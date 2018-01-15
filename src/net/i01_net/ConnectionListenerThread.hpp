#pragma once

#include <i01_core/FD.hpp>

#include <i01_net/EventPoller.hpp>
#include <i01_net/TCPSocket.hpp>

namespace i01 { namespace net {

class SocketListener;

class ConnectionListenerThread : public EpollEventPoller {
public:
    using Callback = void (*)(std::uint32_t, void *);
    using ConnDataGenerator = std::function<void *(int fd, const sockaddr_in& addr, const socklen_t& len)>;

public:
    ConnectionListenerThread(const std::string& n, SocketListener& tps,
                           ConnDataGenerator func = nullptr);
    virtual void * process() override;
    bool add_listener_on_port(std::uint16_t port);

private:
    void * check_for_new_connections();
    void on_new_connection(std::uint32_t events);

    static void listen_callback(std::uint32_t events, void * ptr);

private:
    SocketListener& m_socket_listener;
    ConnDataGenerator m_data_generator;
    TCPSocket m_listen_fd;
    EpollSet m_listen_eps;
};



}}
