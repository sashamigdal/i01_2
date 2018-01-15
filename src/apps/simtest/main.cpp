// Simple app to test the trade sim

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
#include <i01_net/PcapFileMux.hpp>

#include <i01_md/BookBase.hpp>
#include <i01_md/DecoderMux.hpp>
#include <i01_md/BookMuxListener.hpp>
#include <i01_md/RawListener.hpp>

#include <i01_md/ARCA/XDP/BookMux.hpp>
#include <i01_md/BATS/PITCH2/BookMux.hpp>
#include <i01_md/EDGE/NextGen/BookMux.hpp>
#include <i01_md/NASDAQ/ITCH50/BookMux.hpp>
#include <i01_md/NYSE/PDP/BookMux.hpp>

#include <i01_oe/ARCAOrder.hpp>
#include <i01_oe/EquityInstrument.hpp>
#include <i01_oe/L2SimSession.hpp>
#include <i01_oe/NYSEOrder.hpp>
#include <i01_oe/BATSOrder.hpp>
#include <i01_oe/NASDAQOrder.hpp>

using namespace i01::core;
using namespace i01::MD;
using namespace i01::OE;
namespace pdp = i01::MD::NYSE::PDP;
namespace xdp = i01::MD::ARCA::XDP;
namespace itch = i01::MD::NASDAQ::ITCH50;
namespace pitch = i01::MD::BATS::PITCH2;
namespace ng = i01::MD::EDGE::NextGen;

template<typename OrderType, int MICValue>
class TestStrategy : public i01::OE::OrderListener, public NoopBookMuxListener {
public:
    using Timestamp = i01::core::Timestamp;
    struct Event {
        bool is_order;
        i01::OE::Price price;
        i01::OE::Size size;
        Side side;
        TimeInForce tif;
        i01::OE::OrderType type;
        Order *order_p;
    };

    using EventContainer = std::map<Timestamp, Event>;

public:

    TestStrategy(OrderManager *om, OrderSession *osesh) :
        m_order_manager_p(om),
        m_order_session_p(osesh),
        m_inst{nullptr},
        m_out_ask(nullptr),
        m_out_bid(nullptr),
        m_moc_bid(nullptr),
        m_moc_ask(nullptr) {
        m_mics[MICValue] = true;
        for (auto &b : m_in_univ) {
            b = false;
        }

        if (m_order_manager_p->universe().size()) {
            m_in_univ[1] = true;
            m_inst = m_order_manager_p->universe()[1].data();
        }

        if (!m_inst) {
            throw std::runtime_error("no instrument defined");
        }
    }

    void add_event(const Timestamp &ts, const Event &evt) {
        m_events.emplace(ts, evt);
    }

    void on_start_of_data(const PacketEvent &evt) override final {
        if (!m_mics[evt.mic.index()]) {
            return;
        }

        m_ts = evt.timestamp;
        // std::cerr << "SOD," << evt << "," << m_ts << std::endl;
    }

    void on_trading_status_update(const TradingStatusEvent &evt) override final {
        if (!m_mics[evt.m_book.mic().index()]) {
            return;
        }

        m_state.update(evt.m_event);
    }

    void book_added_helper(const FullL2Quote &q, Order*& o, const L3AddEvent &evt) {
        // if (SymbolState::TradingState::TRADING != m_state.trading_state) {
        //     return;
        // }

        if (nullptr == o) {
            auto size = q.bid.size;
            auto price = to_double(q.bid.price);
            if (evt.m_order.side == OrderBook::Order::Side::SELL) {
                size = q.ask.size;
                price = to_double(q.ask.price);
            }

            o = m_order_manager_p->create_order<OrderType>(m_inst, price, size,
                                                           evt.m_order.side == OrderBook::Order::Side::BUY ? Side::BUY : Side::SELL,
                                                           TimeInForce::DAY,
                                                           i01::OE::OrderType::LIMIT, this);
            if (!m_order_manager_p->send(o, m_order_session_p)) {
                std::cerr << "TST,ERR,SEND_ON_NULL," << *o << std::endl;
                o = nullptr;
            }
        }
    }

