#include <utility>
#include <iostream>

#include <boost/filesystem.hpp>

#include <i01_core/Config.hpp>
#include <i01_core/NamedThread.hpp>

#include "NYSEAlgoSession.hpp"

#include <fix8/f8includes.hpp>
#include <fix8/sessionwrapper.hpp>
#include "NYSECCGfix_types.hpp"
#include "NYSECCGfix_router.hpp"
#include "NYSECCGfix_classes.hpp"
#include "NYSECCGfix_session.hpp"

namespace i01 { namespace OE { namespace NYSE {

class NYSEFIX8Impl {
    typedef FIX8::ReliableClientSession<NYSECCGfix_session_client> F8RCS;
    typedef F8RCS::ReliableClientSession_ptr F8RCSPtr;

    NYSEAlgoSession& m_session;
    struct F8SessionState {
        F8RCSPtr m_f8session;
        core::NamedThreadBase * m_session_thread_p;
        std::uint32_t m_recv_seqnum;
        std::uint32_t m_send_seqnum;

        template <typename ... Args>
        F8SessionState(Args&&... args)
            : m_f8session(new F8RCS(std::forward<Args>(args)...))
            , m_session_thread_p(nullptr)
            , m_recv_seqnum(0)
            , m_send_seqnum(0) {}
        ~F8SessionState() {
            if (m_session_thread_p)
                m_session_thread_p->shutdown(false);
            delete m_session_thread_p;
        }
    } m_bag, m_cbs;

    NYSEAlgoSession& session() const { return m_session; }
public:
    NYSEFIX8Impl(NYSEAlgoSession& s)
        : m_session(s)
        , m_bag(FIX8::NYSECCG::ctx(), m_session.session_conf_file(), m_session.m_bag_session_name)
        , m_cbs(FIX8::NYSECCG::ctx(), m_session.session_conf_file(), m_session.m_cbs_session_name)
    {
        m_bag.m_f8session->session_ptr()->_nyse_session_type = NYSECCGfix_session_client::NYSESessionType::BAG;
        m_cbs.m_f8session->session_ptr()->_nyse_session_type = NYSECCGfix_session_client::NYSESessionType::CBS;
        m_bag.m_f8session->session_ptr()->_i01_impl = this;
        m_cbs.m_f8session->session_ptr()->_i01_impl = this;
    }

    ~NYSEFIX8Impl() { }

    bool connect() {
        if (!m_bag.m_f8session || !m_cbs.m_f8session) {
            std::cerr << "BAG or CBS FIX8 Session pointer is null." << std::endl;
            return false;
        }
        //m_bag.m_session_thread_p =
        //    new core::NamedStdFunctionThread("NYSEBAGThread"
        //        , std::move([&](){
                    try {
                        m_bag.m_f8session->start( false
                            , m_bag.m_send_seqnum, m_bag.m_recv_seqnum
                            , m_bag.m_f8session->session_ptr()->get_login_parameters()._davi());
                    } catch (FIX8::f8Exception& e) {
                        std::cerr << "BAG exception caught: " << e.what() << std::endl;
                    } catch (std::exception& e) {
                        std::cerr << "BAG exception caught: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "BAG exception caught (unknown exception)." << std::endl;
                    }
        //            return nullptr;
        //            })
        //        , false);
        //static_cast<core::NamedStdFunctionThread *>(m_bag.m_session_thread_p)->spawn();
        //m_cbs.m_session_thread_p =
        //    new core::NamedStdFunctionThread("NYSECBSThread"
        //        , std::move([&](){
                    try {
                        m_cbs.m_f8session->start( false
                            , m_cbs.m_send_seqnum, m_cbs.m_recv_seqnum
                            , m_cbs.m_f8session->session_ptr()->get_login_parameters()._davi());
                    } catch (FIX8::f8Exception& e) {
                        std::cerr << "BAG exception caught: " << e.what() << std::endl;
                    } catch (std::exception& e) {
                        std::cerr << "BAG exception caught: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "BAG exception caught (unknown exception)." << std::endl;
                    }
        //             return nullptr;
        //             })
        //         , false);
        //static_cast<core::NamedStdFunctionThread *>(m_cbs.m_session_thread_p)->spawn();
        return true;
    }

