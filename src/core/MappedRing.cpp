#include <cassert>
#include <cstring>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>

#include <i01_core/MappedRing.hpp>

namespace i01 { namespace core {

MappedRingBase::MappedRingBase(int fd,
                               std::uint64_t offset,
                               std::uint8_t order)
    : m_count_bytes(1ULL << order)
    , m_write_offset_bytes(0)
    , m_read_offset_bytes(0)
{
    m_address = ::mmap(NULL, capacity() << 2, PROT_NONE,
                       MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (m_address == MAP_FAILED)
        throw std::runtime_error(strerror(errno));

    void *addr = ::mmap(m_address, capacity(),
                        PROT_READ | PROT_WRITE,
                        MAP_FIXED | MAP_SHARED, fd, offset);
    if (addr != m_address)
        throw std::runtime_error(strerror(errno));

    void *addr2 = ::mmap(static_cast<char*>(m_address) + capacity(), capacity(),
                         PROT_READ | PROT_WRITE,
                         MAP_FIXED | MAP_SHARED, fd, offset);
    if (addr2 != static_cast<char*>(m_address) + capacity())
        throw std::runtime_error(strerror(errno));

    
}

MappedRingBase::~MappedRingBase()
{
    if (0 == ::munmap(m_address, capacity() << 2))
        throw std::runtime_error(strerror(errno));
}

void * MappedRingBase::write_address()
{
    return static_cast<char*>(m_address) + m_write_offset_bytes;
}

void MappedRingBase::write_advance(std::uint64_t count)
{
    m_write_offset_bytes += count;
}

const void * MappedRingBase::read_address() const
{
    return static_cast<char*>(m_address) + m_read_offset_bytes;
}

void MappedRingBase::read_advance(std::uint64_t count)
{
    m_read_offset_bytes += count;
    if (m_read_offset_bytes >= capacity()) {
        m_read_offset_bytes -= capacity();
        m_write_offset_bytes -= capacity();
    }
    assert(m_read_offset_bytes <= m_write_offset_bytes);
}

void MappedRingBase::clear()
{
    m_read_offset_bytes = 0;
    m_write_offset_bytes = 0;
}

} }
