#pragma once

#include "pcap-int.h"

#include <string>
#include <i01_core/Time.hpp>

namespace i01 { namespace apps { namespace reorderpcap {

struct Record {
    i01::core::Timestamp ts;
    std::int64_t seqnum;
    pcap_pkthdr hdr;
    void* buf;
};
inline bool operator<(const Record& lhs, const Record& rhs) { return (lhs.ts < rhs.ts) || (lhs.ts == rhs.ts && (lhs.seqnum < rhs.seqnum)); }
inline bool operator>(const Record& lhs, const Record& rhs) { return (lhs.ts > rhs.ts) || (lhs.ts == rhs.ts && (lhs.seqnum > rhs.seqnum)); }


} } }
