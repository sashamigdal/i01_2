// Simple app to print packet counts for UDP addresses

#include <arpa/inet.h>

#include <time.h>

#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <tuple>

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include <i01_core/Application.hpp>
#include <i01_core/Config.hpp>
#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>

#include <i01_net/IPAddress.hpp>
#include <i01_net/Pcap.hpp>

#include <i01_md/DecoderMux.hpp>
#include <i01_md/BookMuxListener.hpp>
#include <i01_md/RawListener.hpp>

#include <i01_md/ARCA/XDP/BookMux.hpp>
#include <i01_md/BATS/PITCH2/BookMux.hpp>
#include <i01_md/EDGE/NextGen/BookMux.hpp>
#include <i01_md/NASDAQ/ITCH50/BookMux.hpp>
#include <i01_md/NYSE/PDP/BookMux.hpp>


using namespace i01::core;
using namespace i01::MD;
namespace pdp = i01::MD::NYSE::PDP;
namespace xdp = i01::MD::ARCA::XDP;
namespace itch = i01::MD::NASDAQ::ITCH50;
namespace pitch = i01::MD::BATS::PITCH2;
namespace ng = i01::MD::EDGE::NextGen;

class NoopRawListener : public RawListener<NoopRawListener> {
public:
    template<typename...Args>
    void on_raw_msg(const Timestamp &ts, Args...args) {}
};


class SamplerApp :
    public Application,
    public ConfigListener,
    public NoopBookMuxListener
{
public:
    enum OutputFormat {
        ORDERBOOK = 1,
        L1_TICKS = 2,
    };

public:
    using Application::Application;
    typedef i01::MD::L2Book L2Book;
    typedef i01::core::MICEnum MICEnum;
    using PDPBookMux = i01::MD::NYSE::PDP::BookMux<SamplerApp>;
    using XDPBookMux = i01::MD::ARCA::XDP::BookMux<SamplerApp>;
    using ITCHBookMux = i01::MD::NASDAQ::ITCH50::BookMux<SamplerApp>;
    using PITCHBookMux = i01::MD::BATS::PITCH2::BookMux<SamplerApp>;
    using NGBookMux = i01::MD::EDGE::NextGen::BookMux<SamplerApp>;

private:
    typedef NoopRawListener noop_type;
    typedef DecoderMux::DecoderMux<PDPBookMux,
                                   XDPBookMux,
                                   ITCHBookMux,
                                   XDPBookMux,
                                   ITCHBookMux,
                                   ITCHBookMux,
                                   PITCHBookMux,
                                   PITCHBookMux,
                                   NGBookMux,
                                   NGBookMux,
                                   i01::MD::EDGE::NextGen::Decoder> multi_listener_decoder_mux_type;
    typedef i01::net::pcap::UDPReader<multi_listener_decoder_mux_type> udpreader_type;
    typedef i01::core::MIC MIC;
    struct SymbolEntry {
        std::string m_cta_symbol;
        std::uint32_t m_fdo_stock_id;
        std::string m_id_string;
    };
    typedef std::vector<SymbolEntry> symbols_container_type;
    typedef i01::core::Timestamp Timestamp;
    typedef std::map<int,FullSummary> stock_summary_map;
    typedef std::vector<stock_summary_map> FullSummaryByMICAndStockMap;

    typedef std::unordered_map<EphemeralSymbolIndex, Timestamp> stock_ts_map;
    typedef std::vector<stock_ts_map> TimestampByMICAndStockMap;
    typedef std::set<const BookBase *> OrderBookSet;
    typedef std::vector<OrderBookSet> OrderBooksByMICMap;
    typedef std::unordered_map<BookBase::SymbolIndex, TradingStatusEvent::Event> BookTradingStatusContainer;
    typedef std::vector<BookTradingStatusContainer> TradingStatusByMICAndStock;

public:

    SamplerApp();
    SamplerApp(int argc, const char *argv[]);
    ~SamplerApp();

    bool init(int argc, const char *argv[] ) override;
    void init(const Config::storage_type &cfg);
    virtual int run() override final;
    virtual void on_config_update(const Config::storage_type &, const Config::storage_type &cfg) noexcept override final;

     virtual void on_l2_update(const L2BookEvent & be) override final;

    virtual void on_book_added(const L3AddEvent &evt) override final;
    virtual void on_book_canceled(const L3CancelEvent &evt) override final;
    virtual void on_book_modified(const L3ModifyEvent& mod_e) override final;
    virtual void on_book_executed(const L3ExecutionEvent& evt) override final;
    virtual void on_trading_status_update(const TradingStatusEvent &evt) override final;
    virtual void on_end_of_data(const PacketEvent & evt) override final;

private:
    void output_l1_(const MIC &mic, std::uint32_t si, const FullSummary &fs);

    void process_file_(const std::string &filename, multi_listener_decoder_mux_type &decoder_mux);

    void orderbook_format_(const L2Book *book, const Timestamp &ts, bool is_bid, bool is_bbo, std::uint64_t price, std::uint64_t size, std::uint32_t num_orders);
    void ticks_format_(OutputFormat format, const L2Book *book, const Timestamp &ts, bool is_bid, bool is_bbo, std::uint64_t price, std::uint64_t size, std::uint32_t num_orders);
    void check_offset_(const MIC &, const Timestamp &ts);

    const std::string & id_str_(std::uint32_t esi) const;


private:
     udpreader_type *m_reader;

    std::vector<std::string> m_pcap_filenames;
    std::string m_current_file;
    OutputFormat m_output_format;
    bool m_show_gaps;
    bool m_show_imbalances;
    std::string m_offset_hhmmss;
    std::vector<std::uint64_t> m_offset_millis;
    std::vector<bool> m_valid_offset;

    symbols_container_type m_symbols;

    std::vector<Timestamp> m_next_interval;
    std::uint32_t m_interval_seconds;

    OrderBooksByMICMap m_stocks_with_event;
    FullSummaryByMICAndStockMap m_last_best;
    TradingStatusByMICAndStock m_trading_status;

    bool m_quiet_mode;
};

