#include <i01_core/PMQOSInterface.hpp>

namespace i01 { namespace core {

const char * const CPUDMALatency::path = "/dev/cpu_dma_latency";
const CPUDMALatency::value_type CPUDMALatency::default_value = 0;
const CPUDMALatency::value_type CPUDMALatency::intolerable_value = -1;

} }
