#pragma once

#include <string>
#include <vector>

extern "C" {
#include <sys/socket.h>
#include <linux/if.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/link/vlan.h>
}

#include <i01_core/Lock.hpp>
#include <i01_net/Interface.hpp>

namespace i01 { namespace net {

class NLInterface : public Interface {
public:
    NLInterface(const char* ifname);
    ~NLInterface();

    /// Returns 0 or negative if unset or error, otherwise returns VLAN id
    //  number.
    int vlan_id() const
    { return ::rtnl_link_is_vlan(m_rtnl_link_p)
        ? ::rtnl_link_vlan_get_id(m_rtnl_link_p) : 0; }

    /// Returns 0 if MTU is unset, otherwise returns MTU in bytes..
    unsigned int mtu() const 
    { return ::rtnl_link_get_mtu(m_rtnl_link_p); }

    /// Returns true if the link is up.
    bool up() const
    { return ::rtnl_link_get_operstate(m_rtnl_link_p) == IF_OPER_UP; }

    /// Returns true if the link supports multicast.
    bool multicast() const
    { return ::rtnl_link_get_flags(m_rtnl_link_p) & IFF_MULTICAST; }

protected:
    NLInterface() = delete;

    struct ::nl_cache                *m_nl_cache_p;
    struct ::nl_sock                 *m_nl_sock_p;
    struct ::rtnl_link               *m_rtnl_link_p;
};

} }
