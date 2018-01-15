// Simple app to print packet counts for UDP addresses

#include <arpa/inet.h>
#include <sys/utsname.h>

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
#include <i01_core/AsyncMappedFile.hpp>
#include <i01_core/Config.hpp>
#include <i01_core/Date.hpp>
#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>

#include <i01_net/IPAddress.hpp>
#include <i01_net/Pcap.hpp>
#include <i01_net/PcapFileMux.hpp>

#include <i01_md/DecoderMux.hpp>
#include <i01_md/FeedState.hpp>
#include <i01_md/MDEventPoller.hpp>
#include <i01_md/util.hpp>

#include "SeqnumCap.hpp"

using namespace i01::core;
using namespace i01::MD;

class SeqnumCapApp :
    public Application
{
private:
public:
    using SeqnumListenerMap = std::multimap<MIC, std::unique_ptr<SeqnumListener> >;
    using ITCHDecoder = NASDAQ::ITCH50::Decoder::ITCH50Decoder<SeqnumListener>;
    using ITCHUnitState = std::nullptr_t;
    using PDPDecoder = NYSE::PDP::Decoder<SeqnumListener>;
    using PDPUnitState = NYSE::PDP::PDPMsgStream;
    using PITCHDecoder = BATS::PITCH2::Decoder<SeqnumListener>;
    using PITCHUnitState = BATS::PITCH2::UnitState;
    using XDPDecoder = ARCA::XDP::Decoder<SeqnumListener>;
    using XDPUnitState = ARCA::XDP::XDPMsgStream;

    template<typename DecoderType>
    using DecoderByMIC = std::map<MIC, DecoderType *>;

    using DecoderMux = DecoderMux::DecoderMux<SeqnumListener,
                                              SeqnumListener,
                                              SeqnumListener,
                                              SeqnumListener,
                                              SeqnumListener,
                                              SeqnumListener,
                                              SeqnumListener,
                                              SeqnumListener,
                                              SeqnumListener,
                                              SeqnumListener>;

    using PcapFileMux = i01::net::PcapFileMux<DecoderMux>;

public:
    SeqnumCapApp();
    SeqnumCapApp(int argc, const char *argv[]);

    virtual int run() override final;

private:
    void record_seqnums_from_live();
    void record_seqnums_from_pcaps();
    void playback_seqnum_files();

private:
    std::string m_hostname;
    std::string m_prefix;
    bool m_is_read;
    std::vector<std::string> m_seqnum_filenames;
    MDEventPoller m_md_pollers;
    SeqnumListenerMap m_seqnum_listeners;
    DecoderByMIC<PITCHDecoder> m_pitch_family_decoders;
    DecoderByMIC<XDPDecoder> m_xdp_family_decoders;
    DecoderByMIC<ITCHDecoder> m_itch_family_decoders;
    DecoderByMIC<PDPDecoder> m_pdp_family_decoders;
    FS::FeedStateByMICFeedName<PITCHUnitState> m_pitch_family_feed_state;
    FS::FeedStateByMICFeedName<XDPUnitState> m_xdp_family_feed_state;
    FS::FeedStateByMICFeedName<ITCHUnitState> m_itch_family_feed_state;
    FS::FeedStateByMICFeedName<PDPUnitState> m_pdp_family_feed_state;
};

SeqnumCapApp::SeqnumCapApp() :
    Application()
{
    // specifying pcap-file as required() causes an uncaught exception to be thrown
    options_description().add_options()
        ("read,r", po::bool_switch(&m_is_read)->default_value(false), "for playing back seqnum files")
        ("prefix,p", po::value<std::string>(&m_prefix)->default_value("./"), "path prefix to write files")
        ("seqnum-file", po::value<std::vector<std::string> >(&m_seqnum_filenames), "seqnum file");
    positional_options_description().add("seqnum-file",-1);
}

SeqnumCapApp::SeqnumCapApp(int argc, const char *argv[]) :
    SeqnumCapApp()
{
    Application::init(argc,argv);
}

void SeqnumCapApp::playback_seqnum_files()
{
    for (const auto& f : m_seqnum_filenames) {
        SeqnumCap::SeqnumReader r(f);
        std::cout << "Reading " << f << ": header: " << r.header() << std::endl;
        for (const auto& e : r) {
            // e is a generic event
            if (e) {
                std::cout << *e << std::endl;
            }
        }

    }
}

