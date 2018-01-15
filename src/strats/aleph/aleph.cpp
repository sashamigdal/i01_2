#include <cstring>
#include <iostream>
#include <algorithm>

#include <i01_core/Version.hpp>
#include <i01_ts/StrategyPlugin.hpp>
#include <i01_ts/EquitiesStrategy.hpp>
#include <i01_ts/engine.hpp>

#include "aleph.hpp"

namespace i01 { namespace TS {

AlephStrategy::AlephStrategy(OE::OrderManager * omp, MD::DataManager *dmp, const std::string& n)
    : i01::TS::EquitiesStrategy(omp, dmp, n)
{
}

AlephStrategy::~AlephStrategy()
{
}

void AlephStrategy::on_order_acknowledged(const OE::Order*)
{
}

void AlephStrategy::on_order_fill(const OE::Order*, const OE::Size, const OE::Price, const OE::Price fee)
{
}

void AlephStrategy::on_order_cancel(const OE::Order*, const OE::Size)
{
}

void AlephStrategy::on_order_cancel_rejected(const OE::Order*)
{
}

void AlephStrategy::on_order_cancel_replaced(const OE::Order*)
{
}

void AlephStrategy::on_order_rejected(const OE::Order*)
{
}

void AlephStrategy::on_symbol_definition(const MD::SymbolDefEvent &)
{
}

void AlephStrategy::on_book_added(const MD::L3AddEvent&)
{
}

void AlephStrategy::on_book_canceled(const MD::L3CancelEvent&)
{
}

void AlephStrategy::on_book_modified(const MD::L3ModifyEvent&)
{
}

void AlephStrategy::on_book_executed(const MD::L3ExecutionEvent&)
{
}

void AlephStrategy::on_book_crossed(const MD::L3CrossedEvent&)
{
}

void AlephStrategy::on_timeout_event(const MD::TimeoutEvent&)
{
}

/// NYSE is a precious snowflake.
void AlephStrategy::on_l2_update(const MD::L2BookEvent&)
{
}

void AlephStrategy::on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent&)
{
}

void AlephStrategy::on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent&)
{
}

void AlephStrategy::on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent& e)
{
}

void AlephStrategy::on_trade(const MD::TradeEvent&)
{
}

void AlephStrategy::on_gap(const MD::GapEvent&)
{
}

void AlephStrategy::on_trading_status_update(const MD::TradingStatusEvent&)
{
}

void AlephStrategy::on_feed_event(const MD::FeedEvent&)
{
}

void AlephStrategy::on_market_event(const MD::MWCBEvent&)
{
}

void AlephStrategy::on_start_of_data(const MD::PacketEvent&)
{
}

void AlephStrategy::on_end_of_data(const MD::PacketEvent&)
{
}


void AlephStrategy::on_timer(const core::Timestamp& I01_UNUSED ts, void * I01_UNUSED userdata, std::uint64_t I01_UNUSED iter)
{
}

} } // namespace i01::TS

using i01::TS::AlephStrategy;

extern "C" {

bool i01_strategy_plugin_init(const i01_StrategyPluginContext * ctx, const char * cfg_name) {
    if (!ctx)
        return false;
    if (0 != std::strcmp(i01::g_COMMIT, ctx->git_commit))
        return false;
    auto * ea(reinterpret_cast<i01::apps::engine::EngineApp*>(ctx->engine_app));
    ea->log().console()->notice() << "Initializing plugin for Strategy " << cfg_name << " version " << i01::g_COMMIT << " against engine " << ctx->git_commit;

    i01::TS::Strategy * strat_p(
            new AlephStrategy(
                reinterpret_cast<i01::OE::OrderManager*>(ctx->order_manager)
                , nullptr
                // , reinterpret_cast<i01::MD::DataManager*>(ctx->data_manager)
              , cfg_name));
    return (*(ctx->register_strategy_op))(reinterpret_cast<i01_strategy_t *>(strat_p));
}

void i01_strategy_plugin_exit(const i01_StrategyPluginContext * ctx, const char * cfg_name) {
    if (!ctx)
        return;
    if (0 != std::strcmp(i01::g_COMMIT, ctx->git_commit))
        return;

    auto * ea(reinterpret_cast<i01::apps::engine::EngineApp*>(ctx->engine_app));
    ea->log().console()->notice() << "Exiting plugin for Strategy " << cfg_name << " version " << i01::g_COMMIT << " against engine " << ctx->git_commit;
}

}
