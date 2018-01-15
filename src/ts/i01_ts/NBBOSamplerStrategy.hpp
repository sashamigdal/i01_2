#pragma once

#include <i01_core/Config.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/TimerListener.hpp>

#include <i01_ts/NBBOEquitiesStrategy.hpp>

namespace i01 { namespace TS {


class NBBOSamplerStrategy : public NBBOEquitiesStrategy {
public:
    NBBOSamplerStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string& n);
    virtual ~NBBOSamplerStrategy() = default;

    void init(const core::Config::storage_type& cfg);

    virtual void on_nbbo_update(const Timestamp& ts, const core::MIC& mic, MD::EphemeralSymbolIndex eis, const MD::FullL2Quote& q) override final;

    virtual void on_timer(const Timestamp& ts, void * userdata, std::uint64_t iter) override final;

protected:
    void narrow_output_top_level(const Timestamp& ts, const MD::OrderBook& book,
                                 const std::string& fdo_symbol,
                                 const std::string& cta_symbol, std::ostream&);


    template<typename Iter>
    void narrow_output_orders(const Timestamp& ts, Iter start, Iter end, const std::string& fdo_symbol,
                              const std::string& cta_symbol, std::ostream&);

private:
    using Mutex = core::SpinRWMutex;
    using QuoteArray = std::array<MD::FullL2Quote,MD::NUM_SYMBOL_INDEX>;

private:
    std::uint64_t m_timer_count;
    std::uint64_t m_interval;
    std::uint64_t m_start_time_ms_since_midnight;
    Mutex m_mutex;
    QuoteArray m_quotes;
    bool m_narrow_output;
    bool m_crossed_only;
};



template<typename Iter>
void NBBOSamplerStrategy::narrow_output_orders(const Timestamp& ts, Iter strt, Iter end,
                                               const std::string& fdo_symbol,
                                               const std::string& cta_symbol,
                                               std::ostream& os)
{
    while (strt != end) {
        const auto& pl = *strt++;
        for (const auto& o : pl.second) {
            os << ts << ","
               << core::MIC::clone(static_cast<std::uint8_t>(refnum_to_index(o->refnum))) << ","
               << fdo_symbol << ","
               << cta_symbol << ","
               << *o << std::endl;
        }
    }
}

}}
