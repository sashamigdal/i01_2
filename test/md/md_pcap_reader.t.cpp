#include <gtest/gtest.h>

#include <iostream>
#include <string.h>
#include <arpa/inet.h>

#include <i01_core/macro.hpp>

#include <i01_net/IPAddress.hpp>
#include <i01_net/Pcap.hpp>

namespace MD_PCAP_TEST {
// have to be outside of a TEST block b/c of the template function
// also has to be in the i01::net namespace b/c of specialization
class TestUDPPktListener : public i01::net::UDPPktListener<TestUDPPktListener> {
public:

    template<typename...Args>
    void handle_payload(std::uint32_t src_ipaddr, std::uint16_t src_udpport, std::uint32_t ipaddr, std::uint16_t udpport, std::uint8_t *buf, size_t len, const i01::core::Timestamp *ts, Args&&...args) {
        i01::net::UDPPktListener<TestUDPPktListener>::handle_payload(src_ipaddr, src_udpport, ipaddr, udpport,buf, len, ts, std::forward<Args>(args)...);
    }
    std::vector<size_t> m_pkt_size;
    std::vector<i01::core::Timestamp> m_udp_ts;

public:
    typedef std::pair<std::uint16_t, std::uint16_t> udp_port_pair_type; // (src, dst)
    typedef std::map<udp_port_pair_type, std::uint64_t> udp_port_count_map_type;

    udp_port_count_map_type get_port_count() const { return m_pair_count_map; }
    udp_port_count_map_type m_pair_count_map;
};

template<>
void TestUDPPktListener::handle_payload(std::uint32_t, std::uint16_t src_port, std::uint32_t, std::uint16_t dst_port, std::uint8_t *buf, size_t len, const i01::core::Timestamp *ts) {
    m_pkt_size.push_back(len);
    m_udp_ts.push_back(*ts);
    m_pair_count_map[std::make_pair(ntohs(src_port), ntohs(dst_port))]++;
}

}


TEST(md_pcap_reader, md_pcap_test)
{
    using namespace i01::net;

    MD_PCAP_TEST::TestUDPPktListener test_udp;
    i01::net::pcap::UDPReader<MD_PCAP_TEST::TestUDPPktListener> reader(STRINGIFY(I01_DATA) "/BZX_UNIT_1_20140724_1500_first5.pcap-ns", &test_udp);

    // full pkt size
    std::vector<size_t> check_pkt_len = { 82, 69, 64, 76, 76};
    std::vector<size_t> check_udp_len = { 40, 27, 22, 34, 34};
    std::vector<i01::core::Timestamp> check_ts = { {1406229654, 131153829},
                                                   {1406229654, 147138495},
                                                   {1406229654, 149306128},
                                                   {1406229654, 189006232},
                                                   {1406229654, 260761871} };

    for (size_t i =0; i < 5; i++) {
        reader.read_packets(1);
        auto channel = reader.last_channel();
        auto udp_src = ntohs(channel.first.second);
        auto udp_dst = ntohs(channel.second.second);
        EXPECT_EQ(ip_addr_to_str(channel.first.first), "208.90.209.241") << "IP Src address wrong.";
        EXPECT_EQ(udp_src, 38121) << "UDP Src port wrong.";

        EXPECT_EQ(ip_addr_to_str(channel.second), "224.0.62.2:30001") << "IP Dst address wrong.";
        EXPECT_EQ(udp_dst, 30001) << "UDP Dst port wrong.";

        EXPECT_EQ(reader.last_ts().tv_sec, 1406229654) << "Pcap capture time seconds wrong.";
        EXPECT_EQ(reader.last_pkt_len(), check_pkt_len[i]) << "Pcap pkt " << i << " size does not match.";

    }

    auto portcount = test_udp.get_port_count();

    ASSERT_EQ(portcount.size(), 1) << "UDP pair count map not equal to size 1";
    EXPECT_EQ(portcount.begin()->second, 5) << "UDP pair port count not equal to 5.";
    EXPECT_EQ(portcount.begin()->first.first == 38121 && portcount.begin()->first.second == 30001, true) << "UDP port pair count map invalid entry.";
    EXPECT_EQ(test_udp.m_pkt_size == check_udp_len, true) << "Payload sizes did not match.";
    EXPECT_EQ(test_udp.m_udp_ts== check_ts, true) << "Packet capture timestamps do not match.";

}

TEST(md_pcap_reader, md_pcapnslz4_test)
{
    using namespace i01::net;

    MD_PCAP_TEST::TestUDPPktListener test_udp;
    i01::net::pcap::UDPReader<MD_PCAP_TEST::TestUDPPktListener> reader(STRINGIFY(I01_DATA) "/BZX_UNIT_1_20140724_1500_first5.pcap-ns.lz4", &test_udp);

    // full pkt size
    std::vector<size_t> check_pkt_len = { 82, 69, 64, 76, 76};
    std::vector<size_t> check_udp_len = { 40, 27, 22, 34, 34};
    std::vector<i01::core::Timestamp> check_ts = { {1406229654, 131153829},
                                                   {1406229654, 147138495},
                                                   {1406229654, 149306128},
                                                   {1406229654, 189006232},
                                                   {1406229654, 260761871} };

    for (size_t i =0; i < 5; i++) {
        reader.read_packets(1);
        auto channel = reader.last_channel();
        auto udp_src = ntohs(channel.first.second);
        auto udp_dst = ntohs(channel.second.second);
        EXPECT_EQ(ip_addr_to_str(channel.first.first), "208.90.209.241") << "IP Src address wrong.";
        EXPECT_EQ(udp_src, 38121) << "UDP Src port wrong.";

        EXPECT_EQ(ip_addr_to_str(channel.second), "224.0.62.2:30001") << "IP Dst address wrong.";
        EXPECT_EQ(udp_dst, 30001) << "UDP Dst port wrong.";

        EXPECT_EQ(reader.last_ts().tv_sec, 1406229654) << "Pcap capture time seconds wrong.";
        EXPECT_EQ(reader.last_pkt_len(), check_pkt_len[i]) << "Pcap pkt " << i << " size does not match.";

    }

    auto portcount = test_udp.get_port_count();

    ASSERT_EQ(portcount.size(), 1) << "UDP pair count map not equal to size 1";
    EXPECT_EQ(portcount.begin()->second, 5) << "UDP pair port count not equal to 5.";
    EXPECT_EQ(portcount.begin()->first.first == 38121 && portcount.begin()->first.second == 30001, true) << "UDP port pair count map invalid entry.";
    EXPECT_EQ(test_udp.m_pkt_size == check_udp_len, true) << "Payload sizes did not match.";
    EXPECT_EQ(test_udp.m_udp_ts== check_ts, true) << "Packet capture timestamps do not match.";

}
