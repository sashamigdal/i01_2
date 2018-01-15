#pragma once

#include <i01_ts/NBBOEquitiesStrategy.hpp>

namespace i01 { namespace TS {


class PricerStrategy : public NBBOEquitiesStrategy {
public:
    using FullL2Quote = MD::FullL2Quote;

    class StockStats
    {
    // public:
    //     static void setSpreadXMALambda(double lambda);
    public:
        struct Event {
            Timestamp timestamp;
            FullL2Quote quote;
        };

    public:
        StockStats (double lambda);

        double spread(const FullL2Quote& me) const {
            return me.ask.price_as_double() - me.bid.price_as_double();
        }
        // double bid(const FullL2Quote& me) const {
        //     return MD::to_double(me.bid.price);
        // }
        // double ask(const FullL2Quote& me) const {
        //     return MD::to_double(me.ask.price);
        // }

        std::int64_t spreadFixed(const FullL2Quote& me) const {
            // return  (std::int64_t) (10000 * me.askPrice ())
            //     - (std::int64_t) (10000 * me.bidPrice ());
            return me.ask.price - me.bid.price;
        }

        void resetIntervalSpread(const Timestamp& t);
        void resetSpreadXMA(const Timestamp& t);

        double getSpreadXMA(const Timestamp& t) const;
        double getBidXMA(const Timestamp& t) const;
        double getAskXMA(const Timestamp& t) const;
        double getIntervalSpread(const Timestamp& t) const;

        void updateFromEvent(const Timestamp& t, const FullL2Quote& evt);
        void updateEventCounts(const FullL2Quote& evt);

    protected:

        void updateCountingStats(const Timestamp& t, const FullL2Quote& evt);
        void updateXMAStats(const Timestamp& t, const FullL2Quote& evt);
        void updateIntervalSpread(const Timestamp& t, const FullL2Quote& evt);

    protected:
        std::int64_t spreadNumerator_; //scale 10000
        std::int64_t availNumerator_;
        std::int64_t spreadIntervalNumerator_; //scale 10000

        std::int64_t numSamples_, numIntervalSamples_;

        Event lastEvent_, lastXMAEvent_, lastIntervalEvent_;

        bool haveEvent_, haveXMAEvent_,
            haveAnyXMAEvent_, haveAnyIntervalEvent_, haveIntervalEvent_;
        std::int64_t quotesSeen_, tradesSeen_;

        double lambda_;
        double spreadXMA_, bidXMA_, askXMA_;
    };


public:
    using NBBOEquitiesStrategy::NBBOEquitiesStrategy;

    virtual ~PricerStrategy() = default;

    virtual void on_nbbo_update(const Timestamp& ts, const core::MIC& mic, MD::EphemeralSymbolIndex eis, const MD::FullL2Quote& q) override final;

    virtual void on_timer(const Timestamp& ts, void * userdata, std::uint64_t iter) override final;
};

}}
