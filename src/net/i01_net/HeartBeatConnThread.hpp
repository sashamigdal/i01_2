#pragma once

#include <i01_core/Lock.hpp>
#include <i01_core/NamedThread.hpp>
#include <i01_core/TimerListener.hpp>

#include <i01_net/EpollSet.hpp>
#include <i01_net/SocketListener.hpp>
#include <i01_net/TCPSocket.hpp>

namespace i01 { namespace net {

class HeartBeatConnThread : public core::NamedThread<HeartBeatConnThread> {
public:
    HeartBeatConnThread(const std::string &n, SocketListener *sl, core::TimerListener *tl);

    void set_heartbeat_seconds(int hbs) { m_heartbeat_seconds = hbs; }

    void pre_process() override final;
    virtual void * process() override final;

    // callbacks for client use
    bool connect(const std::string &hn, int port);
    bool disconnect();
    void disconnect_and_quit();
    ssize_t send(const std::uint8_t *buf, const size_t &len);
    int socket_errno() const { return m_socket_p->get_errno(); }

protected:
    static const int READ_BUFFER_SIZE = 2048;

    using Callback = void (*)(std::uint32_t, void *);

    using Mutex = core::SpinMutex;
    using AtomicBool = core::Atomic<bool>;

protected:

    void handle_hb();
    void handle_socket_data_read(std::uint32_t events);
    void handle_socket_data_write(std::uint32_t events);
    void handle_socket_events(std::uint32_t events);

    static void hb_callback(std::uint32_t events, void *ptr);
    static void socket_callback(std::uint32_t events, void *ptr);

    bool add_and_connect_socket();
    void teardown_socket();

private:
    EpollSet m_eps;
    std::array<std::uint8_t, READ_BUFFER_SIZE> m_recv_buffer;
    core::TimerFD m_tfd;
    std::unique_ptr<TCPSocket> m_socket_p;
    std::uint32_t m_heartbeat_seconds;
    SocketListener *m_socket_listener;
    core::TimerListener *m_timer_listener;
    std::string m_host;
    int m_port;
    mutable Mutex m_mutex;
    AtomicBool m_done_signal;
    AtomicBool m_reconnect_signal;
    AtomicBool  m_disconnect_signal;
};



}}
