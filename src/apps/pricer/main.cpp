// Simple app to print packet counts for UDP addresses

#include <arpa/inet.h>

#include <time.h>

#include <algorithm>
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
#include <i01_net/PcapFileMux.hpp>

#include <i01_md/BookBase.hpp>
#include <i01_md/DataManager.hpp>
#include <i01_md/DecoderMux.hpp>
#include <i01_md/BookMuxListener.hpp>
#include <i01_md/NoopRawListener.hpp>
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

namespace {
std::string ob_side_str(const L3OrderData::Side &s) {
    switch (s) {
    case L3OrderData::Side::BUY:
        return "BID";
    case L3OrderData::Side::SELL:
        return "ASK";
    case L3OrderData::Side::UNKNOWN:
    default:
        return "UNK";
    }
}
}

class PricerApp :
    public Application,
    public NoopBookMuxListener
{
public:
    enum OutputFormat {
        ORDERBOOK = 1,
        L1_TICKS = 2,
    };

public:
    typedef i01::MD::L2Book L2Book;
    typedef i01::core::MICEnum MICEnum;
    using DataManager = i01::MD::DataManager;
    using PDPBookMux = DataManager::PDPBookMux;
    using XDPBookMux = DataManager::XDPBookMux;
    using ITCHBookMux = DataManager::ITCHBookMux;
    using PITCHBookMux = DataManager::PITCHBookMux;

private:
    typedef NoopRawListener noop_type;
    typedef DecoderMux::DecoderMux<PDPBookMux, XDPBookMux, ITCHBookMux, XDPBookMux, ITCHBookMux, ITCHBookMux, PITCHBookMux, PITCHBookMux, PITCHBookMux, PITCHBookMux> multi_listener_decoder_mux_type;

    typedef i01::net::pcap::UDPReader<multi_listener_decoder_mux_type> udpreader_type;

    struct SymbolEntry {
        std::string m_cta_symbol;
        std::uint32_t m_fdo_stock_id;
        std::string m_id_string;
    };
    typedef std::vector<SymbolEntry> symbols_container_type;

    typedef std::unordered_map<EphemeralSymbolIndex,FullSummary> stock_summary_map;
    typedef std::vector<stock_summary_map> FullSummaryByMICAndStockMap;

    typedef std::unordered_map<EphemeralSymbolIndex, i01::core::Timestamp> stock_ts_map;
    typedef std::vector<stock_ts_map> TimestampByMICAndStockMap;

    typedef std::set<const OrderBook *> OrderBookSet;
    typedef std::vector<OrderBookSet> OrderBooksByMICMap;
public:

    PricerApp();
    PricerApp(int argc, const char *argv[]);

    void load_config(const Config::storage_type &cfg);
    virtual int run() override final;

    // virtual void on_gap(const GapEvent &ge) override final;

    virtual void on_nyse_opening_imbalance(const NYSEOpeningImbalanceEvent& oi) override final;
    virtual void on_nyse_closing_imbalance(const NYSEClosingImbalanceEvent& ci) override final;
    virtual void on_nasdaq_imbalance(const NASDAQImbalanceEvent &ci) override final;

    virtual void on_trade(const TradeEvent & te) override final;

    virtual void on_l2_update(const L2BookEvent & be) override final;

    virtual void on_book_added(const L3AddEvent &evt) override final;
    virtual void on_book_canceled(const L3CancelEvent &evt) override final;
    virtual void on_book_modified(const L3ModifyEvent& mod_e) override final;
    virtual void on_book_executed(const L3ExecutionEvent& evt) override final;

    virtual void on_end_of_data(const PacketEvent & evt) override final;

private:
    void orderbook_format_(const L2Book *book, const Timestamp &ts, bool is_bid, bool is_bbo, Price price, Size size, NumOrders num_orders);
    void ticks_format_(OutputFormat format, const L2Book *book, const Timestamp &ts, bool is_bid, bool is_bbo, Price price, Size size, NumOrders num_orders);
    void check_offset_(const Timestamp &ts);

    const std::string & id_str_(std::uint32_t esi) const;


private:
    std::vector<std::string> m_pcap_filenames;
    std::string m_current_file;
    OutputFormat m_output_format;
    bool m_show_gaps;
    bool m_show_imbalances;
    std::string m_offset_hhmmss;
    std::uint64_t m_offset_millis;
    bool m_valid_offset;

    symbols_container_type m_symbols;

    OrderBooksByMICMap m_stocks_with_event;
    FullSummaryByMICAndStockMap m_last_best;
    TimestampByMICAndStockMap m_last_ts;
};

