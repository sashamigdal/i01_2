#include <errno.h>

#include <boost/lexical_cast.hpp>

#include <i01_core/macro.hpp>
#include <i01_core/EventListener.hpp>
#include <i01_core/FD.hpp>
#include <i01_core/Log.hpp>
#include <i01_core/SignalListener.hpp>
#include <i01_core/TimerListener.hpp>

#include <i01_net/EventPoller.hpp>
#include <i01_net/SocketListener.hpp>

namespace i01 { namespace net {

std::ostream & operator<<(std::ostream &os, const EventType &s)
{
    switch (s) {
    case EventType::UNKNOWN:
        os << "UNKNOWN";
        break;
    case EventType::SIGNAL_FD:
        os << "SIGNAL_FD";
        break;
    case EventType::EVENT_FD:
        os << "EVENT_FD";
        break;
    case EventType::TIMER_FD:
        os << "TIMER_FD";
        break;
    case EventType::SOCKET_FD:
        os << "SOCKET_FD";
        break;
    default:
        break;
    }
    return os;
}

EventPoller::EventPoller()
    : m_last_event_ts{0,0}
    , m_active(false)
{
    m_errcount = 0;
}

EventPoller::~EventPoller()
{

}

void EventPoller::on_error(const core::Timestamp& t,
                           EventData *ed,
                           int errno_,
                           const char *msg)
{
    ++m_errcount;
    if (ed)
        ++(ed->errcount);
#ifdef _DEBUG
    std::cerr << "EventPoller::on_error(et="
              << (ed ? ed->type : EventType::UNKNOWN)
              << ", t=" << t.nanoseconds_since_midnight() << "): "
              << ::strerror(errno_) << ": " << (msg ? msg : "") << std::endl;
#endif
}

EpollEventPoller::EpollEventPoller()
    : EventPoller()
    , m_change_mutex()
    , m_eps(s_evq_size, true)
    , m_listeners()
{
}

EpollEventPoller::EpollEventPoller(const std::string& n) : EpollEventPoller()
{
    name(n);
}

EpollEventPoller::~EpollEventPoller()
{
    lockguard_type l(m_change_mutex);

    for (auto& ed : m_listeners) {
        if (m_eps.remove(ed->fd.fd()))
            delete ed;
        ed = nullptr;
    }
    for (auto& ed : m_listeners) {
        if (ed) {
            std::cerr << "File descriptor could not be removed from EpollSet: type=" << ed->type << ", fd=" << ed->fd.fd() << std::endl;
            delete ed;
        }
    }
}

bool EpollEventPoller::add_timer( core::TimerListener& listener
                                , EventUserData userdata
                                , const core::Timestamp& start
                                , const core::Timestamp& interval)
{
    lockguard_type l(m_change_mutex);
    struct itimerspec its{
        .it_interval = interval,
        .it_value = start
    };
    EventData * ed = new EventData{
        .type = EventType::TIMER_FD,
        .managed = true,
        .fd = core::FDBase(::timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC)),
        .listener = { .timer = &listener },
        .userdata = userdata
    };
    if (ed && ed->fd.valid()) {
        ::timerfd_settime(ed->fd.fd(), 0, &its, nullptr);
        if (m_eps.add(ed->fd.fd(),
                    reinterpret_cast<std::uint64_t>(ed),
                    EPOLLIN | EPOLLET)) {
            m_listeners.push_back(ed);
            return true;
        }
    }
    delete ed;
    return false;
}


bool EpollEventPoller::add_socket( SocketListener& listener
                                 , EventUserData userdata
                                 , int fd, bool managed)
{
    lockguard_type l(m_change_mutex);
    EventData * ed = new EventData{
        .type = EventType::SOCKET_FD,
        .managed = managed,
        .fd = core::SocketFD(fd),
        .listener = { .socket = &listener },
        .userdata = userdata
    };

    if (ed && ed->fd.valid()) {
        ed->fd.fcntl(F_SETFL, O_NONBLOCK);
        if (m_eps.add(ed->fd.fd(),
                    reinterpret_cast<std::uint64_t>(ed),
                    EPOLLIN | EPOLLRDHUP)) {
            m_listeners.push_back(ed);
            return true;
        }
    }
    delete ed;
    return false;
}

bool EpollEventPoller::run()
{
    int n = m_eps.wait(0);
    if (UNLIKELY(n < 0))
        return false;
    for (auto it = m_eps.begin(); it != m_eps.end(); ++it) {
        EventData * e = reinterpret_cast<EventData*>(it->data.ptr);
        e->last_event_ts = core::Timestamp::now();
        switch (e->type) {
        case EventType::SIGNAL_FD: {
            struct signalfd_siginfo fdsi;
            if (sizeof(fdsi) == e->fd.read(&fdsi, sizeof(fdsi))) {
                e->listener.signal->on_signal(e->last_event_ts, e->userdata, fdsi);
            } else {
                on_error(e->last_event_ts, e, errno, "signalfd read failed");
            }
        } break;
        case EventType::EVENT_FD: {
            std::uint64_t val = 0;
            if (sizeof(val) == e->fd.read(&val, sizeof(val))) {
                e->listener.event->on_event(e->last_event_ts, e->userdata, val);
            } else {
                on_error(e->last_event_ts, e, errno, "eventfd read failed");
            }
        } break;
        case EventType::TIMER_FD: {
            std::uint64_t val = 0;
            if (sizeof(val) == e->fd.read(&val, sizeof(val))) {
                e->listener.timer->on_timer(e->last_event_ts, e->userdata, val);
            } else {
                on_error(e->last_event_ts, e, errno, "timerfd read failed");
            }
        } break;
        case EventType::SOCKET_FD: {
            if (it->events & EPOLLIN) {
                char buf[2048]{0}; // TODO: pool allocated, zero copy
                ssize_t m = ::recv(e->fd.fd(), buf, 2048, 0); // TODO
                if (LIKELY(m > 0)) {
#ifdef _DEBUG
                    if (m >= (ssize_t)sizeof(buf)) {
                        std::cerr << "EventPoller: read buf bytes in callback " << m << std::endl;
                    }
#endif
                    e->listener.socket->on_recv(e->last_event_ts, e->userdata, (std::uint8_t *)buf, m); // TODO
                } else if (m == 0) {
                    e->listener.socket->on_peer_disconnect(e->last_event_ts, e->userdata);
                    // TODO: remove from epollset, close fd if managed,
                    // delete eventdata, remove listener...
                    {
                        lockguard_type l(m_change_mutex);
                        m_eps.remove(e->fd.fd());
                        auto it2 = std::find(m_listeners.begin(), m_listeners.end(), e);
                        if (it2 != m_listeners.end())
                            *it2 = nullptr;
                    }
                } else {
                    on_error(e->last_event_ts, e, errno, "socketfd read failed");
                }
            }
        } break;
        case EventType::UNKNOWN:
        default:
            std::cerr << "Unknown event type received." << std::endl;
            break;
        }
    }
    return true;
}

void* EpollEventPoller::process()
{
    if (!run()) {
        return (void*)1;
    } else {
        return nullptr;
    }
}

// bool EpollEventPoller::set_affinity(const core::Config::storage_type& cfg)
// {
//     auto my_cfg(cfg.copy_prefix_domain(name() + "."));
//     // now configure

//     return set_cpu_affinity(*my_cfg);

//     // auto keys(my_cfg->get_key_prefix_set());

//     // if (keys.find("affinity") != keys.end()) {
//     //     // if we have an affinity vector it will look like this
//     //     // affinity.<X> = 1
//     //     // affinity.<Y> = 2
//     //     // ... we don't care what <X> & <Y> are since it's a set, but we do
//     //     // expect the value to be a positive integer

//     //     // leave off "." b/c we also want to catch the scalar affinity
//     //     // = <N>, and we don't care what the sub keys actually are

//     //     auto aff(my_cfg->copy_prefix_domain("affinity"));

//     //     auto corelist = core::NamedThreadBase::CPUCoreList{};

//     //     for (const auto& kv : *aff) {
//     //         try {
//     //             auto cpu = boost::lexical_cast<int>(kv.second);
//     //             corelist.push_back(cpu);
//     //             std::cout << "EpollEventPoller: " << name() << " " << cpu << std::endl;
//     //         } catch (const boost::bad_lexical_cast&) {
//     //             std::cerr << "EpollEventPoller: " << name() << ": read bad cpu core in affinity list: " << kv.second << std::endl;
//     //         }
//     //     }
//     //     if (corelist.size()) {
//     //         if (!set_cpu_affinity(corelist)) {
//     //             std::cerr << "EpollEventPoller::set_affinity " << name() << ": error setting cpu affinity: " << strerror(errno) << std::endl;
//     //             return false;
//     //         }
//     //     }
//     // }

//     // return true;
// }

} }
