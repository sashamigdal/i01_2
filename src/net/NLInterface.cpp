#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>

#include <i01_net/NLInterface.hpp>

namespace i01 { namespace net {

NLInterface::NLInterface(const char* ifname)
    : Interface(ifname)
    , m_nl_cache_p(nullptr)
    , m_nl_sock_p(nullptr)
    , m_rtnl_link_p(nullptr)
{
    std::cout << "Creating NLInterface " << ifname << std::endl;
    if ((m_nl_sock_p = ::nl_socket_alloc()) == nullptr)
    { throw std::runtime_error("nl_socket_alloc"); }
    ::nl_connect(m_nl_sock_p, NETLINK_ROUTE);
    if (::rtnl_link_alloc_cache(m_nl_sock_p, AF_UNSPEC, &m_nl_cache_p) < 0)
    { throw std::runtime_error("rtnl_link_alloc_cache"); }
    if (!(m_rtnl_link_p = ::rtnl_link_get_by_name(m_nl_cache_p, ifname)))
    { throw std::runtime_error("rtnl_link_get_by_name"); }
    std::cout << "Created NLInterface " << ifname << std::endl;
}

NLInterface::~NLInterface()
{
    std::cout << "Destroying NLInterface " << name() << std::endl;
    if (m_rtnl_link_p)
        ::rtnl_link_put(m_rtnl_link_p);
    if (m_nl_cache_p)
        ::nl_cache_free(m_nl_cache_p);
    if (m_nl_sock_p)
        ::nl_socket_free(m_nl_sock_p);
    std::cout << "Destroyed NLInterface " << name() << std::endl;
}

} }
