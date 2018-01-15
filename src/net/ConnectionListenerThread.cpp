#include <netinet/in.h>
#include <sys/socket.h>


#include <i01_net/ConnectionListenerThread.hpp>
#include <i01_net/SocketListener.hpp>

namespace i01 { namespace net {


ConnectionListenerThread::ConnectionListenerThread(const std::string& n, SocketListener& sl,
                                                   ConnDataGenerator func) :
    EpollEventPoller(n + "-connlistener"),
    m_socket_listener(sl),
    m_data_generator(func),
    m_listen_fd()
{
    // need to add socket listener here
}

bool ConnectionListenerThread::add_listener_on_port(std::uint16_t port)
{
    // in debug mode, this will assert here if there is an error
    // creating the socket, since TPCSocket will assert it is >=0 in
    // the ctor
    m_listen_fd = TCPSocket::create_listening_socket(port);
    if (!m_listen_fd.fd().valid()) {
        return false;
    }

    if (!m_listen_eps.add(m_listen_fd.fd(),
                          (std::uint64_t)&ConnectionListenerThread::listen_callback,
                          EPOLLIN)) {
        std::string err = ::strerror(errno);
        std::cerr << "ConnectionListenerThread::setup: " << err << std::endl;
        return false;
    }
    return true;
}

void * ConnectionListenerThread::check_for_new_connections()
{
    auto ret = m_listen_eps.wait(1);
    if (ret < 0) {
        std::string err = ::strerror(errno);
        std::cerr << "ConnectionListenerThread: epoll wait error: " << err << std::endl;
        return (void *)1;
    }
    if (ret > 0) {
        for (const auto& e : m_listen_eps) {
            auto cb = reinterpret_cast<Callback>(e.data.u64);
            (cb)(e.events, this);
        }
    }
    return nullptr;
}

void ConnectionListenerThread::listen_callback(std::uint32_t events, void * ptr)
{
    auto * p = static_cast<ConnectionListenerThread *>(ptr);
    p->on_new_connection(events);
}

void ConnectionListenerThread::on_new_connection(std::uint32_t events)
{
    // do the accept here and then add the socket ...
    sockaddr_in addr;
    socklen_t addrlen = sizeof(sockaddr_in);

    auto fd = m_listen_fd.fd().accept(reinterpret_cast<sockaddr *>(&addr), &addrlen);
    if (fd.valid()) {
        void * user_data = nullptr;
        if (m_data_generator) {
            user_data = m_data_generator(fd.fd(), addr, addrlen);
        }
        std::cerr << "ConnectionListenerThread: on_new_connection " << fd.fd() << std::endl;
        if (!add_socket(m_socket_listener, user_data, fd.transfer_ownership(), /* managed= */true)) {
            std::cerr << "ConnectionListenerThread: could not add socket for fd " << fd.fd() << std::endl;
        }
    }
}


void * ConnectionListenerThread::process()
{
    const auto ret = check_for_new_connections();
    if (nullptr != ret) {
        return ret;
    }

    return EpollEventPoller::process();
}


}}
