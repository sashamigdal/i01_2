#pragma once

#include <i01_core/macro.hpp>

namespace i01 { namespace core {

/// Singleton class template.
/// Relies on C++11 thread-safety of static initialization.
template <typename T>
class Singleton {
protected:
    Singleton() {}
    Singleton(const Singleton&) = delete;

public:
    I01_API static T& instance() {
        static T instance_;
        return instance_;
    }
};

} }
