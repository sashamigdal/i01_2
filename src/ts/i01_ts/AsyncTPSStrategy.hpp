#pragma once

#include <i01_net/ConnectionListenerThread.hpp>
#include <i01_net/EventPoller.hpp>
// #include <i01_net/SocketListener.hpp>

#include <i01_ts/AsyncNBBOEquitiesStrategy.hpp>

#include "tps/TPSServer.hpp"

namespace i01 {

namespace net {
class ConnectionListenerThread;
class SocketListener;
}

namespace TS {

class AsyncTPSStrategy : public AsyncNBBOEquitiesStrategy {
public:
    static const std::uint16_t DEFAULT_LISTEN_PORT = 27784;

    class WorkThread : public net::ConnectionListenerThread {
    public:
        WorkThread(AsyncTPSStrategy& tps, const std::string& n, net::SocketListener& sl,
                   net::ConnectionListenerThread::ConnDataGenerator func = nullptr);

        virtual void * process() override;

    private:
        AsyncTPSStrategy& m_tps;

    };

public:
    AsyncTPSStrategy(OE::OrderManager* omp, MD::DataManager* dmp, const std::string& n);

    virtual ~AsyncTPSStrategy();

    virtual void start() override;

    int listen_port() const { return m_listen_port; }

    TPS::TPSServer& server() { return m_server; }

    void write_pending_events();

protected:
    void init(const core::Config::storage_type&);


private:
    TPS::TPSServer m_server;
    WorkThread *m_writer;
    std::uint16_t m_listen_port;

};

}}
