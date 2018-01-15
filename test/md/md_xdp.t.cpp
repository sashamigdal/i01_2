#include <gtest/gtest.h>

#include <i01_net/Pcap.hpp>

#include <i01_md/DiagnosticListener.hpp>
#include <i01_md/ARCA/XDP/Decoder.hpp>
#include <i01_md/ARCA/XDP/BookMux.hpp>

#include "md_decoder_util.hpp"


template<>
struct HeaderWrapper<i01::MD::ARCA::XDP::Messages::PacketHeader>
{

    HeaderWrapper(const i01::MD::ARCA::XDP::Messages::PacketHeader &mh) : m_header(mh) {}

    HeaderWrapper(std::uint32_t seqnum, const i01::core::Timestamp &ts) {
        m_header.seqnum = seqnum;
        m_header.delivery_flag = i01::MD::ARCA::XDP::Types::DeliveryFlag::ORIGINAL;
        m_header.num_msgs = 1;
        m_header.send_time = (std::uint32_t) ts.tv_sec;
        m_header.send_time_ns = (std::uint32_t) ts.tv_nsec;
    }
    void set_length(std::uint16_t len) {
        // dont include msg_size field size
        // get arround narrowing conversion error with this sizeof -> std::uint16_t
        m_header.pkt_size = len;
    }
    i01::MD::ARCA::XDP::Messages::PacketHeader m_header;
};


TEST(md_decoder, md_arca_xdp_decoder)
{
    using namespace i01::MD::ARCA::XDP;
    using namespace i01::core;
    using namespace i01::net;
    namespace p = i01::net::pcap;

    class MyDiagnosticListener : public i01::MD::DiagnosticListener {
    public:
        void on_gap_detected(const i01::core::Timestamp &ts, std::uint32_t addr, std::uint16_t port, std::uint8_t unit, std::uint64_t expected, std::uint64_t recv, const i01::core::Timestamp &last_ts) {
            // std::cout << "GAP " << ts << " " << ip_addr_to_str({addr,port}) << " " << (int) unit << " " << expected << " " << recv << std::endl;
            EXPECT_EQ(expected ==1 && recv == 1941236, true) << "Gap sequence wrong.";
        }
    };

    MyDiagnosticListener mdl;
    Decoder<MyDiagnosticListener> feed(&mdl, MIC::Enum::ARCX);
    p::UDPReader<Decoder<MyDiagnosticListener> > reader(STRINGIFY(I01_DATA) "/mdsfti.20140822.090000.arcaxdp_chan1_10k.pcap-ns", &feed);

    for (const auto &s : {"224.0.59.76:11076", "224.0.59.204:11204" }) {
        feed.add_address_to_stream("ARCX:unit1", to64(addr_str_to_ip_addr(s)));
    }

    reader.read_packets();

    // std::cout << mdl.message_summary() << std::endl;

    EXPECT_EQ(mdl.num_messages(), 15581 + 5000 + 5000) << "Feed num msgs handled wrong.";

    for (const auto &s : feed.streams()) {
        if (s.pkt_count()) {
            // std::cout << s.name() << " " << s.pkt_count() << " " << s.msg_count() << " " << s.expect_seqnum() << " " << s.time_ref() << std::endl;
            EXPECT_EQ(s.name(), "ARCX:unit1") << "stream name wrong.";
            EXPECT_EQ(s.pkt_count(), 5000) << "stream num pkts seen wrong.";
            EXPECT_EQ(s.msg_count(), 15580) << "Stream num messages seen wrong.";
            EXPECT_EQ(s.expect_seqnum(), 1956816) << "Stream seqnum wrong.";
            EXPECT_EQ(s.time_ref(), 1408714198) << "Stream time ref wrong.";
        }
    }
    const MyDiagnosticListener::msgtype_count_map_type & mdl_msgs = mdl.msg_counts();
    std::size_t sum =0 ;
    for (auto m : mdl_msgs) {
        sum += m.second;
    }
    // std::cout << "sum: " << sum << " " << feed.num_messages_handled() << " " << std::endl;
    EXPECT_EQ(sum, 15581 + 5000 + 5000) << "Sum messages handled wrong.";
}

