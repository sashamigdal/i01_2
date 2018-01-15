#pragma once

#include <functional>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>


#include <i01_core/Config.hpp>
#include <i01_core/Date.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/TimerListener.hpp>

#include <i01_md/BookMuxListener.hpp>
#include <i01_md/DecoderMux.hpp>
#include <i01_md/FeedState.hpp>
#include <i01_md/HistoricalData.hpp>
#include <i01_md/LastSale.hpp>
#include <i01_md/MDEventPoller.hpp>
#include <i01_md/util.hpp>

#include <i01_md/ARCA/XDP/BookMux.hpp>
#include <i01_md/BATS/PITCH2/BookMux.hpp>
#include <i01_md/EDGE/NextGen/BookMux.hpp>
#include <i01_md/NASDAQ/ITCH50/BookMux.hpp>
#include <i01_md/NYSE/PDP/BookMux.hpp>

#include <i01_net/PcapFileMux.hpp>

namespace i01 { namespace net {
class EpollEventPoller;
class SocketListener;
}}


namespace i01 { namespace core {
struct MIC;
}}

namespace i01 { namespace MD {

class LastSaleListener;

class DataManager : public BookMuxListener,
                    public core::TimerListener {
public:
    using Date = core::Date;
    using ListenerContainer = std::vector<BookMuxListener *>;
    using L2BookMuxBase = MLBookMux<L2Book,DataManager>;
    using L3BookMuxBase = MLBookMux<OrderBook,DataManager>;

    using L2BookMuxContainer = std::vector<L2BookMuxBase *>;
    using L3BookMuxContainer = std::vector<L3BookMuxBase *>;

    using PITCHBookMux = BATS::PITCH2::BookMux<DataManager>;
    using ITCHBookMux = NASDAQ::ITCH50::BookMux<DataManager>;
    using PDPBookMux = NYSE::PDP::BookMux<DataManager>;
    using XDPBookMux = ARCA::XDP::BookMux<DataManager>;
    using NGBookMux = EDGE::NextGen::BookMux<DataManager>;

    using PITCHDecoder = BATS::PITCH2::Decoder<PITCHBookMux>;
    using ITCHDecoder = NASDAQ::ITCH50::Decoder::ITCH50Decoder<ITCHBookMux>;
    using XDPDecoder = ARCA::XDP::Decoder<XDPBookMux>;
    using PDPDecoder = NYSE::PDP::Decoder<PDPBookMux>;

    using PITCHUnitState = BATS::PITCH2::UnitState;
    using PITCHFeedState = FS::FeedState<PITCHUnitState>;
    using ITCHUnitState = std::nullptr_t;
    using ITCHFeedState = FS::FeedState<ITCHUnitState>;
    using XDPUnitState = ARCA::XDP::XDPMsgStream;
    using XDPFeedState = FS::FeedState<XDPUnitState>;
    using PDPUnitState = NYSE::PDP::PDPMsgStream;
    using PDPFeedState = FS::FeedState<PDPUnitState>;

    using DecoderMux = MD::DecoderMux::DecoderMux<PDPBookMux, // XNYS quote
                                                  XDPBookMux, // XNYS trade
                                                  ITCHBookMux, // XNAS
                                                  XDPBookMux, // ARCX
                                                  ITCHBookMux, // XBOS
                                                  ITCHBookMux, // XPSX
                                                  PITCHBookMux, // BATY
                                                  PITCHBookMux, // BATS
                                                  PITCHBookMux, // EDGA
                                                  PITCHBookMux>; // EDGX

    using PcapFileMux = net::PcapFileMux<DecoderMux>;

    using EdgeNGDecoderMux = MD::DecoderMux::DecoderMux<PDPBookMux, // XNYS quote
                                                        XDPBookMux, // XNYS trade
                                                        ITCHBookMux, // XNAS
                                                        XDPBookMux, // ARCX
                                                        ITCHBookMux, // XBOS
                                                        ITCHBookMux, // XPSX
                                                        PITCHBookMux, // BATY
                                                        PITCHBookMux, // BATS
                                                        NGBookMux, // EDGA
                                                        NGBookMux, // EDGX
                                                        EDGE::NextGen::Decoder>;

    using EdgeNGPcapFileMux = net::PcapFileMux<EdgeNGDecoderMux>;

    using FileMux = net::FileMuxBase;

    using TimerListener = core::TimerListener;
    using TimerListenerEntry = std::pair<TimerListener *, void *>;
    using TimerListenerContainer = std::set<TimerListenerEntry>;

    using LastSaleListenerContainer = std::vector<LastSaleListener *>;


public:
    enum class Era {
        PRESENT,
            EDGENG,
            };

private:
    using MICToL3BMMap = std::vector<L3BookMuxBase *>;
    using MICToL2BMMap = std::vector<L2BookMuxBase *>;

    struct InstData {
        LastSale last_sale;

        InstData() : last_sale() {}
        InstData(const LastSale& l) : last_sale(l) {}
        InstData(LastSale l) : last_sale(std::move(l)) {}
    };

    typedef std::array<InstData, MD::NUM_SYMBOL_INDEX> InstDataArray;

    using MICSocketListener = std::pair<core::MICEnum, net::SocketListener *>;



    template<typename DecoderType>
    using DecoderByMIC = std::map<core::MIC, DecoderType *>;

public:

    DataManager();
    virtual ~DataManager();
    // template function that calls CB on all listeners

    std::string hostname() const { return m_hostname; }
    void hostname(const std::string& hn) { m_hostname = hn; }

    void init(const core::Config::storage_type& cfg, const Date &d = 0);

    void register_listener(BookMuxListener *bml);
    void register_timer(TimerListener *tl, void *userdata);
    void register_last_sale_listener(LastSaleListener *lsl);

    // FIXME replace this single template function trickery? (http://stackoverflow.com/questions/22721201/specializations-only-for-c-template-function-with-enum-non-type-template-param)
    L3BookMuxBase * get_l3(const core::MIC &m) const;
    L2BookMuxBase * get_l2(const core::MIC &m) const;

    /// Allows us to call one function get_bm() to choose between the L2 or L3 version at compile time
    template<typename BMT>
    typename std::enable_if<std::is_same<BMT, L3BookMuxBase>::value,void>::type
    get_bm(const core::MIC& m, BMT *& bm) const {
        bm = get_l3(m);
    }

    template<typename BMT>
    typename std::enable_if<std::is_same<BMT, L2BookMuxBase>::value,void>::type
    get_bm(const core::MIC& m, BMT *& bm) const {
        bm = get_l2(m);
    }

    BookMuxBase * get_bm(const core::MIC&) const;

    // BookMuxListener
    virtual void on_symbol_definition(const SymbolDefEvent &) override final;
    virtual void on_book_added(const L3AddEvent& new_o) override final;
    virtual void on_book_canceled(const L3CancelEvent& cxl_o) override final;
    virtual void on_book_modified(const L3ModifyEvent& mod_e) override final;
    virtual void on_book_executed(const L3ExecutionEvent& exe_e) override final;
    virtual void on_book_crossed(const L3CrossedEvent& evt) override final;
    /// NYSE is a precious snowflake.
    virtual void on_l2_update(const L2BookEvent&) override final;
    virtual void on_nyse_opening_imbalance(const NYSEOpeningImbalanceEvent&) override final;
    virtual void on_nyse_closing_imbalance(const NYSEClosingImbalanceEvent&) override final;
    virtual void on_nasdaq_imbalance(const NASDAQImbalanceEvent&) override final;
    virtual void on_trade(const TradeEvent&) override final;
    virtual void on_gap(const GapEvent&) override final;
    virtual void on_trading_status_update(const TradingStatusEvent&) override final;
    virtual void on_feed_event(const FeedEvent&) override final;
    virtual void on_market_event(const MWCBEvent&) override final;
    virtual void on_start_of_data(const PacketEvent&) override final;
    virtual void on_end_of_data(const PacketEvent&) override final;
    virtual void on_timeout_event(const TimeoutEvent&) override final;
    virtual void on_timer(const core::Timestamp&, void *, std::uint64_t) override final;
    void manual_last_sale(const EphemeralSymbolIndex esi, Price last_sale) {}

public:
    void use_files(const std::set<std::string> &filenames);
    // void use_files(const std::vector<std::string> &filenames);

    bool read_data(int num_packets = -1);
    bool read_until(const core::Timestamp &t);

    void start_event_pollers();

    Date date() const { return m_date; }

private:
    void init_timer(net::EpollEventPoller*);
    void init_locals(const core::Config::storage_type& cfg);

    L3BookMuxBase * l3_book_mux_factory(const Date &d, const core::MIC &m);
    L2BookMuxBase * l2_book_mux_factory(const Date &d, const core::MIC &m);

    bool prepare_historical_data();

    template<typename ArgType>
    void dispatch(void(BookMuxListener::*mfp)(ArgType), ArgType && arg);


private:
    Date m_date;
    Era m_era;
    bool m_simulated;
    bool m_filemux_init;
    L3BookMuxContainer m_l3_bm;
    L2BookMuxContainer m_l2_bm;

    MICToL3BMMap m_mic_l3_bm;
    MICToL2BMMap m_mic_l2_bm;

    ListenerContainer m_listeners;
    TimerListenerContainer m_timer_listeners;
    LastSaleListenerContainer m_last_sale_listeners;

    DecoderMux * m_decoder_mux;
    EdgeNGDecoderMux * m_ng_decoder_mux;

    HistoricalData m_historical_data;
    net::FileWithLatencyContainer m_file_mux_files;
    FileMux * m_file_mux;

    InstDataArray m_data;
    std::string m_hostname;
    int m_verbose_level;

    MDEventPoller m_md_pollers;

    FS::FeedStateByMICFeedName<PITCHUnitState> m_pitch_family_feed_state;
    FS::FeedStateByMICFeedName<ITCHUnitState> m_itch_family_feed_state;
    FS::FeedStateByMICFeedName<XDPUnitState> m_xdp_family_feed_state;
    FS::FeedStateByMICFeedName<PDPUnitState> m_pdp_family_feed_state;

    DecoderByMIC<PITCHDecoder> m_pitch_family_decoders;
    DecoderByMIC<ITCHDecoder> m_itch_family_decoders;
    DecoderByMIC<XDPDecoder> m_xdp_family_decoders;
    DecoderByMIC<PDPDecoder> m_pdp_family_decoders;
};

template<typename ArgType>
void DataManager::dispatch(void(BookMuxListener::*mfp)(ArgType), ArgType && arg)
{
    for (auto l : m_listeners) {
        (l->*mfp)(std::forward<ArgType>(arg));
    }
}

}}
