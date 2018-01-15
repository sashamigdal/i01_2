#include <i01_md/BookBase.hpp>

#include <i01_ts/SamplerStrategy.hpp>

namespace i01 { namespace TS {

SamplerStrategy::SamplerStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string &n) :
    EquitiesStrategy(omp, dmp, n),
    m_output_format(OutputFormat::L1_TICKS),
    m_show_gaps(false),
    m_show_imbalances(false),
    m_offset_hhmmss("000000"),
    m_offset_millis((int) MIC::Enum::NUM_MIC, 0),
    m_valid_offset((int) MIC::Enum::NUM_MIC, false),
    m_next_interval((int) MIC::Enum::NUM_MIC),
    m_stocks_with_event((int)MIC::Enum::NUM_MIC),
    m_last_best((int)MIC::Enum::NUM_MIC),
    m_trading_status((int)MIC::Enum::NUM_MIC),
    m_verbose{false},
    m_show_trades{false}
{
    // look for ts.strategies.sampler.XXX
    // FIXME load params from conf
    m_interval_seconds = 60;
    auto cfg = core::Config::instance().get_shared_state();

    cfg->get("ts.strategies." + n + ".interval", m_interval_seconds);
    cfg->get("ts.strategies." + n + ".verbose", m_verbose);
    cfg->get("ts.strategies." + n + ".show_trades", m_show_trades);
    // this turn off output if true ... WHY??!
    m_quiet_mode = false;

    init_symbols();

    // m_dm_p->register_listener(this);
}

void SamplerStrategy::init_symbols()
{
    using namespace i01::MD::ConfigReader;

    auto cfg = core::Config::instance().get_shared_state();

    const auto & fdo = get_i01_vector_for_symbol(*cfg, "fdo_symbol");
    const auto & cta = get_i01_vector_for_symbol(*cfg, "cta_symbol");

    if (fdo.size()!=cta.size()) {
        std::cerr << "Expected to have equal number of FDO and CTA symbol names" << std::endl;
        return;
    }

    for (auto i= 0U; i < fdo.size(); i++) {
        if (fdo[i] != "" || cta[i] != "") {
            auto se = SymbolEntry{cta[i], boost::lexical_cast<std::uint32_t>(fdo[i]), std::to_string(i) +"," + cta[i] + "," + fdo[i] };
            if (i >= m_symbols.size()) {
                m_symbols.resize(i+1);
                m_symbols[i] = se;
            }
        }
    }

    std::cerr << "SamplerStrategy has " << m_symbols.size() << " symbols" << std::endl;
}

void SamplerStrategy::on_l2_update(const MD::L2BookEvent & be)
{
    if (m_trading_status[be.m_book.mic().index()][be.m_book.symbol_index()] != MD::TradingStatusEvent::Event::TRADING) {
        return;
    }

    m_stocks_with_event[be.m_book.mic().index()].insert(&be.m_book);

#ifdef I01_DEBUG_MESSAGING
    std::cerr << "OL2," << be << std::endl;
#endif
}

void SamplerStrategy::check_offset_(const MIC &mic, const Timestamp &ts)
{
    if (m_valid_offset[mic.index()]) {
        return;
    }

    try {
        std::uint32_t hh = boost::lexical_cast<std::uint32_t>(m_offset_hhmmss.substr(0,2));
        std::uint32_t mm = boost::lexical_cast<std::uint32_t>(m_offset_hhmmss.substr(2,2));
        std::uint32_t ss = boost::lexical_cast<std::uint32_t>(m_offset_hhmmss.substr(4,2));

        struct tm *tm_p = localtime(&ts.tv_sec);

        tm_p->tm_hour = hh;
        tm_p->tm_min = mm;
        tm_p->tm_sec = ss;
        time_t epoch = mktime(tm_p);

        tm_p = gmtime(&epoch);
        m_offset_millis[mic.index()] = (tm_p->tm_hour*3600 + tm_p->tm_min*60 + tm_p->tm_sec)*1000UL;
        // std::cout << "offset: " << m_offset_hhmmss << " epoch: " << epoch << " gm: " << m_offset_millis <<std::endl;

        m_next_interval[mic.index()] = Timestamp{ts.tv_sec  + (m_interval_seconds - (ts.tv_sec % m_interval_seconds)), 0};

    } catch(boost::bad_lexical_cast const &b) {
        std::cerr << "time offset argument not valid HHMMSS format" << std::endl;
        throw b;
    }
    m_valid_offset[mic.index()] = true;
}

void SamplerStrategy::on_symbol_definition(const MD::SymbolDefEvent &evt)
{
    if (m_verbose) {
        std::cerr << "SYMBDEF,"
                  << evt << std::endl;
    }
}

void SamplerStrategy::on_gap(const MD::GapEvent& evt)
{
    std::cerr << "GAP,"
              << evt << std::endl;
}

void SamplerStrategy::on_book_added(const MD::L3AddEvent &evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);
    if (m_verbose) {
        std::cerr << "OBA," << evt << std::endl;
    }
}

void SamplerStrategy::on_book_canceled(const MD::L3CancelEvent &evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);

    if (m_verbose) {
        std::cerr << "OBC," << evt << std::endl;
    }
}

