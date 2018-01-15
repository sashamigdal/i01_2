#pragma once

#include <cstdint>
#include <ostream>

#include "macro.hpp"

namespace i01 { namespace core {

struct MIC {
#define MARKET(mic, description) , mic
    enum class Enum : int {
        UNKNOWN       = 0
#include "MIC.def.hpp"
    , NUM_MIC
    };
#undef MARKET
    static const char *Name[(int)Enum::NUM_MIC];
    static const char *Description[(int)Enum::NUM_MIC];

    static MIC clone(const char *mic_name);
    static MIC clone(const std::uint8_t mic_idx);

    MIC() : market_(Enum::UNKNOWN) {}
    MIC(const MIC& mic_) : market_(mic_.market_) {}
    MIC(const Enum& enum_);

    inline int index() const { return static_cast<int>(market_); }
    Enum market() const { return market_; }
    void market(const Enum& m) { market_ = m; }
    const char * name() const { return Name[(int)market()]; }
    const char * description() const { return Description[(int)market()]; }

    friend inline bool operator==(const MIC &a, const MIC &b) {
        return a.market_ == b.market_;
    }

    inline bool operator<(const MIC& b) const {
        return std::string(name()) < std::string(b.name());
    }

private:
    Enum market_;
};

inline bool operator!=(const MIC &a, const MIC &b)
{
    return !(a==b);
}

inline std::ostream & operator<<(std::ostream &os, const MIC &m)
{
    return os << m.name();
}

typedef MIC::Enum MICEnum;

} }
