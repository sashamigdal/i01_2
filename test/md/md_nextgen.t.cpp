#include <gtest/gtest.h>

#include <i01_net/Pcap.hpp>

#include <i01_md/DiagnosticListener.hpp>
#include <i01_md/EDGE/NextGen/Decoder.hpp>
#include <i01_md/EDGE/NextGen/BookMux.hpp>

#include "md_decoder_util.hpp"

template<>
struct HeaderWrapper<i01::MD::EDGE::NextGen::Messages::CommonSessionMessageHeader>
{

    HeaderWrapper(const i01::MD::EDGE::NextGen::Messages::CommonSessionMessageHeader &mh) : m_header(mh) {}

    HeaderWrapper(std::uint32_t seqnum, std::uint8_t msg_count = 1, std::uint8_t unit = 1) {
        m_header.message_count = msg_count;
        m_header.sequence = seqnum;
        m_header.partition = unit;
    }
    void set_length(std::uint16_t len) {
        // dont include msg_size field size
        // get arround narrowing conversion error with this sizeof -> std::uint16_t
        m_header.length = len;
    }
    i01::MD::EDGE::NextGen::Messages::CommonSessionMessageHeader m_header;
};

TEST(md_decoder, md_edge_nextgen_decoder)
{
    using namespace i01::MD::EDGE::NextGen;
    using namespace i01::net::pcap;

    class MyDiagnosticListener : public i01::MD::DiagnosticListener {
    public:
        void on_gap_detected(const i01::core::Timestamp &ts, std::uint32_t addr, std::uint16_t port, std::uint8_t unit, std::uint64_t expected, std::uint64_t recv, const i01::core::Timestamp &last_ts) {
            // std::cout << "GAP " << ts << " " << ip_addr_to_str({addr,port}) << " " << (int) unit << " " << expected << " " << recv << std::endl;
            EXPECT_EQ(expected == 1 && recv == 1236,true) << "Gap sequence number mismatch.";
            EXPECT_EQ(unit,8) << "Gap unit mismtach.";
        }
    };

    MyDiagnosticListener mdl;
    Decoder<MyDiagnosticListener> feed(&mdl, i01::core::MIC::Enum::EDGX);
    UDPReader<Decoder<MyDiagnosticListener> > reader(STRINGIFY(I01_DATA) "/mdedge.20140903.080000.1409745600.edgx_unit8.first10k.pcap-ns", &feed);

    reader.read_packets();

    for (const auto &s : feed.streams()) {
        if (s.pkt_count()) {
            // std::cout << s.name() << " " << s.pkt_count() << " " << s.msg_count() << " " << s.expect_seqnum() << " " << s.time_ref().u64 << std::endl;
            EXPECT_EQ(s.name(), "EDGX:unit8") << "Unit name mismatch";
            EXPECT_EQ(s.msg_count(),18380) << "Unit message count mismatch.";
            EXPECT_EQ(s.expect_seqnum(), 19616) << "Unit seqnum mismatch";
        }
    }

    // std::cout << mdl.message_summary() << std::endl;

    const MyDiagnosticListener::msgtype_count_map_type & mdl_msgs = mdl.msg_counts();
    const std::vector<std::pair<int,int>> correct_counts = { {0x20,278}, {0x21,47}, {0x22,8433}, {0x23,128}, {0x24, 4}, {0x27, 3}, {0x28, 787}, {0x29, 2001}, {0x2b, 5}, {0x2e, 612}, {0x34, 6082}, {10000, 1}, {10001, 630}, {10003,4733}, {10004, 4733}};
    for (const auto &p : correct_counts) {
        auto it = mdl_msgs.find(p.first);
        EXPECT_EQ(it != mdl_msgs.end() && it->second == (unsigned) p.second,true);
    }
    std::size_t sum =0 ;
    for (auto m : mdl_msgs) {
        sum += m.second;
    }

    EXPECT_EQ(sum,18380 + 1 + 630 + 4733 + 4733) << "Counted messages sum mismatch";
}


