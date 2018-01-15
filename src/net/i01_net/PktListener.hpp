#pragma once

#include <map>
#include <iostream>
#include <arpa/inet.h>
#include <linux/udp.h>

#include <i01_core/Time.hpp>

namespace i01 { namespace net {

template<typename Derived>
class UDPPktListener {
public:
    template<typename... Args>
    void handle(std::uint32_t ip_src, std::uint16_t udp_src, std::uint32_t ip_dst_addr, std::uint16_t udp_dst_port, std::uint8_t *buf, std::size_t len, const i01::core::Timestamp *ts, Args&&... args) {
        // pull off the UDP header and just pass the payload
        static_cast<Derived*>(this)->handle_payload(ip_src, udp_src, ip_dst_addr, udp_dst_port, buf + sizeof(struct udphdr), len - sizeof(struct udphdr), ts, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void handle_payload(std::uint32_t ip_src, std::uint16_t udp_src, std::uint32_t ip_dst_addr, std::uint16_t udp_dst_port, std::uint8_t *buf, std::size_t len, const i01::core::Timestamp *ts, Args&&... args) {
        std::cerr << "Unimplemented UDP payload handler" << std::endl;
    }

};



}}
