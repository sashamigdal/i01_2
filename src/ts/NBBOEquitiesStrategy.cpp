#include <i01_oe/OrderManager.hpp>
#include <i01_ts/NBBOEquitiesStrategy.hpp>

namespace i01 { namespace TS {

NBBOEquitiesStrategy::NBBOEquitiesStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string& n) :
    L1EquitiesStrategy(omp,dmp,n)
{
    m_status_policy_bitfield = static_cast<TradingStateBitfield>(MD::SymbolState::TradingState::TRADING);
    // in case some venues do not send trading status updates when they start (BATS?)
    for (int i = 0; i < (int) core::MIC::Enum::NUM_MIC; i++) {
        m_venue_enabled[i].fill(true);
    }
}


bool NBBOEquitiesStrategy::update_book(const Timestamp& ts,
                                       const core::MIC& mic,
                                       MD::EphemeralSymbolIndex esi,
                                       MD::L3OrderData::Side side,
                                       const MD::L2Quote& q)
{
    MD::L2Quote priorq = MD::L3OrderData::Side::BUY == side ? m_books[esi].best_bid() : m_books[esi].best_ask();

    auto refnum = make_refnum(mic.index(),side);

    auto* p = m_books[esi].find(refnum);

    if (nullptr == p) {
        // due to data gapping, we could be here with an empty book
        // (e.g. if we start middle of day and the first thing we get
        // for a stock is a cancel on an order...)
        if ((MD::L3OrderData::Side::BUY == side && q.price != MD::BookBase::EMPTY_BID_PRICE)
            || (MD::L3OrderData::Side::SELL == side && MD::BookBase::EMPTY_ASK_PRICE != q.price)) {
            m_books[esi].add(make_new_order(refnum, ts, q));
        }
    } else {
        // has the price changed?
        if (q.price != p->price) {
            // then we have to replace .. unless it's a sentinel price
            if ((MD::L3OrderData::Side::BUY == side && q.price == MD::BookBase::EMPTY_BID_PRICE)
                || (MD::L3OrderData::Side::SELL == side && MD::BookBase::EMPTY_ASK_PRICE == q.price)) {
                m_books[esi].erase(*p);
            } else {
                m_books[esi].replace(refnum, make_new_order(refnum, ts, q));
            }
        } else {
            // just a size change
            m_books[esi].modify(*p, q.size);
        }
    }

    // if the BBO on the book has changed, then it's an NBBO event
    MD::L2Quote newq = MD::L3OrderData::Side::BUY == side ? m_books[esi].best_bid() : m_books[esi].best_ask();
    return newq != priorq;
}

MD::OrderBook::Order NBBOEquitiesStrategy::make_new_order(MD::L3OrderData::RefNum refnum, const Timestamp& ts, const MD::L2Quote& q)
{
    return MD::L3OrderData(refnum, refnum_to_side(refnum), q.price, q.size,
                           MD::L3OrderData::TimeInForce::DAY, ts, ts);
}

void NBBOEquitiesStrategy::on_bbo_update(const Timestamp& ts,
                                         const core::MIC& mic,
                                         const MD::EphemeralSymbolIndex& esi,
                                         const MD::L3OrderData::Side& side,
                                         const MD::L2Quote& q)
{
    // std::cerr << "BBO," << ts << "," << esi << "," << mic << ","
    //           << m_om_p->universe()[esi].cta_symbol() << ","
    //           << (side == MD::L3OrderData::Side::BUY ? "BUY," : "SELL,") << q << std::endl;
    // for a given stock, each MIC+side is an order in an orderbook

    if (!m_venue_enabled[mic.index()][esi]) {
        return;
    }

    if (update_book(ts, mic, esi, side, q)) {
        on_nbbo_update(ts, mic, esi, m_books[esi].best());
    }
}

void NBBOEquitiesStrategy::on_bbo_update(const Timestamp& ts,
                                         const core::MIC& mic,
                                         const MD::EphemeralSymbolIndex& esi,
                                         const MD::FullL2Quote& q)
{
    // std::cerr << "FQBBO," << ts << "," << esi << "," << mic << ","
    //           << m_om_p->universe()[esi].cta_symbol() << ","
    //           << q << std::endl;

    if (!m_venue_enabled[mic.index()][esi]) {
        return;
    }

    if (update_book(ts, mic, esi, MD::L3OrderData::Side::BUY, q.bid)
        || update_book(ts, mic, esi, MD::L3OrderData::Side::SELL, q.ask)) {
        on_nbbo_update(ts, mic, esi, m_books[esi].best());
    }
}

const MD::OrderBook& NBBOEquitiesStrategy::bbo_book(MD::EphemeralSymbolIndex esi) const
{
    return m_books[esi];
}

void NBBOEquitiesStrategy::enable_venue(const int mic_index, const MD::EphemeralSymbolIndex esi)
{
    m_venue_enabled[mic_index][esi] = true;
}

void NBBOEquitiesStrategy::disable_venue(const int mic_index, const MD::EphemeralSymbolIndex esi)
{
    if (m_venue_enabled[mic_index][esi]) {
        // we were passing through but now stopping
        // clear from consolidated book
        auto refnum = make_refnum(mic_index,MD::L3OrderData::Side::BUY);
        m_books[esi].remove(refnum);
        refnum = make_refnum(mic_index,MD::L3OrderData::Side::SELL);
        m_books[esi].remove(refnum);
    }
    m_venue_enabled[mic_index][esi] = false;
}

void NBBOEquitiesStrategy::update_venue_from_status(const core::MIC& mic,
                                                    const MD::EphemeralSymbolIndex index,
                                                    const MD::SymbolState::TradingState ts)
{
    if ((m_status_policy_bitfield & static_cast<TradingStateBitfield>(ts)) != 0) {
        enable_venue(mic.index(), index);
    } else {
        disable_venue(mic.index(), index);
    }
}

void NBBOEquitiesStrategy::on_trading_status_update(const MD::TradingStatusEvent& evt)
{
    // check whether we are passing quotes through here
    switch (evt.m_event) {
    case MD::TradingStatusEvent::Event::TRADING:
        update_venue_from_status(evt.m_book.mic(), evt.m_book.symbol_index(),
                                 MD::SymbolState::TradingState::TRADING);
        break;

    case MD::TradingStatusEvent::Event::HALT:
        update_venue_from_status(evt.m_book.mic(), evt.m_book.symbol_index(),
                                 MD::SymbolState::TradingState::HALTED);
        break;

    case MD::TradingStatusEvent::Event::QUOTE_ONLY:
        update_venue_from_status(evt.m_book.mic(), evt.m_book.symbol_index(),
                                 MD::SymbolState::TradingState::QUOTATION_ONLY);
        break;

    case MD::TradingStatusEvent::Event::SSR_IN_EFFECT:
        break;

    case MD::TradingStatusEvent::Event::SSR_NOT_IN_EFFECT:
        break;

    case MD::TradingStatusEvent::Event::PRE_MKT_SESSION:
        break;

    case MD::TradingStatusEvent::Event::REGULAR_MKT_SESSION:
        break;

    case MD::TradingStatusEvent::Event::POST_MKT_SESSION:
        break;

    case MD::TradingStatusEvent::Event::UNKNOWN:
        break;

    default:
        break;
    }
}

void NBBOEquitiesStrategy::enable_quotes_in_state(const MD::SymbolState::TradingState s)
{
    m_status_policy_bitfield |= static_cast<TradingStateBitfield>(s);
}

void NBBOEquitiesStrategy::disable_quotes_in_state(const MD::SymbolState::TradingState s)
{
    m_status_policy_bitfield = static_cast<TradingStateBitfield>(m_status_policy_bitfield & ~static_cast<TradingStateBitfield>(s));
}

}}
