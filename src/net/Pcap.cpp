#include <string>
#include <string.h>

#include <boost/lexical_cast.hpp>
#include <i01_net/Pcap.hpp>


namespace i01 { namespace net { namespace pcap {

Reader::Reader(const std::string &filename) :
    m_filename(filename)
{
    open_file_();
}

void
Reader::open_file_()
{
    m_pcap_p = new core::pcap::FileReader(m_filename.c_str(), m_errbuf);
}

void Reader::close_file_()
{
    if (m_pcap_p) {
        delete m_pcap_p;
        m_pcap_p = nullptr;
    }
}

int Reader::read_packets(int limit)
{
    std::uint32_t count = 0;
    struct pcap_pkthdr *header_p;
    const unsigned char *data_p;
    while (m_pcap_p && (limit < 0 || count < (std::uint32_t)limit)) {
        int retval = m_pcap_p->next_ex(&header_p, &data_p);

        if (retval <0) {
            if (-2 == retval) {
                // no more packets in this file
                close_file_();
                return 0;
            }
            return -1;
        }

        count++;
        m_bytes_read += (std::uint32_t) sizeof(*header_p) + header_p->caplen;

        if (header_p->len != header_p->caplen) {
            // means we didn't capture the full packet .. so quit here..
            std::cerr << "Capture size differs from packet size on pkt " << count << std::endl;
            return -1;
        }

        if (!header_p->len) {
            std::cerr <<"zero length packet read" << std::endl;
            return -1;
        }
        m_pkt_len = header_p->len;
        // this should be in seconds + nanos
        m_ts = i01::core::Timestamp{header_p->ts.tv_sec, header_p->ts.tv_usec};
        const struct ethhdr *eth_p = reinterpret_cast<const struct ethhdr *>(data_p);

        std::uint16_t next_proto = ntohs(eth_p->h_proto);
        std::uint32_t header_size = sizeof(struct ethhdr);
        if (ETH_P_8021Q == next_proto) {
            // vlan extended header
            // just skip the next two bytes to find the next protocol
            header_size += 4;
            next_proto = ntohs(*reinterpret_cast<const std::uint16_t *>(data_p + sizeof(struct ethhdr) + 2));
        }

        std::uint32_t payload_size = header_p->caplen - header_size;

        handle_eth_payload_(next_proto, const_cast<unsigned char*>(data_p+header_size), payload_size, &m_ts);
    }

    return count;
}

} } }
