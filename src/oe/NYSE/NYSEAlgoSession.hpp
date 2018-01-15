#pragma once

#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/NamedThread.hpp>

#include <i01_oe/OrderManager.hpp>
#include <i01_oe/OrderSession.hpp>

#include <i01_oe/NYSEOrder.hpp>
#include <i01_oe/OrderSession.hpp>

namespace i01 { namespace OE { namespace NYSE {

class NYSEFIX8Impl;
class NYSECCGfix_session_client;
class NYSEAlgoSession
    : public OrderSession {
public:
    NYSEAlgoSession(OrderManager *om_p, const std::string& name_);
    virtual ~NYSEAlgoSession();

    // OrderSession:

    virtual bool connect(const bool replay = false) override;
    virtual bool send(Order* order_p) override;
    virtual bool cancel(Order* order_p, Size newqty = 0) override;

    // FIX-specific:

    const std::string& session_conf_file() const { return m_fix8session_conf_file; }
    const std::string& nyse_algo_name() const { return m_nyse_algo_name; }

private:
    void session_conf_file(const std::string& ts) { m_fix8session_conf_file = ts; }

    std::string m_fix8session_conf_file;
    std::string m_bag_session_name;
    std::string m_cbs_session_name;
    std::uint32_t m_bag_max_message_per_second;
    std::uint32_t m_cbs_max_message_per_second;
    std::string m_cbs_parent_mnemonic_filter;
    std::string m_cbs_parent_agency_filter;
    std::string m_nyse_algo_name;

    NYSEFIX8Impl * m_impl_p;

    friend class NYSEFIX8Impl;
    friend class NYSECCGfix_session_client;
};

} } }
