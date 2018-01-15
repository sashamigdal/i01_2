#include <stdexcept>

#include <i01_net/HeartBeatConnThread.hpp>

using namespace i01::core;

namespace i01 { namespace net {

HeartBeatConnThread::HeartBeatConnThread(const std::string &n, SocketListener *sl, TimerListener *tl) :
    NamedThread<HeartBeatConnThread>(n),
    m_tfd(CLOCK_REALTIME, 0),
    m_heartbeat_seconds(1),
    m_socket_listener(sl),
    m_timer_listener(tl),
    m_port(0)
{
    m_done_signal = false;
    m_reconnect_signal = false;
    m_disconnect_signal = false;
}

ssize_t HeartBeatConnThread::send(const std::uint8_t *buf, const size_t & len)
{
    return m_socket_p->send((const void *)buf, len);
}

bool HeartBeatConnThread::connect(const std::string &host, int port)
{
    m_host = host;
    m_port = port;

    if (!spawn()) {
        // means we have already spawned ...

        if (state() != State::ACTIVE) {
            return false;
        }

        m_reconnect_signal = true;
    }
    return true;
}

bool HeartBeatConnThread::disconnect()
{
    // only makes sense if we are already active
    if (state() == State::ACTIVE) {
        m_disconnect_signal = true;
        return true;
    }
    return false;
}

void HeartBeatConnThread::disconnect_and_quit()
{
    m_done_signal = true;
    shutdown(true);
}

void HeartBeatConnThread::hb_callback(std::uint32_t events, void *ptr)
{
    auto * ct = static_cast<HeartBeatConnThread *>(ptr);
    ct->handle_hb();
}

void HeartBeatConnThread::socket_callback(std::uint32_t events, void *ptr)
{
    auto * ct = static_cast<HeartBeatConnThread *>(ptr);

    if (events & EPOLLIN) {
        ct->handle_socket_data_read(events);
    }

    if (events & EPOLLOUT) {
        ct->handle_socket_data_write(events);
    }

    if (events & (EPOLLRDHUP|EPOLLERR|EPOLLHUP)) {
        ct->handle_socket_events(events);
    }
}

void HeartBeatConnThread::handle_hb()
{
    std::uint64_t tmp;
    if (read(m_tfd.fd(), &tmp, sizeof(tmp)) < 0) {
        int errn = errno;
        std::cerr << name() << ": timer read: " << strerror(errn) << std::endl;
    } else {
        // call on timer with the u64 we just read
        m_timer_listener->on_timer(Timestamp::now(), nullptr, tmp);
    }
}

void HeartBeatConnThread::handle_socket_data_read(std::uint32_t events)
{
    ssize_t ret;
    Mutex::scoped_lock lock(m_mutex);
    do {
        ret = m_socket_p->recv(m_recv_buffer.data(), sizeof(m_recv_buffer));
        if (UNLIKELY(ret < 0)) {
            int errn = m_socket_p->get_errno();
            // EINPROGRESS could be seen here...
            if (EINPROGRESS == errn) {
                // lets look for Writable events in epoll again
                if (!m_eps.modify(m_socket_p->fd(), (std::uint64_t)&HeartBeatConnThread::socket_callback, EPOLLIN|EPOLLRDHUP|EPOLLOUT)) {
                    std::string err = strerror(errno);
                    std::cerr << name() << ",ERR,HBCT resetting epoll on EINPROGRESS," << err << std::endl;
                }
            } else {
                std::string err = strerror(errn);
                std::cerr << name() << ": socket recv: " << err << std::endl;
            }
        } else if (ret > 0) {
            m_socket_listener->on_recv(core::Timestamp::now(), nullptr, m_recv_buffer.data(), ret);
        } else {
            // ret == 0 means the socket is closed ...
        }
    } while (ret == READ_BUFFER_SIZE);
}


void HeartBeatConnThread::handle_socket_data_write(std::uint32_t events)
{
    if (!m_eps.modify(m_socket_p->fd(), (std::uint64_t)&HeartBeatConnThread::socket_callback, EPOLLIN|EPOLLRDHUP)) {
        std::string err = strerror(errno);
        std::cerr << name() << ": handle socket data write eps modify: " << err << std::endl;
    }

    m_socket_listener->on_connected(Timestamp::now(), nullptr);
}