PricerApp::PricerApp() :
    Application(),
    m_output_format(OutputFormat::L1_TICKS),
    m_show_gaps(false),
    m_show_imbalances(false),
    m_offset_hhmmss("000000"),
    m_offset_millis(0),
    m_valid_offset(false),
    m_stocks_with_event((int)i01::core::MIC::Enum::NUM_MIC),
    m_last_best((int)i01::core::MIC::Enum::NUM_MIC),
    m_last_ts((int)i01::core::MIC::Enum::NUM_MIC)
{
    // specifying pcap-file as required() causes an uncaught exception to be thrown
    options_description().add_options()
        ("orderbook-format,o", "show output in orderbook ticks format")
        ("time-offset,t", po::value<std::string>(&m_offset_hhmmss), "Only show ticks after <HHMMSS>")
        ("show-gaps,g", po::value<bool>(&m_show_gaps)->default_value(false)->implicit_value(true), "Show gap messages")
        ("show-imb,i", po::value<bool>(&m_show_imbalances)->default_value(false)->implicit_value(true), "Show imbalance messages")
        ("pcap-file", po::value<std::vector<std::string> >(&m_pcap_filenames), "pcap file");
    positional_options_description().add("pcap-file",-1);
}

PricerApp::PricerApp(int argc, const char *argv[]) :
    PricerApp()
{
    if (!Application::init(argc,argv)) {
        std::exit(EXIT_FAILURE);
    }
    auto cfg = Config::instance().get_shared_state();
    load_config(*cfg);
}

void PricerApp::on_nyse_opening_imbalance(const NYSEOpeningImbalanceEvent& oi)
{
    check_offset_(oi.m_timestamp);
    if (oi.m_timestamp.milliseconds_since_midnight() < m_offset_millis) {
        return;
    }

    if (m_show_imbalances) {
        std::cout << "IMB,OI," << id_str_(oi.m_book.symbol_index()) << ","
                  << oi.m_timestamp.nanoseconds_since_midnight() << ","
                  << oi.m_msg << std::endl;
    }
}

void PricerApp::on_nyse_closing_imbalance(const NYSEClosingImbalanceEvent& ci)
{
    check_offset_(ci.m_timestamp);
    if (ci.m_timestamp.milliseconds_since_midnight() < m_offset_millis) {
        return;
    }

    if (m_show_imbalances) {
        std::cout << "IMB,"
                  << ci.m_book.mic().name() << ","
                  << "CI,"
                  << id_str_(ci.m_book.symbol_index()) << ","
                  << ci.m_timestamp.nanoseconds_since_midnight() << ","
                  << ci.m_msg << std::endl;
    }
}

void PricerApp::on_nasdaq_imbalance(const NASDAQImbalanceEvent &ci)
{
    check_offset_(ci.m_timestamp);
    if (ci.m_timestamp.milliseconds_since_midnight() < m_offset_millis) {
        return;
    }

    if (m_show_imbalances) {
        std::cout << "IMB,"
                  << ci.m_book.mic().name() << ","
                  << id_str_(ci.m_book.symbol_index()) << ","
                  << ci.m_timestamp.nanoseconds_since_midnight() << ","
                  << *ci.msg << std::endl;
    }
}

// void PricerApp::on_trade(BookBase *book, const Timestamp &ts, const TradeRefNum & tid, const Price &p, const Size &s, const Timestamp &exch_ts, void *msg)
// {

