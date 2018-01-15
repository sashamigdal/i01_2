#include <cstdlib>
#include <stdexcept>

#include <i01_net/SFCVISet.hpp>
#include <i01_net/SFCInterface.hpp>

namespace i01 { namespace net {

SFCVISet::SFCVISet( SFCInterface& intf
                  , int size)
    : m_intf(intf)
    , m_size(size)
{
    if (::ef_vi_set_alloc_from_pd( &m_vi_set
                                 , m_intf.dh()
                                 , &m_intf.pd()
                                 , m_intf.dh()
                                 , size
                                 ) < 0)
    {
        perror("ef_vi_set_alloc_from_pd");
        throw std::runtime_error("ef_vi_set_alloc_from_pd failed.");
    }
}

SFCVISet::~SFCVISet()
{
}

bool SFCVISet::apply_filter(SFCFilterSpec& fs)
{
    return (::ef_vi_set_filter_add(&m_vi_set, m_intf.dh(), &fs.fs(), &fs.fc()) >= 0);
}

bool SFCVISet::remove_filter(SFCFilterSpec& fs)
{
    return (::ef_vi_set_filter_del(&m_vi_set, m_intf.dh(), &fs.fc()) >= 0);
}


} }
