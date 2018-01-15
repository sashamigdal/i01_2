#include <i01_md/DataManager.hpp>

#include <i01_net/TCPSocket.hpp>

#include <i01_ts/AsyncTPSStrategy.hpp>

namespace i01 { namespace TS {

AsyncTPSStrategy::AsyncTPSStrategy(OE::OrderManager* omp, MD::DataManager* dmp, const std::string& n) :
    AsyncNBBOEquitiesStrategy(omp, dmp, n),
    m_server(m_om_p, m_dm_p),
    m_writer{nullptr},
    m_listen_port(DEFAULT_LISTEN_PORT)
{
    auto cfg = core::Config::instance().get_shared_state()->copy_prefix_domain("ts.strategies." + name() + ".");
    init(*cfg);

    m_dm_p->register_timer(this, nullptr);
}

AsyncTPSStrategy::~AsyncTPSStrategy()
{
    if (m_writer) {
        m_writer->join();
    }
}

void AsyncTPSStrategy::init(const core::Config::storage_type& cfg)
{
    cfg.get("listen_port", m_listen_port);
}

void AsyncTPSStrategy::start()
{
    m_writer = new WorkThread(*this, name(), m_server,
                              [this](int fd, const sockaddr_in&, const socklen_t&) {
                                  return m_server.on_new_client(fd); });
    if (m_writer->add_listener_on_port(m_listen_port)) {
        std::cerr << "AsyncTPSStrategy: listening on port " << m_listen_port << std::endl;
         m_writer->spawn();
    } else {
        std::cerr << "AsyncTPSStrategy: start: could not create listener on port " << m_listen_port << std::endl;
        delete m_writer;
        m_writer = nullptr;
    }
}

void AsyncTPSStrategy::write_pending_events()
{
    auto evt = Event();
    while (m_queue.try_pop(evt)) {
        switch (evt.type) {
        case EventType::QUOTE:
            m_server.on_quote_update(evt.timestamp, evt.mic, evt.esi, evt.data.data.quote);
            break;
        case EventType::TRADE:
            m_server.on_trade_update(evt.timestamp, evt.mic, evt.esi, evt.data.data.trade);
            break;
        case EventType::UNKNOWN:
        default:
            break;
        }
    }

    // pull events off the async queue and pass them to the TPS server

}

AsyncTPSStrategy::WorkThread::WorkThread(AsyncTPSStrategy& tps, const std::string& n,
                                         net::SocketListener& sl,
                                         net::ConnectionListenerThread::ConnDataGenerator func) :
    ConnectionListenerThread(n, sl, func),
    m_tps(tps)
{
}

void * AsyncTPSStrategy::WorkThread::process()
{
    m_tps.write_pending_events();
    return net::ConnectionListenerThread::process();
}

}}
