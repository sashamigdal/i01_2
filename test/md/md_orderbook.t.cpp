#include <gtest/gtest.h>

#include <iostream>

#include <i01_core/MIC.hpp>

#include <i01_md/OrderBook.hpp>


TEST(md_orderbook, md_orderbook_add_test)
{
    using namespace i01::MD;
    using i01::MD::OrderBook;
    using i01::core::Timestamp;
    typedef i01::MD::OrderBook::Order Order;

    OrderBook::Order::RefNum r(1);
    OrderBook b1;
    OrderBook b2(i01::core::MIC::Enum::XNAS);
    auto& o1 = b1.add_order(r++, Order::Side::BUY, 10203000, 100,
            Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
            nullptr);
    ASSERT_EQ(10203000, std::get<0>(b1.best_bid())) << "b1.best_bid() 30";
    auto& o2 = b1.add_order(r++, Order::Side::BUY, 10203100, 100,
            Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
            nullptr);
    ASSERT_EQ(10203100, std::get<0>(b1.best_bid())) << "b1.best_bid() 31";
    b1.add_order(r++, Order::Side::BUY, 10202900, 300,
            Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
            nullptr);
    b1.add_order(r++, Order::Side::BUY, 10202900, 100,
            Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
            nullptr);
    ASSERT_EQ(10203100, std::get<0>(b1.best_bid())) << "b1.best_bid() 31 2";
    b1.remove(o2.refnum);
    ASSERT_EQ(10203000, std::get<0>(b1.best_bid())) << "b1.best_bid() 30 2";
    b1.erase(o1);
    ASSERT_EQ(10202900, std::get<0>(b1.best_bid())) << "b1.best_bid() 29";
    ASSERT_EQ(400, std::get<1>(b1.best_bid())) << "b1.best_bid() sz400";
    {
        auto& o3 = b1.add_order(r++, Order::Side::SELL, 10202900, 100,
                Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                nullptr);
        ASSERT_EQ(10202900, std::get<0>(b1.best_ask())) << "b1.best_ask() 29";
        ASSERT_TRUE(b1.crossed()) << "b1.crossed()";
        b1.remove(o3.refnum);
        std::cout << std::get<0>(b1.best_bid()) << std::endl;
        std::cout << std::get<0>(b1.best_ask()) << std::endl;
        ASSERT_TRUE(!b1.crossed()) << "b1.crossed() 2";
    }
    {
        auto& o4 = b1.add_order(r++, Order::Side::SELL, 10203300, 300,
                Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                nullptr);
        ASSERT_TRUE(!b1.crossed()) << "b1.crossed() 2";
        b1.modify(o4, 200);
        ASSERT_EQ(200, std::get<1>(b1.best_ask())) << "b1.best_ask() sz";
        b1.replace(o4.refnum, Order(r++, Order::Side::SELL, 10203100, 150, Order::TimeInForce::GTC, Timestamp::now(), Timestamp::now(), nullptr));
        b1.add(Order(r++, Order::Side::SELL, 10203100, 50, Order::TimeInForce::GTC, Timestamp::now(), Timestamp::now(), nullptr));
        ASSERT_EQ(10203100, std::get<0>(b1.best_ask())) << "b1.best_ask() 31";
        ASSERT_EQ(200, std::get<1>(b1.best_ask())) << "b1.best_ask() sz 2";
    }
}

