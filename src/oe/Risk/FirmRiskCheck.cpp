#include <i01_md/BookBase.hpp>
#include <i01_md/LastSale.hpp>

#include <i01_oe/Instrument.hpp>
#include <i01_oe/Risk/FirmRiskCheck.hpp>

namespace i01 { namespace OE {

FirmRiskSnapshot::FirmRiskSnapshot(Dollars longn, Dollars shortn,
                                   Dollars longoe, Dollars shortoe,
                                   Dollars real, Dollars unreal) :
    m_long_notional(longn),
    m_short_notional(shortn),
    m_long_open_exposure(longoe),
    m_short_open_exposure(shortoe),
    m_total_realized(real),
    m_total_unrealized(unreal)
{
}

FirmRiskCheck::FirmRiskCheck() :
    m_dirty_mode{false},
    m_mode{Mode::UNKNOWN},
    m_breakers{},
    m_inst_permissions{},
    m_long_notional{0},
    m_short_notional{0},
    m_long_open_exposure{0},
    m_short_open_exposure{0},
    m_total_realized{0},
    m_total_unrealized{0},
    m_realized_loss_limit{0},
    m_unrealized_loss_limit{0},
    m_gross_notional_limit{0},
    m_net_notional_limit{0},
    m_long_open_exposure_limit{0},
    m_short_open_exposure_limit{0},
    m_gross_open_exposure_limit{0}
{
}

FirmRiskCheck::~FirmRiskCheck()
{
}

void FirmRiskCheck::init(const core::Config::storage_type& cfg)
{
    if (auto rll = cfg.get<Dollars>("realized_loss_limit")) {
        m_realized_loss_limit = *rll;
        if (m_realized_loss_limit < 0) {
            throw std::runtime_error("negative realized_loss_limit");
        }

        if (m_realized_loss_limit > HARD_REALIZED_LOSS_LIMIT) {
            throw std::runtime_error("realized loss limit exceeds hard limit " + std::to_string(HARD_REALIZED_LOSS_LIMIT));
        }
    } else {
        std::cerr << "FirmRiskCheck: no realized_loss_limit specified in conf" << std::endl;
    }

    if (auto urll = cfg.get<Dollars>("unrealized_loss_limit")) {
        m_unrealized_loss_limit = *urll;
        if (m_unrealized_loss_limit < 0) {
            throw std::runtime_error("negative unrealized_loss_limit");
        }

        if (m_unrealized_loss_limit > HARD_UNREALIZED_LOSS_LIMIT) {
            throw std::runtime_error("unrealized loss limit exceeds hard limit " + std::to_string(HARD_UNREALIZED_LOSS_LIMIT));
        }
    } else {
        std::cerr << "FirmRiskCheck: no unrealized_loss_limit specified in conf" << std::endl;
    }

    if (auto gnl = cfg.get<Dollars>("gross_notional_limit")) {
        m_gross_notional_limit = *gnl;
        if (m_gross_notional_limit < 0) {
            throw std::runtime_error("negative gross_notional_limit");
        }
        if (m_gross_notional_limit > HARD_GROSS_NOTIONAL_LIMIT) {
            throw std::runtime_error("gross notional limit exceeds hard limit " + std::to_string(HARD_GROSS_NOTIONAL_LIMIT));
        }
    } else {
        std::cerr << "FirmRiskCheck: no gross_notional_limit specified in conf" << std::endl;
    }

    if (auto nnl = cfg.get<Dollars>("net_notional_limit")) {
        m_net_notional_limit = *nnl;
        if (m_net_notional_limit < 0) {
            throw std::runtime_error("negative net_notional_limit");
        }
        if (m_net_notional_limit > HARD_NET_NOTIONAL_LIMIT) {
            throw std::runtime_error("net notional limit exceeds hard limit " + std::to_string(HARD_NET_NOTIONAL_LIMIT));
        }
    } else {
        std::cerr << "FirmRiskCheck: no net_notional_limit specified in conf" << std::endl;
    }

    if (auto loel = cfg.get<Dollars>("long_open_exposure_limit")) {
        m_long_open_exposure_limit = *loel;
        if (m_long_open_exposure_limit < 0) {
            throw std::runtime_error("negative long_open_exposure_limit");
        }
        if (m_long_open_exposure_limit > HARD_LONG_OPEN_EXPOSURE_LIMIT) {
            throw std::runtime_error("long open exposure limit exceeds hard limit " + std::to_string(HARD_LONG_OPEN_EXPOSURE_LIMIT));
        }
    } else {
        std::cerr << "FirmRiskCheck: no long_open_exposure_limit specified in conf" << std::endl;
    }

    if (auto soel = cfg.get<Dollars>("short_open_exposure_limit")) {
        m_short_open_exposure_limit = *soel;
        if (m_short_open_exposure_limit < 0) {
            throw std::runtime_error("negative short_open_exposure_limit");
        }
        if (m_short_open_exposure_limit > HARD_SHORT_OPEN_EXPOSURE_LIMIT) {
            throw std::runtime_error("short open exposure limit exceeds hard limit " + std::to_string(HARD_SHORT_OPEN_EXPOSURE_LIMIT));
        }
    } else {
        std::cerr << "FirmRiskCheck: no short_open_exposure_limit specified in conf" << std::endl;
    }

    if (auto goel = cfg.get<Dollars>("gross_open_exposure_limit")) {
        m_gross_open_exposure_limit = *goel;
        if (m_gross_open_exposure_limit < 0) {
            throw std::runtime_error("negative gross_open_exposure_limit");
        }
        if (m_gross_open_exposure_limit > HARD_GROSS_OPEN_EXPOSURE_LIMIT) {
            throw std::runtime_error("gross open exposure limit exceeds hard limit " + std::to_string(HARD_GROSS_OPEN_EXPOSURE_LIMIT));
        }
    } else {
        std::cerr << "FirmRiskCheck: no gross_open_exposure_limit specified in conf" << std::endl;
    }

    m_mode = Mode::NORMAL;
}

bool FirmRiskCheck::new_order(const Order* op)
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ false);
    auto the_mode = mode();
    switch (the_mode) {
    case Mode::NORMAL:
        break;
    case Mode::DISABLE_ALL:
        // overrides all
        std::cerr << "Validate: Firm: DISABLE ALL mode " << *op << std::endl;
        return false;
    case Mode::FLATTEN_ONLY:
        // only true if flattening...
        goto check_flatten_only;
        break;
    case Mode::LENGTHEN_ONLY:
        if (op->side() != Side::BUY) {
            std::cerr << "Validate: Firm: LENGTHEN ONLY mode " << *op << std::endl;
            return false;
        }
        break;
    case Mode::SHORTEN_ONLY:
        if (op->side() <= Side::BUY) {
            std::cerr << "Validate: Firm: SHORTEN ONLY mode " << *op << std::endl;
            return false;
        }
        break;
    case Mode::UNKNOWN:
    default:
        std::cerr << "Validate: Firm: UNKNOWN mode " << *op << std::endl;
        return false;
    }

