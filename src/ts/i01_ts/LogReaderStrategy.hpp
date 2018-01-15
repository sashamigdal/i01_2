#pragma once

#include <i01_core/Config.hpp>
#include <i01_core/NamedThread.hpp>

#include <i01_oe/BlotterReaderListener.hpp>

#include <i01_ts/Strategy.hpp>

namespace i01 {
namespace core {
class Timestamp;
}

namespace MD {
class DataManager;
}

namespace OE {
class OrderManager;
class FileBlotterReader;
}

namespace TS {

class LogReaderStrategy : public Strategy,
                          public OE::BlotterReaderListener {
public:
    using Timestamp = core::Timestamp;
    static constexpr const char * const DEFAULT_OUTPUT_PATH = "::cout";

    class FollowThread : public core::NamedThread<FollowThread> {
    public:
        FollowThread(OE::FileBlotterReader*, const std::string& input, std::ostream* ostream_p);

        virtual void * process() override final;

    private:
        OE::FileBlotterReader* m_reader_p;
    };

public:
    LogReaderStrategy(OE::OrderManager*, MD::DataManager*, const std::string& name);
    virtual ~LogReaderStrategy();

    void init(const core::Config::storage_type& cfg);

    virtual void start() override final;

    virtual void on_log_start(const Timestamp&) override final;
    virtual void on_log_end(const Timestamp&) override final;
    virtual void on_log_order_sent(const Timestamp&, const OE::OrderLogFormat::OrderSentBody&) override final;
    virtual void on_log_acknowledged(const Timestamp&, const OE::OrderLogFormat::OrderAckBody&) override final;

    virtual void on_log_local_reject(const Timestamp& ts, const OE::OrderLogFormat::OrderLocalRejectBody& b) override final;
    virtual void on_log_pending_cancel(const Timestamp& ts, const OE::OrderLogFormat::OrderCxlReqBody& b) override final;
    virtual void on_log_destroy(const Timestamp& ts, const OE::OrderLogFormat::OrderDestroyBody& b) override final;
    virtual void on_log_rejected(const Timestamp& ts, const OE::OrderLogFormat::OrderRejectBody& b) override final;
    virtual void on_log_filled(const Timestamp& ts, bool partial_fill, const OE::OrderLogFormat::OrderFillBody& b) override final;
    virtual void on_log_cancel_rejected(const Timestamp& ts, const OE::OrderLogFormat::OrderCxlRejectBody& b) override final;
    virtual void on_log_cancelled(const Timestamp& ts, const OE::OrderLogFormat::OrderCxlBody& b) override final;
    virtual void on_log_add_session(const Timestamp& ts, const OE::OrderLogFormat::NewSessionBody& b) override final;
    virtual void on_log_add_strategy(const Timestamp& ts, const OE::OrderLogFormat::NewAccountBody& b) override final;
    virtual void on_log_manual_position_adj(const Timestamp& ts, const OE::OrderLogFormat::ManualPositionAdjBody& b) override final;

private:
    OE::FileBlotterReader* m_reader_p;
    FollowThread* m_thread_p;
    std::string m_path;
    std::string m_output_path;
    std::ostream* m_ostream_p;
    bool m_follow;
};
}}