TEST(md_orderbook, md_orderbook_clear_test)
{
    using namespace i01::MD;
    using i01::MD::OrderBook;
    using i01::core::Timestamp;
    typedef i01::MD::OrderBook::Order Order;

    OrderBook ob(i01::core::MIC::Enum::ARCX, 1);

    ob.add(Order(1, Order::Side::BUY, 550000, 1000, Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(), nullptr, 0));

    ob.add(Order(2, Order::Side::SELL, 560000, 1000, Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(), nullptr, 0));

    ob.clear();
    {
        auto bb = ob.best_bid();
        EXPECT_EQ(std::get<0>(bb), 0);
        EXPECT_EQ(std::get<1>(bb), 0);
        EXPECT_EQ(std::get<2>(bb), 0);

        auto ba = ob.best_ask();
        EXPECT_EQ(std::get<0>(ba), std::numeric_limits<OrderBook::Order::Price>::max());
        EXPECT_EQ(std::get<1>(ba), 0);
        EXPECT_EQ(std::get<2>(ba), 0);
    }

    ob.add(Order(3, Order::Side::BUY, 550000, 1000, Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(), nullptr, Order::FlagBits::PRE_MARKET | Order::FlagBits::REGULAR_MARKET | Order::FlagBits::POST_MARKET));

    ob.add(Order(4, Order::Side::BUY, 550000, 2000, Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(), nullptr, Order::FlagBits::PRE_MARKET ));

    ob.add(Order(5, Order::Side::SELL, 560000, 1000, Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(), nullptr, Order::FlagBits::PRE_MARKET | Order::FlagBits::REGULAR_MARKET));

    // clear only PRE_MARKET
    ob.clear_not_in(Order::FlagBits::REGULAR_MARKET|Order::FlagBits::POST_MARKET);
    {
        auto bb = ob.best_bid();
        EXPECT_EQ(std::get<0>(bb), 550000);
        EXPECT_EQ(std::get<1>(bb), 1000);
        EXPECT_EQ(std::get<2>(bb), 1);

        auto ba = ob.best_ask();
        EXPECT_EQ(std::get<0>(ba), 560000);
        EXPECT_EQ(std::get<1>(ba), 1000);
        EXPECT_EQ(std::get<2>(ba), 1);
    }

    // clear PRE_MARKET & REGULAR_MARKET
    ob.clear_not_in(Order::FlagBits::POST_MARKET);
    {
        auto bb = ob.best_bid();
        EXPECT_EQ(std::get<0>(bb), 550000);
        EXPECT_EQ(std::get<1>(bb), 1000);
        EXPECT_EQ(std::get<2>(bb), 1);

        auto ba = ob.best_ask();
        EXPECT_EQ(std::get<0>(ba), std::numeric_limits<Order::Price>::max());
        EXPECT_EQ(std::get<1>(ba), 0);
        EXPECT_EQ(std::get<2>(ba), 0);
    }

}

TEST(md_orderbook, md_orderbook_clear_crossed)
{
    using namespace i01::MD;
    using i01::MD::OrderBook;
    using i01::core::Timestamp;
    typedef i01::MD::OrderBook::Order Order;

    OrderBook::Order::RefNum r(1);
    OrderBook b1;
    OrderBook b2(i01::core::MIC::Enum::XNAS);
    b1.add_order(r++, Order::Side::BUY, 10202900, 300,
            Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
            nullptr);
    b1.add_order(r++, Order::Side::BUY, 10202900, 100,
            Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
            nullptr);
    ASSERT_EQ(10202900, std::get<0>(b1.best_bid())) << "b1.best_bid() 29";

    {
        b1.add_order(r++, Order::Side::SELL, 10202900, 100,
                     Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                     nullptr);
        ASSERT_TRUE(b1.crossed()) << "b1.crossed()";
        b1.clear_crossed(Order::Side::SELL, 10202900);
        std::cout << std::get<0>(b1.best_bid()) << std::endl;
        std::cout << std::get<0>(b1.best_ask()) << std::endl;
        ASSERT_TRUE(!b1.crossed()) << "b1.crossed() 2";
        ASSERT_EQ(std::get<0>(b1.best_ask()), 10202900);
        ASSERT_EQ(std::get<0>(b1.best_bid()), BookBase::EMPTY_BID_PRICE);
        // EMPTY x 10202900
    }
    {
        b1.add_order(r++, Order::Side::SELL, 10203300, 300,
                     Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                     nullptr);
        b1.add_order(r++, Order::Side::SELL, 10204000, 300,
                     Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                     nullptr);
        b1.add_order(r++, Order::Side::SELL, 10205000, 300,
                     Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                     nullptr);
        b1.add_order(r++, Order::Side::BUY, 10204000, 300,
                     Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                     nullptr);

        ASSERT_TRUE(b1.crossed());
        b1.clear_crossed(Order::Side::BUY, 10204000);
        ASSERT_FALSE(b1.crossed());
        ASSERT_EQ(10205000, std::get<0>(b1.best_ask()));
        ASSERT_EQ(10204000, std::get<0>(b1.best_bid()));
    }
    {
        b1.add_order(r++, Order::Side::BUY, 10203000, 300,
                     Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                     nullptr);
        b1.add_order(r++, Order::Side::BUY, 10202000, 100,
                     Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                     nullptr);
        b1.add_order(r++, Order::Side::SELL, 10202500, 300,
                     Order::TimeInForce::DAY, Timestamp::now(), Timestamp::now(),
                     nullptr);
        ASSERT_TRUE(b1.crossed());
        b1.clear_crossed(Order::Side::SELL, 10202500);
        ASSERT_FALSE(b1.crossed());
        ASSERT_EQ(std::get<0>(b1.best_ask()), 10202500);
        ASSERT_EQ(std::get<0>(b1.best_bid()), 10202000);
    }

}