TEST(md_decoder, md_arca_xdp_decoder2)
{
    using namespace i01::MD::ARCA::XDP;
    using DiagnosticListener = i01::MD::DiagnosticListener;
    using namespace i01::net;

    DiagnosticListener dl;
    Decoder<DiagnosticListener> feed(&dl, i01::core::MICEnum::ARCX);
    i01::net::pcap::UDPReader<Decoder<DiagnosticListener> > reader(STRINGIFY(I01_DATA) "/mdsfti.20140822.130000.1408726800.arcaxdp.chan2.trd.first1k.pcap-ns", &feed);

    feed.init_streams({"ABXDP:AB_2", "TRD:TRD"});

    for (const auto & a : {"224.0.59.77:11077", "224.0.59.205:11205"}) {
        feed.add_address_to_stream("ABXDP:AB_2", to64(addr_str_to_ip_addr(a)));
    }

    for (const auto &a : {"224.0.59.106:11106", "224.0.59.234:11234"}) {
        feed.add_address_to_stream("TRD:TRD", to64(addr_str_to_ip_addr(a)));
    }

    reader.read_packets();

    const DiagnosticListener::msgtype_count_map_type & msgs = dl.msg_counts();
    // std::cout << dl.message_summary() << std::endl;
    std::vector<std::pair<int,int> > correct_counts = { {2, 310}, {100, 882}, {101, 9}, {102, 931}, {103, 32}, {220, 92}, {10000, 2}, {10003, 942}};
    for (const auto &p : correct_counts) {
        auto it = msgs.find(p.first);
        EXPECT_EQ(it != msgs.end() && it->second == (unsigned)p.second,true);
    }
}

