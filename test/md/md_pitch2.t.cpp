#include <gtest/gtest.h>

#include <i01_net/Pcap.hpp>

#include <i01_md/DiagnosticListener.hpp>
#include <i01_md/BATS/PITCH2/BookMux.hpp>
#include <i01_md/BATS/PITCH2/Decoder.hpp>

#include "md_decoder_util.hpp"

template<>
struct HeaderWrapper<i01::MD::BATS::PITCH2::Messages::SequencedUnitHeader>
{

    HeaderWrapper(const i01::MD::BATS::PITCH2::Messages::SequencedUnitHeader &mh) : m_header(mh) {}

    HeaderWrapper(std::uint32_t seqnum, std::uint8_t msg_count = 1, std::uint8_t unit = 1) {
        m_header.count = msg_count;
        m_header.sequence = seqnum;
        m_header.unit = unit;
    }
    void set_length(std::uint16_t len) {
        // dont include msg_size field size
        // get arround narrowing conversion error with this sizeof -> std::uint16_t
        m_header.length = len;
    }
    i01::MD::BATS::PITCH2::Messages::SequencedUnitHeader m_header;
};

TEST(md_decoder, md_bats_pitch2_diagnostic)
{
    using namespace i01::MD::BATS::PITCH2;
    using namespace i01::net::pcap;
    using namespace i01::core;

    using DiagnosticListener = i01::MD::DiagnosticListener;

    class MyDiagnosticListener : public DiagnosticListener {
    public:
        void on_gap_detected(const Timestamp &ts, std::uint32_t addr, std::uint16_t port, std::uint8_t unit, std::uint64_t expected, std::uint64_t recv, const Timestamp &last_ts) {
            EXPECT_EQ(unit,1) << "Gap unit mismatch";
            EXPECT_EQ(expected == 1 && recv == 2976858, true) << "Gap detected seq mismtach";
        }
    };

    MyDiagnosticListener dl;
    Decoder<MyDiagnosticListener> feed(&dl, MIC::Enum::BATS, "BATS");
    UDPReader<Decoder<MyDiagnosticListener> > reader(STRINGIFY(I01_DATA) "/BZX_UNIT_1_20140724_1500.pcap-ns", &feed);

    reader.read_packets();

    // TODO FIXME XXX: unused: const auto &msgs = dl.msg_counts();

    for (const auto &s : feed.streams()) {
        if (s.pkt_count()) {
            EXPECT_EQ(s.name(), "BATS:unit1") << "Unexpected unit saw packets.";
            EXPECT_EQ(s.pkt_count(), 37940) << "Unit packet count mismatch";
            EXPECT_EQ(s.msg_count(), 42837) << "Unit message count mismatch";
            EXPECT_EQ(s.expect_seqnum(),3019695) << "Unit expect seqnum mismatch";
            EXPECT_EQ(s.time_ref().u32, 55641) << "Unit time ref mismatch";
        }
    }
    std::cout << dl.message_summary() <<std::endl;
    // should be number of feed messages plus an additional gap message
    EXPECT_EQ(dl.num_messages(), 42838 + 37940 + 37940) << "Feed number of messages handled.";


    const MyDiagnosticListener::msgtype_count_map_type & mdl_msgs = dl.msg_counts();
    std::vector<std::pair<int, int> > correct_counts = { {0x20, 388}, {0x21, 3}, {0x22, 19153}, {0x23, 1425}, {0x24, 71}, {0x26, 507}, {0x28, 2960}, {0x29, 17869}, {0x2a, 129}, {0x2b,326}, {0x2f, 6}, {10000,1}, {10003, 37940}, {10004, 37940}};
    for (const auto &p : correct_counts) {
        auto it = mdl_msgs.find(p.first);
        EXPECT_EQ(it != mdl_msgs.end() && it->second == (unsigned) p.second,true);
    }

    auto it = mdl_msgs.find((int)i01::MD::DecoderMsgType::GAP_MSG);
    EXPECT_EQ(it != mdl_msgs.end() && it->second == 1, true) << "Gap message count mismatch";
    it = mdl_msgs.find((int)Messages::MessageType::TIME);
    EXPECT_EQ(it != mdl_msgs.end() && it->second == 388, true) << "Time message count mismatch";
    std::size_t sum =0 ;
    for (auto m : mdl_msgs) {
        sum += m.second;
    }

    // this should be feed.num_messages_handled()+1 for the gap
    EXPECT_EQ(sum, 42838 + 37940 + 37940) << "listener number of messages sum mismatch";
}


