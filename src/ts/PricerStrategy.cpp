#include <math.h>

#include <i01_oe/OrderManager.hpp>

#include <i01_ts/PricerStrategy.hpp>

namespace i01 { namespace TS {

void PricerStrategy::on_nbbo_update(const Timestamp& ts, const core::MIC& mic,
                                    MD::EphemeralSymbolIndex esi,
                                    const MD::FullL2Quote& q)
{

}


//ctor
PricerStrategy::StockStats::StockStats (double lambda)
    : spreadNumerator_ (0),
      availNumerator_ (0),
      spreadIntervalNumerator_(0),
      numSamples_ (0),
      numIntervalSamples_(-1),
      haveEvent_ (false),
      haveXMAEvent_(false),
      haveAnyXMAEvent_(false),
      haveAnyIntervalEvent_(false),
      haveIntervalEvent_(false),
      quotesSeen_(0),
      tradesSeen_(0),
      lambda_(lambda),
      spreadXMA_(-1.0)

{
}

/**
 * This gets  called  everytime  we receive  a  valid  event.  It will
 * dispatch    the event to     various and sundry  statistics keeping
 * functions.
 *
 */
void
PricerStrategy::StockStats::updateFromEvent(const Timestamp &t, const FullL2Quote& evt)
{
    updateCountingStats(t, evt);
    updateXMAStats(t, evt);
    updateIntervalSpread(t, evt);
}

double
PricerStrategy::StockStats::getSpreadXMA(const Timestamp& t) const
{
    if (spreadXMA_ < 0) {
        return (haveXMAEvent_ || haveAnyXMAEvent_) ?
            spread(lastXMAEvent_.quote) : 0.0;
    }

    double tmp = spreadXMA_;
    auto n = static_cast<double>(t.tv_sec - lastXMAEvent_.timestamp.tv_sec);
    tmp = (1-::pow(lambda_,n))*spread(lastXMAEvent_.quote) + ::pow(lambda_,n)*tmp;
    return tmp;
}

double
PricerStrategy::StockStats::getBidXMA(const Timestamp& t) const
{
    if (bidXMA_ < 0) {
        return (haveXMAEvent_ || haveAnyXMAEvent_) ?
            lastXMAEvent_.quote.bid.price_as_double() : 0.0;
    }

    double tmp = bidXMA_;
    auto n = static_cast<double>(t.tv_sec - lastXMAEvent_.timestamp.tv_sec);
    tmp = (1-::pow(lambda_,n))*lastXMAEvent_.quote.bid.price_as_double() + ::pow(lambda_,n)*tmp;
    return tmp;
}

double
PricerStrategy::StockStats::getAskXMA(const Timestamp &t) const
{
    if (askXMA_ < 0) {
        return (haveXMAEvent_ || haveAnyXMAEvent_) ?
            lastXMAEvent_.quote.ask.price_as_double() : 0.0;
    }

    double tmp = askXMA_;
    auto n = static_cast<double>(t.tv_sec - lastXMAEvent_.timestamp.tv_sec);
    tmp = (1-::pow(lambda_,n))*lastXMAEvent_.quote.ask.price_as_double() + ::pow(lambda_,n)*tmp;
    return tmp;
}

/**
 * This does not store the updated spread.  If no events have been seen yet, it returns 0.
 *
 * @return the equally weighted spread from the prior reset until time t
 */

double
PricerStrategy::StockStats::getIntervalSpread(const Timestamp &t) const
{
    if (numIntervalSamples_ < 0) {
        return (haveIntervalEvent_ || haveAnyIntervalEvent_) ?
            spread(lastIntervalEvent_.quote) : 0.0;
    }

    auto n = t.tv_sec - lastIntervalEvent_.timestamp.tv_sec;
    int64_t evtSpread = spreadFixed(lastIntervalEvent_.quote);

    int64_t num = spreadIntervalNumerator_ + evtSpread*n;

    double spr = (double)(num) / (double)(numIntervalSamples_ + n) / 10000.0;

    return spr;
}

void
PricerStrategy::StockStats::resetSpreadXMA(const Timestamp& t)
{
    if (!haveAnyXMAEvent_) {
        return;
    }

    haveXMAEvent_ = false;
    spreadXMA_ = spread(lastXMAEvent_.quote);

    if (lastXMAEvent_.timestamp != t) {
        lastXMAEvent_.timestamp = t;
        // make a new market event with this time
        // FullL2Quote *me = &lastXMAEvent_;
        // lastXMAEvent_ = FullL2Quote(me->inst().type(), me->inst().id(), me->date(),
        //                             t,
        //                             me->bidPrice(), me->bidSize(),
        //                             me->askPrice(), me->askSize(),
        //                             me->exchange());
    }
}

/**
 * This starts a new time interval at t.  If  there was a prior event,
 * it gets copied with  its time set  to t, to  correspond to a pseudo
 * event at the beginning of the interval.  Resets the accumulator to 0.
 *
 */
void
PricerStrategy::StockStats::resetIntervalSpread(const Timestamp& t)
{
    if (!haveAnyIntervalEvent_) {
        return;
    }

    haveIntervalEvent_ = false;
    spreadIntervalNumerator_ = 0;
    numIntervalSamples_ = 0;

    if (lastIntervalEvent_.timestamp != t) {
        // make a new market event with this time
        lastIntervalEvent_.timestamp = t;
        // FullL2Quote *me = &lastIntervalEvent_;
        // lastIntervalEvent_ = FullL2Quote(me->inst().type(), me->inst().id(), me->date(),
        //                                  t,
        //                                  me->bidPrice(), me->bidSize(),
        //                                  me->askPrice(), me->askSize(),
        //                                  me->exchange());
    }
}

void
PricerStrategy::StockStats::updateXMAStats(const Timestamp& t, const FullL2Quote& evt)
{
    if (!haveAnyXMAEvent_) {
        haveAnyXMAEvent_ = true;
        lastXMAEvent_ = {t, evt};
        spreadXMA_ = (1.0 - lambda_) * spread(evt);
        bidXMA_ = evt.bid.price_as_double();
        askXMA_ = evt.ask.price_as_double();
        return;
    }

    auto n = static_cast<double>(t.tv_sec - lastXMAEvent_.timestamp.tv_sec);

    if (n > 0) {
        spreadXMA_ = (1.0 - ::pow(lambda_,n))*spread(lastXMAEvent_.quote) + ::pow(lambda_,n)*spreadXMA_;
        bidXMA_ = (1.0 - ::pow(lambda_,n))*lastXMAEvent_.quote.bid.price_as_double() + ::pow(lambda_,n)*bidXMA_;
        askXMA_ = (1.0 - ::pow(lambda_,n))*lastXMAEvent_.quote.ask.price_as_double() + ::pow(lambda_,n)*askXMA_;
    }
    lastXMAEvent_ = {t,evt};
}


void
PricerStrategy::StockStats::updateCountingStats(const Timestamp& t, const FullL2Quote& evt)
{
    if (!haveEvent_) {
        lastEvent_ = {t,evt};
        haveEvent_ = true;
        return;
    }

    int64_t evtSpread = spreadFixed(lastEvent_.quote);
    int64_t evtAvail = (lastEvent_.quote.bid.size + lastEvent_.quote.ask.size) / 2;

    // one sample per second:
    for (auto i = lastEvent_.timestamp.tv_sec; i < t.tv_sec; i++) {
        spreadNumerator_ += evtSpread;
        availNumerator_ += evtAvail;
        numSamples_++;
    }

    lastEvent_ = {t,evt};
    // if (evt) {
    //     lastEvent_ = *evt;
    // } else {
    //     haveEvent_ = false;
    // }
}

void PricerStrategy::StockStats::updateEventCounts(const FullL2Quote& evt)
{
    quotesSeen_++;
    // switch(evt->type()){
    // case QUOTE_EVENT_TYPE:

    //     break;
    // case TRADE_EVENT_TYPE:
    //     tradesSeen_++;
    //     break;

    // }
}


/**
 * This keeps track of the events used to compute spread, and it
 * computes the intermediate value of the EW average spread for events
 * that occur prior to the end of the interval.  When called, it
 * computes the accumulated spread up through the *prior* quote, and
 * stores the current quote, which will be intergrated into the
 * average at the next event.
 *
 */
void
PricerStrategy::StockStats::updateIntervalSpread(const Timestamp& t, const FullL2Quote& evt)
{
    if (!haveAnyIntervalEvent_) {
        // ignore if the event is null
        haveAnyIntervalEvent_ = true;
        lastIntervalEvent_ = {t,evt};
        spreadIntervalNumerator_ = 0;
        numIntervalSamples_ = 0;
        return;
    }

    int64_t evtSpread = spreadFixed(lastIntervalEvent_.quote);

    // one sample per second
    auto n = t.tv_sec - lastIntervalEvent_.timestamp.tv_sec;

    spreadIntervalNumerator_ += evtSpread*n;
    numIntervalSamples_ += n;

    if (!haveIntervalEvent_) {
        haveIntervalEvent_ = true;
        if (0 == n) {
            spreadIntervalNumerator_ = 0;
            numIntervalSamples_ = 0;
        }
    }

    lastIntervalEvent_ = {t,evt};
}

}}
