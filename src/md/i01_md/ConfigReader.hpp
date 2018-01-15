#pragma once

#include <map>
#include <string>
#include <set>
#include <vector>

#include <i01_core/Config.hpp>
// #include <i01_core/MIC.hpp>

namespace i01 { namespace core {
struct MIC;
}}

namespace i01 { namespace MD { namespace ConfigReader {

typedef std::map<std::uint64_t, std::string> addr_name_map_type;

bool read_stream_names(const core::Config::storage_type &cfg, const std::string &exchange_name, const std::string &feed_name, std::vector<std::string> &stream_names, addr_name_map_type &addr_name_map);

std::vector<std::string> get_i01_vector_for_symbol(const core::Config::storage_type &cfg, const std::string &prefix);

bool get_symbol_mapping_filename(const core::Config::storage_type &cfg, const std::string &mic_name, const std::vector<std::string> &feed_names, std::map<std::string, std::string> & filename_map);

std::set<std::string> get_feed_names(const core::Config::storage_type &cfg, const core::MIC &mic);

// using EventPollerAddresses = std::map<std::string, std::vector<std::pair<std::string, std::string> > >;

// EventPollerAddresses get_eventpoller_addresses(const core::Config::storage_type& cfg, const core::MIC& mic);

}}}
