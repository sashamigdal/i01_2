#include <i01_md/DataManager.hpp>
#include <i01_md/L2Book.hpp>

#include <i01_ts/L1EquitiesStrategy.hpp>

namespace i01 { namespace TS {

void L1EquitiesStrategy::UpdateStocksSet::insert(StockSide ss)
{
    for (auto& e : *this) {
        if (e.esi == ss.esi) {
            e.side_flags |= ss.side_flags;
            return;
        }
    }

    // means we didn't find this stock
    emplace_back(ss);
}

// /* BookMuxListener */
void L1EquitiesStrategy::on_book_added(const MD::L3AddEvent& evt)
{
    Mutex::scoped_lock lock(m_mic_mutexes[evt.m_book.mic().index()]);
    m_updated_stocks[evt.m_book.mic().index()].insert({evt.m_book.symbol_index(), (std::uint8_t)evt.m_order.side});
}

void L1EquitiesStrategy::on_book_canceled(const MD::L3CancelEvent& evt)
{
    Mutex::scoped_lock lock(m_mic_mutexes[evt.m_book.mic().index()]);
    m_updated_stocks[evt.m_book.mic().index()].insert({evt.m_book.symbol_index(), (std::uint8_t)evt.m_order.side});
}

void L1EquitiesStrategy::on_book_modified(const MD::L3ModifyEvent& evt)
{
    Mutex::scoped_lock lock(m_mic_mutexes[evt.m_book.mic().index()]);
    m_updated_stocks[evt.m_book.mic().index()].insert({evt.m_book.symbol_index(), (std::uint8_t)evt.m_new_order.side});
}

void L1EquitiesStrategy::on_book_executed(const MD::L3ExecutionEvent& evt)
{
    Mutex::scoped_lock lock(m_mic_mutexes[evt.m_book.mic().index()]);
    m_updated_stocks[evt.m_book.mic().index()].insert({evt.m_book.symbol_index(), (std::uint8_t)evt.m_order.side});
}

/// NYSE is a precious snowflake.
void L1EquitiesStrategy::on_l2_update(const MD::L2BookEvent& evt)
{
    Mutex::scoped_lock lock(m_mic_mutexes[evt.m_book.mic().index()]);
    if (evt.m_is_bbo) {
        m_updated_stocks[evt.m_book.mic().index()].insert({evt.m_book.symbol_index(), (std::uint8_t) (evt.m_is_buy ? MD::L3OrderData::Side::BUY : MD::L3OrderData::Side::SELL)});
    }
}


void L1EquitiesStrategy::on_end_of_data(const MD::PacketEvent& evt)
{
    // based on MIC, look up all stocks that have changed from this packet, and then
    // get their best quote and compare to last

    // lock for this MIC
    Mutex::scoped_lock lock(m_mic_mutexes[evt.mic.index()]);

    // for each stock that has been updated, get the best appropriate side and compare to what we have

    // we have to get the book from the book mux from the data manager
    // ... would be nicer if we got a pointer to the book in the
    // update and kept that instead
    for (const auto& s : m_updated_stocks[evt.mic.index()]) {
        auto& bm = *m_dm_p->get_bm(evt.mic);
        auto * book = bm[s.esi];
        if (UNLIKELY(nullptr == book)) {
            continue;
        }

        auto& entry = m_quotes[evt.mic.index()][s.esi];
        // lock for this stock
        Mutex::scoped_lock stock_lock(entry.mutex);

        auto both = ((std::uint8_t) MD::L3OrderData::Side::BUY | (std::uint8_t) MD::L3OrderData::Side::SELL);
        if ((s.side_flags & both) == both) {
            auto bq = book->best_quote();

            if (bq != entry.quote) {
                on_bbo_update(evt.timestamp, evt.mic, s.esi, bq);
                entry.quote = bq;
                entry.bid_ts = evt.timestamp;
                entry.ask_ts = evt.timestamp;
            }
        } else {
            if ((s.side_flags & (std::uint8_t) MD::L3OrderData::Side::BUY) != 0) {
                auto bb = book->best_bid_quote();

                if (bb != entry.quote.bid) {
                    on_bbo_update(evt.timestamp, evt.mic, s.esi, MD::L3OrderData::Side::BUY, bb);
                    entry.quote.bid = bb;
                    entry.bid_ts = evt.timestamp;

                }
            }

            if ((s.side_flags & (std::uint8_t) MD::L3OrderData::Side::SELL) != 0) {
                auto ba = book->best_ask_quote();
                if (ba != entry.quote.ask) {
                    on_bbo_update(evt.timestamp, evt.mic, s.esi, MD::L3OrderData::Side::SELL, ba);
                    entry.quote.ask = ba;
                    entry.ask_ts = evt.timestamp;

                }
            }
        }
    }
    m_updated_stocks[evt.mic.index()].clear();
}

MD::FullL2Quote L1EquitiesStrategy::unsafe_best_quote(const core::MIC& mic, MD::EphemeralSymbolIndex esi) const
{
    return m_quotes[mic.index()][esi].quote;
}

MD::L2Quote L1EquitiesStrategy::unsafe_best_bid_quote(const core::MIC& mic, MD::EphemeralSymbolIndex esi) const
{
    return m_quotes[mic.index()][esi].quote.bid;
}

MD::L2Quote L1EquitiesStrategy::unsafe_best_ask_quote(const core::MIC& mic, MD::EphemeralSymbolIndex esi) const
{
    return m_quotes[mic.index()][esi].quote.ask;
}

core::Timestamp L1EquitiesStrategy::unsafe_last_ts(const core::MIC& mic, MD::EphemeralSymbolIndex esi) const
{
    const auto& e = m_quotes[mic.index()][esi];
    return e.bid_ts >= e.ask_ts ? e.bid_ts : e.ask_ts;
}



} }