void SamplerStrategy::on_book_modified(const MD::L3ModifyEvent & evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);

    if (m_verbose) {
        std::cerr << "OBM," << evt << std::endl;
    }
}
void SamplerStrategy::on_book_executed(const MD::L3ExecutionEvent & evt)
{
    m_stocks_with_event[evt.m_book.mic().index()].insert(&evt.m_book);

    if (m_show_trades) {
        std::cerr << "OBE," << evt << std::endl;
    }
}

void SamplerStrategy::output_l1_(const MIC &mic, std::uint32_t si, const MD::FullSummary &b)
{
    using i01::MD::operator<<;

    std::cout << mic.name() << ","
              << m_symbols[si].m_fdo_stock_id << ","
              << m_symbols[si].m_cta_symbol << ","
              << m_next_interval[mic.index()].tv_sec << ","
              << b
              << std::endl;
}

void SamplerStrategy::on_trading_status_update(const MD::TradingStatusEvent &evt)
{
    m_trading_status[evt.m_book.mic().index()][evt.m_book.symbol_index()] = evt.m_event;

    if (m_verbose) {
        std::cerr << "TSU," << evt << std::endl;
    }
}

void SamplerStrategy::on_trade(const MD::TradeEvent& evt)
{
    if (m_show_trades) {
        std::cout << "TRD," << evt << std::endl;
    }
}

void SamplerStrategy::on_start_of_data(const MD::PacketEvent& evt)
{
    // if (m_verbose) {
    //     std::cerr << "SOD," << evt.mic << std::endl;
    // }
}

void SamplerStrategy::on_end_of_data(const MD::PacketEvent &evt)
{
    // if (m_verbose) {
    //     std::cerr << "EOD," << evt.mic.name() << std::endl;
    // }

    const auto & mic = evt.mic;
    check_offset_(mic, evt.timestamp);

    if (evt.timestamp.milliseconds_since_midnight() < m_offset_millis[mic.index()]) {
        goto finished;
    }
    // check the time, and if it's past the interval time, then let's write out our prices for everything
    if (evt.timestamp > m_next_interval[mic.index()]) {
        auto tm = evt.timestamp;
        do {
            std::cout << mic.name() << ",INTERVAL,"
                      << m_next_interval[mic.index()].tv_sec << ","
                      << tm
                      << std::endl;
            for (const auto & b : m_last_best[mic.index()]) {
                // print out prices for everything here
                if (!m_quiet_mode) {
                    output_l1_(mic, b.first, b.second);
                }
            }
            tm += Timestamp{m_interval_seconds,0};
        } while (tm < m_next_interval[mic.index()]);

        // set next interval here
        m_next_interval[mic.index()].tv_sec += m_interval_seconds;
    }

    for (const auto & b : m_stocks_with_event[mic.index()]) {
        auto best = b->best();
        // if (std::get<0>(best.first) == std::get<0>(best.second)) {
        //     std::cerr << "ERR,LOCK," << mic.name() << "," << best << std::endl;
        // }

        if (best != m_last_best[b->mic().index()][b->symbol_index()]) {
            // new L1
            m_last_best[b->mic().index()][b->symbol_index()] = best;
        }
    }

 finished:
    m_stocks_with_event[mic.index()].clear();
}

const std::string & SamplerStrategy::id_str_(std::uint32_t si) const
{
    static std::string defstr;
    if (si >= m_symbols.size()) {
        defstr = std::to_string(si) + ",UNKNOWN,0,";
        return defstr;
    }
    return m_symbols[si].m_id_string;
}



}}

// using i01::TS::SamplerStrategy;

// extern "C" {

//     bool i01_strategy_plugin_init(const i01_StrategyPluginContext * ctx, const char * cfg_name) {
//         if (!ctx)
//             return false;
//         if (0 != std::strcmp(i01::g_COMMIT, ctx->git_commit))
//             return false;
//         auto * ea(reinterpret_cast<i01::apps::engine::EngineApp*>(ctx->engine_app));
//         ea->log().console()->notice() << "Initializing plugin for Strategy " << cfg_name << " version " << i01::g_COMMIT << " against engine " << ctx->git_commit;

//         i01::TS::Strategy * strat_p(
//                                     new SamplerStrategy(
//                                                         reinterpret_cast<i01::OE::OrderManager*>(ctx->order_manager)
//                                                         , reinterpret_cast<i01::MD::DataManager*>(ctx->data_manager)
//                                                         , cfg_name));
//         return (*(ctx->register_strategy_op))(reinterpret_cast<i01_strategy_t *>(strat_p));
//     }

//     void i01_strategy_plugin_exit(const i01_StrategyPluginContext * ctx, const char * cfg_name) {
//         if (!ctx)
//             return;
//         if (0 != std::strcmp(i01::g_COMMIT, ctx->git_commit))
//             return;

//         auto * ea(reinterpret_cast<i01::apps::engine::EngineApp*>(ctx->engine_app));
//         ea->log().console()->notice() << "Exiting plugin for Strategy " << cfg_name << " version " << i01::g_COMMIT << " against engine " << ctx->git_commit;
//     }

// }