TEST(md_decoder, md_edge_nextgen_bookmux)
{
    using namespace i01::MD;
    using namespace i01::core;
    using namespace EDGE::NextGen;

    struct NextGenListener : public NoopBookMuxListener {
        void on_end_of_data(const PacketEvent &evt) override final {
            EXPECT_EQ(evt.mic.market(), MIC::Enum::EDGX);
            m_pkt_count++;
        }

        void on_gap(const GapEvent &ge) override final {
            EXPECT_EQ(ge.m_expected_seqnum,1);
            EXPECT_EQ(ge.m_received_seqnum,2);
            m_gap_detected = true;
        }

        void on_book_added(const L3AddEvent &ae) override final {
            using Order = OrderBook::Order;
            const Order &o = ae.m_order;
            const OrderBook &book = ae.m_book;
            switch (m_count) {
            case 3:
                {
                    // ADD ORDER LONG
                    EXPECT_EQ(o.price,550000);
                    EXPECT_EQ(o.size, 100000);
                    EXPECT_EQ(o.side, Order::Side::BUY);
                    EXPECT_EQ(o.tif, Order::TimeInForce::DAY);
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(o.refnum,1);
                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),o.price);
                    EXPECT_EQ(std::get<1>(bb),o.size);
                    EXPECT_EQ(std::get<2>(bb),1);
                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba),std::numeric_limits<Order::Price>::max());
                    EXPECT_EQ(std::get<1>(ba),0);
                    EXPECT_EQ(std::get<2>(ba),0);
                }
                break;
            case 4:
                {
                    // ADD ORDER SHORT
                    EXPECT_EQ(o.price,6000000);
                    EXPECT_EQ(o.size, 5000);
                    EXPECT_EQ(o.side, Order::Side::BUY);
                    EXPECT_EQ(Order::TimeInForce::DAY, o.tif);
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(o.refnum,2);
                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),o.price);
                    EXPECT_EQ(std::get<1>(bb),o.size);
                    EXPECT_EQ(std::get<2>(bb),1);
                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba),std::numeric_limits<Order::Price>::max());
                    EXPECT_EQ(std::get<1>(ba),0);
                    EXPECT_EQ(std::get<2>(ba),0);
                }
                break;
            case 5:
                {
                    // ADD ORDER EXTENDED
                    EXPECT_EQ(o.price, 160000);
                    EXPECT_EQ(o.size, 500);
                    EXPECT_EQ(o.side, Order::Side::SELL);
                    EXPECT_EQ(Order::TimeInForce::DAY, o.tif);
                    EXPECT_EQ(book.symbol_index(), 1);
                    EXPECT_EQ(o.refnum,3);
                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),6000000);
                    EXPECT_EQ(std::get<1>(bb),5000);
                    EXPECT_EQ(std::get<2>(bb),1);
                    auto ba = book.best_ask();
                    EXPECT_EQ(std::get<0>(ba),o.price);
                    EXPECT_EQ(std::get<1>(ba),500);
                    EXPECT_EQ(std::get<2>(ba),1);
                }
                break;

            default:
                ASSERT_EQ(true,false) << "on_book_added: Packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_book_executed(const L3ExecutionEvent &evt) override final {
            const auto & book = evt.m_book;

            switch (m_count) {
            case 6:
                {
                    // ORDER EXECUTED
                    EXPECT_EQ(book.symbol_index(),1);
                    EXPECT_EQ(evt.m_order.refnum, 2);
                    EXPECT_EQ(evt.m_trade_id, 5);
                    EXPECT_EQ(evt.m_exec_price, 6000000);
                    EXPECT_EQ(evt.m_exec_price, evt.m_order.price);
                    EXPECT_EQ(evt.m_exec_size, 200);
                    EXPECT_EQ(evt.m_order.size,4800);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),evt.m_exec_price);
                    EXPECT_EQ(std::get<1>(bb),evt.m_order.size);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;
            case 7:
                {
                    // ORDER EXECUTED AT PRICE SIZE
                    EXPECT_EQ(book.symbol_index(),1);
                    EXPECT_EQ(evt.m_order.refnum, 1);
                    EXPECT_EQ(evt.m_trade_id, 7);
                    EXPECT_EQ(evt.m_exec_price, 2001000);
                    EXPECT_EQ(evt.m_exec_size, 200);
                    EXPECT_EQ(evt.m_order.size,99000);

                    auto bb = book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),6000000);
                    EXPECT_EQ(std::get<1>(bb),4800);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;

            default:
                ASSERT_EQ(true,false) << "on_book_executed: Packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_book_modified(const L3ModifyEvent &evt) override final {
            switch (m_count) {
            case 8:
                {
                    // ORDER MODIFIED LONG
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_new_order.refnum, 1);
                    EXPECT_EQ(evt.m_new_order.size, 10000);
                    EXPECT_EQ(evt.m_new_order.price, 1999000);
                    EXPECT_EQ(evt.m_old_refnum, evt.m_new_order.refnum);
                    EXPECT_EQ(evt.m_old_price, 550000);
                    EXPECT_EQ(evt.m_old_size, 99000);

                    auto bb = evt.m_book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),6000000);
                    EXPECT_EQ(std::get<1>(bb),4800);
                    EXPECT_EQ(std::get<2>(bb),1);
                }
                break;
            case 9:
                {
                    // ORDER_MODIFIED_SHORT
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_new_order.refnum, 2);
                    EXPECT_EQ(evt.m_new_order.size,200 );
                    EXPECT_EQ(evt.m_new_order.price,5990000 );
                    EXPECT_EQ(evt.m_old_refnum, evt.m_new_order.refnum);
                    EXPECT_EQ(evt.m_old_price, 6000000);
                    EXPECT_EQ(evt.m_old_size, 4800);

                    auto bb = evt.m_book.best_bid();
                    EXPECT_EQ(std::get<0>(bb),5990000);
                    EXPECT_EQ(std::get<1>(bb),200);
                    EXPECT_EQ(std::get<2>(bb),1);

                }
                break;

            default:
                ASSERT_EQ(true,false) << "on_book_modified: Packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }

        void on_trade(const TradeEvent &evt) override final {
            switch (m_count) {
            case 10:
                {
                    // TRADE LONG
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_exchange_timestamp, i01::core::Timestamp(1262338200,1009000));
                    EXPECT_EQ(evt.m_trade_id, 13);
                    EXPECT_EQ(evt.m_price, 19000000);
                    EXPECT_EQ(evt.m_size, 70000);
                    EXPECT_EQ(evt.m_passive_side, TradeEvent::PassiveSide::SELL);
                    EXPECT_EQ(evt.m_msg != nullptr,true);
                }
                break;
            case 11:
                {
                    // TRADE SHORT
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_exchange_timestamp, i01::core::Timestamp(1262338200,1010000));
                    EXPECT_EQ(evt.m_trade_id, 14);
                    EXPECT_EQ(evt.m_price, 5000000);
                    EXPECT_EQ(evt.m_size, 2000);
                    EXPECT_EQ(evt.m_passive_side, TradeEvent::PassiveSide::BUY);
                    EXPECT_EQ(evt.m_msg != nullptr,true);
                }
                break;
            case 12:
                {
                    // TRADE EXTENDED
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_exchange_timestamp, i01::core::Timestamp(1262338200,1010100));
                    EXPECT_EQ(evt.m_trade_id, 15);
                    EXPECT_EQ(evt.m_price, 150000);
                    EXPECT_EQ(evt.m_size, 80000);
                    EXPECT_EQ(evt.m_msg != nullptr,true);
                    EXPECT_EQ(evt.m_passive_side, TradeEvent::PassiveSide::UNKNOWN);
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
            case 13:
                {
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_event, TradingStatusEvent::Event::HALT);
                }
                break;
            case 14:
                {
                    EXPECT_EQ(evt.m_book.symbol_index(),1);
                    EXPECT_EQ(evt.m_event, TradingStatusEvent::Event::SSR_NOT_IN_EFFECT);
                }
                break;
            default:
                ASSERT_EQ(true,false) << "on_trading_status_update: packet " << m_count << " not checked.";
                break;
            }
            m_count++;
        }
        std::uint32_t m_count = 3;
        std::uint32_t m_pkt_count = 0;
        bool m_gap_detected = false;
    };

    NextGenListener nextgen;
    using NGBookMux = BookMux<NextGenListener>;
    NGBookMux bm(&nextgen, MICEnum::EDGX);

    bm.create_book_for_symbol(1, "ZVZZT");

    Decoder<NGBookMux> feed(&bm, MICEnum::EDGX);

    feed.init_streams({"EDGX:NextGen:1"});
    auto addr1 = i01::net::addr_str_to_ip_addr("233.130.124.0:36001");
    auto src_addr = i01::net::addr_str_to_ip_addr("127.0.0.1:5555");
    auto now = i01::core::Timestamp::now();
    using HW = HeaderWrapper<Messages::CommonSessionMessageHeader>;

    auto pkt = make(HW(2), Messages::Timestamp{{0x0A, MessageType::TIMESTAMP}, {0x98, 0xc0, 0x3d, 0x4b, 0x00, 0x00, 0x00, 0x00}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    EXPECT_EQ(nextgen.m_gap_detected,true);
    // 3
    pkt = make(HW(3), Messages::AddOrderLong{{0x22, MessageType::ADD_ORDER_LONG},
            {0x40, 0x42, 0x0f, 0x00}, 1,
                                          Side::BID, 100000,
                                      {'Z', 'V', 'Z', 'Z', 'T', ' '}, 550000, (std::uint8_t)AddOrderFlags::DISPLAY});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 4
    pkt = make(HW(4), Messages::AddOrderShort{{0x1a, MessageType::ADD_ORDER_SHORT}, {0x28, 0x46, 0x0f, 0x00}, 2, Side::BID, 5000, {'Z', 'V', 'Z', 'Z', 'T', ' '}, 0xea60, (std::uint8_t)AddOrderFlags::DISPLAY});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 5
    pkt = make(HW(5), Messages::AddOrderExtended{{0x24, MessageType::ADD_ORDER_EXTENDED}, {0x8c, 0x46, 0x0f, 0x00}, 3, Side::OFFER, 500, {'Z', 'V', 'Z', 'Z', 'T', ' ', ' ', ' '}, 160000, (std::uint8_t) AddOrderFlags::DISPLAY});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 6
    pkt = make(HW(6), Messages::OrderExecuted{{0x1a, MessageType::ORDER_EXECUTED}, {0x10, 0x4a, 0x0f, 0x00}, 2, 200, 5});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 7
    pkt = make(HW(7), Messages::OrderExecutedAtPriceSize{{0x26, MessageType::ORDER_EXECUTED_AT_PRICE_SIZE}, {0xf8, 0x4d, 0x0f, 0x00}, 1, 200, 99000, 7, 2001000});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 8
    pkt = make(HW(8), Messages::OrderModifiedLong{{0x1b, MessageType::ORDER_MODIFIED_LONG}, {0xb0, 0x59, 0x0f, 0x00}, 1, 10000, 1999000, (std::uint8_t)0});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 9
    pkt = make(HW(9), Messages::OrderModifiedShort{{0x13, MessageType::ORDER_MODIFIED_SHORT}, {0x98, 0x5d, 0x0f, 0x00}, 2, 200, 0xe9fc, (std::uint8_t)0});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 10
    pkt = make(HW(10), Messages::TradeLong{{0x29, MessageType::TRADE_LONG}, {0x68, 0x65, 0x0f, 0x00}, 3, Types::Side::OFFER, 70000, {'Z', 'V', 'Z', 'Z', 'T', ' '}, 19000000, 13});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 11
    pkt = make(HW(11), Messages::TradeShort{{0x21, MessageType::TRADE_SHORT}, {0x50, 0x69, 0x0f, 0x00}, 4, Types::Side::BID, 2000, {'Z', 'V', 'Z', 'Z', 'T', ' '}, 0xc350, 14});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 12
    pkt = make(HW(12), Messages::TradeExtended{{0x2b, MessageType::TRADE_EXTENDED}, {0xb4, 0x69, 0x0f, 0x00}, 211, Types::Side::UNDISCLOSED, 80000, {'Z', 'V', 'Z', 'Z', 'T', ' ', ' ', ' '}, 150000, 15});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);

    // 13
    pkt = make(HW(13), Messages::SecurityStatus{{0x15, MessageType::SECURITY_STATUS}, {0x20, 0x71, 0x0f, 0x00}, {'Z', 'V', 'Z', 'Z', 'T', ' ', ' ', ' '}, Types::IssueType::COMMON_SHARES, 1, 100, 0x43, 2, Types::SecurityStatusCode::IS_HALTED, {0}});
    feed.handle_payload(src_addr.first, src_addr.second, addr1.first, addr1.second, pkt.m_buf.data(), pkt.m_buf.size(), &now);


    EXPECT_EQ(nextgen.m_count,15);
    EXPECT_EQ(nextgen.m_pkt_count, 12);
}
