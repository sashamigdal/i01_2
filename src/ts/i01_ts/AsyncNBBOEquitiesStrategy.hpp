#pragma once

#include <i01_md/Symbol.hpp>

#include <i01_ts/NBBOEquitiesStrategy.hpp>

#include <tbb/concurrent_queue.h>

namespace i01 { namespace TS {


class AsyncNBBOEquitiesStrategy : public NBBOEquitiesStrategy {
public:

    using NBBOEquitiesStrategy::NBBOEquitiesStrategy;
    virtual ~AsyncNBBOEquitiesStrategy() = default;

    virtual void on_nbbo_update(const Timestamp& ts, const core::MIC& m, MD::EphemeralSymbolIndex esi,
                                const MD::FullL2Quote& q) override;
    virtual void on_trade(const MD::TradeEvent&) override;


protected:
    enum class EventType : std::uint8_t {
        UNKNOWN                 = 0
    , QUOTE                     = 1
    , TRADE                     = 2
    };

    struct EventData {
        union Data {
            MD::FullL2Quote quote;
            MD::TradePair trade;
            Data() : quote{} { }
        } data;
        EventData() : data{} {}
        EventData(const MD::FullL2Quote& q) { data.quote = q;}
        EventData(const MD::TradePair& t) { data.trade = t; }
        // EventData(const EventData& e) : data(e.data) {}
        EventData& operator=(const EventData& e) { data.quote = e.data.quote; return *this; }

    };

    struct Event {
        Timestamp timestamp;
        core::MIC mic;
        MD::EphemeralSymbolIndex esi;
        EventType type;
        EventData data;
        Event() : timestamp{}, mic{}, esi{}, type{}, data{} {}
        Event(const Event& e) : timestamp(e.timestamp), mic(e.mic), esi(e.esi), type(e.type),
                                data(e.data) {}
        Event(const Timestamp& ts, const core::MIC& m, MD::EphemeralSymbolIndex e,
              EventType et, const MD::FullL2Quote& q) :
            timestamp(ts), mic(m), esi(e), type(et), data(q) {
        }
        Event(const Timestamp& ts, const core::MIC& m, MD::EphemeralSymbolIndex e,
              EventType et, const MD::Price p, const MD::Size s) :
            timestamp(ts), mic(m), esi(e), type(et), data(std::make_pair(p,s)) {
        }
        Event& operator=(const Event& e) {
            timestamp = e.timestamp;
            mic = e.mic;
            esi = e.esi;
            type = e.type;
            data = e.data;
            return *this;
        }
    };

    using EventQueue = tbb::concurrent_queue<Event>;

protected:
    EventQueue m_queue;

};


}}