void SeqnumCapApp::record_seqnums_from_pcaps()
{
    auto filename_prefix = m_prefix + "/" + m_hostname + ".";

    auto histcfg(Config::instance().get_shared_state()->copy_prefix_domain("hist."));

    std::map<MIC, SeqnumListener *> listeners;
    std::string filename;
    for (const auto& m : {MICEnum::BATS, MICEnum::BATY, MICEnum::EDGX, MICEnum::EDGA}) {
        filename = filename_prefix + MIC(m).name() + ".PITCH.seqnum";
        listeners[m] = new SeqnumCap::SeqnumWriter(m, "PITCH", filename);
    }

    for (const auto& m : {MICEnum::XNAS, MICEnum::XBOS, MICEnum::XPSX}) {
        filename = filename_prefix + MIC(m).name() + ".ITCH.seqnum";
        listeners[m] = new SeqnumCap::SeqnumWriter(m, "ITCH", filename);
    }

    filename = filename_prefix + "ARCX.INTXDP.seqnum";
    listeners[MICEnum::ARCX] = new SeqnumCap::SeqnumWriter(MICEnum::ARCX, "INTXDP", filename);
    filename = filename_prefix + "XNYS.TRD.seqnum";
    listeners[MICEnum::XNYS] = new SeqnumCap::SeqnumWriter(MICEnum::XNYS, "TRD", filename);

    filename = filename_prefix + "XNYS.OB.seqnum";
    auto xnys_ob = new SeqnumCap::SeqnumWriter(MICEnum::XNYS, "OB", filename);

    auto decoder_mux = new DecoderMux(xnys_ob,
                                      listeners[MICEnum::XNYS],
                                      listeners[MICEnum::XNAS],
                                      listeners[MICEnum::ARCX],
                                      listeners[MICEnum::XBOS],
                                      listeners[MICEnum::XPSX],
                                      listeners[MICEnum::BATY],
                                      listeners[MICEnum::BATS],
                                      listeners[MICEnum::EDGX],
                                      listeners[MICEnum::EDGA]);

    auto files = std::set<std::string>(m_seqnum_filenames.begin(), m_seqnum_filenames.end());

    auto file_mux = new PcapFileMux(files, decoder_mux);

    file_mux->read_packets();
}

