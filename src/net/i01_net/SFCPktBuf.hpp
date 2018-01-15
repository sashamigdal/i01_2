#pragma once

#include <cstddef>
#include <algorithm>

#include <unistd.h>

#include <etherfabric/memreg.h>

namespace i01 { namespace net {

class SFCVirtualInterface;
class SFCPktBuf {
public:
    static constexpr size_t prefix_size() { return offsetof(SFCPktBufPOD, m_dma_buf); }
    static constexpr size_t capacity() { return s_capacity - prefix_size(); }
    static constexpr size_t alloc_size() { return s_capacity; }
    // { return std::max((int)(s_capacity + prefix_size()), (int)(sysconf(_SC_PAGESIZE))); }

    SFCVirtualInterface*
                vi_owner()    { return m_pod.m_vi_owner; }
    ef_addr&    addr()        { return m_pod.m_addr; }
    int         pool_id()     { return m_pod.m_pool_id; }
    size_t      size()        { return m_pod.m_data_size; }

    const char* data() const  { return m_pod.m_dma_buf; }
    char*       data()        { return m_pod.m_dma_buf; }

    const char* operator[](int n) const { return data() + n; }
    char*       operator[](int n)       { return data() + n; }

private:
    static const int s_capacity = 2048;
    static_assert(s_capacity < 4096, "s_capacity must be smaller than pagesize (4096)");
    struct SFCPktBufPOD {
        /// Which SFCVirtualInterface owns this SFCPktBuf, for forward
        //  compatibility with RSS, etc.
        SFCVirtualInterface*  m_vi_owner;
        ef_addr               m_addr;
        int                   m_pool_id;
        size_t                m_data_size;
        char                  m_dma_buf[1]
            __attribute__((aligned(EF_VI_DMA_ALIGN)));
    } m_pod;

    friend class SFCVirtualInterface;
};

} }