    void on_book_added(const L3AddEvent &evt) override final {
        if (!m_mics[evt.m_book.mic().index()]) {
            return;
        }
        if (!m_in_univ[evt.m_book.symbol_index()]) {
            return;
        }
        FullL2Quote q = evt.m_book.best();
        if (q != m_last_best) {

            std::cerr << "TST,OBA," << evt.m_timestamp << "," << q << "," << evt << std::endl;
            book_added_helper(q, evt.m_order.side == OrderBook::Order::Side::BUY ? m_out_bid : m_out_ask, evt);
            check_for_cancel(evt.m_order.side, q, evt.m_book);
        }
    }

    void check_for_cancel(OrderBook::Order::Side order_side,
                          const FullL2Quote & q,
                          const BookBase & book) {
        if (OrderBook::Order::Side::BUY == order_side) {
            if (m_out_bid) {
                if (0 == q.bid.size) {
                    if (m_out_bid->state() == OrderState::ACKNOWLEDGED
                        || m_out_bid->state() == OrderState::PARTIALLY_FILLED) {
                        m_order_manager_p->cancel(m_out_bid);
                    }
                    m_last_best = q;
                    const auto & b = static_cast<const OrderBook &>(book);
                    auto it = b.bids().begin();
                    if (++it != b.bids().end()) {
                        m_last_best.bid = it->second.l2quote();
                    } else {
                        m_last_best.bid = FullL2Quote().bid;
                    }
                } else if (q.bid.price != m_last_best.bid.price) {
                    m_last_best = q;
                    if (m_out_bid->state() == OrderState::ACKNOWLEDGED
                        || m_out_bid->state() == OrderState::PARTIALLY_FILLED) {
                        m_order_manager_p->cancel(m_out_bid);
                    }
                }
            }
        } else {
            if (m_out_ask) {
                if (0 == q.ask.size) {
                    if (m_out_ask->state() == OrderState::ACKNOWLEDGED
                        || m_out_ask->state() == OrderState::PARTIALLY_FILLED) {
                        m_order_manager_p->cancel(m_out_ask);
                    }
                    m_last_best = q;
                    const auto & b = static_cast<const OrderBook &>(book);
                    auto it = b.asks().begin();
                    if (++it != b.asks().end()) {
                        m_last_best.ask = it->second.l2quote();
                    } else {
                        m_last_best.ask = FullL2Quote().ask;
                    }
                } else if (q.ask.price != m_last_best.ask.price) {
                    m_last_best = q;
                    if (m_out_ask->state() == OrderState::ACKNOWLEDGED
                        || m_out_ask->state() == OrderState::PARTIALLY_FILLED) {
                        m_order_manager_p->cancel(m_out_ask);
                    }
                }
            }
        }
    }

    void on_book_canceled(const L3CancelEvent &evt) override final {
        if (!m_mics[evt.m_book.mic().index()]) {
            return;
        }
        if (!m_in_univ[evt.m_book.symbol_index()]) {
            return;
        }

        FullL2Quote q = evt.m_book.best();
        if (q != m_last_best) {
            std::cerr << "TST,OBC," << evt.m_timestamp << "," << q << "," << evt << std::endl;
            check_for_cancel(evt.m_order.side, q, evt.m_book);
        }
    }

    void on_book_modified(const L3ModifyEvent &evt) override final {
        if (!m_mics[evt.m_book.mic().index()]) {
            return;
        }
        if (!m_in_univ[evt.m_book.symbol_index()]) {
            return;
        }

        FullL2Quote q = evt.m_book.best();
        if (q != m_last_best) {
            std::cerr << "TST,OBM," << evt.m_timestamp << "," << q << "," << evt << std::endl;
            check_for_cancel(evt.m_new_order.side, q, evt.m_book);
        }
    }