SamplerApp::SamplerApp() :
    m_output_format(OutputFormat::L1_TICKS),
    m_show_gaps(false),
    m_show_imbalances(false),
    m_offset_hhmmss("000000"),
    m_offset_millis((int) MIC::Enum::NUM_MIC, 0),
    m_valid_offset((int) MIC::Enum::NUM_MIC, false),
    m_next_interval((int) MIC::Enum::NUM_MIC),
    m_stocks_with_event((int)MIC::Enum::NUM_MIC),
    m_last_best((int)MIC::Enum::NUM_MIC),
    m_trading_status((int)MIC::Enum::NUM_MIC)
{
    // specifying pcap-file as required() causes an uncaught exception to be thrown
    options_description().add_options()
        ("orderbook-format,o", "show output in orderbook ticks format")
        ("time-offset,t", po::value<std::string>(&m_offset_hhmmss), "Only show ticks after <HHMMSS>")
        ("interval-seconds,i", po::value<std::uint32_t>(&m_interval_seconds)->default_value(60), "seconds between sampled prices")
        ("quiet,q", po::value<bool>(&m_quiet_mode)->default_value(false)->implicit_value(true), "do not print prices")
        ("pcap-file", po::value<std::vector<std::string> >(&m_pcap_filenames)->required(), "pcap file");
    positional_options_description().add("pcap-file",-1);
}

SamplerApp::SamplerApp(int argc, const char *argv[]) :
    SamplerApp()
{
    Application::init(argc,argv);
    ConfigListener::subscribe();
    init(argc, argv);
    auto cfg = Config::instance().get_shared_state();
    init(*cfg);
}

SamplerApp::~SamplerApp() {
    ConfigListener::unsubscribe();
}

