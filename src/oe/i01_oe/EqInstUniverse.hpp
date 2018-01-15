#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <i01_core/Config.hpp>

#include <i01_md/Universe.hpp>

#include <i01_oe/EquityInstrument.hpp>

namespace i01 { namespace OE {

class EqInstUniverse : public MD::Universe<EquityInstrument> {
public:
    using InstParams = Instrument::Params;
    using EqInstParams = EquityInstrument::Params;
    using EquityInstrumentPtr = std::unique_ptr<EquityInstrument>;

private:
    using EqInstParamsByName = std::unordered_map<std::string, EqInstParams>;

public:
    EqInstUniverse();

    void init(const core::Config::storage_type &md_cfg, const core::Config::storage_type &oe_cfg);

private:
    InstParams inst_params_from_conf(const core::Config::storage_type &cfg, const InstParams& default_up = InstParams{});
    EqInstParams eqinst_params_from_conf(const core::Config::storage_type &cfg, const EqInstParams& default_eqi = EqInstParams{});

    // void instruments_from_conf(const core::Config::storage_type &cfg);

    virtual EquityInstrumentPtr data_for_atom(const MD::EphemeralSymbolIndex &esi, const std::string &symbol) override final;

private:
    EqInstParams m_default_eqinst_params;
    EqInstParamsByName m_params_by_name;
};

}}
