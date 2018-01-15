#include <stdexcept>
#include <i01_net/SFCInterface.hpp>

namespace i01 { namespace net {

SFCInterface::SFCInterface(const char* ifname)
    : NLInterface(ifname)
    , m_dh(-1)
    , m_pd()
{
    if (::ef_driver_open(&m_dh) < 0)
    {
        perror("ef_driver_open");
        throw std::runtime_error("ef_driver_open failed.");
    }

    if (::ef_pd_alloc(&m_pd, m_dh, index(), (::ef_pd_flags)0) < 0)
    {
        perror("ef_pd_alloc");
        throw std::runtime_error("ef_pd_alloc failed.");
    }
}

SFCInterface::~SFCInterface()
{
    ::ef_pd_free(&m_pd, m_dh);
    ::ef_driver_close(m_dh);
}



} }