    // check stock specific...
    if (UNLIKELY(m_inst_permissions[op->instrument()->esi()][(int)InstPermissions::USER_DISABLE])) {
        std::cerr << "Validate: Firm: stock USER DISABLE " << *op << std::endl;
        return false;
    }

 check_flatten_only:
    if (UNLIKELY(m_inst_permissions[op->instrument()->esi()][(int)InstPermissions::FLATTEN_ONLY]
        || Mode::FLATTEN_ONLY == m_mode)) {
        if (op->instrument()->position().quantity() > 0) {
            // position is long, so only allow sell
            if (op->side() == Side::BUY) {
                std::cerr << "Validate: Firm: stock FLATTEN ONLY " << *op << std::endl;
                return false;
            }
        } else {
            // position is short, only allow buys
            if (op->side() != Side::BUY) {
                std::cerr << "Validate: Firm: stock FLATTEN ONLY " << *op << std::endl;
                return false;
            }
        }
    }

    // here means the order does not conflict with a current breaker
    // state... but we have to check whether it will put us in one...

    // check whether the price is NMS compliant (no subpenny orders for prices > $1)
    if (LIKELY(op->price() >= 1.0)) {
        std::int64_t int_price = static_cast<std::int64_t>(op->price() * 10000.00 + 0.5);
        if (UNLIKELY(int_price % 100 != 0)) {
            std::cerr << "Validate: Firm: NMS price check: " << op->price() << " is a subpenny order " << *op << std::endl;
            return false;
        }
    } else {
        std::int64_t int_price = static_cast<std::int64_t>(op->price() * 1000000.00 + 0.5);
        if (UNLIKELY(int_price % 100 != 0)) {
            std::cerr << "Validate: Firm: NMS price check: " << op->price() << " is a submil order " << *op << std::endl;
            return false;
        }
    }

