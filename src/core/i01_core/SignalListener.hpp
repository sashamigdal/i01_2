#pragma once

#include <sys/signalfd.h>
#include <i01_core/Time.hpp>

namespace i01 { namespace core {

    /// Listener base class for signals.
    class SignalListener {
        public:
            virtual void on_signal( const core::Timestamp&
                                  , void * userdata
                                  , const struct signalfd_siginfo&) = 0;
    };

} }
