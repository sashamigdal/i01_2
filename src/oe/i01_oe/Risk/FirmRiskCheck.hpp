#pragma once

#include <bitset>
#include <iosfwd>
#include <unordered_map>

#include <i01_core/Config.hpp>
#include <i01_core/Lock.hpp>
#include <i01_md/Symbol.hpp>

#include "RiskCheck.hpp"

namespace i01 { namespace MD {
struct LastSale;
}}

namespace i01 { namespace OE {

class OrderManager;

class FirmRiskSnapshot {
public:
    FirmRiskSnapshot(Dollars longn, Dollars shortn,
                     Dollars longoe, Dollars shortoe,
                     Dollars real, Dollars unreal);

    Dollars total_realized_pnl() const { return m_total_realized; }
    Dollars total_unrealized_pnl() const { return m_total_unrealized; }
    Dollars total_pnl() const { return total_realized_pnl() + total_unrealized_pnl(); }

    Dollars long_notional() const { return m_long_notional; }
    Dollars short_notional() const { return m_short_notional; }
    Dollars gross_notional() const { return long_notional() - short_notional(); }
    Dollars net_notional() const { return long_notional() + short_notional(); }
    Dollars abs_net_notional() const { return net_notional() < 0 ? -net_notional() : net_notional(); }

    Dollars long_open_exposure() const { return m_long_open_exposure; }
    Dollars short_open_exposure() const { return m_short_open_exposure; }
    Dollars gross_open_exposure() const { return long_open_exposure() - short_open_exposure(); }
    Dollars net_open_exposure() const { return long_open_exposure() + short_open_exposure(); }

private:
    Dollars m_long_notional;
    Dollars m_short_notional;

    Dollars m_long_open_exposure;
    Dollars m_short_open_exposure;

    Dollars m_total_realized;
    Dollars m_total_unrealized;
};

struct FirmRiskLimits {
    Dollars realized_loss_limit;
    Dollars unrealized_loss_limit;

    Dollars gross_notional_limit;
    Dollars net_notional_limit;

    Dollars long_open_exposure_limit;
    Dollars short_open_exposure_limit;
    Dollars gross_open_exposure_limit;
};

class FirmRiskCheck : public RiskCheck {
public:
    static constexpr Dollars HARD_REALIZED_LOSS_LIMIT = 4.8e5;
    static constexpr Dollars HARD_UNREALIZED_LOSS_LIMIT = 4.8e5;
    static constexpr Dollars HARD_GROSS_NOTIONAL_LIMIT = 4.2e7;
    static constexpr Dollars HARD_NET_NOTIONAL_LIMIT = 4.8e6;
    static constexpr Dollars HARD_LONG_OPEN_EXPOSURE_LIMIT = 1.2e7;
    static constexpr Dollars HARD_SHORT_OPEN_EXPOSURE_LIMIT = 1.2e7;
    static constexpr Dollars HARD_GROSS_OPEN_EXPOSURE_LIMIT = 1.8e7;

public:
    using Timestamp = core::Timestamp;

    enum class InstPermissions : std::uint8_t {
        FLATTEN_ONLY=0,
        USER_DISABLE=1,
        NUM_PERMISSIONS
    };

    enum class Breakers : std::uint8_t {
        REALIZED_LOSS=0,
        UNREALIZED_LOSS=1,
        GROSS_NOTIONAL=2,
        NET_NOTIONAL=3,
        LONG_OPEN=4,
        SHORT_OPEN=5,
        GROSS_OPEN=6,
        USER_DISABLE=7,
        NUM_BREAKERS
    };

    using BreakerBits = std::bitset<(int)Breakers::NUM_BREAKERS>;
    using InstPermissionBits = std::bitset<(int)InstPermissions::NUM_PERMISSIONS>;
    using InstPermissionsArray = std::array<InstPermissionBits, MD::NUM_SYMBOL_INDEX>;

    enum class Mode : std::uint8_t {
        UNKNOWN=0
    , NORMAL
    , DISABLE_ALL
    , FLATTEN_ONLY
    , LENGTHEN_ONLY
    , SHORTEN_ONLY
    };

    struct Status {
        Mode mode;
        BreakerBits breakers;
    };

    using Mutex = core::SpinRWMutex;

public:
    /// Create a new risk check.
    FirmRiskCheck();
    /// Destructor.
    virtual ~FirmRiskCheck();

    void user_disable() { Mutex::scoped_lock lock(m_mutex, /*write=*/ true);  m_breakers.set((std::uint8_t)Breakers::USER_DISABLE); update_mode(); }
    void user_enable() { Mutex::scoped_lock lock(m_mutex, /*write=*/ true); m_breakers.reset((std::uint8_t)Breakers::USER_DISABLE); update_mode(); }

    void user_disable(const MD::EphemeralSymbolIndex esi) { Mutex::scoped_lock lock(m_mutex, /*write=*/ true); m_inst_permissions[esi].set((int)InstPermissions::USER_DISABLE); }
    void user_enable(const MD::EphemeralSymbolIndex esi) { Mutex::scoped_lock lock(m_mutex, /*write=*/ true); m_inst_permissions[esi].reset((int)InstPermissions::USER_DISABLE); }

