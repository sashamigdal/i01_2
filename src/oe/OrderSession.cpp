#include <stdexcept>

#include <i01_core/Config.hpp>
#include <i01_oe/OrderSession.hpp>
#include <i01_oe/L2SimSession.hpp>
#include "NASDAQ/OUCH42Session.hpp"
#include "NYSE/NYSEAlgoSession.hpp"
#include "BATS/BOE20Session.hpp"

namespace i01 { namespace OE {

OrderSession::OrderSession(OrderManager* orderManager_p, const std::string& name_, const std::string& type_name, bool simulated)
    : m_name(name_)
    , m_mic()
    , m_active(false)
    , m_order_manager_p(orderManager_p)
    , m_session_risk()
    , m_mocloc_add_deadline_ns(0)
    , m_mocloc_mod_deadline_ns(0)
    , m_simulated(simulated)
{
    if (!m_order_manager_p) {
        throw std::runtime_error("OrderSession " + name() + " has null OrderManager pointer.");
    }

    auto cs = core::Config::instance().get_shared_state()->copy_prefix_domain("oe.sessions." + name() + ".");
    if (boost::optional<std::string> type = cs->get<std::string>("type")) {
        if (type_name != *type && !m_simulated) {
            throw std::runtime_error("OrderSession " + name() + " expected type " + type_name + " does not match configured type " + *type);
        }
    } else {
        throw std::runtime_error("OrderSession " + name() + " not configured.");
    }

    if (boost::optional<std::string> mic_name = cs->get<std::string>("mic")) {
        m_mic = core::MIC::clone(mic_name->c_str());
    }
    if (m_mic == core::MIC::Enum::UNKNOWN)
        throw std::runtime_error("OrderSession " + name() + " has unknown MIC.");

    // ...nanoseconds since midnight deadline for MOC/LOC Add.
    if (!cs->get<std::uint64_t>("mocloc_add_deadline_ns", m_mocloc_add_deadline_ns)) {
        std::cerr << "OrderSession " <<  name() << " missing mocloc_add_deadline_ns, using 0." << std::endl;
    }
    // ...nanoseconds since midnight deadline for MOC/LOC Modify/Cancel.
    if (!cs->get<std::uint64_t>("mocloc_mod_deadline_ns", m_mocloc_mod_deadline_ns)) {
        std::cerr << "OrderSession " << name() << " missing mocloc_mod_deadline_ns, using 0." << std::endl;
    }
}

Order* OrderSession::get_order(LocalID id)
{
    // Don't allow this session to access orders that don't belong to it.
    Order* op = m_order_manager_p->get_order_safe(id);
    if (op && op->session() == this)
        return op;
    return nullptr;
}

OrderSessionPtr OrderSession::factory(OrderManager* omp, const std::string& n, const std::string& t)
{
    OrderSessionPtr osp;
    try {
        if (t == "L2SimSession") {
            osp = std::make_shared<L2SimSession>(omp, n);
        } else if (t == "NYSEAlgoSession") {
            osp = std::make_shared<NYSE::NYSEAlgoSession>(omp, n);
        } else if (t == "OUCH42Session") {
            osp = std::make_shared<NASDAQ::OUCH42Session>(omp, n);
        } else if (t == "BOE20Session") {
            osp = std::make_shared<BATS::BOE20Session>(omp, n);
        } else {
            throw std::runtime_error("Unknown session type.  Did you mistype it or forget to add it to OrderSession::factory?");
        }
        return osp;
    } catch (std::exception& e) {
        std::cerr << "Exception caught while constructing session " << n << " of type " << t << ": " << e.what() << std::endl;
        return nullptr;
    }
}

Order * OrderSession::on_create_replay_order(const Timestamp&, Instrument *, const core::MIC&,
                                             const LocalAccount, const LocalID,
                                             const Price, const Size, const Side, const TimeInForce,
                                             const OrderType, const UserData)
{
    return nullptr;
}

} }
