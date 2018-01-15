#include <cstring>

#include <i01_core/Time.hpp>

namespace i01 { namespace core {

time_t Timestamp::to_local_midnight_seconds_since_epoch_slow(const time_t& tv_sec_utc)
{
    struct tm lt;
    ::memset(&lt, 0, sizeof(lt));
    if (::localtime_r(&tv_sec_utc, &lt) == NULL)
        return 0;
    return (lt.tm_hour * 3600 + lt.tm_min * 60 + lt.tm_sec) % 86400;
}

std::uint64_t Timestamp::to_ns_since_cached_local_midnight(const Timestamp& ts_utc)
{
     static time_t cached_offset_sec((to_local_midnight_seconds_since_epoch_slow(ts_utc.tv_sec) - ts_utc.tv_sec + 86400) % 86400);
     return (std::uint64_t(ts_utc.tv_sec + cached_offset_sec)*1000000000ULL + ts_utc.tv_nsec) % (86400ULL * 1000000000ULL);
}


const Timestamp& Timestamp::last_event(const Timestamp& newts)
{
      static Timestamp ts{0,0};
      if (newts > ts)
          ts = newts;
      return ts;
}

} }