 void HeartBeatConnThread::teardown_socket()
 {
     if (m_socket_p) {
         if (m_socket_p->fd().valid()) {
             m_eps.remove(m_socket_p->fd());
         }
     }
     m_socket_p.release();
 }

void HeartBeatConnThread::handle_socket_events(std::uint32_t events)
{
    std::cerr << name() << ": socket events: " << events << " socket: " << m_socket_p->fd().valid() << std::endl;
    if (events & EPOLLERR) {
        auto first_errno = errno;
        std::cerr << name() << ": handle_socket_events: EPOLLERR errno is: (" << first_errno << "): "
                  << strerror(first_errno) << std::endl;
    }

    if (events & EPOLLRDHUP) {
        teardown_socket();
        m_socket_listener->on_peer_disconnect(Timestamp::now(), nullptr);
    } else if (events & EPOLLHUP) {
        teardown_socket();
        m_socket_listener->on_local_disconnect(Timestamp::now(), nullptr);
    }
}

bool HeartBeatConnThread::add_and_connect_socket()
{
    m_socket_p.reset(new TCPSocket());

    if (!m_eps.add(m_socket_p->fd(), (std::uint64_t)&HeartBeatConnThread::socket_callback, EPOLLIN|EPOLLOUT|EPOLLRDHUP)) {
        auto errn = errno;
        std::string err = strerror(errn);
        std::cerr << name() << ": eps socket add: " << err << std::endl;
        return false;
    }

    if (!m_socket_p->connect(m_host, m_port)) {
        int e = m_socket_p->get_errno();
        if (e != EINPROGRESS) {
            std::string err = strerror(e);
            std::cerr << name() << ": socket connect: " << err << std::endl;
            return false;
        }
    }
    return true;
}

void HeartBeatConnThread::pre_process()
{
    // read() the timer fd before we start the epoll stuff ...
    struct itimerspec newv;
    newv = {{m_heartbeat_seconds,0}, {m_heartbeat_seconds,0}};

    if (m_tfd.timerfd_settime(0, &newv, NULL) < 0) {
        std::string err = strerror(errno);
        std::cerr << name() << ": timerfd settime: " << err << std::endl;
        goto error;
    }

    if (!m_tfd.valid()) {
        std::cerr << name() << ": timerfd is not valid" << std::endl;
        goto error;
    }

    if (!m_eps.add(m_tfd.fd(), (std::uint64_t)&HeartBeatConnThread::hb_callback)) {
        std::string err = strerror(errno);
        std::cerr << name() << ": eps hb add: " << err << std::endl;
        goto error;
    }

    if (add_and_connect_socket() != true) {
        goto error;
    }

    return;

    error:
    throw std::runtime_error("pre_process");
    return;
}

void * HeartBeatConnThread::process()
{
    if (UNLIKELY(m_done_signal)) {
        // this will close the socket
        teardown_socket();
        return (void *)1;
    }

    if (UNLIKELY(m_disconnect_signal)) {
        teardown_socket();
        m_disconnect_signal = false;
    }

    if (UNLIKELY(m_reconnect_signal)) {
        if (!add_and_connect_socket()) {
            std::cerr << name() << ": eps add and reconnect" << std::endl;
            return (void *)1;
        }
        m_reconnect_signal = false;
    }

    int ret = m_eps.wait(2);
    if (UNLIKELY(ret < 0)) {
        std::string err = strerror(errno);
        std::cerr << name() << ": epoll error: " << err << std::endl;
        return (void *) 1;
    }
    if (LIKELY(ret > 0)) {
        for (const auto & e : m_eps) {
            auto cb = reinterpret_cast<Callback>(e.data.u64);
            (cb)(e.events, this);
        }
    }
    return nullptr;
}


}}