    auto order_notional = Dollars{static_cast<Dollars>(op->size()) * op->price()};
    // auto passive = is_passive(op);

    // TODO: marketable orders should be subject to the notional limits, but non-marketable should not

    // if we are in flatten only/length-only/shorten-only, we should be able to increase gross notional...
    if (UNLIKELY(the_mode == Mode::NORMAL && gross_notional() + order_notional >= gross_notional_limit())) {
        // this would put us over gross notional limit
        std::cerr << "Validate: Firm: gross notional " << gross_notional() << " + " << order_notional << " >= " << gross_notional_limit() << " " << *op << std::endl;
        return false;
    }

    if (UNLIKELY(gross_open_exposure() + order_notional >= gross_open_exposure_limit())) {
        std::cerr << "Validate: Firm: gross open exposure " << gross_open_exposure() << " + " << order_notional << " >= " << gross_open_exposure_limit() << " " << *op << std::endl;
        return false;
    }

    if (op->side() == Side::BUY) {
        if (UNLIKELY(net_notional() + order_notional >= net_notional_limit())) {
            std::cerr << "Validate: Firm: net notional " << net_notional() << " + " << order_notional << " >= " << net_notional_limit() << " " << *op << std::endl;
            return false;
        }
        if (UNLIKELY(long_open_exposure() + order_notional >= long_open_exposure_limit())) {
            std::cerr << "Validate: Firm: long open exposure " << long_open_exposure() << " + " << order_notional << " >= " << long_open_exposure_limit() << " "<< *op << std::endl;
            return false;
        }
    } else {
        if (UNLIKELY(net_notional() - order_notional <= -net_notional_limit())) {
            std::cerr << "Validate: Firm: net notional " << net_notional() << " + " << -order_notional << " <= " << -net_notional_limit() << " " << *op << std::endl;
            return false;
        }
        if (UNLIKELY(short_open_exposure() - order_notional <= short_open_exposure_limit())) {
            std::cerr << "Validate: Firm: short open exposure " << short_open_exposure() << " - " << order_notional << " <= " << short_open_exposure_limit() << " " << *op << std::endl;
            return false;
        }
    }

    return true;
}

bool FirmRiskCheck::is_passive(const Order *op) const {
    return op->tif() != TimeInForce::IMMEDIATE_OR_CANCEL && op->tif() != TimeInForce::FILL_OR_KILL;
}

bool FirmRiskCheck::cancel_order(const Order* I01_UNUSED o_p)
{
    return true;
}

bool FirmRiskCheck::replace_order(const Order* old_o_p, const Order* new_o_p)
{
    (void)(old_o_p);
    (void)(new_o_p);
    // TODO: FirmRiskCheck::replace_order not implemented
    return false;
}

void FirmRiskCheck::on_order_adds(const Order *op, const Size add_size)
{
    Instrument::mutex_type::scoped_lock instlock(op->instrument()->mutex());
    Mutex::scoped_lock lock(m_mutex, /*write=*/ true);
    unsafe_on_order_adds(op, add_size);
}

void FirmRiskCheck::unsafe_on_order_adds(const Order *op, const Size add_size)
{
    // TODO: does open exposure only depend on order price?
    auto& pos = op->instrument()->position();
    if (op->side() == Side::BUY) {
        auto prior_long_exposure = pos.long_open_exposure();
        pos.add_open_buys(op->price(), add_size);
        m_long_open_exposure += (pos.long_open_exposure() - prior_long_exposure);
    } else {
        auto prior_short_exposure = pos.short_open_exposure();
        pos.add_open_sells(op->price(), add_size);
        m_short_open_exposure += (pos.short_open_exposure() - prior_short_exposure);
    }
    update_open_exposure_breaker_state(op->side());
}

void FirmRiskCheck::on_order_removes(const Order *op, const Size cancel_size)
{
    Instrument::mutex_type::scoped_lock instlock(op->instrument()->mutex());
    Mutex::scoped_lock lock(m_mutex, /*write=*/ true);
    unsafe_on_order_removes(op, cancel_size);
}

void FirmRiskCheck::unsafe_on_order_removes(const Order *op, const Size cancel_size)
{
    auto& pos = op->instrument()->position();
    if (op->side() == Side::BUY) {
        auto prior_long_exposure = pos.long_open_exposure();
        pos.sub_open_buys(op->price(), cancel_size);
        m_long_open_exposure += (pos.long_open_exposure() - prior_long_exposure);
    } else {
        auto prior_short_exposure = pos.short_open_exposure();
        pos.sub_open_sells(op->price(), cancel_size);
        m_short_open_exposure += (pos.short_open_exposure() - prior_short_exposure);
    }
    update_open_exposure_breaker_state(op->side());
}