    void on_book_executed(const L3ExecutionEvent &evt) override final {
        if (!m_mics[evt.m_book.mic().index()]) {
            return;
        }
        if (!m_in_univ[evt.m_book.symbol_index()]) {
            return;
        }

        FullL2Quote q = evt.m_book.best();
        std::cerr << "TST,OBE," << evt.m_timestamp << "," << q << "," << evt << std::endl;
        if (q != m_last_best) {
            check_for_cancel(evt.m_order.side, q, evt.m_book);
        }
    }

    void moc_helper(Order *& order_p, Size sz, Side s) {
        i01::OE::Price p = 70.80;
        if (Side::BUY == s) {
            p = 70.80;
        }
        order_p = m_order_manager_p->create_order<OrderType>(m_inst, p, sz,
                                                             s,
                                                             TimeInForce::AUCTION_CLOSE,
                                                             i01::OE::OrderType::LIMIT, this);
        if (!m_order_manager_p->send(order_p, m_order_session_p)) {
            std::cerr << "TST,ERR,SEND_MOC," << *order_p << std::endl;
            order_p = nullptr;
        }
    }

    void l2_update_helper(Order *& order_p, const L2BookEvent &evt) {
        if (SymbolState::TradingState::TRADING != m_state.trading_state) {
            return;
        }

        if (!m_in_univ[evt.m_book.symbol_index()]) {
            return;
        }

        if (nullptr == order_p) {
            i01::OE::Price price = 0.01;
            Size size = 0;
            if (evt.m_is_buy) {
                price = to_double(m_last_best.bid.price);
                size = m_last_best.bid.size;
            } else {
                price = to_double(m_last_best.ask.price);
                size = m_last_best.ask.size;
            }

            order_p = m_order_manager_p->create_order<OrderType>(m_inst,
                                                                 price,
                                                                 size,
                                                                 evt.m_is_buy ? Side::BUY : Side::SELL,
                                                                 TimeInForce::DAY,
                                                                 i01::OE::OrderType::LIMIT, this);
            if (!m_order_manager_p->send(order_p, m_order_session_p)) {
                std::cerr << "TST,ERR,SEND_ON_NULL," << *order_p << std::endl;
                order_p = nullptr;
            }
        } else {
            auto ref_price = evt.m_is_buy ? m_last_best.bid.price : m_last_best.ask.price;
            if (to_fixed(order_p->price()) != ref_price
                && order_p->state() != OrderState::PENDING_CANCEL) {
                // cancel outstanding order
                m_order_manager_p->cancel(order_p);
            }
        }
    }

    void on_l2_update(const L2BookEvent &evt) override final {
        if (!m_mics[evt.m_book.mic().index()]) {
            return;
        }
        // just try and peg the inside quote...
        if (!evt.m_is_bbo) {
            return;
        }

        if (nullptr == m_moc_bid) {
            moc_helper(m_moc_bid, 100, Side::BUY);
        }
        if (nullptr == m_moc_ask) {
            moc_helper(m_moc_ask, 100, Side::SELL);
        }


        m_last_best = evt.m_book.best();
        // need to use the best quote b/c this could be an update with size ==0
        l2_update_helper(evt.m_is_buy ? m_out_bid : m_out_ask, evt);
        if (evt.m_is_bbo) {
            std::cerr << "TST,L2Q,"
                      << evt.m_timestamp << ","
                      << m_last_best << std::endl;
        }
    }

    void on_trade(const TradeEvent &evt) override final {
        if (!m_mics[evt.m_book.mic().index()]) {
            return;
        }
        if (!m_in_univ[evt.m_book.symbol_index()]) {
            return;
        }

        std::cerr << "TST,TRD," << evt << std::endl;
    }

