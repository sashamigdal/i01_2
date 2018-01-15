#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cassert>
#include <errno.h>
#include <stdint.h>

#include <i01_core/macro.hpp>
#include <i01_core/FD.hpp>

namespace i01 { namespace net {

    class EpollSet;

    class TCPSocket {
        i01::core::SocketFD m_socket;
        int m_errno;
        TCPSocket(TCPSocket const&) = delete;
        TCPSocket& operator=(TCPSocket const&) = delete;

        bool set_no_delay();

    public:
        explicit TCPSocket(int socket);
        TCPSocket(const bool nonblocking = true);
        TCPSocket(TCPSocket&& t) : m_socket(std::move(t.m_socket)), m_errno(t.m_errno) {}
        TCPSocket& operator=(TCPSocket&& t) {
            m_socket = std::move(t.m_socket);
            m_errno = t.m_errno;
            return *this;
        }
        ~TCPSocket();

        int get_errno() const { return m_errno; }
        std::pair<uint32_t, uint16_t> get_peer() const;

        bool connect(std::string const& host, int port);
        bool set_nonblocking();
        inline ssize_t recv(void* buf, std::size_t len, int flags = 0);
        inline ssize_t send(void const* buf, std::size_t len);

        i01::core::SocketFD& fd() { return m_socket; }
        bool bind(std::uint16_t port);
        bool listen(int backlog = 128);

        static TCPSocket create_listening_socket(std::uint16_t port);
    };

    ssize_t TCPSocket::recv(void* buf, std::size_t len, int flags) {
        assert(buf != NULL);

        ssize_t ret = m_socket.recv(buf, len, flags);
        if(ret <= 0 ) {
            ret = ((ret == -1) && (errno == EAGAIN)) ? 0 : - 1;
        }
        return ret;
    }

    ssize_t TCPSocket::send(void const* buf, std::size_t len) {
        assert(buf != NULL);

        return m_socket.send(buf, len, MSG_NOSIGNAL);
    }

} }
