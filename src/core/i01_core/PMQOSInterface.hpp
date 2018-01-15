#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <iostream>
#include <stdexcept>

namespace i01 { namespace core {

/// PMQOSInterface Traits class for /dev/cpu_dma_latency.
struct CPUDMALatency {
    static const char * const path;
    typedef std::int32_t value_type;
    static const value_type default_value;
    static const value_type intolerable_value;
};

template <typename InterfaceTraits>
class PMQOSInterface {
public:
    PMQOSInterface();
    virtual ~PMQOSInterface();

    bool set(const typename InterfaceTraits::value_type value);
    bool tolerable() const;
private:
    int m_fd;
    typename InterfaceTraits::value_type m_value;
};

template <typename InterfaceTraits>
PMQOSInterface<InterfaceTraits>::PMQOSInterface()
{
    m_fd = ::open(InterfaceTraits::path, O_RDWR);
    if (m_fd < 0)
    {
        std::cerr << "PMQOSInterface error opening: "
                  << InterfaceTraits::path
                  << std::endl;
        ::perror("PMQOSInterface");
    }
}

template <typename InterfaceTraits>
PMQOSInterface<InterfaceTraits>::~PMQOSInterface()
{
    if (m_fd >= 0)
        ::close(m_fd);
    m_fd = -1;
}

template <typename InterfaceTraits>
bool PMQOSInterface<InterfaceTraits>::set( const typename InterfaceTraits::value_type value)
{
    m_value = value;
    ssize_t ret = ::write(m_fd, &m_value, sizeof(m_value));
    if ((ret == sizeof(m_value)) && tolerable())
    {
        return true;
    } else if (ret < 0) {
        std::cerr << "PMQOSInterface error writing: "
                  << InterfaceTraits::path
                  << std::endl;
        ::perror("PMQOSInterface");
    } else {
        std::cerr << "PMQOSInterface unexpected return value while writing: "
                  << ret
                  << std::endl;
        ::perror("PMQOSInterface");
    }
    return false;
}

template <typename InterfaceTraits>
bool PMQOSInterface<InterfaceTraits>::tolerable() const
{ 
    typename InterfaceTraits::value_type current_value = InterfaceTraits::intolerable_value;
    if (::read(m_fd, &current_value, sizeof(current_value))
            == sizeof(current_value))
        if ((current_value >= 0) && (current_value <= m_value))
            return true;

    return false;
}

} }
