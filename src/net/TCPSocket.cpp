#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <i01_net/EpollSet.hpp>
#include <i01_net/TCPSocket.hpp>

namespace i01 { namespace net {

    TCPSocket::TCPSocket(int socket)
        : m_socket(socket)
        , m_errno(0)
    {
        assert(socket >= 0);
        if(!set_no_delay()) {
            char errbuff[256];
            char* errstr = strerror_r(errno, errbuff, sizeof(errbuff));
            throw std::runtime_error( std::string(__PRETTY_FUNCTION__) + "Failed set_no_delay: " + errstr);
        }
    }

    TCPSocket::TCPSocket(const bool nonblocking)
        : m_socket(PF_INET, SOCK_STREAM | (nonblocking ? SOCK_NONBLOCK : 0), IPPROTO_TCP)
        , m_errno(0)
    {
        if(m_socket.fd() < 0) {
            char errbuff[256];
            char* errstr = strerror_r(errno, errbuff, sizeof(errbuff));
            throw std::runtime_error( std::string(__PRETTY_FUNCTION__) + "Failed socket call: " + errstr);
        }
        if(!set_no_delay()) {
            char errbuff[256];
            char* errstr = strerror_r(errno, errbuff, sizeof(errbuff));
            throw std::runtime_error( std::string(__PRETTY_FUNCTION__) + "Failed set_no_delay: " + errstr);
        }
    }

    TCPSocket::~TCPSocket() {
        if (m_socket.valid()) {
            m_socket.close();
        }
    }

    bool TCPSocket::set_no_delay() {
        int flag = 1;
        return 0 == m_socket.setsockopt(IPPROTO_TCP, TCP_NODELAY, static_cast<void*>(&flag), sizeof(flag));
    }

    bool TCPSocket::set_nonblocking() {
        return m_socket.fcntl(F_SETFL, O_NONBLOCK) == 0;
    }

    bool TCPSocket::connect(std::string const& host, int port) {
        bool ret = true;
        sockaddr_in clientsockaddr;
        memset(&clientsockaddr, 0, sizeof(clientsockaddr));
        clientsockaddr.sin_family = AF_INET;
        clientsockaddr.sin_addr.s_addr = inet_addr(host.c_str());
        clientsockaddr.sin_port = htons(static_cast<uint16_t>(port));
        int cret = m_socket.connect(reinterpret_cast<sockaddr*>(&clientsockaddr), sizeof(clientsockaddr));
        if(cret < 0) {
            m_errno = errno;
            ret = false;
        }
        return ret;
    }

    std::pair<std::uint32_t, std::uint16_t> TCPSocket::get_peer() const {

        sockaddr_in myaddr;
        memset(&myaddr, 0, sizeof(myaddr));
        socklen_t len=sizeof(myaddr);
        int I01_UNUSED ret = m_socket.getpeername(reinterpret_cast<sockaddr*>(&myaddr), &len);
        assert(ret != -1);
        return std::make_pair(ntohl(myaddr.sin_addr.s_addr), ntohs(myaddr.sin_port));
    }

bool TCPSocket::bind(std::uint16_t port)
{
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    return m_socket.bind(reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) >= 0;
}

bool TCPSocket::listen(int backlog)
{
    return m_socket.listen(backlog) >= 0;
}

TCPSocket TCPSocket::create_listening_socket(std::uint16_t port)
{
    TCPSocket socket;
    if (!socket.bind(port)) {
        auto e = socket.get_errno();
        std::cerr << "TCPSocket: create_listening_socket: bind error: " << strerror(e) << std::endl;
        return TCPSocket(-1);
    }
    if (!socket.listen()) {
        auto e = socket.get_errno();
        std::cerr << "TCPSocket: create_listening_socket: listen error: " << strerror(e) << std::endl;
        return TCPSocket(-1);
    }

    return socket;
}

} }
