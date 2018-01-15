#include <string.h>
#include <i01_core/MIC.hpp>

namespace i01 { namespace core {

#define MARKET(mic, description) , [(int)MIC::Enum::mic] = #mic
const char *MIC::Name[] =  {
    [(int)Enum::UNKNOWN] = "UNKNOWN"
#include <i01_core/MIC.def.hpp>
};
static_assert(sizeof(MIC::Name)/sizeof(*MIC::Name) == (int)MIC::Enum::NUM_MIC, "MIC::Name is the wrong size.");
#undef MARKET
#define MARKET(mic, description) , [(int)MIC::Enum::mic] = #description
const char *MIC::Description[] = {
    [(int)Enum::UNKNOWN] = "UNKNOWN"
#include <i01_core/MIC.def.hpp>
    };
    static_assert((sizeof(MIC::Description)/sizeof(*MIC::Description)) == (int)MIC::Enum::NUM_MIC, "MIC::Description is the wrong size.");
#undef MARKET

MIC::MIC(const Enum& e) :
    market_(e >= Enum::NUM_MIC ? Enum::UNKNOWN : e)
{
}

MIC MIC::clone(const char *mic_name) {
    for (int i = 0; i < (int)MIC::Enum::NUM_MIC; ++i) {
        if (0 == ::strcmp(mic_name, MIC::Name[i]))
            return MIC((MIC::Enum)i);
    }
    return MIC::Enum::UNKNOWN;
}

MIC MIC::clone(const std::uint8_t mic_idx)
{
    if (0 == mic_idx || mic_idx >= (std::uint8_t) MIC::Enum::NUM_MIC) {
        return MIC();
    }
    return static_cast<MIC::Enum>(mic_idx);
}

} }