TEST(md_decoder, md_arca_xdp_bookmux)
{
    using namespace i01::MD::ARCA::XDP;
    using namespace i01::net;
    using namespace i01::core;
    using namespace i01::MD;
    static const int EPOCH_TIME = 1414432273;

    class XDPListener : public i01::MD::NoopBookMuxListener {
    public:
        void on_end_of_data(const PacketEvent &evt) override final {
            EXPECT_EQ(evt.mic.market(), MIC::Enum::ARCX);
            m_pkt_count++;
        }

        void on_gap(const GapEvent &evt) override final {
            EXPECT_EQ(evt.m_expected_seqnum, 9);
            EXPECT_EQ(evt.m_received_seqnum, 10);
            m_gap_detected = true;
        }

        void on_book_added(const i01::MD::L3AddEvent &ae) override final {
            auto o = ae.m_order;
            const auto & book = ae.m_book;
            using Order = i01::MD::OrderBook::Order;
            switch (m_count) {
            case 0:
                {
                    // first pkt
                    // ADD BID
                    EXPECT_EQ(351100,o.price);
                    EXPECT_EQ(o.size,500);
                    EXPECT_EQ(Order::Side::BUY, o.side);
                    EXPECT_EQ(Order::TimeInForce::DAY, o.tif);
                    EXPECT_EQ(book.symbol_index(),1);
                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb) == o.price
                              && std::get<1>(bb) == o.size
                              && std::get<2>(bb) == 1, true);
                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba) == std::numeric_limits<Order::Price>::max()
                              && std::get<1>(ba) == 0
                              && std::get<2>(ba) == 0, true);
                }
                break;
            case 1:
                {
                    // second pkt
                    // ADD SELL
                    EXPECT_EQ(354700,o.price);
                    EXPECT_EQ(100, o.size);
                    EXPECT_EQ(Order::Side::SELL,o.side);
                    EXPECT_EQ(Order::TimeInForce::GTC, o.tif);
                    EXPECT_EQ(book.symbol_index(),1);
                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),351100);
                    EXPECT_EQ(std::get<1>(bb),500);
                    EXPECT_EQ(std::get<2>(bb),1);
                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba),o.price);
                    EXPECT_EQ(std::get<1>(ba),o.size);
                    EXPECT_EQ(std::get<2>(ba),1);
                }
                break;
            case 12:
                EXPECT_EQ(std::get<0>(book.best_ask()), 355000);
                break;
            case 13:
                EXPECT_EQ(std::get<0>(book.best_ask()), 354800);
                break;
            case 14:
                EXPECT_EQ(std::get<0>(book.best_ask()), 354700);
                break;
            case 15:
                {
                    EXPECT_EQ(std::get<0>(book.best_ask()), 355000);
                    EXPECT_EQ(std::get<0>(book.best_bid()), 354900);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_book_added: packet " << m_count << " not checked.";
                break;
            }
            m_count++;

        }
        void on_book_modified(const L3ModifyEvent &evt) override final {
            switch(m_count) {
            case 2:
                {
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_new_order.size, 200);
                    EXPECT_EQ(evt.m_old_size, 100);
                    EXPECT_EQ(evt.m_new_order.price,352000 );
                    EXPECT_EQ(evt.m_old_price,354700);
                    EXPECT_EQ(evt.m_old_refnum,evt.m_new_order.refnum);

                    auto ba = evt.m_book.best_ask();
                    EXPECT_EQ(std::get<0>(ba), 352000);
                    EXPECT_EQ(std::get<1>(ba),200);
                    EXPECT_EQ(std::get<2>(ba),1);
                }
                break;
            case 5:
                {
                    // PIGGY BACK ON EXECUTION PKT 6
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_new_order.size, 300);
                    EXPECT_EQ(evt.m_new_order.price, 351100);
                    EXPECT_EQ(evt.m_old_size, 500);
                    EXPECT_EQ(evt.m_old_price, evt.m_new_order.price);
                    EXPECT_EQ(evt.m_old_refnum, evt.m_new_order.refnum);

                    auto bb = evt.m_book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), 351100);
                    EXPECT_EQ(std::get<1>(bb), 300);
                    EXPECT_EQ(std::get<2>(bb), 1);
                }
                break;
            case 7:
                {
                    // PIGGY BACK ON EXECUTION PKT 8
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_new_order.size, 200);
                    EXPECT_EQ(evt.m_new_order.price, 351100);
                    EXPECT_EQ(evt.m_old_size, 300);
                    EXPECT_EQ(evt.m_old_price, evt.m_new_order.price);
                    EXPECT_EQ(evt.m_old_refnum, evt.m_new_order.refnum);

                    auto bb = evt.m_book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), 351100);
                    EXPECT_EQ(std::get<1>(bb), 200);
                    EXPECT_EQ(std::get<2>(bb), 1);
                }
                break;
            case 16:
                {
                    EXPECT_EQ(std::get<0>(evt.m_book.best_ask()), 354900);
                    EXPECT_EQ(std::get<0>(evt.m_book.best_bid()), BookBase::EMPTY_BID_PRICE);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_book_modified: packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_book_canceled(const L3CancelEvent &evt) override final {
            switch(m_count) {
            case 3:
                {
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_order.refnum, 20503);
                    EXPECT_EQ(evt.m_order.price, 352000);
                    EXPECT_EQ(evt.m_order.size, 0);
                    EXPECT_EQ(evt.m_old_size, 200);
                    auto ba = evt.m_book.best_ask();
                    EXPECT_EQ(std::get<0>(ba), 352000);
                    EXPECT_EQ(std::get<1>(ba),0);
                    EXPECT_EQ(std::get<2>(ba),1);

                }
                break;
            case 9:
                {
                    // DELETE THAT FOLLOWS EXECUTION IN PKT 11
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_order.refnum,1488);
                    EXPECT_EQ(evt.m_order.size,0);
                    EXPECT_EQ(evt.m_old_size, 200);
                    EXPECT_EQ(evt.m_order.price,351100);
                    auto bb= evt.m_book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), 351100);
                    EXPECT_EQ(std::get<1>(bb), 0);
                    EXPECT_EQ(std::get<2>(bb), 1);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_book_canceled: packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_book_executed(const L3ExecutionEvent &evt) override final {
            switch (m_count) {
            case 4:
                {
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_exchange_timestamp.tv_sec, EPOCH_TIME);
                    EXPECT_EQ(evt.m_exchange_timestamp.tv_nsec, 790000000);
                    EXPECT_EQ(evt.m_order.refnum, 1488);
                    EXPECT_EQ(evt.m_order.size, 500);
                    EXPECT_EQ(evt.m_order.price, 351100);
                    EXPECT_EQ(evt.m_trade_id,1);
                    EXPECT_EQ(evt.m_exec_price, evt.m_order.price);
                    EXPECT_EQ(evt.m_exec_size, 200);

                    // this execution does not modify the book
                    auto bb = evt.m_book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), evt.m_exec_price);
                    EXPECT_EQ(std::get<1>(bb), 500);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;
            case 6:
                {
                    // EXECUTION LEG PKT 8
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_exchange_timestamp.tv_sec, EPOCH_TIME);
                    EXPECT_EQ(evt.m_exchange_timestamp.tv_nsec, 791000000);
                    EXPECT_EQ(evt.m_order.refnum, 1488);
                    EXPECT_EQ(evt.m_order.size, 300);
                    EXPECT_EQ(evt.m_order.price, 351100);
                    EXPECT_EQ(evt.m_trade_id,2);
                    EXPECT_EQ(evt.m_exec_size, 100);
                    EXPECT_EQ(evt.m_exec_price,evt.m_order.price);
                    // this execution does not modify the book
                    auto bb = evt.m_book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), evt.m_exec_price);
                    EXPECT_EQ(std::get<1>(bb), 300);
                    EXPECT_EQ(std::get<2>(bb), 1);

                }
                break;
            case 8:
                {
                    // EXECUTION FROM PKT 10
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_exchange_timestamp.tv_sec, EPOCH_TIME);
                    EXPECT_EQ(evt.m_exchange_timestamp.tv_nsec, 792000000);
                    EXPECT_EQ(evt.m_order.refnum, 1488);
                    EXPECT_EQ(evt.m_order.size, 200);
                    EXPECT_EQ(evt.m_order.price, 351100);
                    EXPECT_EQ(evt.m_trade_id, 3);
                    EXPECT_EQ(evt.m_exec_price, 351100);
                    EXPECT_EQ(evt.m_exec_size, 200);
                }
                break;

            default:
                ASSERT_EQ(true,false) << "on_book_executed: packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }
        void on_trade(const TradeEvent &evt) override final {
            switch (m_count) {
            case 10:
                {
                    // TRADE PKT 11
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_exchange_timestamp.tv_sec, EPOCH_TIME);
                    EXPECT_EQ(evt.m_exchange_timestamp.tv_nsec, 793000000);
                    EXPECT_EQ(evt.m_trade_id, 4);
                    EXPECT_EQ(evt.m_price, 351100);
                    EXPECT_EQ(evt.m_size, 100);
                    EXPECT_EQ(evt.m_passive_side, TradeEvent::PassiveSide::BUY);
                    EXPECT_EQ(evt.m_msg!=nullptr,true);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_trade: packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_trading_status_update(const TradingStatusEvent &evt) override final {
            switch(m_count) {
            case 11:
                {
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_event, TradingStatusEvent::Event::REGULAR_MKT_SESSION);
                }
                break;
            default:
                break;
            }
            m_count++;
        }

        void on_book_crossed(const L3CrossedEvent& evt) override final {
            std::cerr << "XDP CROSSED " << m_count << " " << evt << std::endl;
            switch (m_count) {
            case 15:
            case 16:
                break;
            default:
                ASSERT_TRUE(false) << "on_book_crossed: callback " << m_count << " not checked.";
            }
            // don't increcemtn m_count since another callback will get it...
        }

        int m_count = 0;
        int m_pkt_count = 0;
        bool m_gap_detected = false;
    };
    XDPListener xdp;

    using XDPBookMux = BookMux<XDPListener>;
    XDPBookMux bm(&xdp, MICEnum::ARCX, {"ABXDP:AB_1", "TRD:TRD"});

    Decoder<XDPBookMux> feed(&bm, MICEnum::ARCX);

    feed.init_streams({"ABXDP:AB_2", "TRD:TRD"});
    auto addr1 = addr_str_to_ip_addr("224.0.59.77:11077");
    auto src_addr = addr_str_to_ip_addr("127.0.0.1:5555");

    for (const auto & a : {"224.0.59.77:11077", "224.0.59.205:11205"}) {
        feed.add_address_to_stream("ABXDP:AB_2", to64(addr_str_to_ip_addr(a)));
    }

    for (const auto &a : {"224.0.59.106:11106", "224.0.59.234:11234"}) {
        feed.add_address_to_stream("TRD:TRD", to64(addr_str_to_ip_addr(a)));
    }

    bm.load_symbol_mapping(STRINGIFY(I01_DATA) "/ARCASymbolMapping_INTC.xml");
    bm.create_book_for_symbol(1, "INTC");

    using HW = HeaderWrapper<Messages::PacketHeader>;
    auto now = Timestamp::now();

    // source time for INTC
    auto pkt = make(HW(1,now), Messages::SourceTimeReference{{16, Types::MsgType::TIME_REFERENCE}, 2074, 1,EPOCH_TIME});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // m_count == 0
    // add order for INTC
    Messages::AddOrder ao{ {31, Types::MsgType::ADD_ORDER}, 1064000, 2074, 2, 1488, 351100, 500, Types::Side::BUY, Types::OrderIDGTCIndicator::DAY, {7}};

    pkt = make(HW(2,now), ao);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // m_count == 1
    ao = Messages::AddOrder{ {31, Types::MsgType::ADD_ORDER}, 774518000, 2074, 3, 20503, 354700, 100, Types::Side::SELL, Types::OrderIDGTCIndicator::GTC, {7}};

    pkt = make(HW(3,now), ao);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // m_count == 2
    pkt = make(HW(4,now), Messages::ModifyOrder{{31, Types::MsgType::MODIFY_ORDER}, 775000000, 2074, 4, 20503, 352000, 200, Types::Side::SELL, Types::OrderIDGTCIndicator::GTC, Types::ReasonCode::CHANGE});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // m_count == 3
    // generate a symbol gap
    pkt = make(HW(5,now), Messages::DeleteOrder{{23, Types::MsgType::DELETE_ORDER}, 780000000, 2074, 5, 20503, Types::Side::SELL, Types::OrderIDGTCIndicator::GTC, Types::ReasonCode::USER_CANCEL});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // m_count == 4 & 5
    // now generate executes
    pkt = make(HW(6,now), Messages::Execution{{34, Types::MsgType::EXECUTION}, 790000000, 2074, 6, 1488, 351100, 200, Types::OrderIDGTCIndicator::DAY, Types::ReasonCode::NONE, 1},
               Messages::ModifyOrder{{31, Types::MsgType::MODIFY_ORDER}, 790000000, 2074, 7, 1488, 351100, 300, Types::Side::BUY, Types::OrderIDGTCIndicator::DAY, Types::ReasonCode::MODIFY_FILL});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // m_count == 6 & 7
    // this is not yet implemented ... modify fill on execution
    pkt = make(HW(8,now), Messages::Execution{{34, Types::MsgType::EXECUTION}, 791000000, 2074, 8, 1488, 351100, 100, Types::OrderIDGTCIndicator::DAY, Types::ReasonCode::MODIFY_FILL, 2});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // m_count == 8 & 9
    // generate GAP
    // not yet implemented .. delete fill on execution with Symbol gap
    pkt = make(HW(10,now), Messages::Execution{{34, Types::MsgType::EXECUTION}, 792000000, 2074, 9, 1488, 351100, 200, Types::OrderIDGTCIndicator::DAY, Types::ReasonCode::DELETE_FILLED, 3});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    EXPECT_EQ(xdp.m_gap_detected, true);

    // m_count == 10
    pkt = make(HW(11,now), Messages::Trade{{54, Types::MsgType::TRADE}, EPOCH_TIME, 793000000, 2074, 10, 4, 351100, 100, Types::TradeCondition1::REGULAR_SALE, Types::TradeCondition2::ISO, Types::TradeCondition3::NOT_APPLICABLE, Types::TradeCondition4::NOT_APPLICABLE, Types::TradeThroughExempt::NOT_APPLICABLE, Types::LiquidityIndicatorFlag::BUY_SIDE, 351200, 200, 351100, 500});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    pkt = make(HW(12,now), Messages::TradingSessionChange{{21, Types::MsgType::TRADING_SESSION_CHANGE}, EPOCH_TIME, 794000000, 2074, 11, (std::uint8_t) Types::TradingSession::NATIONAL});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // test book uncrossing
    pkt = make(HW(13,now), Messages::AddOrder{ {31, Types::MsgType::ADD_ORDER}, 794000000, 2074, 11, 20504, 355000, 100, Types::Side::SELL, Types::OrderIDGTCIndicator::GTC, {7}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    pkt = make(HW(14,now), Messages::AddOrder{ {31, Types::MsgType::ADD_ORDER}, 794000001, 2074, 12, 20505, 354800, 100, Types::Side::SELL, Types::OrderIDGTCIndicator::GTC, {7}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    pkt = make(HW(15,now), Messages::AddOrder{ {31, Types::MsgType::ADD_ORDER}, 794000002, 2074, 13, 20506, 354700, 100, Types::Side::SELL, Types::OrderIDGTCIndicator::GTC, {7}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    pkt = make(HW(16,now), Messages::AddOrder{ {31, Types::MsgType::ADD_ORDER}, 794000003, 2074, 14, 20507, 354900, 100, Types::Side::BUY, Types::OrderIDGTCIndicator::GTC, {7}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    pkt = make(HW(17,now), Messages::ModifyOrder{{31, Types::MsgType::MODIFY_ORDER}, 794000003, 2074, 15, 20504, 354900, 200, Types::Side::SELL, Types::OrderIDGTCIndicator::GTC, Types::ReasonCode::CHANGE});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);


    EXPECT_EQ(xdp.m_count, 17);
    EXPECT_EQ(xdp.m_pkt_count,15);
}
