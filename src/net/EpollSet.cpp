
#include <errno.h>

#include <cstring>
#include <cassert>
#include <stdexcept>

#include <i01_net/EpollSet.hpp>

namespace i01 { namespace net {

    EpollSet::EpollSet(std::size_t nevents, bool close_on_exec)
      : m_epollfd(close_on_exec ? EPOLL_CLOEXEC : 0)
      , m_num_events(0)
      , m_maxevents(static_cast<int>(nevents))
      , m_events(new epoll_event[nevents])
    {
        if(m_epollfd.fd() < 0) {
            throw std::runtime_error(
                std::string(__PRETTY_FUNCTION__)
              + " EpollFD failed: " + strerror(errno));
        }
        assert(nevents > 0);
    }

    EpollSet::~EpollSet() {
        delete[] m_events;
    }

    bool EpollSet::add(int sock, uint64_t epoll_data_, uint32_t events) {
        assert(sock > 0);
        struct epoll_event event;
        event.events = events;
        event.data.u64 = epoll_data_;
        int ret = m_epollfd.epoll_ctl(EPOLL_CTL_ADD, sock, &event);
        return ret != -1;
    }

    bool EpollSet::modify(int sock, uint64_t epoll_data_, uint32_t events) {
        assert(sock > 0);
        struct epoll_event event;
        event.events = events;
        event.data.u64 = epoll_data_;
        int ret = m_epollfd.epoll_ctl(EPOLL_CTL_MOD, sock, &event);
        return ret != -1;
    }

    bool EpollSet::remove(int sock) {
        assert(sock > 0);
        struct epoll_event event;
        int ret = m_epollfd.epoll_ctl(EPOLL_CTL_DEL, sock, &event);
        return ret != -1;
    }

} }
