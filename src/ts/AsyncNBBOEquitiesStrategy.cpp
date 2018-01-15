#include <i01_ts/AsyncNBBOEquitiesStrategy.hpp>

namespace i01 { namespace TS {

void AsyncNBBOEquitiesStrategy::on_nbbo_update(const Timestamp& ts, const core::MIC& m,
                                               MD::EphemeralSymbolIndex esi,
                                               const MD::FullL2Quote& q)
{
    m_queue.push(Event{ts, m, esi, EventType::QUOTE, q});
}

void AsyncNBBOEquitiesStrategy::on_trade(const MD::TradeEvent& e)
{
    // TODO: add to event queue here
    m_queue.push(Event{e.m_timestamp, e.m_book.mic(), e.m_book.symbol_index(),
                EventType::TRADE, e.m_price, static_cast<MD::Size>(e.m_size)});
}


}}