void SamplerApp::on_config_update(const Config::storage_type &, const Config::storage_type &cfg) noexcept
{
    init(cfg);
}

void SamplerApp::on_l2_update(const L2BookEvent & be)
{
    if (m_trading_status[be.m_book.mic().index()][be.m_book.symbol_index()] != TradingStatusEvent::Event::TRADING) {
        return;
    }

    m_stocks_with_event[be.m_book.mic().index()].insert(&be.m_book);

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OL2," << be << std::endl;
#endif
}

void SamplerApp::check_offset_(const MIC &mic, const Timestamp &ts)
{
    if (m_valid_offset[mic.index()]) {
        return;
    }

    try {
        std::uint32_t hh = boost::lexical_cast<std::uint32_t>(m_offset_hhmmss.substr(0,2));
        std::uint32_t mm = boost::lexical_cast<std::uint32_t>(m_offset_hhmmss.substr(2,2));
        std::uint32_t ss = boost::lexical_cast<std::uint32_t>(m_offset_hhmmss.substr(4,2));

        struct tm *tm_p = localtime(&ts.tv_sec);

        tm_p->tm_hour = hh;
        tm_p->tm_min = mm;
        tm_p->tm_sec = ss;
        time_t epoch = mktime(tm_p);

        tm_p = gmtime(&epoch);
        m_offset_millis[mic.index()] = (tm_p->tm_hour*3600 + tm_p->tm_min*60 + tm_p->tm_sec)*1000UL;
        // std::cout << "offset: " << m_offset_hhmmss << " epoch: " << epoch << " gm: " << m_offset_millis <<std::endl;

        m_next_interval[mic.index()] = Timestamp{ts.tv_sec  + (m_interval_seconds - (ts.tv_sec % m_interval_seconds)), 0};

    } catch(boost::bad_lexical_cast const &b) {
        std::cerr << "time offset argument not valid HHMMSS format" << std::endl;
        throw b;
    }
    m_valid_offset[mic.index()] = true;
}


void SamplerApp::on_book_added(const L3AddEvent &evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OBA," << evt << std::endl;
#endif
}

void SamplerApp::on_book_canceled(const L3CancelEvent &evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OBC," << evt << std::endl;
#endif
}

void SamplerApp::on_book_modified(const L3ModifyEvent& evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OBM," << evt << std::endl;
#endif
}
void SamplerApp::on_book_executed(const L3ExecutionEvent& evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OBE," << evt << std::endl;
#endif
}

void SamplerApp::output_l1_(const MIC &mic, std::uint32_t si, const FullSummary &b)
{
    std::cout << mic.name() << ","
              << m_symbols[si].m_fdo_stock_id << ","
              << m_next_interval[mic.index()].tv_sec << ","
              << b
              << std::endl;
}

void SamplerApp::on_trading_status_update(const TradingStatusEvent &evt)
{
    m_trading_status[evt.m_book.mic().index()][evt.m_book.symbol_index()] = evt.m_event;

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "TSU," << evt << std::endl;
#endif
}

void SamplerApp::on_end_of_data(const PacketEvent &evt)
{
#ifdef I01_DEBUG_MESSAGING
    std::cerr << "EOD," << evt.mic.name() << std::endl;
#endif

    const auto & mic = evt.mic;
    check_offset_(mic, evt.timestamp);

    if (evt.timestamp.milliseconds_since_midnight() < m_offset_millis[mic.index()]) {
        goto finished;
    }
    // check the time, and if it's past the interval time, then let's write out our prices for everything
    if (evt.timestamp > m_next_interval[mic.index()]) {
        auto tm = evt.timestamp;
        do {
            std::cout << mic.name() << ",INTERVAL,"
                      << m_next_interval[mic.index()].tv_sec << ","
                      << tm
                      << std::endl;
            for (const auto & b : m_last_best[mic.index()]) {
                // print out prices for everything here
                if (!m_quiet_mode) {
                    output_l1_(mic, b.first, b.second);
                }
            }
            tm += Timestamp{m_interval_seconds,0};
        } while (tm < m_next_interval[mic.index()]);

        // set next interval here
        m_next_interval[mic.index()].tv_sec += m_interval_seconds;
    }

    for (const auto & b : m_stocks_with_event[mic.index()]) {
        auto best = b->best();
        // if (std::get<0>(best.first) == std::get<0>(best.second)) {
        //     std::cerr << "ERR,LOCK," << mic.name() << "," << best << std::endl;
        // }

        if (best != m_last_best[b->mic().index()][b->symbol_index()]) {
            // new L1
            m_last_best[b->mic().index()][b->symbol_index()] = best;
        }
    }

 finished:
    m_stocks_with_event[mic.index()].clear();
}

