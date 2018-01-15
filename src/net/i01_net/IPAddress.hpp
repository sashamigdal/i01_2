#pragma once

#include <i01_core/macro.hpp>
#include <functional>
#include <utility>

namespace i01 { namespace net {




inline namespace deprecated {

typedef std::pair<std::uint32_t, std::uint16_t> ip_addr_port_type;
constexpr std::uint64_t to64(const ip_addr_port_type &addr) {
    return static_cast<std::uint64_t>(addr.first) << 16 | static_cast<std::uint64_t>(addr.second);
}
constexpr std::uint64_t to64(std::uint32_t addr, std::uint16_t port) {
    return to64({addr, port});
}
constexpr ip_addr_port_type from64(std::uint64_t u64) {
    return {static_cast<std::uint32_t>(u64 >> 16), static_cast<std::uint16_t>(u64 & 0xFFFF)};
}
struct ip_addr_port_type_hasher
{
    std::size_t operator() (const ip_addr_port_type &addr) const {
        return std::hash<std::uint64_t>()(to64(addr));
    }
};
std::string ip_addr_to_str(std::uint32_t ipaddr);
std::string ip_addr_to_str(const ip_addr_port_type & ip);
std::string ip_addr_to_str(std::uint64_t addr);
ip_addr_port_type addr_str_to_ip_addr(const std::string &str);

}

}}
