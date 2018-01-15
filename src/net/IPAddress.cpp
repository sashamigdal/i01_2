#include <i01_net/IPAddress.hpp>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <boost/lexical_cast.hpp>

#include <string>

namespace i01 { namespace net {

inline namespace deprecated {

std::string ip_addr_to_str(std::uint32_t ipaddr)
{
    // assumes argument is network byte order
    struct in_addr in;
    in.s_addr = ipaddr;
    char buf[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &in, buf, INET_ADDRSTRLEN);
    return buf;
}

std::string ip_addr_to_str(const ip_addr_port_type &ipaddr)
{
    // argument is network byte order
    struct in_addr in;
    in.s_addr = ipaddr.first;
    char buf[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &in, buf, INET_ADDRSTRLEN);
    return std::string(buf) + ":" + boost::lexical_cast<std::string>(ntohs(ipaddr.second));
}

std::string ip_addr_to_str(std::uint64_t addr) {
    return ip_addr_to_str(from64(addr));
}

ip_addr_port_type addr_str_to_ip_addr(const std::string &str)
{
    // assumes that string is in host byte order
    std::size_t p = str.find_first_of(':');
    if (p == std::string::npos) {
        throw std::runtime_error(str + " is not is ADDR:PORT form");
    }
    std::uint16_t port = htons(static_cast<std::uint16_t>(std::stoul(str.substr(p+1))));

    struct sockaddr_in sa;
    inet_pton(AF_INET, str.substr(0,p).c_str(), &(sa.sin_addr));

    return std::make_pair(sa.sin_addr.s_addr, port);
}

}

} }