    InstPermissionBits get_inst_permissions(const MD::EphemeralSymbolIndex esi) const { Mutex::scoped_lock lock(m_mutex, /*write=*/ false); return m_inst_permissions[esi]; }

    Status status() const;

    void print(std::ostream& os) const;

    friend std::ostream& operator<<(std::ostream& os, const FirmRiskCheck& frc) {
        frc.print(os);
        return os;
    }

    friend std::ostream& operator<<(std::ostream& os, const Breakers& b);

    friend std::ostream& operator<<(std::ostream& os, const Mode& m);

    friend std::ostream& operator<<(std::ostream& os, const Status& s);

    FirmRiskSnapshot snapshot() const;
    FirmRiskLimits limits() const;

protected:
    void init(const core::Config::storage_type &cfg);

    /// Returns true if the new order passes the risk check.
    bool new_order(const Order*) override final;
    /// Returns true if the cancel passes the risk check.
    bool cancel_order(const Order*) override final;
    /// Returns true if the replace passes the risk check.
    bool replace_order(const Order* old_o_p, const Order* new_o_p) override final;

    void on_last_sale_change(Instrument *inst, const MD::LastSale& ls);
    void unsafe_on_last_sale_change(Instrument *inst, const MD::LastSale& ls);

    void on_order_adds(const Order *op, const Size add_size);
    void on_order_removes(const Order *op, const Size cancel_size);
    void on_order_fill(const Order*, const Size, const Price p, const Dollars fee);

    /// The unsafe version does not use the mutex
    void unsafe_on_order_adds(const Order *op, const Size add_size);
    void unsafe_on_order_removes(const Order *op, const Size cancel_size);
    void unsafe_on_order_fill(const Order*, const Size, const Price p, const Dollars fee);

    Status unsafe_status() const;

    Mode mode();

    void update_pnl_breaker_state();
    void update_notional_breaker_state();
    void update_open_exposure_breaker_state(const Side s);
    Mode update_mode();
    bool is_passive(const Order*) const;

    Dollars total_realized_pnl() const { return m_total_realized; }
    Dollars total_unrealized_pnl() const { return m_total_unrealized; }
    Dollars total_pnl() const { return total_realized_pnl() + total_unrealized_pnl(); }

    Dollars realized_loss_limit() const { return -m_realized_loss_limit; }
    Dollars unrealized_loss_limit() const { return -m_unrealized_loss_limit; }

    Dollars long_notional() const { return m_long_notional; }
    Dollars short_notional() const { return m_short_notional; }
    Dollars gross_notional() const { return long_notional() - short_notional(); }
    Dollars net_notional() const { return long_notional() + short_notional(); }
    Dollars abs_net_notional() const { return net_notional() < 0 ? -net_notional() : net_notional(); }

    Dollars gross_notional_limit() const { return m_gross_notional_limit; }
    Dollars net_notional_limit() const { return m_net_notional_limit; }

    Dollars long_open_exposure() const { return m_long_open_exposure; }
    Dollars short_open_exposure() const { return m_short_open_exposure; }
    Dollars gross_open_exposure() const { return long_open_exposure() - short_open_exposure(); }
    Dollars net_open_exposure() const { return long_open_exposure() + short_open_exposure(); }

    Dollars long_open_exposure_limit() const { return m_long_open_exposure_limit; }
    Dollars short_open_exposure_limit() const { return -m_short_open_exposure_limit; }
    Dollars gross_open_exposure_limit() const { return m_gross_open_exposure_limit; }

    bool realized_loss_breaker() const { return total_realized_pnl() <= realized_loss_limit(); }
    bool unrealized_loss_breaker() const { return total_unrealized_pnl() <= unrealized_loss_limit(); }

    bool gross_notional_breaker() const { return gross_notional() >= gross_notional_limit(); }
    bool net_notional_breaker() const { return abs_net_notional() >= net_notional_limit(); }

    bool long_open_exposure_breaker() const { return long_open_exposure() >= long_open_exposure_limit(); }
    bool short_open_exposure_breaker() const { return short_open_exposure() <= short_open_exposure_limit(); }
    bool gross_open_exposure_breaker() const { return gross_open_exposure() >= gross_open_exposure_limit(); }

protected:
    mutable Mutex m_mutex;
    bool m_dirty_mode;
    Mode m_mode;
    BreakerBits m_breakers;

    InstPermissionsArray m_inst_permissions;

    Dollars m_long_notional;
    Dollars m_short_notional;

    Dollars m_long_open_exposure;
    Dollars m_short_open_exposure;

    Dollars m_total_realized;
    Dollars m_total_unrealized;

    Dollars m_realized_loss_limit;
    Dollars m_unrealized_loss_limit;

    Dollars m_gross_notional_limit;
    Dollars m_net_notional_limit;

    Dollars m_long_open_exposure_limit;
    Dollars m_short_open_exposure_limit;
    Dollars m_gross_open_exposure_limit;

    friend class OrderManager;
};

} }