    void on_end_of_data(const PacketEvent &evt) override final {
        if (!m_mics[evt.mic.index()]) {
            return;
        }
        if (m_events.size()) {
            auto e = m_events.begin();
            while (e != m_events.end() && evt.timestamp > e->first) {
                if (e->second.is_order) {
                    auto * op = m_order_manager_p->create_order<OrderType>(m_inst, e->second.price,
                                                                           e->second.size, e->second.side,
                                                                           e->second.tif,
                                                                           e->second.type, this);
                    if (op) {
                        if (m_order_manager_p->send(op, m_order_session_p)) {

                            add_event(e->first + Timestamp{1,0}, {false, 0, 0, Side::UNKNOWN, TimeInForce::UNKNOWN, i01::OE::OrderType::UNKNOWN, op});
                        } else {
                            std::cerr << "TST,ERR,EOD_SEND," << *op << std::endl;
                        }
                    }
                } else {
                    // cancel
                    m_order_manager_p->cancel(e->second.order_p);
                }
                e = m_events.erase(e);
            }
        }
    }

    void on_order_acknowledged(const Order *o) override final {
        std::cerr << "TST,ACK," << o->last_response_time() << "," << m_ts << "," << *o << std::endl;
    }

    void on_order_fill(const Order *o, const i01::OE::Size s, const i01::OE::Price p, const i01::OE::Price fee) override final {
        std::cerr << "TST,FILL,"
                  << o->last_response_time() << ","
                  << m_ts << ","
                  << s << ","
                  << p << ","
                  << fee << ","
                  << (o->side() == Side::BUY ? -(s*p + fee) : (s*p-fee)) << ","
                  << *o
                  << std::endl;

        if (o->state() == OrderState::FILLED) {
            if (Side::BUY == o->side()) {
                assert(o->localID() == m_out_bid->localID() || o->localID() == m_moc_bid->localID());
                m_out_bid = nullptr;
            } else {
                assert(o->localID() == m_out_ask->localID() || o->localID() == m_moc_ask->localID());
                m_out_ask = nullptr;
            }
        }
    }


    void on_order_cancel(const Order *o, const Size s) override final {
        std::cerr << "TST,CXL,"
                  << o->last_response_time() << ","
                  << m_ts << ","
                  << s << ","
                  << *o << std::endl;

        if (m_out_ask && o->localID() == m_out_ask->localID()) {
            if (m_last_best.ask.price != BookBase::EMPTY_ASK_PRICE) {
                m_out_ask = m_order_manager_p->create_order<OrderType>(m_inst, to_double(m_last_best.ask.price), m_last_best.ask.size, Side::SELL, TimeInForce::DAY, i01::OE::OrderType::LIMIT, this);
                if (!m_order_manager_p->send(m_out_ask, m_order_session_p)) {
                    std::cerr << "TST,ERR,CXL_SEND," << *m_out_ask <<std::endl;
                }
            } else {
                m_out_ask = nullptr;
            }
        } else if (m_out_bid && o->localID() == m_out_bid->localID()) {
            if (m_last_best.bid.price != 0) {
                m_out_bid = m_order_manager_p->create_order<OrderType>(m_inst, to_double(m_last_best.bid.price), m_last_best.bid.size, Side::BUY, TimeInForce::DAY, i01::OE::OrderType::LIMIT, this);
                if (!m_order_manager_p->send(m_out_bid, m_order_session_p)) {
                    std::cerr << "TST,ERR,CXL_SEND," << *m_out_bid << std::endl;
                }
            } else {
                m_out_bid = nullptr;
            }
        } else {
            assert(true);
        }
    }

    void on_order_cancel_rejected(const Order *o) override final {
        std::cerr << "TST,CXLREJ,"
                  << o->last_response_time() << ","
                  << m_ts << ","
                  << *o
                  << std::endl;
    }

private:
    OrderManager *m_order_manager_p;
    OrderSession *m_order_session_p;
    EquityInstrument* m_inst;
    EventContainer m_events;
    SymbolState m_state;
    Timestamp m_ts;
    std::array<bool, static_cast<size_t>(i01::core::MICEnum::NUM_MIC)> m_mics;
    FullL2Quote m_last_best;
    Order * m_out_ask;
    Order * m_out_bid;
    Order * m_moc_bid;
    Order * m_moc_ask;
    std::array<bool,i01::MD::NUM_SYMBOL_INDEX> m_in_univ;
};


