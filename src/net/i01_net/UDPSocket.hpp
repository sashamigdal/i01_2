#pragma once

#include <errno.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <i01_core/FD.hpp>

namespace i01 { namespace net {

    class EpollSet;

    class UDPSocket {
        protected:
            i01::core::SocketFD m_socket;
        private:
            sockaddr_in m_addr;
            UDPSocket(const UDPSocket&) = delete;
            UDPSocket& operator=(const UDPSocket&) = delete;
        public:
            explicit UDPSocket(int socket);
            UDPSocket(const bool nonblocking = true);
        UDPSocket(UDPSocket&& u) : m_socket(std::move(u.m_socket)) {}

            virtual ~UDPSocket();
            inline ssize_t send(const void* buf, std::size_t len);
            inline ssize_t recv(void* buf, std::size_t len, sockaddr_in& from, int flags = 0);

            void set_peer(std::uint32_t ip, std::uint16_t port);
            void set_peer(const std::string& ipstr, std::uint16_t port);
            bool set_nonblocking();
            bool set_rcvbuf(int sz = 16777216);
            bool bind(std::uint16_t port, const std::string& ipstr="INADDR_ANY" );
            bool join_multicast_group(const std::string& localipaddr, const std::string& groupaddr);
            bool bind_to_device(const std::string&);
            bool set_ttl(std::uint8_t ttl);
            bool set_reuseaddr();
            bool set_multicast_loopback(bool val);
            bool set_multicast_interface(const std::string& localipaddr);
            i01::core::SocketFD& fd() { return m_socket; };
        static UDPSocket create_multicast_socket(const std::string& localaddr, const std::string& groupaddr);
        static UDPSocket create_multicast_socket(const std::string& localaddr, const std::uint16_t bindport, const std::vector<std::string>& groupaddrs);
    };

    ssize_t UDPSocket::send(const void* buf, std::size_t len) {
        assert(buf != NULL);
        return m_socket.sendto(buf, len, MSG_NOSIGNAL, reinterpret_cast<const sockaddr*>(&m_addr), sizeof(m_addr));
    }

    ssize_t UDPSocket::recv(void* buf, std::size_t len, sockaddr_in& from, int flags) {
        assert(buf != NULL);
        socklen_t fromlen = sizeof(from);
        ssize_t ret = m_socket.recvfrom(buf, len, flags,
                reinterpret_cast<sockaddr*>(&from),
                &fromlen);
        if(ret <= 0) {
            ret = ((ret == -1) && (errno == EAGAIN)) ? 0 : -1;
        }
        return ret;
    }

} }
