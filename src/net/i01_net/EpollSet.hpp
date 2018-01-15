#pragma once

#include <stdint.h>
#include <cstddef>
#include <sys/epoll.h>

#include <i01_core/FD.hpp>

#include <i01_core/macro.hpp>

namespace i01 { namespace net {

    class EpollSet {
        i01::core::EpollFD m_epollfd;
        int m_num_events;
        int m_maxevents;
        ::epoll_event* m_events;

        EpollSet(EpollSet const&) = delete;
        EpollSet& operator=(EpollSet const&) = delete;
    public:
        EpollSet(std::size_t nevents = 64, bool close_on_exec = false);
        ~EpollSet();

        bool add(int sock, uint64_t user_data_, uint32_t events = EPOLLIN);
        bool modify(int sock, uint64_t user_data_, uint32_t events = EPOLLIN);
        bool remove(int sock);
        /// Returns the number of events (possibly 0) or negative on error
        /// @param timeout_milliseconds wait forever = -1, don't wait = 0
        inline int wait(int timeout_milliseconds = -1);

        typedef epoll_event const* const_iterator;
        const_iterator begin() const { return m_events; }
        const_iterator end()   const { return m_events + m_num_events;   }

        i01::core::EpollFD& fd() { return m_epollfd; }
    };

    int EpollSet::wait(int timeout_milliseconds) {
        int ret = m_epollfd.epoll_wait(m_events, m_maxevents, timeout_milliseconds);
        if(LIKELY(ret >= 0)) {
            m_num_events = ret;
        }
        return ret;
    }

} }
