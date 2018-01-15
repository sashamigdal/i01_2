#pragma once

#include <queue>

#include <i01_core/macro.hpp>
#include <i01_core/FD.hpp>
#include <i01_net/EpollSet.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/Config.hpp>
#include <i01_core/NamedThread.hpp>

namespace i01 { namespace core {
class TimerListener;
class EventListener;
class SignalListener;
}}

namespace i01 { namespace net {

class SocketListener;

    typedef void * EventUserData;

    enum class EventType {
        UNKNOWN               = 0
      , SIGNAL_FD             = 1
      , EVENT_FD              = 2
      , TIMER_FD              = 3
      , SOCKET_FD             = 4
    };
    std::ostream & operator<<(std::ostream &os, const EventType &s);


    struct EventData {
        EventType type;
        bool managed;
        core::FDBase fd;
        union {
            void * raw;
            core::SignalListener * signal;
            core::EventListener * event;
            core::TimerListener * timer;
            SocketListener * socket;
        } listener;
        EventUserData userdata;
        core::Timestamp last_event_ts;
        core::Atomic<std::uint64_t> errcount;

        ~EventData() {
            if (managed) {
                switch (type) {
                    case EventType::SIGNAL_FD:
                    case EventType::EVENT_FD:
                    case EventType::TIMER_FD:
                    case EventType::SOCKET_FD:
                        if (fd.valid())
                            fd.close();
                    case EventType::UNKNOWN:
                    default:
                        break;
                }
            }
        }
    };

    class EventPoller {
    public:
        EventPoller();
        virtual ~EventPoller();

        virtual bool add_timer( core::TimerListener&
                              , EventUserData
                              , const core::Timestamp&
                              , const core::Timestamp&) = 0;
        virtual bool add_socket( SocketListener&
                               , EventUserData
                               , int fd
                               , bool managed) = 0;

        const core::Timestamp& time() { return m_last_event_ts; }

        virtual bool run() = 0;
    protected:
        core::Timestamp m_last_event_ts;
        core::Atomic<bool> m_active;
        core::Atomic<std::uint64_t> m_errcount;

        virtual void on_error(const core::Timestamp& t, EventData *ed = nullptr, int errno_ = 0, const char *msg = nullptr);
    };

    class EpollEventPoller : public EventPoller, public core::NamedThread<EpollEventPoller> {
        core::RecursiveMutex m_change_mutex;
        typedef core::LockGuard<decltype(m_change_mutex)> lockguard_type;

        net::EpollSet m_eps;
        std::vector<EventData *> m_listeners;

        static const std::uint32_t s_evq_size = 64;

    public:
        EpollEventPoller();
        EpollEventPoller(const std::string&);
        virtual ~EpollEventPoller();

        virtual bool add_timer( core::TimerListener&
                              , EventUserData
                              , const core::Timestamp&
                              , const core::Timestamp&) override final;
        virtual bool add_socket( SocketListener&
                               , EventUserData
                               , int fd
                               , bool managed = true) override final;

        // bool set_affinity(const core::Config::storage_type&);

        virtual bool run() override final;
        virtual void* process() override;

    public:
        static EpollEventPoller * create_and_config(const std::string& name, const core::Config::storage_type& cfg);
    };

} }