    bool send(Order* I01_UNUSED op)
    {
        return false;
    }
    bool cancel(Order* I01_UNUSED op, Size I01_UNUSED newqty = 0)
    {
        return false;
    }

    friend class NYSEAlgoSession;
    friend class NYSECCGfix_router_client;
    friend class NYSECCGfix_session_client;
};

NYSEAlgoSession::NYSEAlgoSession(OrderManager *omp, const std::string& name_)
    : OrderSession(omp, name_, "NYSEAlgoSession")
    , m_fix8session_conf_file()
    , m_bag_session_name()
    , m_cbs_session_name()
    , m_bag_max_message_per_second(0)
    , m_cbs_max_message_per_second(0)
    , m_cbs_parent_mnemonic_filter()
    , m_cbs_parent_agency_filter()
    , m_nyse_algo_name()
    , m_impl_p(nullptr)
{
    auto cs = core::Config::instance().get_shared_state()->copy_prefix_domain("oe.sessions." + name() + ".");

    if (!cs->get("fix8_session_xml", m_fix8session_conf_file) || m_fix8session_conf_file.empty()) {
        throw std::runtime_error("NYSEAlgoSession " + name() + " missing fix8_session_xml.");
    }

    if (m_fix8session_conf_file[0] != '/')
        m_fix8session_conf_file = (boost::filesystem::path("conf") / boost::filesystem::path(m_fix8session_conf_file)).string();

    if (!boost::filesystem::exists(m_fix8session_conf_file)) {
        throw std::runtime_error("NYSEAlgoSession " + name() + " fix8_session_xml file nonexistent.");
    }

    if (!cs->get("bag_session_name", m_bag_session_name) || m_bag_session_name.empty())
        throw std::runtime_error("NYSEAlgoSession " + name() + " missing bag_session_name.");
    if (!cs->get("cbs_session_name", m_cbs_session_name) || m_cbs_session_name.empty())
        throw std::runtime_error("NYSEAlgoSession " + name() + " missing cbs_session_name.");
    if (!cs->get("bag_max_message_per_second", m_bag_max_message_per_second) || (m_bag_max_message_per_second == 0))
        throw std::runtime_error("NYSEAlgoSession " + name() + " missing bag_max_message_per_second.");
    if (!cs->get("cbs_max_message_per_second", m_cbs_max_message_per_second) || (m_cbs_max_message_per_second == 0))
        throw std::runtime_error("NYSEAlgoSession " + name() + " missing cbs_max_message_per_second.");
    if (!cs->get("cbs_parent_mnemonic_filter", m_cbs_parent_mnemonic_filter) || m_cbs_parent_mnemonic_filter.empty())
        throw std::runtime_error("NYSEAlgoSession " + name() + " missing cbs_parent_mnemonic_filter.");
    if (!cs->get("cbs_parent_agency_filter", m_cbs_parent_agency_filter) || m_cbs_parent_agency_filter.empty())
        throw std::runtime_error("NYSEAlgoSession " + name() + " missing cbs_parent_agency_filter.");
    if (!cs->get("nyse_algo_name", m_nyse_algo_name) || m_nyse_algo_name.empty())
        throw std::runtime_error("NYSEAlgoSession " + name() + " missing nyse_algo_name.");

    // Create the NYSEFIX8Impl once configuration is complete:
    m_impl_p = new NYSEFIX8Impl(*this);
}

NYSEAlgoSession::~NYSEAlgoSession()
{
    delete m_impl_p;
}

bool NYSEAlgoSession::connect(const bool I01_UNUSED replay) {
    return m_impl_p->connect();
}

bool NYSEAlgoSession::send(Order* I01_UNUSED order_p) {
    return m_impl_p->send(order_p);
}

bool NYSEAlgoSession::cancel(Order* I01_UNUSED order_p, Size I01_UNUSED newqty) {
    return m_impl_p->cancel(order_p);
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::Heartbeat *msg) const
{
    std::cout << "===CCG Heartbeat===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::TestRequest *msg) const
{
    std::cout << "===CCG TestRequest===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::ResendRequest *msg) const
{
    std::cout << "===CCG ResendRequest===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::Reject *msg) const
{
    std::cout << "===CCG Reject===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::SequenceReset *msg) const
{
    std::cout << "===CCG SequenceReset===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::Logout *msg) const
{
    std::cout << "===CCG Logout===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::ExecutionReport *msg) const
{
    std::cout << "===CCG ExecutionReport===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::OrderCancelReject *msg) const
{
    std::cout << "===CCG OrderCancelReject===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::Logon *msg) const
{
    std::cout << "===CCG Logon===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::NewOrderSingle *msg) const
{
    std::cout << "===CCG NewOrderSingle===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::OrderCancelRequest *msg) const
{
    std::cout << "===CCG OrderCancelRequest===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::OrderCancelReplaceRequest *msg) const
{
    std::cout << "===CCG OrderCancelReplaceRequest===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::BusinessMessageReject *msg) const
{
    std::cout << "===CCG BusinessMessageReject===" << std::endl << *msg;
    return true;
}

bool NYSECCGfix_router_client::operator() (const FIX8::NYSECCG::DontKnowTrade *msg) const
{
    std::cout << "===CCG DontKnowTrade===" << std::endl << *msg;
    return true;
}

NYSECCGfix_session_client::NYSECCGfix_session_client(
        const FIX8::F8MetaCntx& ctx
      , const FIX8::SessionID& sid
      , FIX8::Persister *persist
      , FIX8::Logger *logger
      , FIX8::Logger *plogger)
    : Session(ctx, sid, persist, logger, plogger)
    , _nyse_session_type(NYSESessionType::UNKNOWN)
    , _router(*this)
    , _i01_impl(nullptr)
{
}

FIX8::Message * NYSECCGfix_session_client::generate_logon(const unsigned heartbeat_interval, const FIX8::f8String davi)
{
    FIX8::Message *msg(Session::generate_logon(heartbeat_interval, davi));
    if (get_login_parameters()._reset_sequence_numbers)
        *msg << new FIX8::NYSECCG::ResetSeqNumFlag(FIX8::NYSECCG::ResetSeqNumFlag_YES);
    return msg;
}

FIX8::Message *NYSECCGfix_session_client::generate_logout(const char * I01_UNUSED msgstr)
{
    FIX8::Message *msg(Session::generate_logout());
    return msg;
}

FIX8::Message *NYSECCGfix_session_client::generate_heartbeat(const FIX8::f8String& testReqID)
{
    FIX8::Message *msg(Session::generate_heartbeat(testReqID));
    return msg;
}

FIX8::Message *NYSECCGfix_session_client::generate_resend_request(const unsigned begin, const unsigned end)
{
    FIX8::Message *msg(Session::generate_resend_request(begin, end));
    return msg;
}

FIX8::Message *NYSECCGfix_session_client::generate_sequence_reset(const unsigned newseqnum, const bool gapfillflag)
{
    FIX8::Message *msg(Session::generate_sequence_reset(newseqnum, gapfillflag));
    return msg;
}

FIX8::Message *NYSECCGfix_session_client::generate_test_request(const FIX8::f8String& testReqID)
{
    FIX8::Message *msg(Session::generate_test_request(testReqID));
    return msg;
}

FIX8::Message *NYSECCGfix_session_client::generate_reject(const unsigned seqnum, const char *what, const char * I01_UNUSED msgstr)
{
    FIX8::Message *msg(Session::generate_reject(seqnum, what));
    return msg;
}


void NYSECCGfix_session_client::modify_outbound(FIX8::Message *msg)
{
    FIX8::Session::modify_outbound(msg);
    add_custom_fields(msg);
}

bool NYSECCGfix_session_client::handle_admin(const unsigned seqnum, const FIX8::Message *msg)
{
    bool ret = FIX8::Session::handle_admin(seqnum, msg);
    return ret && (enforce(seqnum, msg) || msg->process(_router));
}

bool NYSECCGfix_session_client::handle_application(const unsigned seqnum, const FIX8::Message *&msg)
{
      return enforce(seqnum, msg) || msg->process(_router);
}

void NYSECCGfix_session_client::add_custom_fields(FIX8::Message *msg)
{
    if ((_nyse_session_type == NYSESessionType::BAG) && msg && _i01_impl && msg->Header())
    {
        msg->Header()->add_field(
            new FIX8::NYSECCG::OnBehalfOfCompID(
                _i01_impl->m_session.m_cbs_parent_mnemonic_filter
            )
        );
    }
}

} } }

