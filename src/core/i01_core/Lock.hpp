#pragma once

#include <mutex>

#include <tbb/atomic.h>
#include <tbb/spin_mutex.h>
#include <tbb/recursive_mutex.h>
#include <tbb/spin_rw_mutex.h>
#include <tbb/null_mutex.h>
#include <tbb/null_rw_mutex.h>

namespace i01 { namespace core {

    template<typename T>
    using Atomic = tbb::atomic<T>;

    template <typename Mutex>
    using LockGuard = std::lock_guard<Mutex>;

    template <typename Mutex>
    using ScopedLock = typename Mutex::scoped_lock;

    typedef tbb::spin_mutex       SpinMutex;
    typedef tbb::recursive_mutex  RecursiveMutex;
    typedef tbb::spin_rw_mutex    SpinRWMutex;
    typedef tbb::null_mutex       NullMutex;
    typedef tbb::null_rw_mutex    NullRWMutex;

}}

