#pragma once

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/signalfd.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <netdb.h>

#include <utility>
#include <array>

namespace i01 { namespace core {

class FDBase {
public:
    FDBase() : m_fd(-1) {}
    explicit FDBase(int fd_) : m_fd(fd_) {}
    FDBase(FDBase&& o) { m_fd = o.m_fd; o.m_fd = -1; }
    virtual ~FDBase() {}
    int fd() const { return m_fd; }
    int close() { int ret = ::close(m_fd); m_fd = -1; return ret; }
    // TODO: consider removing (int) cast operator...
    inline operator int() { return m_fd; }

    inline bool valid() { return m_fd >= 0; }
    inline operator bool() { return valid(); }

    ssize_t read(void *buf, size_t count) { return ::read(m_fd, buf, count); }
    ssize_t write(const void* buf, size_t count) { return ::write(m_fd, buf, count); }

    ssize_t readv(const struct iovec *iov, int iovcnt) { return ::readv(m_fd, iov, iovcnt); }
    ssize_t writev(const struct iovec *iov, int iovcnt) { return ::writev(m_fd, iov, iovcnt); }

    int transfer_ownership() { auto ret = m_fd; m_fd = -1; return ret; }

    template <typename ... Args>
    int fcntl(int cmd, Args&&... args)
    { return ::fcntl(m_fd, cmd, std::forward<Args>(args)...); }

    template <typename ... Args>
    int ioctl(unsigned long request, Args&&... args)
    { return ::ioctl(m_fd, request, std::forward<Args>(args)...); }

protected:
    FDBase(const FDBase&) = delete;
    int m_fd;
};

class FD : public FDBase {
public:
    static FD open(const char *pathname, int flags) { return FD(::open(pathname, flags)); }
    static FD open(const char *pathname, int flags, mode_t mode) { return FD(::open(pathname, flags, mode)); }
    static FD shm_open(const char *name, int oflag, mode_t mode) { return FD(::shm_open(name, oflag, mode)); }

    FD() : FDBase() {}
    explicit FD(int fd_) : FDBase(fd_) {}
    FD(const FD& o) : FDBase(::dup(o.m_fd)) {}
    FD(FD&& o) : FDBase(std::move(o)) {}
    virtual ~FD() { if (m_fd >= 0) close(); }

    off_t lseek(off_t offset, int whence) { return ::lseek(m_fd, offset, whence); }
    off64_t lseek64(off64_t offset, int whence) { return ::lseek64(m_fd, offset, whence); }

    int fchmod(mode_t mode) { return ::fchmod(m_fd, mode); }
    int fchown(uid_t owner, gid_t group) { return ::fchown(m_fd, owner, group); }

    int flock(int operation) { return ::flock(m_fd, operation); }
    int lockf(int cmd, off_t len) { return ::lockf(m_fd, cmd, len); }


    int fsync() { return ::fsync(m_fd); }
    int fdatasync() { return ::fdatasync(m_fd); }

    void *mmap(void *addr, size_t length, int prot, int flags, off_t offset)
    { return ::mmap(addr, length, prot, flags, m_fd, offset); }
    int munmap(void *addr, size_t length) { return ::munmap(addr, length); }

    int ftruncate(off_t length) { return ::ftruncate(m_fd, length); }
    int fstat(struct stat *buf) { return ::fstat(m_fd, buf); }

    int futimes(const struct timeval tv[2]) { return ::futimes(m_fd, tv); }
    int futimens(const struct timespec times[2]) { return ::futimens(m_fd, times); }

};

class SignalFD : public FD {
public:
    SignalFD(const sigset_t *mask, int flags) : FD(::signalfd(-1, mask, flags)) {}
    int signalfd(const sigset_t *mask, int flags) { return (m_fd = ::signalfd(m_fd, mask, flags)); }
};

class EventFD : public FD {
public:
    EventFD(const unsigned int initval, int flags) : FD(::eventfd(initval, flags)) {}
    int eventfd(const unsigned int initval, int flags) { return (m_fd = ::eventfd(initval, flags)); }
};

class TimerFD : public FD {
public:
    TimerFD(int clockid, int flags) : FD(::timerfd_create(clockid, flags)) {}
    int timerfd_create(int clockid, int flags) { return (m_fd = ::timerfd_create(clockid, flags)); }
    int timerfd_settime(int flags, const struct itimerspec* new_value, struct itimerspec *old_value) { return ::timerfd_settime(m_fd, flags, new_value, old_value); }
    int timerfd_gettime(struct itimerspec *curr_value) { return ::timerfd_gettime(m_fd, curr_value); }

};

class EpollFD : public FD {
public:
    EpollFD() : FD(::epoll_create1(EPOLL_CLOEXEC)) {}
    explicit EpollFD(int flags) : FD(::epoll_create1(flags)) {}