void PricerApp::on_trade(const TradeEvent & te)
{
    using namespace ARCA::XDP::Messages;
    using namespace ARCA::XDP::Types;
    const auto & ts = te.m_timestamp;
    auto tid = te.m_trade_id;
    auto p = te.m_price;
    auto s = te.m_size;
    const auto & exch_ts = te.m_exchange_timestamp;

    std::cout << "TRD,"
              << te.m_book.mic() << ","
              << id_str_(te.m_book.symbol_index()) << ","
              << ts.nanoseconds_since_midnight() << ","
              << tid << ","
              << p << ","
              << s << ","
              << exch_ts.nanoseconds_since_midnight() << ",";

    std::ostringstream os;
    switch((int)te.m_book.mic().market()) {
    case (int)MICEnum::XNYS:
    case (int)MICEnum::ARCX:
        {
            auto trd = reinterpret_cast<const Trade *>(te.m_msg);

            os << (char)trd->trade_condition1 << ","
               << (char)trd->trade_condition2 << ","
               << (char)trd->trade_condition3 << ","
               << (char)trd->trade_condition4 << ","
               << (int)trd->liquidity_indicator_flag;
        }
        break;
    default:
        os << " , , , , ";
        break;
    }

    std::cout << os.str() << std::endl;
}

void PricerApp::ticks_format_(OutputFormat format, const L2Book *book, const Timestamp &ts, bool is_bid, bool is_bbo, Price price, Size size, NumOrders num_orders)
{
    if (OutputFormat::L1_TICKS == format) {
        if (is_bbo) {
            auto best = book->best();
            std::cout << "L1," << id_str_(book->symbol_index()) << ","
                      << ts.nanoseconds_since_midnight() << ","
                      << std::get<2>(best.first) << ","
                      << std::get<1>(best.first) << ","
                      << std::get<0>(best.first) << ","
                      << std::get<0>(best.second) << ","
                      << std::get<1>(best.second) << ","
                      << std::get<2>(best.second) << std::endl;
        }
    }
}

void PricerApp::orderbook_format_(const L2Book *book, const Timestamp &ts, bool is_bid, bool is_bbo, Price price, Size size, NumOrders num_orders)
{
    std::ostringstream oss;
    // for orderbook format
    std::uint32_t id = static_cast<std::uint32_t>(price) | (is_bid << 31);
    std::uint64_t millis = ts.milliseconds_since_midnight();
    if (0 == size) {
        // only print out a cancel if we removed a level that existed

        if ((is_bid && book->last_order_deleted_existing_level(is_bid))
            || (!is_bid && book->last_order_deleted_existing_level(!is_bid))) {
            oss << "C," << millis << "," << id;
        } else {
            std::cout << "DE," << millis << "," << id << "," << price << std::endl;
        }
    } else {
        // whether it's an ADD or REPLACE depends on whether this is a new level
        if (is_bid) {
            if (book->last_order_added_level(true)) {
                oss << "A," << millis << "," << id << "," << price << "," << size << ",BID,FALSE";
            } else {
                oss << "R," << millis << "," << id << "," << size;
            }
        } else {
            if (book->last_order_added_level(false)) {
                oss << "A," << millis << "," << id << "," << price << "," << size << ",ASK,FALSE";
            } else {
                oss << "R," << millis << "," << id << "," << size;
            }
        }
    }
    std::cout << oss.str() << std::endl;
}

 // void PricerApp::on_l2_change(L2Book *book,
 //                              const Timestamp &ts,
 //                              bool is_bid,
 //                              bool is_bbo,
 //                              std::uint64_t price,
 //                              std::uint64_t size,
 //                              std::uint32_t num_orders)

 void PricerApp::on_l2_update(const L2BookEvent & be)
{
    const auto & ts = be.m_timestamp;
    auto is_bid = be.m_is_buy;
    auto is_bbo = be.m_is_bbo;
    auto price = be.m_price;
    auto size = be.m_size;
    auto num_orders = be.m_num_orders;

    check_offset_(ts);

    if (ts.milliseconds_since_midnight() < m_offset_millis) {
        return;
    }

    auto l2book = &be.m_book;
    switch (m_output_format) {
    case OutputFormat::ORDERBOOK:
        orderbook_format_(l2book, ts, is_bid, is_bbo, price, size, num_orders);
        break;
    case OutputFormat::L1_TICKS:
        ticks_format_(m_output_format,l2book, ts, is_bid, is_bbo, price, size, num_orders);
        break;
    default:
        break;
    }

}

void PricerApp::check_offset_(const Timestamp &ts)
{
    if (m_valid_offset) {
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
        m_offset_millis = (tm_p->tm_hour*3600 + tm_p->tm_min*60 + tm_p->tm_sec)*1000UL;
        // std::cout << "offset: " << m_offset_hhmmss << " epoch: " << epoch << " gm: " << m_offset_millis <<std::endl;
    } catch(boost::bad_lexical_cast const &b) {
        std::cerr << "time offset argument not valid HHMMSS format" << std::endl;
        throw b;
    }
    m_valid_offset = true;
}


