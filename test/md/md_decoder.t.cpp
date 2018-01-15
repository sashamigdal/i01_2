#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <memory>
#include <map>

#include <i01_net/Pcap.hpp>

#include <i01_md/BookBase.hpp>
#include <i01_md/DiagnosticListener.hpp>

#include <i01_md/CTA/Decoder.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "md_decoder_util.hpp"


TEST(md_decoder, md_config_reader)
{
    namespace po = boost::program_options;
    namespace c = i01::core;
    namespace n = i01::net;
    namespace cr = i01::MD::ConfigReader;

    po::variables_map vm;
    po::options_description od;
    std::string exchanges_arg = "md.exchanges.ARCX.feeds.ABXDP.units.AB_1.A.address";
    std::string univ_arg = "md.universe.symbol.7.pdp_symbol";
    od.add_options()
        (exchanges_arg.c_str(), po::value<std::string>(), "ArcaBook XDP Channel 1A")
        (univ_arg.c_str(), po::value<std::string>(), "PDP symbol for ESI");
    po::store(po::command_line_parser({std::string("--") + exchanges_arg + "=224.0.59.76:11076",
                    std::string("--") + univ_arg + "=BAC"}).options(od).run(),vm);
    c::Config::instance().load_variables_map(vm);

    std::vector<std::string> stream_names;
    cr::addr_name_map_type addr_name_map;

    ASSERT_EQ(cr::read_stream_names(*c::Config::instance().get_shared_state().get(), "ARCX", "ABXDP",stream_names,addr_name_map),true);

    EXPECT_EQ(stream_names[0],"ARCX:ABXDP:AB_1");
    EXPECT_EQ(n::ip_addr_to_str(addr_name_map.begin()->first), "224.0.59.76:11076");

    auto res = cr::get_i01_vector_for_symbol(*c::Config::instance().get_shared_state().get(), "pdp_symbol");
    ASSERT_EQ(res[7], "BAC");
}
