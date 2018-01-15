#pragma once

#include <etherfabric/vi.h>
#include <etherfabric/pd.h>
#include <etherfabric/pio.h>
#include <etherfabric/memreg.h>

#include <i01_net/NLInterface.hpp>

namespace i01 { namespace net {

class SFCVirtualInterface;
class SFCInterface : public NLInterface {
public:
    SFCInterface(const char* ifname);
    ~SFCInterface();

    ::ef_driver_handle&       dh() { return m_dh; }
    ::ef_pd&                  pd() { return m_pd; }

//    SFCVirtualInterface*      createVI();
//    void                      deleteVI(SFCVirtualInterface*);

protected:
    SFCInterface() = delete;

    ::ef_driver_handle                m_dh;
    struct ::ef_pd                    m_pd;


};

} }