TEST(md_decoder, md_bats_pitch2_bespoke_data)
{
    using namespace i01::MD::BATS::PITCH2;
    using namespace i01::net;
    using namespace i01::core;
    using namespace i01::MD;

    class PITCHListener : public i01::MD::NoopBookMuxListener {
    public:
        void on_end_of_data(const PacketEvent &evt) override final {
            EXPECT_EQ(evt.mic.market(), MIC::Enum::BATS);
            m_pkt_count++;
        }

        void on_gap(const GapEvent &ge) override final {
            // auto ad1 = addr_str_to_ip_addr("224.0.62.2:30001");
            // EXPECT_EQ(1 == unit && ad1.first == addr && ad1.second == port,true);
            EXPECT_EQ(ge.m_expected_seqnum,2);
            EXPECT_EQ(ge.m_received_seqnum,3);
            m_gap_detected = true;
        }
        void on_book_added(const L3AddEvent &ae) override final {
            using Order = OrderBook::Order;
            const Order &o = ae.m_order;
            const OrderBook &book = ae.m_book;
            switch (m_count) {
            case 3:
                {
                    // ADD ORDER SHORT
                    EXPECT_EQ(1025000 == o.price
                              && 20000 == o.size
                              && Order::Side::BUY == o.side
                              && Order::TimeInForce::DAY == o.tif, true);
                    EXPECT_EQ(book.symbol_index(), 1);
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
            case 4:
                {
                    // ADD ORDER LONG
                    EXPECT_EQ(9050 == o.price
                              && 20000 == o.size
                              && Order::Side::BUY == o.side
                              && Order::TimeInForce::DAY == o.tif, true);
                    EXPECT_EQ(book.symbol_index(), 1);
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
            case 5:
                {
                    // ADD ORDER EXPANDED
                    EXPECT_EQ(9050 == o.price
                              && 20000 == o.size
                              && Order::Side::BUY == o.side
                              && Order::TimeInForce::DAY == o.tif, true);
                    EXPECT_EQ(book.symbol_index(), 1);
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
            case 12:
                {
                    // ADD ORDER SHORT
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(o.price, 1026000);
                    EXPECT_EQ(o.size, 20000);
                    EXPECT_EQ(o.side, Order::Side::SELL);
                    EXPECT_EQ(o.tif, Order::TimeInForce::DAY);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), 1025000);
                    EXPECT_EQ(std::get<1>(bb), 19900);
                    EXPECT_EQ(std::get<2>(bb),1);

                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba), o.price);
                    EXPECT_EQ(std::get<1>(ba),o.size);
                    EXPECT_EQ(std::get<2>(ba),1);

                }
                break;
            case 20:
                {
                    // ADD ORDER SHORT
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(o.price, 10000);
                    EXPECT_EQ(o.size, 737);
                    EXPECT_EQ(o.side, Order::Side::BUY);
                    EXPECT_EQ(o.tif, Order::TimeInForce::DAY);
                }
                break;
            case 22:
            case 23:
            case 24:
                {
                    // ADD ORDER SHORT
                    EXPECT_EQ(book.symbol_index(),1);
                }
                break;
            case 25:
                {
                    // ADD ORDER SHORT
                    // this should have crossed the book
                    EXPECT_EQ(book.symbol_index(),1);
                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), 10000);
                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba), 10100);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_book_added: callback " << m_count << " not checked.";
                break;
            }
            m_count++;

        }

        void on_book_executed(const L3ExecutionEvent &ee) override final {
            auto& book = ee.m_book;
            auto& exch_ts = ee.m_exchange_timestamp;

            switch (m_count) {
            case 10:
                {
                    // ORDER_EXECUTED
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(exch_ts.tv_sec,34200);
                    EXPECT_EQ(exch_ts.tv_nsec, 447000);
                    EXPECT_EQ(ee.m_exec_price,1025000);
                    EXPECT_EQ(ee.m_order.price, ee.m_exec_price);
                    EXPECT_EQ(ee.m_order.size, 75);
                    EXPECT_EQ(ee.m_exec_size,25);
                    Types::ExecutionId bats_id;
                    bats_id.u64 = ee.m_trade_id;
                    auto check_id = Types::ExecutionId::base36_array_type{{'0', 'A', 'A', 'P', '0', '9', 'V', 'E', 'C'}};
                    EXPECT_EQ(bats_id.to_base36(), check_id);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), ee.m_order.price);
                    EXPECT_EQ(std::get<1>(bb), 75);
                    EXPECT_EQ(std::get<2>(bb), 1);
                }
                break;
            case 11:
                {
                    // ORDER_EXECUTED_AT_PRICE_SIZE
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(exch_ts.tv_sec,34200);
                    EXPECT_EQ(exch_ts.tv_nsec, 447000);
                    EXPECT_EQ(ee.m_exec_price,1025000);
                    EXPECT_EQ(ee.m_order.price, ee.m_exec_price);
                    EXPECT_EQ(ee.m_exec_size,100);
                    EXPECT_EQ(ee.m_order.size, 19900);
                    Types::ExecutionId bats_id;
                    bats_id.u64 = ee.m_trade_id;
                    auto check_id = Types::ExecutionId::base36_array_type{{'0', 'A', 'A', 'P', '0', '9', 'V', 'E', 'C'}};
                    EXPECT_EQ(bats_id.to_base36(), check_id);

                    // test that there is a new order in the book now
                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), ee.m_exec_price);
                    EXPECT_EQ(std::get<1>(bb), ee.m_order.size);
                    EXPECT_EQ(std::get<2>(bb), 1);

                }
                break;
            case 13:
                {
                    // ORDER_EXECUTED_AT_PRICE_SIZE
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(exch_ts.tv_sec,34200);
                    EXPECT_EQ(exch_ts.tv_nsec, 447000);
                    EXPECT_EQ(ee.m_exec_price,1026000);
                    EXPECT_EQ(ee.m_order.price, ee.m_exec_price);
                    EXPECT_EQ(ee.m_exec_size,5000);
                    EXPECT_EQ(ee.m_order.size,0);
                    Types::ExecutionId bats_id;
                    bats_id.u64 = ee.m_trade_id;
                    auto check_id = Types::ExecutionId::base36_array_type{{'0', 'A', 'A', 'P', '0', '9', 'V', 'E', 'C'}};
                    EXPECT_EQ(bats_id.to_base36(), check_id);

                    // the order gets reduced to 0, but is still on the book
                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba), 1026000);
                    EXPECT_EQ(std::get<1>(ba), 0);
                    EXPECT_EQ(std::get<2>(ba), 1);

                }
                break;

            default:
                ASSERT_EQ(true,false) << "on_book_executed: callback " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_book_modified(const L3ModifyEvent &me) override final {
            auto& book = me.m_book;
            auto new_order = me.m_new_order;
            auto old_refnum = me.m_old_refnum;
            auto old_price = me.m_old_price;
            auto old_size = me.m_old_size;
            switch (m_count) {
            case 6:
                {
                    EXPECT_EQ(book.symbol_index(),1);
                    EXPECT_EQ(new_order.size,10000);
                    EXPECT_EQ(old_size,20000);
                    EXPECT_EQ(old_price, new_order.price);
                    EXPECT_EQ(old_refnum,new_order.refnum);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),old_price);
                    EXPECT_EQ(std::get<1>(bb), 10000);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;
            case 7:
                {
                    EXPECT_EQ(book.symbol_index(),1);
                    EXPECT_EQ(new_order.size,9900);
                    EXPECT_EQ(old_size,10000);
                    EXPECT_EQ(old_price,new_order.price);
                    EXPECT_EQ(old_refnum, new_order.refnum);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),new_order.price);
                    EXPECT_EQ(std::get<1>(bb),new_order.size);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;
            case 8:
                {
                    EXPECT_EQ(book.symbol_index(),1);
                    EXPECT_EQ(new_order.size,75000);
                    EXPECT_EQ(old_size,9900);
                    EXPECT_EQ(old_price,9050);
                    EXPECT_EQ(new_order.price,1025000);
                    EXPECT_EQ(old_refnum, new_order.refnum);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),new_order.price);
                    EXPECT_EQ(std::get<1>(bb),new_order.size);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;
            case 9:
                {
                    EXPECT_EQ(book.symbol_index(),1);
                    EXPECT_EQ(new_order.size,100);
                    EXPECT_EQ(old_size,75000);
                    EXPECT_EQ(old_price,1025000);
                    EXPECT_EQ(new_order.price,old_price);
                    EXPECT_EQ(old_refnum, new_order.refnum);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),new_order.price);
                    EXPECT_EQ(std::get<1>(bb),new_order.size);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;
            case 21:
                {
                    // REDUCE SIZE SHORT
                    EXPECT_EQ(book.symbol_index(),1);
                    EXPECT_EQ(me.m_old_size, 737);
                    EXPECT_EQ(me.m_new_order.size, 237);
                    EXPECT_EQ(me.m_old_price, me.m_new_order.price);
                }
                break;
            case 26:
                {
                    // MODIFIED TO CROSS BOOK
                    EXPECT_EQ(book.symbol_index(),1);
                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), 10200);
                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba), BookBase::EMPTY_ASK_PRICE);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_book_modified: callback " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_book_canceled(const L3CancelEvent &ce) override final {
            auto& book = ce.m_book;
            auto& order = ce.m_order;
            switch(m_count) {
            case 14:
                {
                    EXPECT_EQ(book.symbol_index(),1);
                    EXPECT_EQ(ce.m_old_size, 19900);
                    EXPECT_EQ(order.size,0);
                    EXPECT_EQ(order.price,1025000);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb), order.price);
                    EXPECT_EQ(std::get<1>(bb),0);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "Packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_trade(const TradeEvent &te) override final {
            const BookBase &book = te.m_book;
            const Timestamp &exch_ts = te.m_exchange_timestamp;
            const TradeEvent::TradeRefNum &exec_id = te.m_trade_id;
            switch(m_count) {
            case 15:
                {
                    // TRADE_LONG
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(exch_ts.tv_sec,34200);
                    EXPECT_EQ(exch_ts.tv_nsec, 447000);
                    EXPECT_EQ(te.m_price,1035000);
                    EXPECT_EQ(te.m_size,75000);
                    Types::ExecutionId bats_id;
                    bats_id.u64 = exec_id;
                    auto check_id = Types::ExecutionId::base36_array_type{{'0', 'A', 'A', 'P', '0', '9', 'V', 'E', 'C'}};
                    EXPECT_EQ(bats_id.to_base36(), check_id);

                    // test that there is a new order in the book now
                    auto& ob = reinterpret_cast<const OrderBook &>(book);
                    auto bb = ob.best_bid();
                    EXPECT_EQ(std::get<0>(bb),0);
                    EXPECT_EQ(std::get<1>(bb),0);
                    EXPECT_EQ(std::get<2>(bb),0);

                }
                break;
            case 16:
                {
                    // TRADE_SHORT
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(exch_ts.tv_sec,34200);
                    EXPECT_EQ(exch_ts.tv_nsec, 447000);
                    EXPECT_EQ(te.m_price,1050600);
                    EXPECT_EQ(te.m_size,100);
                    Types::ExecutionId bats_id;
                    bats_id.u64 = exec_id;
                    auto check_id = Types::ExecutionId::base36_array_type{{'0', 'A', 'A', 'P', '0', '9', 'V', 'E', 'C'}};
                    EXPECT_EQ(bats_id.to_base36(), check_id);

                    // test that there is a new order in the book now
                    auto& ob = reinterpret_cast<const OrderBook &>(book);
                    auto bb = ob.best_bid();
                    EXPECT_EQ(std::get<0>(bb),0);
                    EXPECT_EQ(std::get<1>(bb),0);
                    EXPECT_EQ(std::get<2>(bb),0);

                }
                break;
            case 17:
                {
                    // TRADE_EXPANDED
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(exch_ts.tv_sec,34200);
                    EXPECT_EQ(exch_ts.tv_nsec, 447000);
                    EXPECT_EQ(te.m_price,1026000);
                    EXPECT_EQ(te.m_size,75000);
                    Types::ExecutionId bats_id;
                    bats_id.u64 = exec_id;
                    auto check_id = Types::ExecutionId::base36_array_type{{'0', 'A', 'A', 'P', '0', '9', 'V', 'E', 'C'}};
                    EXPECT_EQ(bats_id.to_base36(), check_id);

                    // test that there is a new order in the book now
                    auto& ob = reinterpret_cast<const  OrderBook &>(book);
                    auto bb = ob.best_bid();
                    EXPECT_EQ(std::get<0>(bb),0);
                    EXPECT_EQ(std::get<1>(bb),0);
                    EXPECT_EQ(std::get<2>(bb),0);

                }
                break;
            default:
                ASSERT_EQ(true,false) << "Packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_trading_status_update(const TradingStatusEvent &evt) override final
        {
            switch(m_count) {
            case 18:
                {
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_event, TradingStatusEvent::Event::HALT);
                }
                break;
            case 19:
                {
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_event, TradingStatusEvent::Event::SSR_IN_EFFECT);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_trading_status_update: callback " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_book_crossed(const L3CrossedEvent& evt) override final {
            std::cerr << "PITCH CROSSED " << m_count << " " << evt << std::endl;
            switch (m_count) {
            case 25:
            case 26:
                break;
            default:
                ASSERT_TRUE(false) << "on_book_crossed: callback " << m_count << " not checked.";
            }
            // don't increcemtn m_count since another callback will get it...
        }

        void on_feed_event(const FeedEvent &evt) override final {
            switch(m_count) {
            case 27:
                {
                    EXPECT_EQ(evt.mic.market(), i01::core::MIC::Enum::BATS);
                    EXPECT_EQ(evt.event_code, FeedEvent::EventCode::START_OF_MESSAGES);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_feed_event: callback " << m_count << " not checked.";
            }
            m_count++;
        }

        int m_count = 0;
        int m_pkt_count = 0;
        bool m_gap_detected = false;
    };

    PITCHListener pitch;
    using PITCHBookMux = BookMux<PITCHListener>;
    PITCHBookMux bm(&pitch, MICEnum::BATS);

    bm.create_book_for_symbol(1, "ZVZZT");

    Decoder<PITCHBookMux> feed(&bm, MICEnum::BATS);

    feed.init_streams({"BATS:PITCH:1"});
    auto addr1 = addr_str_to_ip_addr("224.0.62.2:30001");
    auto src_addr = addr_str_to_ip_addr("127.0.0.1:5555");
    auto now = Timestamp::now();
    using HW = HeaderWrapper<Messages::SequencedUnitHeader>;

    EXPECT_EQ(bm.last_sale(1).price,0);
    EXPECT_EQ(bm.last_sale(1).timestamp, Timestamp{});

    // 1
    auto time_pkt = make(HW(1,1,1), Messages::Time{6, MessageType::TIME, {{0x98,0x85,0,0}}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, time_pkt.m_buf.data(), time_pkt.m_buf.size(), &now);

    // generate a gap
    pitch.m_count = 3;
    // 3
    // BUY ZVZZT 20000 @ 1025000
    Messages::AddOrderShort aos{26, MessageType::ADD_ORDER_SHORT,
        {{0x18, 0xD2, 0x06, 0x00}},
        {{0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B}},
        {Types::SideIndicator::Values::BUY},
            20000,
            {{'Z', 'V', 'Z', 'Z', 'T', 0x20}},
                Types::Price2{{10250}}, Types::AddFlags::DISPLAY};

    auto pkt = make(HW(3,1,1), aos);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    EXPECT_EQ(pitch.m_gap_detected,true);
    // 4
    // BUY ZVZZT 20000 @ 9050
    Messages::AddOrderLong aol{34, MessageType::ADD_ORDER_LONG,
        {{0x18, 0xD2, 0x06, 0x00}},
        {{0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B}},
        {Types::SideIndicator::Values::BUY},
            20000,
            {{'Z', 'V', 'Z', 'Z', 'T', 0x20}},
                Types::Price8{{0x235A}}, Types::AddFlags::DISPLAY};

    pkt = make(HW(4,1,1), aol);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 5
    // BUY ZVZZT 20000 @ 9050
    Messages::AddOrderExpanded aoe{40, MessageType::ADD_ORDER_EXPANDED,
        {{0x18, 0xD2, 0x06, 0x00}},
        {{0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B}},
        {Types::SideIndicator::Values::BUY},
            20000,
            {{'Z', 'V', 'Z', 'Z', 'T', 0x20, 0x20, 0x20}},
                Types::Price8{{0x235A}}, Types::AddFlags::DISPLAY, {{'M', 'P', 'I', 'D'}}};
    pkt = make(HW(5,1,1), aoe);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 6
    Messages::ReduceSizeLong rsl{18, MessageType::REDUCE_SIZE_LONG,
        {0x18, 0xD2, 0x06, 0x00},
        {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
            10000};
    pkt = make(HW(6,1,1), rsl);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 7
    Messages::ReduceSizeShort rss{16, MessageType::REDUCE_SIZE_SHORT,
        {0x18, 0xD2, 0x06, 0x00},
        {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
            100};
    pkt = make(HW(7,1,1), rss);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 8
    // mod BUY ZVZZT 9900 shs -> 75000 shs @ 9050 -> 9900
    pkt = make(HW(8,1,1), Messages::ModifyOrderLong{27, MessageType::MODIFY_ORDER_LONG,
            {0x18, 0xD2, 0x06, 0x00},
            {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
                75000,
                    1025000, 0x03});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 9
    // mod BUY ZVZZT 75000 shs -> 100 @ 1025000
    pkt = make(HW(9,1,1), Messages::ModifyOrderShort{19, MessageType::MODIFY_ORDER_SHORT,
            {0x18, 0xD2, 0x06, 0x00},
            {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
                100,
                    0x280A, 0x03});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 10
    Messages::OrderExecuted oe{26, MessageType::ORDER_EXECUTED,
        {0x18, 0xD2, 0x06, 0x00},
        {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
            25,
            {0x34, 0x2B, 0x46, 0xE0, 0xBB, 0x00, 0x00, 0x00}};

    pkt = make(HW(10,1,1), oe);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    EXPECT_EQ(bm.last_sale(1).price, 1025000);
    EXPECT_EQ(bm.last_sale(1).timestamp, now);

    now = Timestamp::now();
    // 11
    Messages::OrderExecutedAtPriceSize oea{38, MessageType::ORDER_EXECUTED_AT_PRICE_SIZE,
        {0x18, 0xD2, 0x06, 0x00},
        {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
            100, 19900,
            {0x34, 0x2B, 0x46, 0xE0, 0xBB, 0x00, 0x00, 0x00},
            1025000};
    pkt = make(HW(11,1,1), oea);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    EXPECT_EQ(bm.last_sale(1).price, 1025000);
    EXPECT_EQ(bm.last_sale(1).timestamp, now);

    // bm.clear_book(1);

    // 12
    // SELL ZVZZT 20000 @ 1026000
    pkt = make(HW(12,1,1), Messages::AddOrderShort{26, MessageType::ADD_ORDER_SHORT,
            {{0x18, 0xD2, 0x06, 0x00}},
            {{0x06, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B}},
            {Types::SideIndicator::Values::SELL},
                20000,
                {{'Z', 'V', 'Z', 'Z', 'T', 0x20}},
                    Types::Price2{{10260}}, Types::AddFlags::DISPLAY});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    now = Timestamp::now();
    // 13
    pkt = make(HW(13,1,1), Messages::OrderExecutedAtPriceSize{38, MessageType::ORDER_EXECUTED_AT_PRICE_SIZE,
            {0x18, 0xD2, 0x06, 0x00},
            {0x06, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
                5000, 0,
                {0x34, 0x2B, 0x46, 0xE0, 0xBB, 0x00, 0x00, 0x00},
                    1026000});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    EXPECT_EQ(bm.last_sale(1).price, 1026000);
    EXPECT_EQ(bm.last_sale(1).timestamp, now);

    // 14
    pkt = make(HW(14,1,1), Messages::DeleteOrder{14, MessageType::DELETE_ORDER,
            {0x18, 0xD2, 0x06, 0x00},
            {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    now = Timestamp::now();

    // 15
    Messages::TradeLong tl{41, MessageType::TRADE_LONG,
        {0x18, 0xD2, 0x06, 0x00},
        {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
            Types::SideIndicator::Values::BUY,
                75000,
                {'Z', 'V', 'Z', 'Z', 'T', 0x20},
                1035000,
                {0x34, 0x2B, 0x46, 0xE0, 0xBB, 0x00, 0x00, 0x00}};

    pkt = make(HW(15,1,1), tl);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);
    EXPECT_EQ(bm.last_sale(1).price, 1035000);
    EXPECT_EQ(bm.last_sale(1).timestamp, now);

    now = Timestamp::now();

    // 16
    Messages::TradeShort ts{33, MessageType::TRADE_SHORT,
        {0x18, 0xD2, 0x06, 0x00},
        {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
            Types::SideIndicator::Values::BUY,
                100,
                {'Z', 'V', 'Z', 'Z', 'T', 0x20},
                0x290A,
                {0x34, 0x2B, 0x46, 0xE0, 0xBB, 0x00, 0x00, 0x00}};
    pkt = make(HW(16,1,1), ts);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    EXPECT_EQ(bm.last_sale(1).price, 1050600);
    EXPECT_EQ(bm.last_sale(1).timestamp, now);

    now = Timestamp::now();

    // 17
    Messages::TradeExpanded te{43, MessageType::TRADE_EXPANDED,
        {0x18, 0xD2, 0x06, 0x00},
        {0x05, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
            Types::SideIndicator::Values::BUY,
                75000,
                {'Z', 'V', 'Z', 'Z', 'T', 0x20, 0x20, 0x20},
                1026000,
                {0x34, 0x2B, 0x46, 0xE0, 0xBB, 0x00, 0x00, 0x00}};
    pkt = make(HW(17,1,1), te);
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    EXPECT_EQ(bm.last_sale(1).price, 1026000);
    EXPECT_EQ(bm.last_sale(1).timestamp, now);

    // 18
    pkt = make(HW(18,1,1), Messages::TradingStatus{18, MessageType::TRADING_STATUS, {0x18, 0xD2, 0x06, 0x00}, {'Z', 'V', 'Z', 'Z', 'T', ' ', ' ', ' '}, Types::TradingStatusCode::HALTED, Types::RegSHOAction::REG_SHO_PRICE_RESTRICTION});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 19
    pkt = make(HW(19,2,1), Messages::AddOrderShort{26, MessageType::ADD_ORDER_SHORT, {0x18, 0xd2, 0x06, 0x00}, {0x04, 0x40, 0x5b, 0x77, 0x8f, 0x56, 0x1d, 0x0b}, Types::SideIndicator::Values::BUY, 737, {'Z', 'V', 'Z', 'Z', 'T', ' '}, Types::Price2{100}, Types::AddFlags::DISPLAY},
               Messages::ReduceSizeShort{16, MessageType::REDUCE_SIZE_SHORT, {0xe8, 0xd9, 0x06, 0x00}, {0x04, 0x40, 0x5b, 0x77, 0x8f, 0x56, 0x1d, 0x0b}, 500 });
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);


    bm.clear_book(1);

    // 21, 22, 23
    // BUY ZVZZT 737 @ 100 -> reduced to 500 shares
    // BUY 500 ZVZZT @ 101
    // BUY 500 ZVZZT @ 102
    pkt = make(HW(21,1,1), Messages::AddOrderShort{26, MessageType::ADD_ORDER_SHORT, {0x18, 0xd2, 0x06, 0x00}, {0x04, 0x40, 0x5b, 0x77, 0x8f, 0x56, 0x1d, 0x0b}, Types::SideIndicator::Values::BUY, 500, {'Z', 'V', 'Z', 'Z', 'T', ' '}, Types::Price2{99}, Types::AddFlags::DISPLAY});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    pkt = make(HW(22,1,1), Messages::AddOrderShort{26, MessageType::ADD_ORDER_SHORT, {0x18, 0xd2, 0x06, 0x00}, {0x04, 0x40, 0x5b, 0x77, 0x8f, 0x56, 0x1d, 0x0c}, Types::SideIndicator::Values::BUY, 500, {'Z', 'V', 'Z', 'Z', 'T', ' '}, Types::Price2{100}, Types::AddFlags::DISPLAY});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    pkt = make(HW(23,1,1), Messages::AddOrderShort{26, MessageType::ADD_ORDER_SHORT, {0x18, 0xd2, 0x06, 0x00}, {0x04, 0x40, 0x5b, 0x77, 0x8f, 0x56, 0x1d, 0x0d}, Types::SideIndicator::Values::BUY, 500, {'Z', 'V', 'Z', 'Z', 'T', ' '}, Types::Price2{102}, Types::AddFlags::DISPLAY});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);


    // 24
    // SELL ZVZZT 100 @ 101
    pkt = make(HW(24,1,1), Messages::AddOrderShort{26, MessageType::ADD_ORDER_SHORT, {0x18, 0xd2, 0x06, 0x00}, {0x04, 0x40, 0x5b, 0x77, 0x8f, 0x56, 0x1d, 0x0e}, Types::SideIndicator::Values::SELL, 100, {'Z', 'V', 'Z', 'Z', 'T', ' '}, Types::Price2{101}, Types::AddFlags::DISPLAY});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 25
    // book is 10000 x 10100 ... cross it via modify
    pkt = make(HW(25,1,1), Messages::ModifyOrderShort{19, MessageType::MODIFY_ORDER_SHORT,
            {0x18, 0xD2, 0x06, 0x00},
            {0x04, 0x40, 0x5B, 0x77, 0x8F, 0x56, 0x1D, 0x0B},
                100,
                    Types::Price2{102}, 0x03});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);


    // 26
    pkt = make(HW(26,1,1), Messages::UnitClear{6, MessageType::UNIT_CLEAR,
            {0x18, 0xD2, 0x06, 0x00}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    auto bb = bm[1]->best_bid();
    EXPECT_EQ(std::get<0>(bb),0);
    EXPECT_EQ(std::get<1>(bb),0);
    EXPECT_EQ(std::get<2>(bb),0);
    auto ba = bm[1]->best_ask();
    EXPECT_EQ(std::get<0>(ba),std::numeric_limits<i01::MD::OrderBook::Order::Price>::max());
    EXPECT_EQ(std::get<1>(ba),0);
    EXPECT_EQ(std::get<2>(ba),0);

    // one less than sequence, since first Time did not go through to listener
    EXPECT_EQ(pitch.m_count,28);
    EXPECT_EQ(pitch.m_pkt_count, 24);
}