void FirmRiskCheck::on_order_fill(const Order *op, const Size size, const Price price, const Dollars fee)
{
    Instrument::mutex_type::scoped_lock instlock(op->instrument()->mutex());
    Mutex::scoped_lock lock(m_mutex, /*write=*/ true);
    unsafe_on_order_fill(op, size, price, fee);
}

void FirmRiskCheck::unsafe_on_order_fill(const Order *op, const Size size, const Price price, const Dollars fee)
{
    auto& pos = op->instrument()->position();
    {
        auto prior_realized = pos.realized_pnl();
        auto prior_unrealized = pos.unrealized_pnl();
        auto prior_notional = pos.notional();
        auto prior_pos = pos.trading_quantity();

        if (prior_pos > 0) {
            m_long_notional -= prior_notional;
        } else if (prior_pos < 0) {
            m_short_notional -= prior_notional;
        }

        pos.update_realized_and_unrealized_pnl(op->side(), size, price, fee);

        m_total_realized += (pos.realized_pnl() - prior_realized);
        m_total_unrealized += (pos.unrealized_pnl() - prior_unrealized);

        auto new_pos = pos.trading_quantity();
        if (new_pos > 0) {
            m_long_notional += pos.notional();
        } else if (new_pos < 0) {
            m_short_notional += pos.notional();
        }

        // adjust open exposure here
        unsafe_on_order_removes(op, size);
    }

    // TODO: update concentration here

    update_pnl_breaker_state();

    update_notional_breaker_state();

    update_open_exposure_breaker_state(op->side());
}

void FirmRiskCheck::update_pnl_breaker_state()
{
    m_dirty_mode = true;
    if (UNLIKELY(realized_loss_breaker())) {
        if (!m_breakers[(std::uint8_t)Breakers::REALIZED_LOSS]) {
            // first time
            std::cerr << "FirmRiskCheck: realized loss of " << m_total_realized << " breaks limit of " << realized_loss_limit() << std::endl;
            m_breakers.set((std::uint8_t)Breakers::REALIZED_LOSS);
        }
    } else {
        m_breakers.reset((std::uint8_t)Breakers::REALIZED_LOSS);
    }

    if (UNLIKELY(unrealized_loss_breaker())) {
        if (!m_breakers[(std::uint8_t)Breakers::UNREALIZED_LOSS]) {
            std::cerr << "FirmRiskCheck: unrealized loss of " << m_total_unrealized << " breaks limit of " << unrealized_loss_limit() << std::endl;
            m_breakers.set((std::uint8_t)Breakers::UNREALIZED_LOSS);
        }
    } else {
        m_breakers.reset((std::uint8_t)Breakers::UNREALIZED_LOSS);
    }
}

void FirmRiskCheck::update_notional_breaker_state()
{
    m_dirty_mode = true;
    if (UNLIKELY(gross_notional_breaker())) {
        if (!m_breakers[(std::uint8_t)Breakers::GROSS_NOTIONAL]) {
            std::cerr << "FirmRiskCheck: gross notional of " << gross_notional() << " breaks limit of " << gross_notional_limit() << std::endl;
            m_breakers.set((std::uint8_t)Breakers::GROSS_NOTIONAL);
        }
    } else {
        m_breakers.reset((std::uint8_t)Breakers::GROSS_NOTIONAL);
    }

    if (UNLIKELY(net_notional_breaker())) {
        if (!m_breakers[(std::uint8_t)Breakers::NET_NOTIONAL]) {
            std::cerr << "FirmRiskCheck: absolute net notional of " << abs_net_notional() << " breaks limits of " << net_notional_limit() << std::endl;
            m_breakers.set((std::uint8_t)Breakers::NET_NOTIONAL);
        }
    } else {
        m_breakers.reset((std::uint8_t)Breakers::NET_NOTIONAL);
    }
}