using XNYSTestStrategy = TestStrategy<i01::OE::NYSEOrder, (int)i01::core::MICEnum::XNYS>;
using BATSTestStrategy = TestStrategy<i01::OE::BATSOrder, (int)i01::core::MICEnum::BATS>;
using XNASTestStrategy = TestStrategy<i01::OE::NASDAQOrder, (int)i01::core::MICEnum::XNAS>;
using ARCXTestStrategy = TestStrategy<i01::OE::ARCAOrder, (int)i01::core::MICEnum::ARCX>;


class NoopRawListener : public RawListener<NoopRawListener> {
public:
    template<typename...Args>
    void on_raw_msg(const Timestamp &ts, Args...args) {}
};


template<typename StrategyType, typename SimType = i01::OE::L2SimSession>
class SimTestApp :
    public Application
{
public:
    using Application::Application;

private:
    using PDPBookMux = i01::MD::NYSE::PDP::BookMux<SimType, StrategyType>;
    using XDPBookMux = ARCA::XDP::BookMux<SimType, StrategyType>;
    using BATSBookMux = i01::MD::BATS::PITCH2::BookMux<SimType, StrategyType>;
    using XNASBookMux = i01::MD::NASDAQ::ITCH50::BookMux<SimType, StrategyType>;
    using ARCXBookMux = i01::MD::ARCA::XDP::BookMux<SimType, StrategyType>;

    using XNYSDecoderMux = DecoderMux::DecoderMux<PDPBookMux, XDPBookMux, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener>;
    using BATSDecoderMux = DecoderMux::DecoderMux<NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, BATSBookMux, NoopRawListener, NoopRawListener>;
    using XNASDecoderMux = DecoderMux::DecoderMux<NoopRawListener, NoopRawListener, XNASBookMux, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener>;

    using ARCXDecoderMux = DecoderMux::DecoderMux<NoopRawListener, NoopRawListener, NoopRawListener,ARCXBookMux, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener, NoopRawListener>;

    // using udpreader_type = i01::net::pcap::UDPReader<XNYSDecoderMux>;
    using udpreader_type = i01::net::pcap::UDPReader<BATSDecoderMux>;
    // using udpreader_type = i01::net::pcap::UDPReader<XNASDecoderMux>;
    // using udpreader_type = i01::net::pcap::UDPReader<ARCXDecoderMux>;

    struct SymbolEntry {
        std::string m_cta_symbol;
        std::uint32_t m_fdo_stock_id;
        std::string m_id_string;
    };

    using symbols_container_type = std::vector<SymbolEntry>;

public:

    SimTestApp();
    SimTestApp(const std::string& name_, int argc, const char *argv[]);
    ~SimTestApp() = default;

    bool init(int argc, const char *argv[]) override final;
    // void init(const Config::storage_type &cfg);
    virtual int run() override final;
    // virtual void on_config_update(const Config::storage_type &, const Config::storage_type &cfg) noexcept override final;

private:
    void process_file_(const std::string &filename, BATSDecoderMux &decoder_mux);

    // const std::string & id_str_(std::uint32_t esi) const;


private:
    udpreader_type *m_reader;

    std::vector<std::string> m_pcap_filenames;
    std::string m_current_file;

    OrderManager m_order_manager;
    SimType *m_simulator;
    StrategyType *m_strategy;

    int m_ack_latency;
    int m_cxl_latency;
};

template<typename SIMT, typename ST>
SimTestApp<SIMT,ST>::SimTestApp()
{
}

template<typename SIMT, typename ST>
bool SimTestApp<SIMT,ST>::init(int argc, const char *argv[]) {
    return true;
}