    int epoll_ctl(int op, int ofd, struct epoll_event *event)
    { return ::epoll_ctl(m_fd, op, ofd, event); }
    int epoll_ctl(int op, FD& ofd, struct epoll_event *event)
    { return ::epoll_ctl(m_fd, op, ofd.fd(), event); }
    int epoll_wait(struct epoll_event *events, int maxevents, int timeout)
    { return ::epoll_wait(m_fd, events, maxevents, timeout); }
    int epoll_pwait(struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask)
    { return ::epoll_pwait(m_fd, events, maxevents, timeout, sigmask); }

private:
    EpollFD(const EpollFD&) = delete;
    EpollFD(EpollFD&&) = delete;
    EpollFD& operator=(const EpollFD&) = delete;
};

class SocketFD : public FD {
public:
    typedef std::uint32_t IP4Addr;
    typedef std::uint16_t Port;

    SocketFD() : FD() {}
    explicit SocketFD(int fd_) : FD(fd_) {}
    SocketFD(int domain, int type, int protocol)
        : FD(::socket(domain, type, protocol)) {}
    SocketFD(SocketFD&& s) : FD(std::move(s)) {}
    SocketFD& operator=(SocketFD&& s) {
        m_fd = s.transfer_ownership();
        return *this;
    }
    static SocketFD socket(int domain, int type, int protocol)
    { return SocketFD(domain, type, protocol); }
    static int socketpair(int domain, int type, int protocol, std::array<SocketFD, 2>& sv)
    { return ::socketpair(domain, type, protocol, reinterpret_cast<int*>(sv.data())); }

    SocketFD accept(struct sockaddr *addr, socklen_t *addrlen)
    { return SocketFD(::accept(fd(), addr, addrlen)); }
    SocketFD accept4(struct sockaddr *addr, socklen_t *addrlen, int flags)
    { return SocketFD(::accept4(fd(), addr, addrlen, flags)); }
    int bind(const struct sockaddr *addr, socklen_t addrlen)
    { return ::bind(fd(), addr, addrlen); }
    int connect(const struct sockaddr *addr, socklen_t addrlen)
    { return ::connect(fd(), addr, addrlen); }
    int getpeername(struct sockaddr *addr, socklen_t *addrlen) const
    { return ::getpeername(fd(), addr, addrlen); }
    int getsockname(struct sockaddr *addr, socklen_t *addrlen) const
    { return ::getsockname(fd(), addr, addrlen); }
    int getsockopt(int level, int optname, void *optval, socklen_t *optlen) const
    { return ::getsockopt(fd(), level, optname, optval, optlen); }
    int setsockopt(int level, int optname, void *optval, socklen_t optlen)
    { return ::setsockopt(fd(), level, optname, optval, optlen); }
    int listen(int backlog = 128)
    { return ::listen(fd(), backlog); }
    ssize_t recv(void *buffer, size_t length, int flags = 0)
    { return ::recv(fd(), buffer, length, flags); }
    ssize_t recvfrom(void *buffer, size_t length, int flags,
                     struct sockaddr *address, socklen_t *address_len)
    { return ::recvfrom(fd(), buffer, length, flags, address, address_len); }
    ssize_t send(const void *message, size_t length, int flags = 0)
    { return ::send(fd(), message, length, flags); }
    ssize_t sendto(const void *message, size_t length, int flags,
                   const struct sockaddr *dest_addr, socklen_t dest_len)
    { return ::sendto(fd(), message, length, flags, dest_addr, dest_len); }
    int shutdown(int how) { return ::shutdown(fd(), how); }
};

} }