void FirmRiskCheck::update_open_exposure_breaker_state(const Side s)
{
    m_dirty_mode = true;

    if (Side::BUY == s) {
        // long exposure changed
        if (UNLIKELY(long_open_exposure_breaker())) {
            if (!m_breakers[(std::uint8_t)Breakers::LONG_OPEN]) {
                std::cerr << "FirmRiskCheck: long open exposure of " << long_open_exposure() << " breaks limit of " << long_open_exposure_limit() << std::endl;
                m_breakers.set((std::uint8_t)Breakers::LONG_OPEN);
            }
        } else {
            m_breakers.reset((std::uint8_t)Breakers::LONG_OPEN);
        }
    } else {
        if (UNLIKELY(short_open_exposure_breaker())) {
            if (!m_breakers[(std::uint8_t)Breakers::SHORT_OPEN]) {
                std::cerr << "FirmRiskCheck: short open exposure of " << short_open_exposure() << " breaks limit of " << short_open_exposure_limit() << std::endl;
                m_breakers.set((std::uint8_t)Breakers::SHORT_OPEN);
            }
        } else {
            m_breakers.reset((std::uint8_t)Breakers::SHORT_OPEN);
        }
    }

    if (gross_open_exposure_breaker()) {
        if (UNLIKELY(!m_breakers[(std::uint8_t)Breakers::GROSS_OPEN])) {
            std::cerr << "FirmRiskCheck: gross open exposure of " << gross_open_exposure() << " breaks limit of " << gross_open_exposure_limit() << std::endl;
            m_breakers.set((std::uint8_t)Breakers::GROSS_OPEN);
        }
    } else {
        m_breakers.reset((std::uint8_t)Breakers::GROSS_OPEN);
    }
}

auto FirmRiskCheck::update_mode() -> Mode
{
    if (m_breakers[(std::uint8_t)Breakers::USER_DISABLE]) {
        return Mode::DISABLE_ALL;
    }

    if (m_breakers[(std::uint8_t)Breakers::REALIZED_LOSS]
        || m_breakers[(std::uint8_t)Breakers::UNREALIZED_LOSS]) {
        return Mode::DISABLE_ALL;
    }

    if (m_breakers[(std::uint8_t)Breakers::GROSS_NOTIONAL]) {
        return Mode::FLATTEN_ONLY;
    }

    if (m_breakers[(std::uint8_t)Breakers::NET_NOTIONAL]) {
        // are we long or short ...
        if (net_notional() > 0) {
            return Mode::SHORTEN_ONLY;
        } else {
            return Mode::LENGTHEN_ONLY;
        }
    }

    if (m_breakers[(std::uint8_t)Breakers::GROSS_OPEN]
        || m_breakers[(std::uint8_t)Breakers::LONG_OPEN]
        || m_breakers[(std::uint8_t)Breakers::SHORT_OPEN]) {
        return Mode::DISABLE_ALL;
    }

    return Mode::NORMAL;
}

auto FirmRiskCheck::mode() -> Mode
{
    // TODO: put this logic into its own object...
    if (UNLIKELY(m_dirty_mode)) {
        // update the mode here
        m_mode = update_mode();
    }

    return m_mode;
}

auto FirmRiskCheck::status() const -> Status
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ false);
    return unsafe_status();
}

auto FirmRiskCheck::unsafe_status() const -> Status
{
    auto s = Status{};

    // this is not going to update the mode here since we are a const
    // member function
    s.mode = m_mode;
    s.breakers = m_breakers;

    return s;
}

void FirmRiskCheck::on_last_sale_change(Instrument *inst, const MD::LastSale& ls)
{
    Instrument::mutex_type::scoped_lock instlock(inst->mutex());
    Mutex::scoped_lock lock(m_mutex, /*write=*/ true);
    unsafe_on_last_sale_change(inst, ls);
}

void FirmRiskCheck::unsafe_on_last_sale_change(Instrument *inst, const MD::LastSale& ls)
{
    auto& pos = inst->position();
    // we only care about this if we have a position....
    if (pos.trading_quantity() == 0) {
        return;
    }

    auto p = MD::to_double(ls.price);

    auto prior_unrealized = pos.unrealized_pnl();
    auto prior_notional = pos.notional();

    pos.on_update_mark(p);

    m_total_unrealized += (pos.unrealized_pnl() - prior_unrealized);

    if (pos.trading_quantity() > 0) {
        m_long_notional += (pos.notional() - prior_notional);
    } else {
        m_short_notional += (pos.notional() - prior_notional);
    }
    // do a check of whether we hit the breaks now?
    update_pnl_breaker_state();
    update_notional_breaker_state();
}

