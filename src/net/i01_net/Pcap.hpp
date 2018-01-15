#pragma once

#include <unordered_map>

#include <pcap.h>
#include <string>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/udp.h>

#include <i01_core/Pcap.hpp>

#include <i01_net/IPAddress.hpp>
#include <i01_net/PktListener.hpp>

namespace i01 { namespace net { namespace pcap {


class Reader {
public:
    Reader() = delete;
    Reader(const std::string &filename);
    virtual ~Reader() {}
    int read_packets(int limit = -1);

    const i01::core::Timestamp & last_ts() const { return m_ts; }
    std::uint32_t last_pkt_len() const { return m_pkt_len; }

    std::uint32_t bytes_read() const { return m_bytes_read; }
protected:
    std::uint8_t ip_hdr_size_(const struct iphdr *hdr) const {
        return static_cast<std::uint8_t>(4u*(hdr->ihl & 0x0FU));
    }

private:
    void open_file_();
    void close_file_();

    virtual void handle_eth_payload_(std::uint16_t proto, std::uint8_t * buf, std::size_t len, const i01::core::Timestamp *ts) {}


private:
    std::string m_filename;
    char m_errbuf[PCAP_ERRBUF_SIZE];
    core::pcap::FileReader *m_pcap_p = nullptr;
    i01::core::Timestamp m_ts = {0,0};
    std::uint32_t m_pkt_len = 0;
    std::uint32_t m_bytes_read = 0;
};

template<typename HandlerType>
class UDPReader : public Reader {
public:
    typedef std::pair<ip_addr_port_type, ip_addr_port_type> channel_type; // (src, dst)
    channel_type make_channel(std::uint32_t src_addr, std::uint16_t src_port, std::uint32_t dst_addr, std::uint16_t dst_port) {
        return {std::make_pair(src_addr, src_port), std::make_pair(dst_addr, dst_port)};
    }

public:
    UDPReader(const std::string &filename, HandlerType *handler) :
        Reader(filename),
        m_udp_handler_p(handler) {}

    channel_type last_channel() const { return m_last_channel; }

private:
    void handle_eth_payload_(std::uint16_t proto, std::uint8_t *buf, std::size_t len, const i01::core::Timestamp *ts) {

        if (ETH_P_IP != proto) {
            return;
        }
        const struct iphdr *ip = reinterpret_cast<const struct iphdr *>(buf);
        std::uint8_t ip_hdr_size = ip_hdr_size_(ip);

        m_ip_src_addr = ip->saddr;
        m_ip_dst_addr = ip->daddr;

        m_udp_src_port = 0;
        m_udp_dst_port = 0;

        if (IPPROTO_UDP == ip->protocol) {
            const struct udphdr *udp = reinterpret_cast<const struct udphdr *>(buf + ip_hdr_size);
            m_udp_src_port = udp->source;
            m_udp_dst_port = udp->dest;

            m_last_channel = make_channel(m_ip_src_addr, m_udp_src_port, m_ip_dst_addr, m_udp_dst_port);

            m_udp_handler_p->handle(m_ip_src_addr, m_udp_src_port, m_ip_dst_addr, m_udp_dst_port, buf + ip_hdr_size, len - (std::size_t)ip_hdr_size, ts);
        }
    }


private:
    HandlerType *m_udp_handler_p;

    std::uint32_t m_ip_src_addr = 0;
    std::uint32_t m_ip_dst_addr = 0;

    std::uint16_t m_udp_src_port = 0;
    std::uint16_t m_udp_dst_port = 0;
    channel_type m_last_channel;
};


}}}