void SeqnumCapApp::record_seqnums_from_live()
{
    auto filename_prefix = m_prefix + "/" + m_hostname + "." + std::to_string(i01::core::Date::today().date())
        + "." + std::to_string(i01::core::Timestamp::now().tv_sec) + ".";

    auto mdcfg = Config::instance().get_shared_state()->copy_prefix_domain("md.");
    auto exchcfg = mdcfg->copy_prefix_domain("exchanges.");

    m_md_pollers.init(*mdcfg);

    auto pitch_family = std::vector<MIC>{{MICEnum::BATS, MICEnum::BATY, MICEnum::EDGX, MICEnum::EDGA}};
    for (const auto& m : pitch_family) {
        auto filename = filename_prefix + std::string(m.name()) + ".PITCH.seqnum";
        auto p = std::unique_ptr<SeqnumListener>(new SeqnumCap::SeqnumWriter(m, "PITCH", filename));
        m_pitch_family_decoders[m] = new PITCHDecoder(p.get(), m);
        m_seqnum_listeners.emplace(m,std::move(p));

    }
    auto pitch_gen = UnitStateGenerator<PITCHUnitState>{PITCHUnitState::create};
    FS::init_feed_states(m_pitch_family_feed_state, pitch_gen, pitch_family, "PITCH", *exchcfg, m_hostname);

    auto itch_family = std::vector<MIC>{{MICEnum::XNAS, MICEnum::XBOS, MICEnum::XPSX}};

    for (const auto& m: itch_family) {
        auto filename = filename_prefix + std::string(m.name()) + ".ITCH.seqnum";
        auto p = std::unique_ptr<SeqnumListener>(new SeqnumCap::SeqnumWriter(m, "ITCH", filename));
        m_itch_family_decoders[m] = new ITCHDecoder(p.get(), NASDAQ::MoldUDP64::Decoder::SessionState());
        m_seqnum_listeners.emplace(m, std::move(p));
    }
    UnitStateGenerator<ITCHUnitState> itch_gen = [](const MIC&, const std::string&,
                                                    const std::string&,
                                                    UnitIndex,
                                                    const i01::core::Timestamp&,
                                                    const i01::core::Config::storage_type&) {
        return nullptr;
    };
    FS::init_feed_states(m_itch_family_feed_state, itch_gen, itch_family, "ITCH5", *exchcfg, m_hostname);

    auto pdp_gen = UnitStateGenerator<PDPUnitState>{PDPUnitState::create};
    {
        auto p = std::unique_ptr<SeqnumListener>(new SeqnumCap::SeqnumWriter(MICEnum::XNYS, "OB", filename_prefix + "XNYS.OB.seqnum"));
        m_pdp_family_decoders[MICEnum::XNYS] = new PDPDecoder(p.get());
        m_seqnum_listeners.emplace(MICEnum::XNYS, std::move(p));
    }
    FS::init_feed_states(m_pdp_family_feed_state, pdp_gen, {{MICEnum::XNYS, "OB"}, {MICEnum::XNYS, "IMB"}}, *exchcfg, m_hostname);

    auto xdp_gen = UnitStateGenerator<XDPUnitState>{XDPUnitState::create};

    auto xdpp = std::unique_ptr<SeqnumListener>(new SeqnumCap::SeqnumWriter(MICEnum::ARCX, "INTXDP", filename_prefix + "ARCX.INTXDP.seqnum"));
    m_xdp_family_decoders[MICEnum::ARCX] = new XDPDecoder(xdpp.get(), MICEnum::ARCX);
    m_seqnum_listeners.emplace(MICEnum::ARCX, std::move(xdpp));

    xdpp = std::unique_ptr<SeqnumListener>(new SeqnumCap::SeqnumWriter(MICEnum::XNYS, "TRD", filename_prefix + "XNYS.TRD.seqnum"));
    m_xdp_family_decoders[MICEnum::XNYS] = new XDPDecoder(xdpp.get(), MICEnum::XNYS);
    m_seqnum_listeners.emplace(MICEnum::XNYS,std::move(xdpp));

    FS::init_feed_states(m_xdp_family_feed_state, xdp_gen,
                         {{MICEnum::ARCX, "INTXDP"}, {MICEnum::XNYS, "TRD"}} , *exchcfg, m_hostname);


    // for (const auto& f: m_pdp_family_feed_state) {
    //     std::cerr << "Feed state for " << f.first.first << ": " << f.first.second << ": ";
    //     if (f.second) {
    //         std::cerr << *f.second;
    //     } else {
    //         std::cerr << "<NULL>";
    //     }
    //     std::cerr << std::endl;
    // }

    // std::cerr << m_md_pollers.get_config() << std::endl;

    m_md_pollers.init_feed_event_pollers(m_pitch_family_decoders, m_pitch_family_feed_state);
    m_md_pollers.init_feed_event_pollers(m_xdp_family_decoders, m_xdp_family_feed_state);
    m_md_pollers.init_feed_event_pollers(m_itch_family_decoders, m_itch_family_feed_state);
    m_md_pollers.init_feed_event_pollers(m_pdp_family_decoders, m_pdp_family_feed_state);

    auto md_epollers(mdcfg->copy_prefix_domain("eventpollers."));
    for (auto& p : m_md_pollers) {
        auto poller_cfg(md_epollers->copy_prefix_domain(p.first + "."));
        p.second->spawn();
        p.second->set_cpu_affinity(*poller_cfg);
    }

    for (auto& p : m_md_pollers) {
        if (p.second) {
            p.second->join();
        }
    }
}

int SeqnumCapApp::run()
{
    struct utsname uname_result;
    if (0 == ::uname(&uname_result)) {
        m_hostname = uname_result.nodename;
    }

    if (m_is_read) {
        playback_seqnum_files();
    } else {
        if (m_seqnum_filenames.size()) {
            record_seqnums_from_pcaps();
        } else {
            record_seqnums_from_live();
        }
    }
    return 0;
}

int
main(int argc, const char *argv[])
{
    try {
        SeqnumCapApp app(argc, argv);

        return app.run();
    } catch (const std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }
    return 1;
}
