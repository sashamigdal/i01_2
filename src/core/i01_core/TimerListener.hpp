#pragma once

#include <cstdint>

namespace i01 { namespace core {

    class Timestamp;
    /// Listener base class for Timer events.
    class TimerListener {
        public:
            virtual void on_timer( const core::Timestamp&, void * userdata
                                 , std::uint64_t iter) = 0;
    };

} }
