#pragma once

/* See http://www.mcs.anl.gov/~kazutomo/rdtsc.html
 * ...and the Intel Software Optimization guides.
 */

#include <cstdint>
#include <time.h>
#include <utility>
#include <iostream>

namespace i01 { namespace core {

typedef struct timespec POSIXTimeSpec;
typedef POSIXTimeSpec TimeSpec;

class Timestamp : public TimeSpec
{
public:
  Timestamp() : TimeSpec{0,0} { }
  // Timestamp(const POSIXTimeSpec& pts) : TimeSpec{pts.tv_sec, pts.tv_nsec} {}
  Timestamp(const time_t s, const long n)
  {
    const decltype(tv_nsec) m = n % 1000000000ULL;
    tv_sec = s + ((n - m) / 1000000000ULL);
    tv_nsec = m;
  }

  Timestamp operator+(const Timestamp& t) const
  {
    Timestamp sum(tv_sec + t.tv_sec,
                  tv_nsec + t.tv_nsec);
    if (sum.tv_nsec > 1000000000)
    {
      sum.tv_sec++;
      sum.tv_nsec -= 1000000000;
    }
    return sum;
  }

  Timestamp& operator+=(const Timestamp& t)
  {
    this->tv_sec += t.tv_sec;
    this->tv_nsec += t.tv_nsec;
    if (this->tv_nsec > 1000000000)
    {
      this->tv_sec--;
      this->tv_nsec -= 1000000000;
    }
    return *this;
  }

  Timestamp operator-(const Timestamp& t) const
  {
    Timestamp diff(tv_sec - t.tv_sec, 0);
    if (t.tv_nsec > tv_nsec)
    {
      diff.tv_sec--;
      diff.tv_nsec = tv_nsec + 1000000000 - t.tv_nsec;
    }
    else
    {
      diff.tv_nsec = tv_nsec - t.tv_nsec;
    }
    return diff;
  }

  Timestamp& operator-=(const Timestamp& t)
  {
    this->tv_sec -= t.tv_sec;
    if (t.tv_nsec > this->tv_nsec)
    {
      this->tv_sec--;
      this->tv_nsec = this->tv_nsec + 1000000000 - t.tv_nsec;
    }
    else
    {
      this->tv_nsec -= t.tv_nsec;
    }
    return *this;
  }

  Timestamp get_midnight(const std::uint64_t& add_ns = 0) const
  {
      Timestamp ts(tv_sec - (tv_sec % 86400), add_ns % 1000000000ULL);
      ts.tv_sec += ((add_ns - ts.tv_nsec) / 1000000000ULL);
      return ts;
  }
  
  std::uint64_t seconds_since_midnight() const {
    return (tv_sec % (86400ULL));
  }

  std::uint64_t milliseconds_since_midnight() const {
    return (tv_sec*1000ULL + tv_nsec/1000000ULL) % (86400ULL * 1000ULL);
  }

    std::uint64_t nanoseconds_since_midnight() const {
        return (tv_sec*1000000000ULL + tv_nsec) % (86400ULL * 1000000000ULL);
    }

  static const Timestamp now()
  {
    Timestamp ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    return ts;
  }

  static bool now(Timestamp& ts)
  {
    return (::clock_gettime(CLOCK_REALTIME, &ts) == 0);
  }

  static time_t to_local_midnight_seconds_since_epoch_slow(const time_t& tv_sec_utc);
  time_t local_midnight_seconds_since_epoch_slow()
  {
    return to_local_midnight_seconds_since_epoch_slow(tv_sec);
  }

  static std::uint64_t to_ns_since_cached_local_midnight(const Timestamp& ts_utc);
  std::uint64_t nanoseconds_since_midnight_local_cached() const
  {
    return to_ns_since_cached_local_midnight(*this);
  }

  static const Timestamp& last_event(const Timestamp& newts = {0,0});

