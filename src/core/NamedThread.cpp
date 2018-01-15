#include <sys/prctl.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>

#include <boost/lexical_cast.hpp>

#include <i01_core/Config.hpp>
#include <i01_core/NamedThread.hpp>

namespace i01 { namespace core {

NamedThreadBase::NamedThreadBase()
    : m_id()
    , m_state(State::UNINITIALIZED)
    , m_retval(nullptr)
    , m_throw_exceptions(true)
{
}

NamedThreadBase::NamedThreadBase(const std::string& thread_name, bool throw_exceptions)
    : NamedThreadBase()
{
    m_name = thread_name;
    m_throw_exceptions = throw_exceptions;
}

NamedThreadBase::~NamedThreadBase()
{
    if (m_state.compare_and_swap(State::STOPPING, State::ACTIVE) == State::ACTIVE) {
        ::usleep(1000); // wait 1ms for thread to die.
        if (m_state != State::STOPPED) {
            ::sleep(2); // wait 2 seconds, then kill the thread forcefully.
            cancel(/* blocking = */ false);
        }
    }
}

bool NamedThreadBase::name(const std::string& new_name)
{
    m_name = new_name;
    if (active()) {
        // Alternatively, use prctl:
        //return (0 == ::prctl(PR_SET_NAME, m_name.substr(0,16).c_str()));
        // Thread names are limited to 16 characters, but keep the longer name
        // internally for debugging / log output / etc.
        return (0 == ::pthread_setname_np(m_id, m_name.substr(0, 16).c_str()));
    }
    return false;
}

void NamedThreadBase::shutdown(bool blocking) {
    if (m_state.compare_and_swap(State::STOPPING, State::ACTIVE) == State::ACTIVE) {
        if (blocking) {
            join();
        }
    }
}

void NamedThreadBase::cancel(bool blocking, std::uint32_t timeout_us) {
    shutdown(/* blocking = */ false);
    if (State::STOPPING == m_state.compare_and_swap(State::CANCELED, State::STOPPING))
    {
        if (timeout_us > 0)
            ::usleep(timeout_us);
        ::pthread_cancel(m_id);
        if (blocking)
            join();
    }
}

bool NamedThreadBase::join() {
    if (!::pthread_equal(::pthread_self(), m_id))
        if (0 == ::pthread_join(m_id, &m_retval))
            return true;

    return false;
}

bool NamedThreadBase::set_cpu_affinity(const CPUCoreList& cpumask) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (auto i : cpumask) {
        CPU_SET(i, &cpuset);
    }
    if (0 == ::pthread_setaffinity_np(m_id, sizeof(cpuset), &cpuset)) {
        return true;
    }
    return false;
}

bool NamedThreadBase::set_cpu_affinity(const core::Config::storage_type& cfg)
{
    auto keys(cfg.get_key_prefix_set());

    if (keys.find("affinity") != keys.end()) {
        // if we have an affinity vector it will look like this
        // affinity.<X> = 1
        // affinity.<Y> = 2
        // ... we don't care what <X> & <Y> are since it's a set, but we do
        // expect the value to be a positive integer

        // leave off "." b/c we also want to catch the scalar affinity
        // = <N>, and we don't care what the sub keys actually are

        auto aff(cfg.copy_prefix_domain("affinity"));

        auto corelist = core::NamedThreadBase::CPUCoreList{};

        for (const auto& kv : *aff) {
            try {
                auto cpu = boost::lexical_cast<int>(kv.second);
                corelist.push_back(cpu);
                std::cerr << "NamedThreadBase::set_cpu_affinity:" << name() << ": " << cpu << std::endl;
            } catch (const boost::bad_lexical_cast&) {
                std::cerr << "NamedThreadBase::set_cpu_affinity:" << name() << ": read bad cpu core in affinity list: " << kv.second << std::endl;
            }
        }
        if (corelist.size()) {
            if (!set_cpu_affinity(corelist)) {
                std::cerr << "NamedThreadBase:set_cpu_affinity:" << name() << ": error setting cpu affinity: " << strerror(errno) << std::endl;
                return false;
            }
        }
    }

    return true;
}

bool NamedThreadBase::set_scheduler(int prio, const std::string& policystr) {
    int policy = SCHED_FIFO;
    if (policystr == "fifo")
        policy = SCHED_FIFO;
    else if (policystr == "rr")
        policy = SCHED_RR;
    else if (policystr == "other")
        policy = SCHED_OTHER;
    else if (policystr == "batch")
        policy = SCHED_BATCH;
    else if (policystr == "idle")
        policy = SCHED_IDLE;
    else
        return false;
    return set_scheduler(prio, policy);
}

bool NamedThreadBase::set_scheduler(int prio, int policy) {
    struct sched_param tmp { .sched_priority = prio };
    return (0 == ::pthread_setschedparam(m_id, policy, &tmp));
}

std::ostream& operator<<(std::ostream& os, const NamedThreadBase::State& s)
{
    switch (s) {
    case NamedThreadBase::State::UNINITIALIZED:
        os << "UNINITIALIZED";
        break;
    case NamedThreadBase::State::STARTING:
        os << "STARTING";
        break;
    case NamedThreadBase::State::ACTIVE:
        os << "ACTIVE";
        break;
    case NamedThreadBase::State::STOPPING:
        os << "STOPPING";
        break;
    case NamedThreadBase::State::STOPPED:
        os << "STOPPED";
        break;
    case NamedThreadBase::State::CANCELED:
        os << "CANCELED";
        break;
    case NamedThreadBase::State::EXCEPTION:
        os << "EXCEPTION";
        break;
    default:
        break;
    }
    return os;
}

} }