void PricerApp::on_book_added(const L3AddEvent &evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);
    m_last_ts[evt.m_book.mic().index()][evt.m_book.symbol_index()] = evt.m_timestamp;
    if (OutputFormat::ORDERBOOK == m_output_format) {
        std::cout << "OB,"
                  << evt.m_book.mic().name() << ","
                  << id_str_(evt.m_book.symbol_index())
                  << ",A," << evt.m_timestamp.milliseconds_since_midnight() << ","
                  << evt.m_order.refnum << ","
                  << evt.m_order.price << ","
                  << evt.m_order.size << ","
                  << ob_side_str(evt.m_order.side)
                  << std::endl;
    }

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OBA," << evt << std::endl;
#endif
}

void PricerApp::on_book_canceled(const L3CancelEvent &evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);
    m_last_ts[evt.m_book.mic().index()][evt.m_book.symbol_index()] = evt.m_timestamp;
    if (OutputFormat::ORDERBOOK == m_output_format) {
        std::cout << "OB,"
                  << evt.m_book.mic().name() << ","
                  << id_str_(evt.m_book.symbol_index())
                  << ",C," << evt.m_timestamp.milliseconds_since_midnight() << ","
                  << evt.m_order.refnum
                  << std::endl;
    }

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OBC," << evt << std::endl;
#endif
}

void PricerApp::on_book_modified(const L3ModifyEvent& evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);
    m_last_ts[evt.m_book.mic().index()][evt.m_book.symbol_index()] = evt.m_timestamp;
    if (OutputFormat::ORDERBOOK == m_output_format) {
        // if the price changes then output a cancel and add
        if (evt.m_new_order.price != evt.m_old_price || evt.m_new_order.refnum != evt.m_old_refnum) {
            std::cout << "OB,"
                      << evt.m_book.mic().name() << ","
                      << id_str_(evt.m_book.symbol_index())
                      << ",C,"
                      << evt.m_timestamp.milliseconds_since_midnight() << ","
                      << evt.m_old_refnum
                      << std::endl;
            std::cout << "OB,"
                      << evt.m_book.mic().name() << ","
                      << id_str_(evt.m_book.symbol_index())
                      << ",A,"
                      << evt.m_timestamp.milliseconds_since_midnight() << ","
                      << evt.m_new_order.refnum << ","
                      << evt.m_new_order.price << ","
                      << evt.m_new_order.size << ","
                      << ob_side_str(evt.m_new_order.side)
                      << std::endl;
        } else {
            // just a size change
            std::cout << "OB,"
                      << evt.m_book.mic().name() << ","
                      << id_str_(evt.m_book.symbol_index())
                      << ",R," << evt.m_timestamp.milliseconds_since_midnight() << ","
                      << evt.m_old_refnum << ","
                      << evt.m_new_order.size
                      << std::endl;
        }
    }

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OBM," << evt << std::endl;
#endif
}
void PricerApp::on_book_executed(const L3ExecutionEvent& evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);
    m_last_ts[evt.m_book.mic().index()][evt.m_book.symbol_index()] = evt.m_timestamp;
    if (OutputFormat::ORDERBOOK == m_output_format) {
        if (0 == evt.m_order.size) {
            std::cout << "OB,"
                      << evt.m_book.mic().name() << ","
                      << id_str_(evt.m_book.symbol_index()) << ","
                      << "C," << evt.m_timestamp.milliseconds_since_midnight() << ","
                      << evt.m_order.refnum
                      << std::endl;
        } else {
            std::cout << "OB,"
                      << evt.m_book.mic().name() << ","
                      << id_str_(evt.m_book.symbol_index()) << ","
                      << "R," << evt.m_timestamp.milliseconds_since_midnight() << ","
                      << evt.m_order.refnum << ","
                      << evt.m_order.size
                      << std::endl;
        }

        std::cout << "OB,"
                  << evt.m_book.mic().name() << ","
                  << id_str_(evt.m_book.symbol_index()) << ","
                  << "T," << evt.m_timestamp.milliseconds_since_midnight() << ","
                  << evt.m_order.refnum << ","
                  << evt.m_exec_price << ","
                  << evt.m_exec_size
                  << std::endl;
    } else {
        const auto & ts = evt.m_timestamp;
        auto tid = evt.m_trade_id;
        auto p = evt.m_exec_price;
        auto s = evt.m_exec_size;
        const auto & exch_ts = evt.m_exchange_timestamp;

        std::cout << "TRD,"
                  << evt.m_book.mic() << ","
                  << id_str_(evt.m_book.symbol_index()) << ","
                  << ts.nanoseconds_since_midnight() << ","
                  << tid << ","
                  << p << ","
                  << s << ","
                  << exch_ts.nanoseconds_since_midnight() << ","
                  << " , , , , " << std::endl;
    }

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OBE," << evt << std::endl;
#endif
}

