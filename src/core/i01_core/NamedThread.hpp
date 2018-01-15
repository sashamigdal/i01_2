#pragma once

#include <sys/prctl.h>
#include <pthread.h>
#include <sched.h>
#include <cxxabi.h>

#include <cstdint>
#include <string>
#include <iosfwd>
#include <vector>
#include <thread>
#include <atomic>

#include <type_traits>

#include <i01_core/macro.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/Config.hpp>

namespace i01 { namespace core {

/// ABC wrapper for a named thread.
class NamedThreadBase {
public:
    enum class State : std::uint8_t {
        UNINITIALIZED = 0
      , STARTING      = 1
      , ACTIVE        = 2
      , STOPPING      = 3
      , STOPPED       = 4
      , CANCELED      = 5
      , EXCEPTION     = 6
    };
    friend std::ostream& operator<<(std::ostream& os, const State& s);
    typedef std::vector<int> CPUCoreList;

    NamedThreadBase();
    explicit NamedThreadBase(const std::string& thread_name, bool throw_exceptions = true);

    virtual ~NamedThreadBase();

    pthread_t& tid() { return m_id; }
    const pthread_t& tid() const { return m_id; }
    /// Returns the thread name.
    const std::string& name() const { return m_name; }
    /// Sets the thread name.
    bool name(const std::string& new_name);

    State state() const { return m_state; }
    bool active() const { return m_state == State::ACTIVE; }
    void * retval() const { return m_retval; }

    bool set_cpu_affinity(const CPUCoreList& cpumask);
    bool set_cpu_affinity(const core::Config::storage_type& cfg);
    bool set_scheduler(int prio, const std::string& policy = "fifo");
    bool set_scheduler(int prio, int policy = SCHED_FIFO);

    template <typename Worker>
    inline bool spawn();

    void shutdown(bool blocking = false);
    /// Warning: this sleeps for 1 ms while attempting to shut down the thread
    /// gracefully, before cancelling it.
    void cancel(bool blocking = false, std::uint32_t timeout_us = 1000);
    bool join();

    virtual void pre_process() {}
    virtual void *process() = 0;
    virtual void post_process() {}

    bool operator==(const NamedThreadBase& o) { return 0 != ::pthread_equal(tid(), o.tid()); }

protected:
    NamedThreadBase(const NamedThreadBase&) = delete;
    NamedThreadBase& operator=(const NamedThreadBase&) = delete;

    void state(const State& new_state) { m_state = new_state; }

    template <typename Worker>
    inline static void *run(void* id);

protected:
    pthread_t m_id;
    std::string m_name;
    Atomic<State> m_state;
    void *m_retval;
    bool m_throw_exceptions;
};

/// Worker must have three member functions: pre_process, process, and
/// post_process.  At the very minimum, it must define process(), which is a
/// pure virtual in NamedThread.  When process() returns non-zero, the thread
/// will exit.
template <typename Worker>
class NamedThread : public NamedThreadBase {
public:
    NamedThread() : NamedThreadBase() {}
    explicit NamedThread(const std::string& thread_name, bool throw_exceptions = true)
        : NamedThreadBase(thread_name, throw_exceptions) {}

    bool spawn();