    friend std::ostream & operator<<(std::ostream &o, const Timestamp &t);
};

inline bool operator==(const Timestamp &a, const Timestamp &b) {
    return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
}

inline bool operator!=(const Timestamp& a, const Timestamp& b) {
    return !(a == b);
}

inline bool operator<(const Timestamp &lhs, const Timestamp &rhs) {
    return lhs.tv_sec < rhs.tv_sec || (lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec < rhs.tv_nsec);
}

inline bool operator<=(const Timestamp &lhs, const Timestamp &rhs) {
    return lhs == rhs || lhs < rhs;
}

inline bool operator>(const Timestamp &lhs, const Timestamp &rhs) {
    return !(lhs <= rhs);
}

inline bool operator>=(const Timestamp &lhs, const Timestamp &rhs) {
    return !(lhs < rhs);
}

inline std::ostream & operator<<(std::ostream &o, const Timestamp &t) {
    // return o << (unsigned int) t.tv_sec << "." << (std::uint32_t) t.tv_nsec;
    char buf[64];
    snprintf(buf, 64, "%ld.%09ld", t.tv_sec, t.tv_nsec);
    return o << buf;
}

#if defined(__x86_64__)
// CPUID instruction for instruction serialization.
inline void cpuid(int code, std::uint32_t *a , std::uint32_t *d)
{
    __asm__ __volatile__ ("cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ecx", "ebx");
}

/// RDTSC instruction for reading the TSC at the start of an interval.
inline std::uint64_t rdtsc()
{
    std::uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((std::uint64_t)lo | ((std::uint64_t)hi << 32));
}

/// RDTSC instruction for reading the TSC at the end of an interval.
inline std::uint64_t rdtscp()
{
    std::uint32_t hi, lo;
    __asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi));
    return ((std::uint64_t)lo | ((std::uint64_t)hi << 32));
}

/// Combined CPUID; RDTSC to start an interval.
inline std::uint64_t cpuid_rdtsc()
{
    std::uint32_t hi, lo;
    __asm__ __volatile__ (
        "cpuid\n\t"
        "rdtsc\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
      : "=r"(hi), "=r"(lo)
      :
      : "%rax", "%rbx", "%rcx", "%rdx");
    return ((std::uint64_t)lo | ((std::uint64_t)hi << 32));
}

/// Combined RDTSCP; CPUID to end an interval.
inline std::uint64_t rdtscp_cpuid()
{
    std::uint32_t hi, lo;
    __asm__ __volatile__ (
        "rdtscp\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        "cpuid\n\t"
      : "=r"(hi), "=r"(lo)
      :
      : "%rax", "%rbx", "%rcx", "%rdx");
    return ((std::uint64_t)lo | ((std::uint64_t)hi << 32));
}
#elif defined(__i386__)
inline std::uint64_t rdtsc()
{
    std::uint64_t x;
    __asm__ __volatile__ (".byte 0x0f, 0x31" : "=A"(x));
    return x;
}
#elif defined(__powerpc__)
inline std::uint64_t rdtsc()
{
    std::uint32_t hi, lo, tmp;
    __asm__ __volatile__(
        "0:\n"
        "\tmftbu\t%0\n"
        "\tmftb\t%1\n"
        "\tmftbu\t%2\n"
        "\tcmpw\t%2,%0\n"
        "\tbne\t0b\n"
      : "=r"(hi), "=r"(lo), "=r"(tmp));
    return ((std::uint64_t)lo | ((std::uint64_t)hi << 32));
}
#else
#error "Unsupported architecture."
#endif

#if defined(__x86_64__)
class MonotonicTimer
{
public:
  MonotonicTimer()
   : start_(0), stop_(0) {}

  /// Returns the current TSC value and starts the interval.
  inline std::uint64_t start() { return start_ = stop_ = cpuid_rdtsc(); }
  /// Returns the current TSC value and ends the interval.
  inline std::uint64_t stop() { return stop_ = rdtscp_cpuid(); }
  /// Returns the TSC count for the interval between start() and
  //  stop().
  inline std::uint64_t interval() { return stop_ - start_; }
protected:
  std::uint64_t start_; //< TSC at start of interval.
  std::uint64_t stop_; //< TSC at end of interval.
};
#else
#error "Unsupported architecture for MonotonicTimer."
#endif

}}
