#include <i01_md/NYSE/PDP/Decoder.hpp>


namespace i01 { namespace MD { namespace NYSE { namespace PDP {

std::unique_ptr<UnitState> create_pdp_unit_state(const core::MIC& mic, const std::string& feed_name, const std::string& unit_name, const core::Config::storage_type& cfg)
{
    return std::unique_ptr<UnitState>(new UnitState(std::string(mic.name()) + ":" + feed_name + ":" + unit_name));
}


}}}}