void PricerApp::on_end_of_data(const PacketEvent &evt)
{
    const auto & mic = evt.mic;

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "EOD," << mic.name() << std::endl;
#endif
    if (OutputFormat::L1_TICKS == m_output_format) {
        for (const auto & b : m_stocks_with_event[mic.index()]) {
            i01::MD::FullSummary best = b->best();
            if (best != m_last_best[b->mic().index()][b->symbol_index()]) {
                // new L1
                std::cout << "L1,"
                          << mic.name() << ","
                          << id_str_(b->symbol_index()) << ","
                          << m_last_ts[b->mic().index()][b->symbol_index()] << ","
                          << best
                          << std::endl;
                m_last_best[b->mic().index()][b->symbol_index()] = best;
            }
        }
    }
    m_stocks_with_event[mic.index()].clear();
}

const std::string & PricerApp::id_str_(std::uint32_t si) const
{
    static std::string defstr;
    if (si >= m_symbols.size()) {
        defstr = std::to_string(si) + ",UNKNOWN,0,";
        return defstr;
    }
    return m_symbols[si].m_id_string;
}

void PricerApp::load_config(const i01::core::Config::storage_type &cfg)
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

int PricerApp::run()
{
    if (variables_map().count("orderbook-format")) {
        m_output_format = OutputFormat::ORDERBOOK;
    }
    DataManager dm;

    dm.register_listener(this);

    i01::net::FileWithLatencyContainer files;
    std::transform(m_pcap_filenames.begin(), m_pcap_filenames.end(), std::inserter(files, files.begin()), [](const std::string& s) -> i01::net::FileWithLatency { return {s,0}; });

    i01::net::PcapFileMux<multi_listener_decoder_mux_type> pcapmux(files, nullptr);

    if (pcapmux.packets_enqueued()) {
        auto ts = pcapmux.top().pkt.ts;
        auto d = i01::core::Date::from_epoch(ts.tv_sec);
        std::cerr << "Pricer date from data is " << d << std::endl;
        dm.init(*i01::core::Config::instance().get_shared_state()->copy_prefix_domain("md."), d);
        multi_listener_decoder_mux_type decoder_mux(static_cast<PDPBookMux *>(dm.get_l2(MICEnum::XNYS)),
                                                    static_cast<XDPBookMux *>(dm.get_l3(MICEnum::XNYS)),
                                                    static_cast<ITCHBookMux *>(dm.get_l3(MICEnum::XNAS)),
                                                    static_cast<XDPBookMux *>(dm.get_l3(MICEnum::ARCX)),
                                                    static_cast<ITCHBookMux *>(dm.get_l3(MICEnum::XBOS)),
                                                    static_cast<ITCHBookMux *>(dm.get_l3(MICEnum::XPSX)),
                                                    static_cast<PITCHBookMux *>(dm.get_l3(MICEnum::BATY)),
                                                    static_cast<PITCHBookMux *>(dm.get_l3(MICEnum::BATS)),
                                                    static_cast<PITCHBookMux *>(dm.get_l3(MICEnum::EDGX)),
                                                    static_cast<PITCHBookMux *>(dm.get_l3(MICEnum::EDGA)));
        pcapmux.listener(&decoder_mux);
        pcapmux.read_packets();
    }
    return 0;
}

int
main(int argc, const char *argv[])
{
    try {
        PricerApp app(argc, argv);

        return app.run();
    } catch (const std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Caught exception" << std::endl;
    }
    return 1;
}
