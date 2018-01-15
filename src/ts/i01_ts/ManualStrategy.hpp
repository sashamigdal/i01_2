#pragma once

#include <functional>
#include <list>
#include <set>
#include <unordered_map>
#include <vector>

// #include <tbb/concurrent_queue.h>
#include <boost/tokenizer.hpp>


#include <i01_core/NamedThread.hpp>
#include <i01_core/Readline.hpp>

#include <i01_oe/OrderSessionPtr.hpp>

#include <i01_ts/L1EquitiesStrategy.hpp>

namespace i01 { namespace OE {
class EquityInstrument;
class BOE20Order;
}}

namespace i01 { namespace TS {


class ManualStrategy : public L1EquitiesStrategy {
public:
    using CommandArgs = std::vector<std::string>;
    using CommandFunction = std::function<bool(const CommandArgs &)>;

    struct Command {
        std::string description;
        CommandFunction func;
    };

    using CommandMap = std::map<std::string, Command>;

    using InstMap = std::map<std::string, OE::EquityInstrument *>;

    using SessionMap = std::array<OE::OrderSessionPtr, (int)core::MICEnum::NUM_MIC>;

    using Tokenizer = boost::tokenizer<boost::char_separator<char>>;

    constexpr static double HARD_CLOSEOUT_CROSS_PCT = 0.05;

    // enum class CmdType {
    //     UKNOWN =0,
    //         ORDER,
    //         CANCEL,
    //         };

    // struct Cmd {
    //     CmdType type;
    // };

    // struct OrderCmd : Cmd {

    // };

    // using CmdQueue = tbb::concurrent_queue<Cmd *>;

    class InputThread : public core::NamedThread<InputThread> {
    public:
        InputThread(const std::string &n, ManualStrategy *ms, const CommandMap &commands);

        // pre-process can add commands
        // just gets commands from the user .. check if input matches the command map
        virtual void * process() override final;
    private:
        bool process_string(const std::string &str);

    private:
        ManualStrategy * m_strat;
        CommandMap m_commands;
        boost::char_separator<char> m_ws_sep;
        std::string m_prompt;
    };

    using OrderContainer = std::set<OE::Order *>;
    using OrdersByMIC = std::array<OrderContainer, (int) core::MICEnum::NUM_MIC>;

    using NoArgCmdFunctor = std::function<bool()>;
    using OneArgCmdFunctor = std::function<bool(const std::string&)>;

public:
    ManualStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string &n);
    virtual ~ManualStrategy();

    virtual void start() override;

    void order_command(const CommandArgs &args);
    void cancel_command(const CommandArgs &args);
    void list_instruments_command(const CommandArgs &args) const;
    void list_orders_command(const CommandArgs &args);
    void update_locates_command(const CommandArgs &args);
    void list_config_command(const CommandArgs& args);
    void quote_command(const CommandArgs& args);
    void manual_position_adj_command(const CommandArgs& args);
    void status_command(const CommandArgs& args);
    void bulk_cancel_command(const CommandArgs& args);
    void no_arg_command_helper(const CommandArgs& args, const std::string& name,
                               NoArgCmdFunctor func);
    void at_least_one_arg_command_helper(const CommandArgs& args, const std::string& name,
                                         OneArgCmdFunctor func);
    void toggle_stock_command(bool enable, const std::string& symbol);
    void closeout_position_command(const CommandArgs& args);
    void toggle_connect_session_command(bool connect, const CommandArgs& args);

private:
    CommandMap load_commands();
    void load_instruments();
    void load_sessions();
    void load_orders();

    void show_config_domain(const core::Config::storage_type&) const;

protected:
    bool handle_boe_order_arguments(OE::BOE20Order* op, const CommandArgs& args);

    void do_order_command(OE::EquityInstrument *inst, const OE::Side &side, const OE::Size &size,
                          const OE::Price &p, const OE::OrderType &type, const OE::TimeInForce &tif,
                          const core::MIC &mic, const CommandArgs& exch_specific);

    void do_cancel_command(const core::MIC & mic, const OE::LocalID &localid, const OE::Size &newsize);

    void do_quote_command(const std::string& symbol, const std::set<core::MIC>& mics);

    void do_list_orders_command(const core::MIC& mic, bool show_terminal);

    void do_status_command();

    void do_cancel_all_command();
    void do_cancel_all_command(OE::OrderSessionPtr p);
    void do_cancel_all_command(OE::LocalAccount account);

    void do_toggle_trading_status(bool enable);
    void do_toggle_stock_status(bool enable, const OE::EquityInstrument *inst);

    void do_closeout_position(OE::EquityInstrument *inst, const core::MIC& mic, double cross_pct);

    void do_connect_session_command(OE::OrderSessionPtr p);
    void do_disconnect_session_command(OE::OrderSessionPtr p);

    void do_load_orders(bool include_terminal = false);

    void do_show_time(const core::Timestamp& ts);

    std::string print_inst_status(const OE::EquityInstrument *inst) const;

    virtual void on_bbo_update(const core::Timestamp&
                               , const core::MIC&
                               , const MD::EphemeralSymbolIndex&
                               , const MD::L3OrderData::Side&
                               , const MD::L2Quote& ) override final {}

    virtual void on_bbo_update(const core::Timestamp&
                               , const core::MIC&
                               , const MD::EphemeralSymbolIndex&
                               , const MD::FullL2Quote& ) override final {}


    virtual void on_order_acknowledged(const OE::Order*) override final;
    virtual void on_order_fill(const OE::Order*, const OE::Size, const OE::Price, const OE::Price) override final;
    virtual void on_order_cancel(const OE::Order*, const OE::Size) override final;
    virtual void on_order_cancel_rejected(const OE::Order*) override final;
    virtual void on_order_cancel_replaced(const OE::Order*) override final;
    virtual void on_order_rejected(const OE::Order*) override final;

    /* BookMuxListener */
    virtual void on_symbol_definition(const MD::SymbolDefEvent &) override final;
    virtual void on_book_added(const MD::L3AddEvent& new_o) override final;
    virtual void on_book_canceled(const MD::L3CancelEvent& cxl_o) override final;
    virtual void on_book_modified(const MD::L3ModifyEvent& mod_e) override final;
    virtual void on_book_executed(const MD::L3ExecutionEvent& exe_e) override final;
    /// NYSE is a precious snowflake.
    virtual void on_l2_update(const MD::L2BookEvent&) override final;
    virtual void on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent&) override final;
    virtual void on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent&) override final;
    virtual void on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent&) override final;
    virtual void on_trade(const MD::TradeEvent&) override final;
    virtual void on_gap(const MD::GapEvent&) override final;
    virtual void on_trading_status_update(const MD::TradingStatusEvent&) override final;
    virtual void on_feed_event(const MD::FeedEvent&) override final;
    virtual void on_market_event(const MD::MWCBEvent&) override final;
    virtual void on_start_of_data(const MD::PacketEvent&) override final;
    virtual void on_end_of_data(const MD::PacketEvent&) override final;
    virtual void on_timeout_event(const MD::TimeoutEvent&) override final;

    virtual void on_timer(const core::Timestamp& ts, void *, std::uint64_t) override final;

private:
    InputThread *m_thread;

    InstMap m_inst_map;

    SessionMap m_session_map;

    OrdersByMIC m_orders;

    core::Timestamp m_dm_ts;
};


}}
