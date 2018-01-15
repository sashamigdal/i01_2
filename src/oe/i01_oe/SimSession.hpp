#pragma once

#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>

#include <i01_oe/OrderManager.hpp>
#include <i01_oe/OrderSession.hpp>

namespace i01 { namespace OE {

class SimSession : public OrderSession {
public:
    typedef core::Timestamp Timestamp;

public:
    SimSession(OrderManager *om_p, const std::string& name, const std::string& type_name = "SimSession");
    virtual ~SimSession() = default;

    const Timestamp & ack_latency() const { return m_ack_latency; }
    void ack_latency(const Timestamp &ts) { m_ack_latency = ts; }

    const Timestamp & cxl_latency() const { return m_cxl_latency; }
    void cxl_latency(const Timestamp &ts) { m_cxl_latency = ts; }

protected:
    virtual bool connect(const bool replay) override { return true; }

protected:
    Timestamp m_ack_latency;
    Timestamp m_cxl_latency;

};

}}