void FirmRiskCheck::print(std::ostream& os) const
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ false);
    os << "REALPNL," << total_realized_pnl() << ","
       << "UNREALPNL," << total_unrealized_pnl() << ","
       << "GROSSNOT," << gross_notional() << ","
       << "NETNOT," << net_notional() << ","
       << "LONGOPEN," << long_open_exposure() << ","
       << "SHORTOPEN," << short_open_exposure() << ","
       << "STATUS,"
       << unsafe_status();
}

std::ostream& operator<<(std::ostream& os, const FirmRiskCheck::Mode& m)
{
    switch (m) {
    case FirmRiskCheck::Mode::UNKNOWN:
        os << "UNKNOWN";
        break;
    case FirmRiskCheck::Mode::NORMAL:
        os << "NORMAL";
        break;
    case FirmRiskCheck::Mode::DISABLE_ALL:
        os << "DISABLE_ALL";
        break;
    case FirmRiskCheck::Mode::FLATTEN_ONLY:
        os << "FLATTEN_ONLY";
        break;
    case FirmRiskCheck::Mode::LENGTHEN_ONLY:
        os << "LENGTHEN_ONLY";
        break;
    case FirmRiskCheck::Mode::SHORTEN_ONLY:
        os << "SHORTEN_ONLY";
        break;
    default:
        break;
    };
    return os;
}

std::ostream& operator<<(std::ostream& os, const FirmRiskCheck::Breakers& b)
{
    switch (b) {
    case FirmRiskCheck::Breakers::REALIZED_LOSS:
        os << "REALIZED_LOSS";
        break;
    case FirmRiskCheck::Breakers::UNREALIZED_LOSS:
        os << "UNREALIZED_LOSS";
        break;
    case FirmRiskCheck::Breakers::GROSS_NOTIONAL:
        os << "GROSS_NOTIONAL";
        break;
    case FirmRiskCheck::Breakers::NET_NOTIONAL:
        os << "NET_NOTIONAL";
        break;
    case FirmRiskCheck::Breakers::LONG_OPEN:
        os << "LONG_OPEN";
        break;
    case FirmRiskCheck::Breakers::SHORT_OPEN:
        os << "SHORT_OPEN";
        break;
    case FirmRiskCheck::Breakers::GROSS_OPEN:
        os << "GROSS_OPEN";
        break;
    case FirmRiskCheck::Breakers::USER_DISABLE:
        os << "USER_DISABLE";
        break;
    case FirmRiskCheck::Breakers::NUM_BREAKERS:
        default:
        break;
    }
    return os;
}

void print_breaker_helper(std::ostream& os, const FirmRiskCheck::BreakerBits& b, FirmRiskCheck::Breakers which, bool comma=true)
{
    if (b[(int) which]) {
        os << which;
    }
    if (comma) {
        os << ",";
    }
}

std::ostream& operator<<(std::ostream& os, const FirmRiskCheck::Status& s)
{
    os << s.mode << ",";
    print_breaker_helper(os, s.breakers, FirmRiskCheck::Breakers::REALIZED_LOSS);
    print_breaker_helper(os, s.breakers, FirmRiskCheck::Breakers::UNREALIZED_LOSS);
    print_breaker_helper(os, s.breakers, FirmRiskCheck::Breakers::GROSS_NOTIONAL);
    print_breaker_helper(os, s.breakers, FirmRiskCheck::Breakers::NET_NOTIONAL);
    print_breaker_helper(os, s.breakers, FirmRiskCheck::Breakers::LONG_OPEN);
    print_breaker_helper(os, s.breakers, FirmRiskCheck::Breakers::SHORT_OPEN);
    print_breaker_helper(os, s.breakers, FirmRiskCheck::Breakers::GROSS_OPEN);
    print_breaker_helper(os, s.breakers, FirmRiskCheck::Breakers::USER_DISABLE, false);
    return os;
}

FirmRiskSnapshot FirmRiskCheck::snapshot() const
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ false);
    return FirmRiskSnapshot{long_notional(), short_notional(),
            long_open_exposure(), short_open_exposure(),
            total_realized_pnl(), total_unrealized_pnl()};
}

FirmRiskLimits FirmRiskCheck::limits() const
{
    Mutex::scoped_lock lock(m_mutex, /*write=*/ false);
    return FirmRiskLimits{realized_loss_limit(), unrealized_loss_limit(),
            gross_notional_limit(), net_notional_limit(),
            long_open_exposure_limit(), short_open_exposure_limit(),
            gross_open_exposure_limit()};
}

} }
