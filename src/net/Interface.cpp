
extern "C" {
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
}

#include <stdexcept>
#include <i01_net/Interface.hpp>

namespace i01 { namespace net {

Interface::InterfaceLock Interface::s_mutex;

std::vector<Interface> Interface::s_interfaces;

Interface::Interface(const char* ifname)
    : m_if_name(ifname)
    , m_if_index(::if_nametoindex(ifname))
{
    if (m_if_index == 0)
    {
        throw std::runtime_error("No such interface.");
    }
}

Interface::~Interface()
{
}

std::vector<Interface>& Interface::list()
{
    Interface::InterfaceLock::scoped_lock lock(Interface::s_mutex);

    if (!Interface::s_interfaces.empty())
        return Interface::s_interfaces;

    struct ::if_nameindex *if_ni, *i;
    if_ni = ::if_nameindex();
    if (if_ni != NULL)
    {
        for (i = if_ni; !(i->if_index == 0 && i->if_name == NULL); ++i)
        {
            Interface::s_interfaces.emplace_back(i->if_name);
        }
        ::if_freenameindex(if_ni);
    }
    return Interface::s_interfaces;
}

const std::string Interface::get_name_from_address(const std::string& address_)
{
    std::string ret;
    in_addr_t addr_ = 0;

    // Resolve address_ as a hostname or IP address:
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    s = ::getaddrinfo(address_.c_str(), NULL, &hints, &result);
    if (s != 0)
    { return ""; }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        if ((rp->ai_addr != NULL) && (rp->ai_addr->sa_family == AF_INET) && (rp->ai_family == AF_INET))
        {
            struct sockaddr_in si;
            ::memcpy(&si, rp->ai_addr, sizeof(struct sockaddr_in));
            addr_ = si.sin_addr.s_addr;
        }
    }
    ::freeaddrinfo(result);
    // Not a valid hostname, try by numeric IP:
    if (addr_ == 0)
    { addr_ = ::inet_addr(address_.c_str()); }

    struct ifaddrs* al;
    ::getifaddrs(&al);
    for (struct ifaddrs* i = al; i != NULL; i = i->ifa_next)
    {
        if ( (i->ifa_addr != NULL)
          && (i->ifa_flags & IFF_UP)  /* interface is administratively up */
          && (i->ifa_flags & IFF_RUNNING) /* interface resources allocated */
          && (i->ifa_addr->sa_family == AF_INET)
           )
        {
            struct sockaddr_in si;
            ::memcpy(&si, (i->ifa_addr), sizeof(struct sockaddr_in));
            if (addr_ == si.sin_addr.s_addr)
            {
                ret = i->ifa_name;
                break;
            }
        }
    }
    ::freeifaddrs(al);
    return ret;
}

} }
