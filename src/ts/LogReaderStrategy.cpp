#include <fstream>
#include <iostream>
#include <exception>

#include <i01_oe/FileBlotterReader.hpp>
#include <i01_oe/OrderLogFormat.hpp>
#include <i01_oe/Types.hpp>

#include <i01_ts/LogReaderStrategy.hpp>

namespace i01 { namespace TS {

LogReaderStrategy::LogReaderStrategy(OE::OrderManager* omp, MD::DataManager* dmp, const std::string& n) :
    Strategy(omp, dmp, n),
    m_reader_p{nullptr},
    m_thread_p{nullptr},
    m_path(OE::DEFAULT_ORDERLOG_PATH),
    m_output_path(DEFAULT_OUTPUT_PATH),
    m_ostream_p(nullptr),
    m_follow(false)
{
    auto cfg = core::Config::instance().get_shared_state();
    auto this_cfg(cfg->copy_prefix_domain("ts.strategies." + name() + "."));
    init(*this_cfg);
}

LogReaderStrategy::~LogReaderStrategy()
{
    m_thread_p->shutdown(/* blocking = */ true);
    delete m_thread_p;
    delete m_reader_p;
}

void LogReaderStrategy::start()
{
    m_reader_p = new OE::FileBlotterReader(m_path);
    m_reader_p->register_listeners(this);

    m_reader_p->replay();

    // now if we are in follow mode, we spawn a thread to check the order log indefinitely
    if (m_follow) {
        m_thread_p = new FollowThread(m_reader_p, m_path, m_ostream_p);
        m_thread_p->spawn();
   }
}

void LogReaderStrategy::init(const core::Config::storage_type& cfg)
{
    cfg.get("input", m_path);
    cfg.get("output", m_output_path);

    if ("::cout" == m_output_path || "" == m_output_path) {
        m_ostream_p = &std::cout;
    } else if ("::cerr" == m_output_path) {
        m_ostream_p = &std::cerr;
    } else {
        // create the file
        m_ostream_p = new std::ofstream(m_output_path);
        if (!(*m_ostream_p)) {
            throw std::runtime_error("LogReaderStrategy::init: could not open output file " + m_output_path);
        }
    }

    cfg.get("follow", m_follow);
}

LogReaderStrategy::FollowThread::FollowThread(OE::FileBlotterReader* reader, const std::string& input, std::ostream* osp) :
    core::NamedThread<FollowThread>("log_reader:" + input),
    m_reader_p(reader)
{
}

void * LogReaderStrategy::FollowThread::process()
{
    m_reader_p->replay();
    sleep(1);
    return nullptr;
}

void LogReaderStrategy::on_log_start(const Timestamp& ts)
{
    *m_ostream_p << ts << ",START" << std::endl;
}

void LogReaderStrategy::on_log_end(const Timestamp& ts)
{
    *m_ostream_p << ts << ",END" << std::endl;
}

void LogReaderStrategy::on_log_order_sent(const Timestamp& ts, const OE::OrderLogFormat::OrderSentBody& body)
{
    *m_ostream_p << ts << ",ORDER_SENT," << body << std::endl;
}

void LogReaderStrategy::on_log_acknowledged(const Timestamp& ts, const OE::OrderLogFormat::OrderAckBody& body)
{
    *m_ostream_p << ts << ",ORDER_ACK," << body << std::endl;
}

void LogReaderStrategy::on_log_local_reject(const Timestamp& ts, const OE::OrderLogFormat::OrderLocalRejectBody& b)
{
    *m_ostream_p << ts << ",LOCAL_REJECT," << b << std::endl;
}

void LogReaderStrategy::on_log_pending_cancel(const Timestamp& ts, const OE::OrderLogFormat::OrderCxlReqBody& b)
{
    *m_ostream_p << ts << ",PENDING_CANCEL," << b << std::endl;
}

void LogReaderStrategy::on_log_destroy(const Timestamp& ts, const OE::OrderLogFormat::OrderDestroyBody& b)
{
    *m_ostream_p << ts << ",DESTROY," << b << std::endl;
}

void LogReaderStrategy::on_log_rejected(const Timestamp& ts, const OE::OrderLogFormat::OrderRejectBody& b)
{
    *m_ostream_p << ts << ",REJECTED," << b << std::endl;
}

void LogReaderStrategy::on_log_filled(const Timestamp& ts, bool partial_fill, const OE::OrderLogFormat::OrderFillBody& b)
{
    *m_ostream_p << ts << ",FILLED," << b << std::endl;
}

void LogReaderStrategy::on_log_cancel_rejected(const Timestamp& ts, const OE::OrderLogFormat::OrderCxlRejectBody& b)
{
    *m_ostream_p << ts << ",CANCEL_REJECTED," << b << std::endl;
}

void LogReaderStrategy::on_log_cancelled(const Timestamp& ts, const OE::OrderLogFormat::OrderCxlBody& b)
{
    *m_ostream_p << ts << ",CANCELLED," << b << std::endl;
}

void LogReaderStrategy::on_log_add_session(const Timestamp& ts, const OE::OrderLogFormat::NewSessionBody& b)
{
    *m_ostream_p << ts << ",ADD_SESSION," << b << std::endl;
}

void LogReaderStrategy::on_log_add_strategy(const Timestamp& ts, const OE::OrderLogFormat::NewAccountBody& b)
{
    *m_ostream_p << ts << ",ADD_STRATEGY," << b << std::endl;
}

void LogReaderStrategy::on_log_manual_position_adj(const Timestamp& ts, const OE::OrderLogFormat::ManualPositionAdjBody& b)
{
    *m_ostream_p << ts << ",MANUAL_POSITION_ADJ," << b << std::endl;
}



}}
