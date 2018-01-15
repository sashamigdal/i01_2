#include <algorithm>
#include <cctype>
#include <exception>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <i01_md/DataManager.hpp>

#include <i01_oe/BATSOrder.hpp>
#include <i01_oe/NASDAQOrder.hpp>
#include <i01_oe/OrderManager.hpp>
#include <i01_oe/OrderSession.hpp>

#include <i01_ts/ManualStrategy.hpp>




namespace i01 { namespace TS {

ManualStrategy::ManualStrategy(OE::OrderManager *omp, MD::DataManager *dmp, const std::string &n) :
    L1EquitiesStrategy(omp, dmp, n)
{
    m_dm_p->register_timer(this, nullptr);
}

ManualStrategy::~ManualStrategy()
{
    m_thread->join();
}


void ManualStrategy::start()
{
    load_instruments();

    load_sessions();

    do_load_orders();

    m_thread = new InputThread(name() + "-input", this, load_commands());

    m_om_p->default_listener(this);

    m_thread->spawn();
}

void ManualStrategy::load_instruments()
{
    for (const auto & e : m_om_p->universe()) {
        m_inst_map[e.cta_symbol()] = e.data();
    }
}

void ManualStrategy::load_sessions()
{
    auto sesh = m_om_p->get_sessions();

    // this will just keep one session per MIC
    for (const auto & s : sesh) {
        m_session_map[s->market().index()] = s;
    }
}

auto ManualStrategy::load_commands() -> CommandMap
{
    CommandMap cm;

    cm.emplace("QUIT", Command{"quit the app", [](const CommandArgs &) {
                return false;
            }});

    cm.emplace("O", Command{"send an order. o <symbol> <b|s> <size> <price> <type=l|m|p> <tif=d|i|f|x|c|o> <MIC> ...",
                [this](const CommandArgs &args) {
                this->order_command(args); return true;
            }});

    cm.emplace("T", Command{"current time from the DM.",
                [this](const CommandArgs&) {
                this->do_show_time(m_dm_ts);
                return true;
            }});

    cm.emplace("C", Command{"cancel an order. c <mic> <local-id> [<new-size>]",
                [this](const CommandArgs &args){
                this->cancel_command(args); return true;
            }});

    cm.emplace("I", Command{"list instruments. i [*|<symbol>]", [this](const CommandArgs& args) {
                this->list_instruments_command(args); return true;
            }});

    cm.emplace("L", Command{"list orders. l [*|<MIC> [*]]", [this](const CommandArgs &args) {
                this->list_orders_command(args); return true;
            }});

    cm.emplace("UPDATELOCATES", Command{"Update locates from file. l [conf/update_locates.lua]", [this](const CommandArgs &args) {
                this->update_locates_command(args); return true;
            }});

    cm.emplace("CFG", Command{"show config. cfg <key>", [this](const CommandArgs& args) {
                this->list_config_command(args); return true;
            }});

    cm.emplace("K", Command{"show quote. k <symbol> [<mic> ..]", [this](const CommandArgs& args) {
                this->quote_command(args); return true;
            }});

    cm.emplace("MANPOSADJ", Command{"manually adjust position. manposadj <symbol> <delta_start_qty> <delta_trading_qty>", [this](const CommandArgs& args) {
                this->manual_position_adj_command(args); return true;
            }});

    cm.emplace("S", Command{"OM status. s", [this](const CommandArgs& args) {
                this->status_command(args);
                return true;
            }});

    cm.emplace("BULKCXL", Command{"bulk cancel open orders. bulkcxl [*|<session name>|<MIC> ..]", [this] (const CommandArgs& args) {
                this->bulk_cancel_command(args);
                return true;
            }});

    cm.emplace("ENABLE", Command{"enable trading for all strategies. enable",
                [this] (const CommandArgs& args) {
                this->no_arg_command_helper(args, "enable trading", [this]() {
                        this->do_toggle_trading_status(true);
                        return true;
                    });
                return true;
            }});

    cm.emplace("DISABLE", Command{"disable trading for strategy. disable",
                [this] (const CommandArgs& args) {
                this->no_arg_command_helper(args, "disable trading", [this]() {
                        this->do_toggle_trading_status(false);
                        return true;
                    });
                return true;
            }});

    cm.emplace("ENABLESTOCK", Command{"enable trading for stock. enablestock <symbol>[..]",
                [this] (const CommandArgs& args) {
                this->at_least_one_arg_command_helper(args, "enable stock", [this](const std::string& a) {
                        this->toggle_stock_command(true, a);
                        return true;
                    });
                return true;
            }});

    cm.emplace("DISABLESTOCK", Command{"disable trading for stock. disable <symbol>[..]",
                [this] (const CommandArgs& args) {
                this->at_least_one_arg_command_helper(args, "disable stock", [this](const std::string& a) {
                        this->toggle_stock_command(false, a);
                        return true;
                    });
                return true;
            }});

    cm.emplace("CLOSEOUT", Command{"send order to aggressively close position using <MIC>. closeout <MIC> [*|<symbol>|..]",
                [this] (const CommandArgs& args) {
                this->closeout_position_command(args);
                return true;
            }});

    cm.emplace("CONNECT", Command{"connect a session. connect <session-name>",
                [this] (const CommandArgs& args) {
                this->toggle_connect_session_command(true, args);
                return true;
            }});

    cm.emplace("DISCONNECT", Command{"disconnect a session. disconnect <session-name>",
                [this] (const CommandArgs& args) {
                this->toggle_connect_session_command(false, args);
                return true;
            }});

    cm.emplace("HELP", Command{"list all commands", [cm](const CommandArgs &) {
                for (const auto c : cm) {
                    std::cout << c.first << " -- " << c.second.description << std::endl;
                }
                return true;
            }});
    return cm;
}

/* OrderManagerListener */
void ManualStrategy::on_order_acknowledged(const OE::Order *o)
{
    std::cerr << "ManualStrategy: on_order_acknowledged: " << *o << std::endl;
}
void ManualStrategy::on_order_fill(const OE::Order *o, const OE::Size s, const OE::Price p, const OE::Price fee)
{
    std::cerr << "ManualStrategy: on_fill: " << s << "," << p << "," << fee << "," << *o << std::endl;
}
void ManualStrategy::on_order_cancel(const OE::Order *o, const OE::Size s)
{
    std::cerr << "ManualStrategy: on_order_cancel: " << s << "," << *o << std::endl;
}
void ManualStrategy::on_order_cancel_rejected(const OE::Order *o)
{
    std::cerr << "ManualStrategy: on_order_cancel_rejected: " << *o << std::endl;
}
void ManualStrategy::on_order_cancel_replaced(const OE::Order *o)
{
    std::cerr << "ManualStrategy: on_order_cancel_replaced: " << *o << std::endl;
}
void ManualStrategy::on_order_rejected(const OE::Order *o)
{
    std::cerr << "ManualStrategy: on_order_rejected: " << *o << std::endl;
}

/* BookMuxListener */
void ManualStrategy::on_symbol_definition(const MD::SymbolDefEvent & evt)
{
}
void ManualStrategy::on_book_added(const MD::L3AddEvent& new_o )
{
}
void ManualStrategy::on_book_canceled(const MD::L3CancelEvent& cxl_o )
{
}
void ManualStrategy::on_book_modified(const MD::L3ModifyEvent& mod_e )
{
}
void ManualStrategy::on_book_executed(const MD::L3ExecutionEvent& exe_e )
{
}
/// NYSE is a precious snowflake.
void ManualStrategy::on_l2_update(const MD::L2BookEvent& evt)
{
}
void ManualStrategy::on_nyse_opening_imbalance(const MD::NYSEOpeningImbalanceEvent& evt)
{
}
void ManualStrategy::on_nyse_closing_imbalance(const MD::NYSEClosingImbalanceEvent& evt)
{
}
void ManualStrategy::on_nasdaq_imbalance(const MD::NASDAQImbalanceEvent& evt)
{
}
void ManualStrategy::on_trade(const MD::TradeEvent& evt)
{
}
void ManualStrategy::on_gap(const MD::GapEvent& evt)
{
}
void ManualStrategy::on_trading_status_update(const MD::TradingStatusEvent& evt)
{
}
void ManualStrategy::on_feed_event(const MD::FeedEvent& evt)
{
}
void ManualStrategy::on_market_event(const MD::MWCBEvent& evt)
{
}
void ManualStrategy::on_start_of_data(const MD::PacketEvent& evt)
{
}
void ManualStrategy::on_end_of_data(const MD::PacketEvent& evt)
{
}

void ManualStrategy::on_timeout_event(const MD::TimeoutEvent& evt)
{
    std::cerr << "TimeoutEvent: " << evt << std::endl;
}

void ManualStrategy::order_command(const CommandArgs &args)
{
    using boost::lexical_cast;
    using boost::bad_lexical_cast;

    if (args.size() < 7) {
        throw std::runtime_error("order_command expected more arguments");
    }
    auto aa = args.begin();
    auto symbola = *aa++;
    auto &sidea = *aa++;
    auto &sizea = *aa++;
    auto &pricea = *aa++;
    auto &typea = *aa++;
    auto &tifa = *aa++;
    auto mica = *aa++;

    auto exchange_specific_args = CommandArgs(aa, args.end());

    std::transform(symbola.begin(), symbola.end(), symbola.begin(), ::toupper);
    std::transform(mica.begin(), mica.end(), mica.begin(), ::toupper);

    auto it = m_inst_map.find(symbola);
    if (it == m_inst_map.end()) {
        throw std::runtime_error("unknown symbol " + symbola);
    }

    auto side = OE::Side{};
    if ('B' == sidea[0] || 'b' == sidea[0]) {
        side = OE::Side::BUY;
    } else if ('S' == sidea[0] || 's' == sidea[0]) {
        side = OE::Side::SELL;
    }

    auto size = lexical_cast<OE::Size>(sizea);
    auto price = lexical_cast<OE::Price>(pricea);

    auto type = OE::OrderType{};
    if ('L' == typea[0] || 'l' == typea[0]) {
        type = OE::OrderType::LIMIT;
    } else if ('M' == typea[0] || 'm' == typea[0]) {
        type = OE::OrderType::MARKET;
    } else if ('P' == typea[0] || 'p' == typea[0]) {
        type = OE::OrderType::MIDPOINT_PEG;
    } else {
        throw std::runtime_error("unsupported order type " + typea);
    }

    auto tif = OE::TimeInForce{};
    if ('D' == tifa[0] || 'd' == tifa[0]) {
        tif = OE::TimeInForce::DAY;
    } else if ('I' == tifa[0] || 'i' == tifa[0]) {
        tif = OE::TimeInForce::IMMEDIATE_OR_CANCEL;
    } else if ('X' == tifa[0] || 'x' == tifa[0]) {
        tif = OE::TimeInForce::EXT;
    } else if ('F' == tifa[0] || 'f' == tifa[0]) {
        tif = OE::TimeInForce::FILL_OR_KILL;
    } else if ('C' == tifa[0] || 'c' == tifa[0]) {
        tif = OE::TimeInForce::AUCTION_CLOSE;
    } else if ('O' == tifa[0] || 'o' == tifa[0]) {
        tif = OE::TimeInForce::AUCTION_OPEN;
    } else {
        throw std::runtime_error("unsupported tif " + tifa);
    }

    auto mic = core::MIC::clone(mica.c_str());
    if (core::MICEnum::UNKNOWN == mic) {
        throw std::runtime_error("unknown MIC " + mica);
    }

    do_order_command(it->second, side, size, price, type, tif, mic, exchange_specific_args);
}

void ManualStrategy::cancel_command(const CommandArgs &args)
{
    using boost::lexical_cast;
    using boost::bad_lexical_cast;

    if (args.size() < 2) {
        throw std::runtime_error("order_command expected more arguments");
    }

    auto mica = args[0];
    std::transform(mica.begin(), mica.end(), mica.begin(), ::toupper);
    auto mic = core::MIC::clone(mica.c_str());
    if (core::MICEnum::UNKNOWN == mic) {
        throw std::runtime_error("cancel: unknown MIC " + mica);
    }

    auto localid = lexical_cast<OE::LocalID>(args[1]);

    auto newsize = OE::Size{0};

    if (args.size() > 2) {
        newsize = lexical_cast<OE::Size>(args[2]);
    }

    do_cancel_command(mic, localid, newsize);
}

void ManualStrategy::quote_command(const CommandArgs& args)
{
    if (args.size() < 1) {
        throw std::runtime_error("quote_command expected at least 1 arguments");
    }

    auto symbola = args[0];
    std::transform(symbola.begin(), symbola.end(), symbola.begin(), ::toupper);

    std::set<core::MIC> mics;

    if (args.size() > 1) {
        for (auto i = 1U; i < args.size(); i++) {
            auto mica = args[i];
            std::transform(mica.begin(), mica.end(), mica.begin(), ::toupper);
            auto mic = core::MIC::clone(mica.c_str());
            if (core::MICEnum::UNKNOWN == mic) {
                throw std::runtime_error("quote: unknown MIC " + mica);
            }
            mics.insert(mic);
        }
    } else {
        for (auto i = 0; i < (int)core::MICEnum::NUM_MIC; i++) {
            mics.insert((core::MICEnum)i);
        }
    }

    do_quote_command(symbola, mics);
}

void ManualStrategy::manual_position_adj_command(const CommandArgs& args)
{
    if (args.size() < 3) {
        throw std::runtime_error("manual_position_adj_command expected at least 2 arguments");
    }

    auto symbola = args[0];
    std::transform(symbola.begin(), symbola.end(), symbola.begin(), ::toupper);
    auto it = m_inst_map.find(symbola);
    if (it == m_inst_map.end()) {
        throw std::runtime_error("unknown symbol " + symbola);
    }
    m_om_p->manual_position_adj(core::Timestamp::now(), it->second->esi(), boost::lexical_cast<int64_t>(args[1]), boost::lexical_cast<int64_t>(args[2]));
}

void ManualStrategy::status_command(const CommandArgs& args)
{
    do_status_command();
}

void ManualStrategy::bulk_cancel_command(const CommandArgs& args)
{
    if (args.size() < 1) {
        throw std::runtime_error("bulk cancel: expected at least one argument");
    }

    for (auto a : args) {
        std::transform(a.begin(), a.end(), a.begin(), ::toupper);
        if ("*" == a) {
            do_cancel_all_command();
            break;
        }

        // is it a MIC?
        auto mic = core::MIC::clone(a.c_str());
        if (core::MICEnum::UNKNOWN != mic) {
            if (m_session_map[mic.index()]) {
                do_cancel_all_command(m_session_map[mic.index()]);
            }
        } else {
            // it could be a local account number
            try {
                auto acc = boost::lexical_cast<OE::LocalAccount>(a);
                do_cancel_all_command(acc);
            } catch (boost::bad_lexical_cast& e) {
                // it may be a session name...
                for (auto& s: m_session_map) {
                    if (s) {
                        if (a == s->name()) {
                            do_cancel_all_command(s);
                        }
                    }
                }
            }
        }
    }
}

void ManualStrategy::no_arg_command_helper(const CommandArgs& args, const std::string& aname,
                                           NoArgCmdFunctor func)
{
    if (args.size()) {
        throw std::runtime_error(aname + ": expected no arguments");
    }
    func();
}

void ManualStrategy::at_least_one_arg_command_helper(const CommandArgs& args, const std::string& aname,
                                                     OneArgCmdFunctor func)
{
    if (args.size() < 1) {
        throw std::runtime_error(aname + ": expected at least one argument");
    }

    for (auto a : args) {
        std::transform(a.begin(), a.end(), a.begin(), ::toupper);
        if (!func(a)) {
            break;
        }
    }
}

void ManualStrategy::toggle_stock_command(bool enable, const std::string& symbol)
{
    // look up the instrument for this stock
    auto it = m_inst_map.find(symbol);
    if (it == m_inst_map.end()) {
        throw std::runtime_error("unknown symbol " + symbol);
    }

    do_toggle_stock_status(enable, it->second);
}

void ManualStrategy::toggle_connect_session_command(bool connect, const CommandArgs &args)
{
    if (args.size() != 1) {
        throw std::runtime_error("toggle_connect_session: expect one argument");
    }

    auto nam = args[0];
    std::transform(nam.begin(), nam.end(), nam.begin(), ::toupper);

    auto mic = core::MIC::clone(nam.c_str());
    if (core::MICEnum::UNKNOWN != mic) {
        if (m_session_map[mic.index()]) {
            if (connect) {
                do_connect_session_command(m_session_map[mic.index()]);
            } else {
                do_disconnect_session_command(m_session_map[mic.index()]);
            }
        } else {
            throw std::runtime_error("toggle_connect_session: no session for MIC " + nam);
        }
    } else {
        // this may be a name of a session
        bool found = false;
        for (auto p : m_session_map) {
            if (p != nullptr && p->name() == nam) {
                if (connect) {
                    do_connect_session_command(p);
                } else {
                    do_disconnect_session_command(p);
                }
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::runtime_error("toggle_connect_session: could not find session " + nam);
        }
    }
}

void ManualStrategy::closeout_position_command(const CommandArgs& args)
{
    // expect <mic> *|<symbol> [<symbol> ..]

    if (args.size() < 2) {
        throw std::runtime_error("closeout_position: expect at least two arguments");
    }

    auto insts = std::vector<OE::EquityInstrument *>{};

    auto mic = core::MIC{};
    for (auto i = 0U; i < args.size(); i++) {
        auto a = args[i];
        std::transform(a.begin(), a.end(), a.begin(), ::toupper);

        if (0 == i) {
            // this should be a MIC
            mic = core::MIC::clone(a.c_str());
            if (core::MICEnum::UNKNOWN == mic) {
                throw std::runtime_error("closeout_position: unknown MIC " + a);
            }
        } else {
            if ("*" == a) {
                for (const auto& ii : m_inst_map) {
                    insts.push_back(ii.second);
                }
            } else {
                auto it = m_inst_map.find(a);
                if (it == m_inst_map.end()) {
                    std::cerr << "closeout_position: unknown symbol " << a << ", ignoring..." << std::endl;
                    continue;
                }
                insts.push_back(it->second);
            }
        }
    }

    for (const auto& ii : insts) {
        do_closeout_position(ii, mic, 0.02);
    }
}

ManualStrategy::InputThread::InputThread(const std::string &n, ManualStrategy *ms, const CommandMap &cm) :
    NamedThread<InputThread>(n),
    m_strat(ms),
    m_commands(cm),
    m_ws_sep(" \t"),
    m_prompt(n + "> ")
{
    (void)m_strat;
}

void * ManualStrategy::InputThread::process()
{
    // std::cout << "manual> ";
    // std::cout.flush();

    std::string in;
    try {
        in = core::readline::readline(m_prompt);
        if (in.size()) {
            core::readline::add_history(in);
        }
    } catch (...) {
        return (void *)1;
    }

    if (!process_string(in)) {
        return (void *)1;
    }

    // look up the command ...

    return nullptr;
}

bool ManualStrategy::InputThread::process_string(const std::string &str)
{
    // tokenize it...
    Tokenizer toks(str, m_ws_sep);
    if (toks.begin() == toks.end()) {
        return true;
    }
    auto it = toks.begin();
    auto cmd = *it++;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    auto cc = m_commands.find(cmd);
    if (cc == m_commands.end()) {
        std::cerr << "ManualStrategy: could not find command '" << *toks.begin() << "'.  Try 'help'." << std::endl;
        return true;
    }

    // run command function with the rest of the args
    bool ret = true;
    try {
        ret = cc->second.func(std::vector<std::string>(it, toks.end()));
    } catch (const std::exception &e) {
        std::cerr << "ManualStrategy caught exception processing " << str << ": " << e.what() << std::endl;
    } catch(...) {
        std::cerr << "ManualStrategy caught exception processing " << str << std::endl;
    }
    return ret;
}

void ManualStrategy::list_instruments_command(const CommandArgs &args) const
{
    auto sym = std::string{};
    if (args.size()) {
        sym = args[0];
        std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    }

    for (const auto &e : m_inst_map) {
        // only print out insts with a position or PNL
        if (("" == sym && ((e.second->position().quantity()
                            || e.second->position().realized_pnl() > 1e-4
                            || e.second->position().unrealized_pnl() > 1e-4)))
            || sym == "*"
            || sym == e.first) {
            std::cout << print_inst_status(e.second) << std::endl;
        }
    }
}

std::string ManualStrategy::print_inst_status(const OE::EquityInstrument *inst) const
{
    // get firm risk status ...
    auto risk = m_om_p->get_firm_risk_status(inst->esi());
    std::ostringstream ss;
    ss << *inst << ",";
    if (risk[(int) OE::FirmRiskCheck::InstPermissions::FLATTEN_ONLY]) {
        ss << "FLATTEN_ONLY";
    }
    ss << ",";
    if (risk[(int) OE::FirmRiskCheck::InstPermissions::USER_DISABLE]) {
        ss << "USER_DISABLE";
    }
    return ss.str();
}

void ManualStrategy::list_orders_command(const CommandArgs &args)
{

    bool show_terminal = false;
    auto mic = core::MIC{};

    if (args.size()) {
        if (args.size() > 2) {
            throw std::runtime_error("list_orders: expect at most two arguments");
        }

        if ("*" == args[0]) {
            show_terminal = true;

            if (args.size() > 1) {
                throw std::runtime_error("list_orders: expect only one argument if first is *");
            }
        } else {
            auto arga = args[0];
            std::transform(arga.begin(), arga.end(), arga.begin(), ::toupper);

            mic = core::MIC::clone(arga.c_str());
            if (mic == core::MICEnum::UNKNOWN) {
                throw std::runtime_error("list_orders: could not find MIC for " + args[0]);
            }

            if (args.size() == 2 && "*" == args[1]) {
                show_terminal = true;
            } else {
                std::cout << "Showing only non-terminal orders.  Use 'l " << mic << " *' to show all orders." << std::endl;
            }
        }
    } else {
        std::cout << "Showing only non-terminal orders.  Use 'l *' to show all orders." << std::endl;
    }

    do_list_orders_command(mic, show_terminal);
}

void ManualStrategy::update_locates_command(const CommandArgs &args)
{
    if (args.size()) {
        if (args.size() > 1) {
            throw std::runtime_error("update_locates: expect at most one argument");
        }

        m_om_p->load_and_update_locates(args[0]);
    } else {
        m_om_p->load_and_update_locates("");
    }
}

void ManualStrategy::list_config_command(const CommandArgs& args)
{
    auto cfg = core::Config::instance().get_shared_state();

    if (args.size()) {
        for (const auto& s: args) {
            auto cs(cfg->copy_prefix_domain(s));
            show_config_domain(*cs);
        }
    } else {
        show_config_domain(*cfg);
    }
}

void ManualStrategy::show_config_domain(const core::Config::storage_type& cfg) const
{
    for (const auto& s: cfg) {
        std::cout << s.first << ": " << s.second << std::endl;
    }
}

void ManualStrategy::do_order_command(OE::EquityInstrument *inst, const OE::Side &side, const OE::Size &size,
                                      const OE::Price &price, const OE::OrderType &type, const OE::TimeInForce &tif,
                                      const core::MIC &mic, const CommandArgs& exch_specific_args)
{
    using namespace i01::core;

    if (!m_session_map[mic.index()]) {
        std::cerr << "do_order: no session available for MIC " << mic << std::endl;
        return;
        // enqueue_command_result(ERROR,msg);
    }

    OE::Order *op = nullptr;

    if (MICEnum::BATS == mic
        || MICEnum::BATY == mic
        || MICEnum::EDGX == mic
        || MICEnum::EDGA == mic) {

        op = m_om_p->create_order<OE::BATSOrder>(inst, price, size, side, tif, type, this);

        if (!handle_boe_order_arguments(static_cast<OE::BOE20Order*>(op), exch_specific_args)) {
            std::cerr << "do_order: unsupported BATS specific arguments" << std::endl;
            return;
        }
    } else if (MICEnum::XNAS == mic || MICEnum::XBOS == mic || MICEnum::XPSX == mic) {
        op = m_om_p->create_order<OE::NASDAQOrder>(inst, price, size, side, tif, type, this);
    } else {
        std::cerr << "do_order: unsupported MIC " << mic << std::endl;
        return;
    }

    if (!m_om_p->send(op, m_session_map[mic.index()].get())) {
        // local reject
        std::cerr << "do_order: local reject" << std::endl;
    }

    m_orders[mic.index()].insert(op);
}

bool ManualStrategy::handle_boe_order_arguments(OE::BOE20Order* op, const CommandArgs& args)
{
    // these arguments should be of the form tag=value
    for (const auto& v : args) {
        Tokenizer toks(v, boost::char_separator<char>("="));
        auto toksv = std::vector<std::string>(toks.begin(), toks.end());
        if (toksv.size() != 2) {
            std::cerr << "handle_boe: argument pair malformed: \"" << v << "\", expect \"tag=value\"" << std::endl;
            return false;
        }
        if (!op->set_attribute(toksv[0], toksv[1])) {
            std::cerr << "handle_boe: invalid set attribute " << toksv[0] << " = " << toksv[1] << std::endl;
            return false;
        }
    }
    return true;
}

void ManualStrategy::do_cancel_command(const core::MIC & mic, const OE::LocalID &localid, const OE::Size &newsize)
{
    auto found = false;
    // find the order...
    for (const auto &o : m_orders[mic.index()]) {
        if (o->localID() == localid) {
            if (!m_om_p->cancel(o, newsize)) {
                std::cerr << "do_cancel: OM returned false on cancel() for " << mic << " " << localid << std::endl;
            }
            found = true;
        }
    }

    if (!found) {
        std::cerr << "do_cancel: could not find order for " << mic << " " << localid << std::endl;
    }
}

template<typename BookMuxType>
MD::FullL2Quote best_quote_from_book_mux(BookMuxType* bm, const core::MIC& mic, const OE::EquityInstrument *ei)
{
    auto *book = (*bm)[ei->esi()];
    if (book) {
        return (MD::FullL2Quote) book->best();
    }
    return MD::FullL2Quote{};
}

void ManualStrategy::do_quote_command(const std::string& symbol, const std::set<core::MIC>& mics)
{
    auto it = m_inst_map.find(symbol);
    if (it == m_inst_map.end()) {
        std::cerr << "do_quote: could not find symbol " << symbol << std::endl;
        return;
    }

    std::cout << symbol << " ";

    for (const auto& m : mics) {
        if (core::MICEnum::XNYS == m) {
            // special for NYSE...
            auto* bm = m_dm_p->get_l2(m);
            if (bm) {
                auto bq = best_quote_from_book_mux(bm, m, it->second);
                std::cout << m << " " << bq << "  ";
            }
        } else {
            auto* bm = m_dm_p->get_l3(m);
            if (bm) {
                auto bq = best_quote_from_book_mux(bm, m, it->second);
                std::cout << m << " " << bq << "  ";
            }
        }
    }

    std::cout << std::endl;
}

void ManualStrategy::do_list_orders_command(const core::MIC& mic, bool show_terminal)
{
    do_load_orders(show_terminal);

    for (auto i = 0U; i < m_orders.size(); i++) {
        if (core::MICEnum::UNKNOWN == mic || static_cast<int>(i) == mic.index()) {
            for (const auto &o : m_orders[i]) {
                if (!o->is_terminal() || show_terminal) {
                    std::cout << core::MIC(static_cast<core::MICEnum>(i)).name() << " " << o->localID() << ": ";
                    switch (static_cast<core::MICEnum>(i)) {
                    case core::MICEnum::BATS:
                    case core::MICEnum::BATY:
                    case core::MICEnum::EDGX:
                    case core::MICEnum::EDGA:
                        std::cout << *static_cast<const OE::BOE20Order*>(o);
                        break;
                    case core::MICEnum::XNAS:
                    case core::MICEnum::XBOS:
                    case core::MICEnum::XPSX:
                    case core::MICEnum::ARCX:
                    case core::MICEnum::XNYS:
                    default:
                        std::cout << *o;
                    }
                    std::cout << std::endl;
                }
            }
        }
    }
}

void ManualStrategy::do_status_command()
{
    std::cout << m_om_p->status() << std::endl;
}

void ManualStrategy::do_cancel_all_command()
{
    m_om_p->cancel_all();
}

void ManualStrategy::do_cancel_all_command(OE::OrderSessionPtr op)
{
    m_om_p->cancel_all(op);
}

void ManualStrategy::do_cancel_all_command(OE::LocalAccount account)
{
    m_om_p->cancel_all_local_account(account);
}

void ManualStrategy::do_load_orders(bool include_terminal)
{
    auto orders = OE::OrderManager::OrderPtrContainer{};
    if (include_terminal) {
        orders = m_om_p->all_orders();
    } else {
        orders = m_om_p->open_orders();
    }

    for (const auto& p : orders) {
        m_orders[p->market().index()].insert(p);
    }
}

void ManualStrategy::do_toggle_trading_status(bool enable)
{
    if (enable) {
        m_om_p->enable_all_trading();
    } else {
        m_om_p->disable_all_trading();
    }
}

void ManualStrategy::do_toggle_stock_status(bool enable, const OE::EquityInstrument *inst)
{
    if (enable) {
        m_om_p->enable_stock(inst->esi());
    } else {
        m_om_p->disable_stock(inst->esi());
    }
}

void ManualStrategy::do_closeout_position(OE::EquityInstrument *inst, const core::MIC& mic, double cross_pct)
{
    // just call the do_order_command
    auto quantity = inst->position().quantity();

    if (cross_pct <= 0) {
        std::cerr << "ManualStrategy: do_closeout_position: cross pct must be greater than zero" << std::endl;
    }
    if (cross_pct > HARD_CLOSEOUT_CROSS_PCT) {
        std::cerr << "ManualStrategy: do_closeout_position: cross_pct " << cross_pct << " greater than hard limit " << HARD_CLOSEOUT_CROSS_PCT << ", clamping at limit" << std::endl;
        cross_pct = HARD_CLOSEOUT_CROSS_PCT;
    }


    if (0 == quantity) {
        return;
    }

    auto mark_price = inst->position().mark();

    auto size = OE::Size{0};
    auto side = OE::Side::BUY;
    if (quantity > 0) {
        side = OE::Side::SELL;
        size = static_cast<OE::Size>(quantity);
    } else {
        size = static_cast<OE::Size>(-quantity);
    }

    auto tif = OE::TimeInForce::IMMEDIATE_OR_CANCEL;
    auto type = OE::OrderType::LIMIT;

    auto limit_price = mark_price;
    if (OE::Side::BUY == side) {
        limit_price *= (1.0 + cross_pct);
        limit_price += 0.005;
    } else {
        limit_price *= (1.0 - cross_pct);
        limit_price -= 0.005;
    }
    auto cents_price = static_cast<std::uint32_t>(limit_price*100.0);
    limit_price = static_cast<double>(cents_price) / 100.0;
    std::cerr << "Sending closeout for " << inst->symbol() << " " << side << " size: " << size
              << " mark_price: " << mark_price
              << " limit_price: " << limit_price << " tif: " << tif << " mic: " << mic << std::endl;

    do_order_command(inst, side, size, limit_price, type, tif, mic, CommandArgs{});
}

void ManualStrategy::on_timer(const core::Timestamp& ts, void *, std::uint64_t)
{
    m_dm_ts = ts;
}

void ManualStrategy::do_show_time(const core::Timestamp& ts)
{
    char buf[64];
    ::ctime_r(&ts.tv_sec, buf);
    std::cout << "ManualStrategy: time: " << buf << std::flush;
}

void ManualStrategy::do_connect_session_command(OE::OrderSessionPtr p)
{
    auto ret = p->manual_connect();
    std::cout << "ManualStrategy: try_connect: " << ret << std::endl;
}

void ManualStrategy::do_disconnect_session_command(OE::OrderSessionPtr p)
{
    auto ret = p->manual_disconnect();
    std::cout << "ManualStrategy: try_disconnect: " << ret << std::endl;
}

}}