const std::string & SamplerApp::id_str_(std::uint32_t si) const
{
    static std::string defstr;
    if (si >= m_symbols.size()) {
        defstr = std::to_string(si) + ",UNKNOWN,0,";
        return defstr;
    }
    return m_symbols[si].m_id_string;
}

bool SamplerApp::init(int argc, const char *argv[])
{
    return true;
}

void SamplerApp::init(const i01::core::Config::storage_type &cfg)
{
    using namespace i01::MD::ConfigReader;

    const auto & fdo = get_i01_vector_for_symbol(cfg, "fdo_symbol");
    const auto & cta = get_i01_vector_for_symbol(cfg, "cta_symbol");

    if (fdo.size()!=cta.size()) {
        std::cerr << "Expected to have equal number of FDO and CTA symbol names" << std::endl;
        return;
    }

    for (auto i= 0U; i < fdo.size(); i++) {
        if (fdo[i] != "" || cta[i] != "") {
            auto se = SymbolEntry{cta[i], boost::lexical_cast<std::uint32_t>(fdo[i]), std::to_string(i) +"," + cta[i] + "," + fdo[i] };
            if (i >= m_symbols.size()) {
                m_symbols.resize(i+1);
                m_symbols[i] = se;
            }
        }
    }
}

int SamplerApp::run()
{
    PDPBookMux pdp_book_mux(this);
    XDPBookMux nysetrd_book_mux(this, MICEnum::XNYS, {"TRD"});
    XDPBookMux arca_book_mux(this, MICEnum::ARCX, {"INTXDP"});
    ITCHBookMux inet_book_mux(this, MICEnum::XNAS);
    ITCHBookMux xbos_book_mux(this, MICEnum::XBOS);
    ITCHBookMux xpsx_book_mux(this, MICEnum::XPSX);
    PITCHBookMux bats_book_mux(this, MICEnum::BATS);
    PITCHBookMux baty_book_mux(this, MICEnum::BATY);
    NGBookMux edgx_book_mux(this, MICEnum::EDGX);
    NGBookMux edga_book_mux(this, MICEnum::EDGA);

    // decoder_mux_type m_decoder_mux;
    multi_listener_decoder_mux_type decoder_mux(&pdp_book_mux,
                                                &nysetrd_book_mux,
                                                &inet_book_mux,
                                                &arca_book_mux,
                                                &xbos_book_mux,
                                                &xpsx_book_mux,
                                                &baty_book_mux,
                                                &bats_book_mux,
                                                &edga_book_mux,
                                                &edgx_book_mux);

    for (std::size_t i = 0; i < m_pcap_filenames.size(); ++i) {
        process_file_(m_pcap_filenames[i], decoder_mux);
    }

    return 0;
}

void SamplerApp::process_file_(const std::string &filename, multi_listener_decoder_mux_type &decoder_mux)
{
    m_reader = new udpreader_type(filename, &decoder_mux);

    m_current_file = filename;

    m_reader->read_packets();

    delete m_reader;
}


int
main(int argc, const char *argv[])
{
    try {
        SamplerApp app(argc, argv);

        return app.run();
    } catch (const std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Caught exception" << std::endl;
    }
    return 1;
}
