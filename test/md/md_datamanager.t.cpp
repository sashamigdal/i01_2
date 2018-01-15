#include <gtest/gtest.h>

#include <i01_md/DataManager.hpp>
#include <i01_md/HistoricalData.hpp>
#include <i01_md/LastSaleListener.hpp>

#include <i01_net/IPAddress.hpp>

using namespace i01::MD;
using namespace i01::core;

class Configger {
public:
    Configger(const std::string& luafile) {
        i01::core::Config::instance().load_lua_file(luafile);
    }
    Configger(const std::string &key, const std::string &value) {
        i01::core::Config::instance().load_strings({{key, value}});
    }
    Configger(const std::map<std::string, std::string>& values) {
        i01::core::Config::instance().load_strings(values);
    }
};

struct TestListener : public NoopBookMuxListener, public TimerListener {
    virtual void on_timer(const Timestamp&, void*ud, std::uint64_t iter) override {
        // std::cerr << "Timer " << Timestamp::last_event() << " iter: " << iter << " ud: " << ud << std::endl;
        count++;
    }
    int count = 0;
};


TEST(md_datamanager, md_dm_test1)
{

    TestListener tl;
    std::set<std::string> files = {{STRINGIFY(I01_DATA) "/mdnasdaq.20150218.090000_090030.pcap-ns"}, {STRINGIFY(I01_DATA) "/mdedge.20150218.090000_090030.pcap-ns"}};
    Configger(STRINGIFY(I01_DATA) "/md_unittest.lua");
    // auto xnas = std::make_pair(i01::net::addr_str_to_ip_addr("233.54.12.111:26477"),23746);
    // auto xbos = std::make_pair(i01::net::addr_str_to_ip_addr("233.54.12.40:25475"),9281);

    // auto bats1 = std::make_pair(i01::net::addr_str_to_ip_addr("224.0.62.2:30001"), 45);
    // auto edgx5 = std::make_pair(i01::net::addr_str_to_ip_addr("224.0.130.65:30405"), 38);

    {
        DataManager dm;

        dm.register_listener(&tl);

        dm.init(*i01::core::Config::instance().get_shared_state()->copy_prefix_domain("md."), 20150218);
        dm.use_files(files);
        dm.register_timer(&tl, (void *)1);

        ASSERT_TRUE(dm.read_data());
    }
}


TEST(md_datamanager, md_historical_data)
{
    Config::instance().reset();
    Config::instance().load_strings({
            {"md.hist.search_path", STRINGIFY(I01_DATA)},
            {"md.hist.suffix_regex", "\\.[0-9]{6}_[0-9]{6}\\.pcap-ns"},
            {"md.hist.latency.A.regex", ".*mdedge.*"},
            {"md.hist.latency.A.latency_ns", "1000"}
        });
    i01::MD::HistoricalData hd;
    hd.init(*i01::core::Config::instance().get_shared_state()->copy_prefix_domain("md.hist."), 20150218);
    auto ret = hd.get_files(i01::core::MICEnum::BATS);

    auto mdedge = STRINGIFY(I01_DATA) "/mdedge.20150218.090000_090030.pcap-ns";
    ASSERT_EQ(ret.size(),1);
    EXPECT_EQ(*ret.begin(), mdedge);
    for (auto r : ret) {
        std::cerr << r << std::endl;
    }

    auto test = std::string("foo.pcap-ns");
    auto lat = hd.get_latencies(std::set<std::string>{mdedge, test }, 300);

    ASSERT_EQ(lat.size(),2);
    for (const auto& e : lat) {
        if (e.first == mdedge) {
            EXPECT_EQ(e.second, 1000) << mdedge;
        } else if (e.first == test) {
            EXPECT_EQ(e.second, 300) << test;
        } else {
            FAIL() << "bad filenames returned";
        }
    }
}

TEST(md_datamanager, dm_last_sale)
{
    using namespace i01::MD;
    using namespace i01::core;

    struct Listener : public LastSaleListener {
        void on_last_sale_change(const EphemeralSymbolIndex esi, const LastSale& ls) {
            EXPECT_EQ(esi,1);
            count++;
            last_sale = ls;
        }

        int count = 0;
        LastSale last_sale;
    };


    Config::instance().reset();
    Config::instance().load_lua_file(STRINGIFY(I01_DATA) "/md_unittest.lua");
    Config::instance().load_strings({
            {"md.universe.symbol.1.cta_symbol", "BHP"},
            {"md.universe.symbol.1.fdo_symbol", "3004247"},
            {"md.universe.symbol.1.pdp_symbol", "BHP"},
            {"md.hist.search_path", STRINGIFY(I01_DATA)},
            {"md.hist.suffix_regex", "\\.[0-9]{6}_[0-9]{6}\\.pcap-ns"}
        });

    DataManager dm;
    Listener listener;
    dm.register_last_sale_listener(&listener);
    dm.init(*i01::core::Config::instance().get_shared_state()->copy_prefix_domain("md."), 20150218);
    dm.use_files(std::set<std::string>{{STRINGIFY(I01_DATA) "/mdedge.20150218.090000_090030.pcap-ns"}});
    ASSERT_TRUE(dm.read_data());

    EXPECT_EQ(listener.count,3);
    EXPECT_EQ(listener.last_sale.price, 506700);
    EXPECT_EQ(listener.last_sale.mic, MICEnum::EDGA);
}
