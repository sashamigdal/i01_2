// Simple app to print packet counts for UDP addresses

#include <arpa/inet.h>

#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <i01_core/Application.hpp>
#include <i01_core/Config.hpp>
#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/util.hpp>

#include <i01_net/IPAddress.hpp>
#include <i01_net/Pcap.hpp>
#include <i01_net/PcapFileMux.hpp>

#include <i01_md/DecoderMux.hpp>
#include <i01_md/DiagnosticListener.hpp>

using namespace i01::core;
using namespace i01::MD;

class DecoderListener : public DiagnosticListener {
public:
    virtual void on_gap_detected(const Timestamp &ts, std::uint32_t addr, std::uint16_t port, std::uint8_t unit, std::uint64_t expect_seqnum, std::uint64_t recv_seqnum, const Timestamp &last_ts) override final;
    virtual void on_gap_detected(const Timestamp &ts, const i01::MD::NASDAQ::MoldUDP64::Types::Session &session, std::uint64_t expect_seqnum, std::uint64_t recv_seqnum, const Timestamp &last_ts) override final;

    std::string pretty_gap_message(const Timestamp &ts, std::uint64_t expect_seqnum, std::uint64_t recv_seqnum, const Timestamp &last_ts);
};

template<typename DecoderMuxT>
class PcapStats {
private:
    typedef i01::MD::NASDAQ::MoldUDP64::Decoder::MUDecoder MUDecoder;

    struct ConversationSort {
        bool operator() (const typename DecoderMuxT::Conversation& a, const typename DecoderMuxT::Conversation& b) const {
            return a.first < b.first ? true : (a.first == b.first ? a.second < b.second : false);
        }
    };
    using SortedConversations = std::map<typename DecoderMuxT::Conversation, std::uint64_t, ConversationSort>;

    using FileMux = i01::net::PcapFileMux<DecoderMuxT>;

public:
    PcapStats() : m_decoder_mux(&m_decoder_listener) {}

    void seqnum_summary(DecoderMuxT &dm);

    // this is for non-ITCH
    template<typename DecoderType>
    typename std::enable_if<!std::is_base_of<MUDecoder, DecoderType>::value, void>::type
    seqnum_summary_helper(const DecoderType &d)
    {
        for (const auto &s : d.streams()) {
            std::cout << "SEQ," << s.name() << "," << s.expect_seqnum() << std::endl;
        }
    }

    // for ITCH
    template<typename DecoderType>
    typename std::enable_if<std::is_base_of<MUDecoder, DecoderType>::value, void>::type
    seqnum_summary_helper(const DecoderType &d) {
        using i01::core::operator<<;
        if (d.session_state().bytes_received) {
            std::cout << "SEQ," << d.session_state().session << "," << d.session_state().last_seqnum << std::endl;
        }
    }

    void process_files(const std::vector<std::string> &files);

private:
    DecoderListener m_decoder_listener;
    DecoderMuxT m_decoder_mux;
    FileMux *m_file_mux;
};


class PcapStatsApp :
    public Application
{
private:
    typedef DecoderMux::SingleListenerDecoderMux<DecoderListener, EDGE::NextGen::Decoder, true> NextGenEdgeDecoderMux;
    typedef DecoderMux::SingleListenerDecoderMux<DecoderListener, BATS::PITCH2::Decoder, true> PitchEdgeDecoderMux;

public:
    PcapStatsApp();
    PcapStatsApp(int argc, const char *argv[]);

    virtual int run() override final;

    bool check_for_nextgen_edge();

private:
    std::vector<std::string> m_pcap_filenames;
};

PcapStatsApp::PcapStatsApp() :
    Application()
{
    // specifying pcap-file as required() causes an uncaught exception to be thrown
    options_description().add_options()
       ("pcap-file", po::value<std::vector<std::string> >(&m_pcap_filenames), "pcap file");
    positional_options_description().add("pcap-file",-1);
}

PcapStatsApp::PcapStatsApp(int argc, const char *argv[]) :
    PcapStatsApp()
{
    Application::init(argc,argv);
}

std::string DecoderListener::pretty_gap_message(const Timestamp &ts, std::uint64_t expect_seqnum, std::uint64_t recv_seqnum, const Timestamp &last_ts)
{
    std::ostringstream os;

    struct tm lts;
    lts = *localtime(&ts.tv_sec);
    char buf[256];

    strftime(buf, sizeof(buf), "%Y%m%d-%H:%M:%S", &lts);

    os << expect_seqnum << ","
       << buf;

    snprintf(buf, sizeof(buf), ".%09ld", ts.tv_nsec);

    os << buf << ","
       << recv_seqnum - expect_seqnum << ","
       << ts - last_ts;
    return os.str();
}