template<typename SIMT, typename ST>
SimTestApp<SIMT,ST>::SimTestApp(const std::string& name_, int argc, const char *argv[]) // :

    // m_simulator(&m_order_manager, name_),
    // m_strategy(&m_order_manager, &m_simulator, EquityInstrument("MSFT", 10000, 10000, 1e7, -1, 1e5, 1e7, 100, 100000))
{
    // specifying pcap-file as required() causes an uncaught exception to be thrown
    options_description().add_options()
        ("ack-lat,a", po::value<int>(&m_ack_latency)->default_value(200000), "ACK latency ns")
        ("cxl-lat,c", po::value<int>(&m_cxl_latency)->default_value(200000), "CXL latency ns")
        ("pcap-file", po::value<std::vector<std::string> >(&m_pcap_filenames), "pcap file");
    positional_options_description().add("pcap-file",-1);

    init(argc, argv);
    Application::init(argc,argv);

    auto osp = m_order_manager.add_session(name_, "L2SimSession");
    m_simulator = static_cast<ST *>(osp.first.get());
    m_strategy = new SIMT(&m_order_manager, m_simulator);
}

template<typename SIMT, typename ST>
int SimTestApp<SIMT,ST>::run()
{
    // create test strategy here

    if (m_pcap_filenames.size() == 0) {
        throw std::runtime_error("Need to give pcap file");
    }

    NoopRawListener noop_raw_listener;
    // PDPBookMux pdp_book_mux(&m_simulator, &m_strategy);
    // XDPBookMux xdp_book_mux(&m_simulator, &m_strategy, MICEnum::XNYS, {"TRD"});
    BATSBookMux bats_book_mux(m_simulator, m_strategy, MICEnum::BATS);
    // XNASBookMux xnas_book_mux(&m_simulator, &m_strategy, MICEnum::XNAS);
    // ARCXBookMux arcx_book_mux(&m_simulator, &m_strategy, MICEnum::ARCX, {"INTXDP"});

    // decoder_mux_type m_decoder_mux;
    // XNYSDecoderMux decoder_mux(&pdp_book_mux,
    //                            &xdp_book_mux,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener);

    BATSDecoderMux decoder_mux(&noop_raw_listener,
                               &noop_raw_listener,
                               &noop_raw_listener,
                               &noop_raw_listener,
                               &noop_raw_listener,
                               &noop_raw_listener,
                               &noop_raw_listener,
                               &bats_book_mux,
                               &noop_raw_listener,
                               &noop_raw_listener);

    // XNASDecoderMux decoder_mux(&noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &xnas_book_mux,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener);

    // ARCXDecoderMux decoder_mux(&noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &arcx_book_mux,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener,
    //                            &noop_raw_listener);


    std::cout << "ack lat: " << m_ack_latency << " cxl lat: " << m_cxl_latency << std::endl;
    m_simulator->ack_latency(Timestamp{0,m_ack_latency});
    m_simulator->cxl_latency(Timestamp{0,m_cxl_latency});

    // m_strategy.add_event({1415736056,0}, {true, 70.54, 100, Side::SELL, TimeInForce::DAY, i01::OE::OrderType::LIMIT,nullptr});
    // m_strategy.add_event({1415736000, 453000000}, {true, 70.54, 100, Side::BUY, TimeInForce::DAY, i01::OE::OrderType::LIMIT,nullptr});

    i01::net::PcapFileMux<BATSDecoderMux> pcapmux(std::set<std::string>(m_pcap_filenames.begin(), m_pcap_filenames.end()), &decoder_mux);
    // for (std::size_t i = 0; i < m_pcap_filenames.size(); ++i) {
    //     process_file_(m_pcap_filenames[i], decoder_mux);
    // }

    if (pcapmux.packets_enqueued()) {
        pcapmux.read_packets();
    }

    // for (auto & e : m_order_manager.universe()) {
    //     std::cout << "PNL," << e.data()->position().realized_gross_pnl() << "," << e.data()->position().realized_net_pnl() << std::endl;
    // }

    return 0;
}

template<typename SIMT, typename ST>
void SimTestApp<SIMT,ST>::process_file_(const std::string &filename, BATSDecoderMux &decoder_mux)
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
        // SimTestApp<XNYSSimulator, XNYSTestStrategy> app(argc, argv);
        SimTestApp<BATSTestStrategy> app("OESimTestSession2", argc, argv);

        return app.run();
    } catch (const std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Caught exception" << std::endl;
    }
    return 1;
}
