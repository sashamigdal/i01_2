#pragma once

#include <etherfabric/vi.h>

#include <i01_net/SFCInterface.hpp>
#include <i01_net/SFCFilterSpec.hpp>

namespace i01 { namespace net {

class SFCVISet : public SFCFilterable {
  public:
    SFCVISet(SFCInterface& intf, int size);
    virtual ~SFCVISet();

    SFCInterface& interface() { return m_intf; }

    ::ef_vi_set& vi_set() { return m_vi_set; }

    bool apply_filter(SFCFilterSpec& fs) override final;
    bool remove_filter(SFCFilterSpec& fs) override final;

  protected:
    SFCInterface&                     m_intf;
    int                               m_size;
    ::ef_vi_set                       m_vi_set;

  private:
    SFCVISet() = delete;
    SFCVISet(const SFCVISet&) = delete;
    SFCVISet& operator=(const SFCVISet&) = delete;

};

} }
