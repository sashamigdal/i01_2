#include <i01_md/DataManager.hpp>

#include <i01_oe/OrderManager.hpp>

#include <i01_ts/NBBOSamplerStrategy.hpp>

namespace i01 { namespace TS {

NBBOSamplerStrategy::NBBOSamplerStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string& n) :
    NBBOEquitiesStrategy(omp,dmp,n),
    m_timer_count(0),
    m_interval(60),
    m_start_time_ms_since_midnight(0),
    m_narrow_output{false},
    m_crossed_only{false}
{
    auto cfg = core::Config::instance().get_shared_state()->copy_prefix_domain("ts.strategies." + name() + ".");
    init(*cfg);

    m_dm_p->register_timer(this,nullptr);
}

void NBBOSamplerStrategy::init(const core::Config::storage_type& cfg)
{
    cfg.get("interval", m_interval);
    cfg.get("narrow-output", m_narrow_output);
    cfg.get("crossed-only", m_crossed_only);

    // TODO: need to convert this to UTC!!
    cfg.get("start-time-seconds-since-midnight", m_start_time_ms_since_midnight);
    m_start_time_ms_since_midnight *= 1000;
}



void NBBOSamplerStrategy::on_nbbo_update(const Timestamp &ts, const core::MIC &mic, MD::EphemeralSymbolIndex esi, const MD::FullL2Quote &q)
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ true);
    m_quotes[esi] = q;
}

void NBBOSamplerStrategy::on_timer(const Timestamp& ts, void * userdata, std::uint64_t iter)
{
    if (++m_timer_count % m_interval == 0
        && ts.milliseconds_since_midnight() >= m_start_time_ms_since_midnight) {
        Mutex::scoped_lock lock(m_mutex, /*write=*/ false);
        for (auto i = 0U; i < m_quotes.size(); i++) {
            const auto& q = m_quotes[i];
            auto esi = static_cast<MD::EphemeralSymbolIndex>(i);


            if (q.bid.price != MD::BookBase::EMPTY_BID_PRICE
                || q.ask.price != MD::BookBase::EMPTY_ASK_PRICE) {
                if (m_crossed_only && q.bid.price <= q.ask.price) {
                    return;
                }

                if (m_narrow_output) {
                    // not strictly thread safe...
                    narrow_output_top_level(ts, bbo_book(esi), m_om_p->universe()[esi].fdo_symbol_string(),
                                            m_om_p->universe()[esi].cta_symbol(), std::cout);
                } else {
                    std::cout << ts << ","
                              << esi << ","
                              << m_om_p->universe()[esi].fdo_symbol_string() << ","
                              << m_om_p->universe()[esi].cta_symbol() << ","
                              << q
                              << std::endl;
                }
            }
        }
    }
}

void NBBOSamplerStrategy::narrow_output_top_level(const Timestamp& ts, const MD::OrderBook& book,
                                                  const std::string& fdo_symbol,
                                                  const std::string& cta_symbol, std::ostream& os)
{
    auto bb = book.bids().begin();
    if (bb != book.bids().end()) {
        narrow_output_orders(ts, book.bids().begin(), ++bb, fdo_symbol, cta_symbol, os);
    }

    auto aa = book.asks().begin();
    if (aa != book.asks().end()) {
        narrow_output_orders(ts, book.asks().begin(), ++aa, fdo_symbol, cta_symbol, os);
    }

}

}}