void DecoderListener::on_gap_detected(const Timestamp &ts, std::uint32_t addr, std::uint16_t port, std::uint8_t unit, std::uint64_t expect_seqnum, std::uint64_t recv_seqnum, const Timestamp &last_ts)
{
    std::cout << "GAP," << ts << "," << i01::net::ip_addr_to_str({addr,port}) << ","
              << expect_seqnum << ","
              << recv_seqnum << ","
              << last_ts
              << std::endl;

    std::cout << "GAP2," << ts << "," << i01::net::ip_addr_to_str({addr,port}) << ","
              << pretty_gap_message(ts, expect_seqnum, recv_seqnum, last_ts)
              << std::endl;

}

void DecoderListener::on_gap_detected(const Timestamp &ts, const i01::MD::NASDAQ::MoldUDP64::Types::Session &session, std::uint64_t expect_seqnum, std::uint64_t recv_seqnum, const Timestamp &last_ts)
{
    using i01::core::operator<<;
    std::cout << "GAP," << ts << "," << session << ","
              << expect_seqnum << ","
              << recv_seqnum << ","
              << last_ts
              << std::endl;

    std::cout << "GAP2," << ts << "," << session << ","
              << pretty_gap_message(ts, expect_seqnum, recv_seqnum, last_ts)
              << std::endl;
}

template<typename DecoderMuxT>
void PcapStats<DecoderMuxT>::seqnum_summary(DecoderMuxT &dm)
{
    const auto &t = dm.decoder_tuple();
    seqnum_summary_helper(std::get<0>(std::get<DecoderMuxT::XNYSDecoderIndex>(t)));
    seqnum_summary_helper(std::get<1>(std::get<DecoderMuxT::XNYSDecoderIndex>(t)));
    seqnum_summary_helper(std::get<DecoderMuxT::XNASDecoderIndex>(t));
    seqnum_summary_helper(std::get<DecoderMuxT::ARCXDecoderIndex>(t));
    seqnum_summary_helper(std::get<DecoderMuxT::XBOSDecoderIndex>(t));
    seqnum_summary_helper(std::get<DecoderMuxT::XPSXDecoderIndex>(t));
    seqnum_summary_helper(std::get<DecoderMuxT::BATYDecoderIndex>(t));
    seqnum_summary_helper(std::get<DecoderMuxT::BATSDecoderIndex>(t));
    seqnum_summary_helper(std::get<DecoderMuxT::EDGADecoderIndex>(t));
    seqnum_summary_helper(std::get<DecoderMuxT::EDGXDecoderIndex>(t));
}

template<typename DMT>
void PcapStats<DMT>::process_files(const std::vector<std::string> & filenames)
{
    m_file_mux = new FileMux(std::set<std::string>(filenames.begin(), filenames.end()), &m_decoder_mux);
    m_file_mux->read_packets();

    auto conversations = m_decoder_mux.conversations();
    SortedConversations sc;

    for (const auto & c : conversations) {
        sc[c.first] = c.second;
    }

    for (const auto& c: sc) {
        std::cout << "PKT," << n::ip_addr_to_str(c.first.first) << "," << n::ip_addr_to_str(c.first.second) << "," << c.second << std::endl;
    }

    if (m_decoder_listener.num_messages()) {
        std::cout << "MSG," << m_decoder_listener.message_summary() << std::endl;
    }

    seqnum_summary(m_decoder_mux);
}

bool PcapStatsApp::check_for_nextgen_edge()
{
    const auto cfg = Config::instance().get_shared_state();

    std::vector<MICEnum> mics = {MICEnum::EDGA, MICEnum::EDGX};
    for (const auto &m : mics) {
        const auto & names = ConfigReader::get_feed_names(*cfg, m);
        for (const auto &n : names) {
            std::string upc;
            for (const auto & c : n) {
                upc.push_back(static_cast<char>(std::toupper(c)));
            }
            if (upc == "NEXTGEN") {
                return true;
            }
        }
    }
    return false;
}

int PcapStatsApp::run()
{
    PcapStats<PitchEdgeDecoderMux> pitch_edge_pcap_stats;
    PcapStats<NextGenEdgeDecoderMux> nextgen_edge_pcap_stats;

    if (check_for_nextgen_edge()) {
        nextgen_edge_pcap_stats.process_files(m_pcap_filenames);
    } else {
        pitch_edge_pcap_stats.process_files(m_pcap_filenames);
    }

    return 0;
}

int
main(int argc, const char *argv[])
{
    try {
        PcapStatsApp app(argc, argv);

        return app.run();
    } catch (const std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }
    return 1;
}
