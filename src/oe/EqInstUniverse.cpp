#include <boost/lexical_cast.hpp>

#include <i01_oe/EqInstUniverse.hpp>

namespace i01 { namespace OE {

EqInstUniverse::EqInstUniverse() : MD::Universe<EquityInstrument>(), m_default_eqinst_params{}
{
}

void EqInstUniverse::init(const core::Config::storage_type& md_cfg,
                          const core::Config::storage_type& oe_cfg)
{
    MD::Universe<EquityInstrument>::init(md_cfg);
    auto csd(oe_cfg.copy_prefix_domain("default."));
    if (csd->size()) {
        m_default_eqinst_params = eqinst_params_from_conf(*csd);
    }

    // we load params by symbol b/c that makes it easier for conf
    // scripts to just generate conf params without having to know the
    // ESI... the MD universe creates the ESI <-> Symbol mapping which
    // is used in the code, but easier to configure without knowing
    // it.

    auto eqd(oe_cfg.copy_prefix_domain("eqinst."));
    auto keys(eqd->get_key_prefix_set());
    for (const auto& k : keys) {
        auto kcfg(eqd->copy_prefix_domain(k + "."));
        auto eqp = eqinst_params_from_conf(*kcfg, m_default_eqinst_params);

        m_params_by_name[k] = eqp;
    }

    // force the universe to call data_for_atom for all defined symbols
    reset_data();
}

auto EqInstUniverse::inst_params_from_conf(const core::Config::storage_type &cfg, const InstParams& default_ip) -> InstParams
{
    auto ip = default_ip;
    cfg.get<Size>("order_size_limit",ip.order_size_limit);
    cfg.get<Price>("order_price_limit",ip.order_price_limit);
    cfg.get<Price>("order_value_limit",ip.order_value_limit);
    cfg.get<int>("order_rate_limit",ip.order_rate_limit);
    cfg.get<Size>("position_limit",ip.position_limit);
    cfg.get<Price>("position_value_limit",ip.position_value_limit);
    cfg.get<Quantity>("start_quantity", ip.start_quantity);
    cfg.get<Price>("prior_adjusted_close", ip.prior_adjusted_close);
    return ip;
}

auto EqInstUniverse::eqinst_params_from_conf(const core::Config::storage_type &cfg, const EqInstParams& default_eqi) -> EqInstParams
{
    auto eq = default_eqi;

    auto *pip = static_cast<Instrument::Params*>(&eq);
    *pip = inst_params_from_conf(cfg, static_cast<const Instrument::Params&>(default_eqi));

    cfg.get<Size>("lot_size", eq.lot_size);
    cfg.get<Quantity>("locate_size", eq.locate_size);
    eq.norecycle = cfg.get_or_default<bool>("norecycle", false);

    Size adv1 = 0, adv2 = 0, adv3 = 0;
    cfg.get("adv_5day", adv1);
    cfg.get<Size>("adv_10day", adv2);
    cfg.get<Size>("adv_20day", adv3);

    eq.adv = ADV(adv1, adv2, adv3);

    Price adr1 = 0, adr2 = 0, adr3 = 0;
    cfg.get("adr_5day", adr1);
    cfg.get("adr_10day", adr2);
    cfg.get("adr_20day", adr3);

    eq.adr = ADR(adr1, adr2, adr3);

    return eq;
}

auto EqInstUniverse::data_for_atom(const MD::EphemeralSymbolIndex &esi, const std::string &str) -> EquityInstrumentPtr
{
    // look for params by name first
    auto it = m_params_by_name.find(str);
    if (it == m_params_by_name.end()) {
        return EquityInstrumentPtr{new EquityInstrument(str, esi, core::MICEnum::UNKNOWN, m_default_eqinst_params)};
    } else {
        return EquityInstrumentPtr{new EquityInstrument(str, esi, core::MICEnum::UNKNOWN, it->second)};
    }
}

}}
