#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/lexical_cast.hpp>

#include <i01_net/UDPSocket.hpp>
#include <i01_net/EpollSet.hpp>

namespace i01 { namespace net {

    UDPSocket::UDPSocket(int socket)
        : m_socket(socket)
    {
        assert(socket >= 0);
    }

    UDPSocket::UDPSocket(const bool nonblocking)
        : m_socket(AF_INET, SOCK_DGRAM | (nonblocking ? SOCK_NONBLOCK : 0), IPPROTO_UDP)
    {
        if(m_socket.fd() < 0) {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": Failed socket call.");
        }
        ::memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
    }

    UDPSocket::~UDPSocket() {
        if (m_socket.fd() >= 0)
            m_socket.close();
    }

    void UDPSocket::set_peer(std::uint32_t ip, std::uint16_t port) {
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = htonl(ip);
    }

    void UDPSocket::set_peer(const std::string& ipstr, std::uint16_t port) {
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = ipstr != "INADDR_ANY" ? inet_addr(ipstr.c_str()) : htonl(INADDR_ANY);
    }

    bool UDPSocket::set_nonblocking() {
        return 0 == m_socket.fcntl(F_SETFL, O_NONBLOCK);
    }

    bool UDPSocket::set_rcvbuf(int sz) {
        return 0 == m_socket.setsockopt(SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&sz), sizeof(sz));
    }

    bool UDPSocket::bind(std::uint16_t port, const std::string& ipstr) {
        sockaddr_in myaddr;
        memset(&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_port = htons(port);
        myaddr.sin_addr.s_addr = ipstr != "INADDR_ANY" ? inet_addr(ipstr.c_str()) : htonl(INADDR_ANY);

        int ret = m_socket.bind(reinterpret_cast<const sockaddr*>(&myaddr), sizeof(myaddr));
        return (ret != -1);
    }

    bool UDPSocket::bind_to_device(const std::string& strifname) {
        const char* ifname = strifname.c_str();
        return m_socket.setsockopt(SOL_SOCKET, SO_BINDTODEVICE, const_cast<char*>(ifname), sizeof(ifname)) == 0;
    }

    bool UDPSocket::join_multicast_group(const std::string& localipaddr, const std::string& groupaddr) {
        ip_mreq mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(groupaddr.c_str());
        mreq.imr_interface.s_addr =
            localipaddr != "INADDR_ANY" ? inet_addr(localipaddr.c_str()) : htonl(INADDR_ANY);
        return 0 == m_socket.setsockopt(IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char*>(&mreq), sizeof(mreq));
    }

    bool UDPSocket::set_ttl(std::uint8_t ttl) {
        return m_socket.setsockopt(IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl))==0;
    }

    bool UDPSocket::set_multicast_loopback(bool val) {
        std::uint8_t valu8 = val;
        return m_socket.setsockopt(IPPROTO_IP, IP_MULTICAST_LOOP, &valu8, sizeof(valu8))==0;
    }

    bool UDPSocket::set_multicast_interface(const std::string& localipaddr) {
        in_addr_t intf_addr = inet_addr(localipaddr.c_str());
        return m_socket.setsockopt(IPPROTO_IP, IP_MULTICAST_IF, &intf_addr, sizeof(intf_addr))==0;
    }

    bool UDPSocket::set_reuseaddr() {
        std::uint32_t reuse=1;
        return m_socket.setsockopt(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))==0;
    }

UDPSocket UDPSocket::create_multicast_socket(const std::string& localaddr, const std::uint16_t bindport, const std::vector<std::string>& groupaddrs)
{
    
    if (UNLIKELY(groupaddrs.empty())) {
        std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << "" << ") " << "no groupaddrs provided" << std::endl;
        return UDPSocket(-1);
    }
    if (bindport == 0) {
        std::cerr << "UDPSocket: create_multicast_socket: no bind() port provided" << std::endl;
        return UDPSocket(-1);
    }

    UDPSocket u;

    if (!u.set_reuseaddr()) {
        std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << bindport << ") could not set reuseaddr" << std::endl;
        return UDPSocket(-1);
    }

    if (!u.bind(bindport)) {
        std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << bindport << ") could not bind to port " << bindport << std::endl;
        return UDPSocket(-1);
    }

    for (auto& group_host : groupaddrs) {
        if (!u.join_multicast_group(localaddr, group_host)) {
            std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << bindport << ") could not join multicast group " << group_host << " errno: " << errno << " " << strerror(errno) << std::endl;
            return UDPSocket(-1);
        }
    }

    return u;
}

/// localaddr is the interface address.  groupaddr is a host:port string
UDPSocket UDPSocket::create_multicast_socket(const std::string& localaddr, const std::string& groupaddr)
{
    auto p = groupaddr.find_first_of(':');
    if (std::string::npos == p) {
        std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << groupaddr << ") groupaddr should be in form HOST[,HOST,...]]:PORT" << std::endl;
        return UDPSocket(-1);
    }
    std::uint16_t port = 0;
    try {
        port = boost::lexical_cast<std::uint16_t>(groupaddr.substr(p+1));
    } catch (const boost::bad_lexical_cast&) {
        std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << groupaddr << ") could not parse PORT from groupaddr" << std::endl;
        return UDPSocket(-1);
    }
    auto group_host = groupaddr.substr(0,p);
    if (std::string::npos != group_host.find_first_of(',')) {
        // multiple multicast group addresses specified for one local bind port:
        std::vector<std::string> group_hosts;
        for (size_t cur = 0; cur != std::string::npos; ) {
            auto next = group_host.find_first_of(',', cur);
            group_hosts.push_back(group_host.substr(cur, next));
            cur = next;
        }
        return create_multicast_socket(localaddr, port, group_hosts);
    }

    UDPSocket u;

    if (!u.set_reuseaddr()) {
        std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << groupaddr << ") could not set reuseaddr" << std::endl;
        return UDPSocket(-1);
    }

    if (!u.bind(port, group_host)) {
        std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << groupaddr << ") could not bind to port " << port << std::endl;
        return UDPSocket(-1);
    }

    if (!u.join_multicast_group(localaddr, group_host)) {
        std::cerr << "UDPSocket: create_multicast_socket: (" << localaddr << ", " << groupaddr << ") could not join multicast group " << group_host << " errno: " << errno << " " << strerror(errno) << std::endl;
        return UDPSocket(-1);
    }

    return u;
}

} }