    virtual void pre_process() override {}
    virtual void *process() override = 0;
    virtual void post_process() override {}

private:
    NamedThread(const NamedThread&) = delete;
    NamedThread& operator=(const NamedThread&) = delete;
};

template <typename Worker>
inline bool NamedThreadBase::spawn() {
    if (m_state.compare_and_swap(State::STARTING, State::UNINITIALIZED) == State::UNINITIALIZED) {
        if (0 == ::pthread_create(&m_id, nullptr, &NamedThreadBase::run<Worker>, static_cast<void*>(this)))
            return true;
    }
    return false;
}

template <typename Worker>
inline void * NamedThreadBase::run(void * id) {
    static_assert(std::is_base_of<NamedThreadBase, Worker>::value, "Worker must be derived from NamedThread<Worker>.");

    NamedThread<Worker> * t = reinterpret_cast<NamedThread<Worker> *>(id);
    if (!(t->name().empty())) {
        auto cs = Config::instance().get_shared_state()->copy_prefix_domain("threads." + t->name() + ".");
        int cpu = -1;
        if (cs->get("cpu_affinity", cpu) && !(t->set_cpu_affinity(CPUCoreList{cpu})))
            std::cerr << "NamedThread " << t->name() << " failed to set cpu affinity." << std::endl;

        int priority = -1;
        std::string policy;
        if (cs->get("sched_priority", priority) && cs->get("sched_policy", policy) && !(t->set_scheduler(priority, policy)))
            std::cerr << "NamedThread " << t->name() << " failed to set scheduler." << std::endl;
        ::prctl(PR_SET_NAME, t->m_name.substr(0,16).c_str());
        ::pthread_setname_np(t->m_id, t->m_name.substr(0,16).c_str());
    }
    {
        Worker * w = static_cast<Worker *>(t);
        if (w == nullptr)
            return nullptr;
        t->m_state = State::ACTIVE;
        try {
            w->pre_process();
            while (t->m_state == State::ACTIVE)
                if (UNLIKELY(nullptr != (t->m_retval = w->process())))
                    break;
            t->m_state = State::STOPPING;
            w->post_process();
        } catch (abi::__forced_unwind& e) {
            // pthread_cancel + C++ = fun times.  Consider std::thread.
            t->m_state = State::CANCELED;
            if (t->m_throw_exceptions)
                throw;
        } catch (std::exception& e) { // exceptions caught should count as cancellation.
            t->m_state = State::EXCEPTION;
            std::cerr << "Exception caught in thread " << t->name() << ": " << e.what() << std::endl;
            if (t->m_throw_exceptions)
                throw;
        } catch (...) { // exceptions caught should count as cancellation.
            t->m_state = State::EXCEPTION;
            std::cerr << "Exception caught in thread " << t->name() << std::endl;
            if (t->m_throw_exceptions)
                throw;
        }
    }
    t->m_state = State::STOPPED;
    std::atomic_thread_fence(std::memory_order_acquire);
    return t->m_retval;
}

template <typename Worker>
bool NamedThread<Worker>::spawn()
{
    return NamedThreadBase::spawn<Worker>();
}

/// Special NamedThread that'll run an std::function (e.g. lambda).
/// It is safe to swap out the function while it is running, however there is
/// a risk of livelock if the function does not return frequently or blocks,
/// since a spinlock is held during use of the function.
class NamedStdFunctionThread : public NamedThread<NamedStdFunctionThread> {
public:
    typedef std::function<void*()> function_type;

    NamedStdFunctionThread() : NamedThread() {}
    explicit NamedStdFunctionThread(const std::string& thread_name, bool throw_exceptions = true)
        : NamedThread(thread_name, throw_exceptions) {}
    explicit NamedStdFunctionThread(const std::string& thread_name, function_type func, bool throw_exceptions = true)
        : NamedThread(thread_name, throw_exceptions), m_process(func) {}

    bool spawn() {
        LockGuard<SpinMutex> l(m_func_mutex);
        if (!m_process)
            return false;
        return NamedThread::spawn();
    }

    bool insert_function(function_type func) {
        LockGuard<SpinMutex> l(m_func_mutex);
        if (!!m_process)
            return false;
        m_process = std::move(func);
        return true;
    }

    bool emplace_function(function_type&& func) {
        LockGuard<SpinMutex> l(m_func_mutex);
        if (!!m_process)
            return false;
        m_process = std::move(func);
        return true;
    }

    void * process() override final {
        LockGuard<SpinMutex> l(m_func_mutex);
        if (LIKELY(!!m_process))
            return m_process();
        throw std::runtime_error("NamedStdFunctionThread " + name() + " spawned without a callback function.");
        shutdown();
        return nullptr;
    }

protected:
    core::SpinMutex m_func_mutex;
    function_type m_process;

private:
    NamedStdFunctionThread(const NamedStdFunctionThread&) = delete;
    NamedStdFunctionThread& operator=(const NamedStdFunctionThread&) = delete;
};

} }
