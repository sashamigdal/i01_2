#pragma once

#include <cstdint>

// Loosely based off optimized POSIX mapped ring buffer implementation:
//   http://en.wikipedia.org/w/index.php?title=Circular_buffer&oldid=600431497#Optimized_POSIX_implementation

namespace i01 { namespace core {

class MappedRingBase {

    void *        m_address;
    std::uint64_t m_count_bytes;
    std::uint64_t m_write_offset_bytes;
    std::uint64_t m_read_offset_bytes;

public:
    /// Map a ring using file descriptor `fd`, starting at offset `offset`, in
    /// segments of size `2^(order)`.
    MappedRingBase(int fd,
                   std::uint64_t offset = 0,
                   std::uint8_t order = 27 /* 2^27 = 128MB */);
    virtual ~MappedRingBase();

    void * write_address();
    void write_advance(std::uint64_t count);

    const void * read_address() const;
    void read_advance(std::uint64_t count);

    std::uint64_t size() const
    { return m_write_offset_bytes - m_read_offset_bytes; }

    std::uint64_t capacity() const
    { return m_count_bytes; }

    std::int64_t free() const
    { return capacity() - size(); }

    void clear();
};

template <typename T>
class MappedRing : private MappedRingBase {
public:
    using MappedRingBase::MappedRingBase;
    T* write_address() { return static_cast<T*>(MappedRingBase::write_address()); }
    const T* read_address() const { return static_cast<T*>(MappedRingBase::read_address()); }
};

} } 
