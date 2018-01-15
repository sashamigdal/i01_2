#pragma once

#include <i01_core/Time.hpp>

namespace i01 { namespace core {

    /// Listener base class for eventfd.
    class EventListener {
        public:
            virtual void on_event( const core::Timestamp&
                                 , void * userdata
                                 , std::uint64_t value) = 0;
    };

} }
