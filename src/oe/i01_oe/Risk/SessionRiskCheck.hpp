
#pragma once

#include "RiskCheck.hpp"

namespace i01 { namespace OE {

    class SessionRiskCheck : public RiskCheck {
    public:
        /// Create a new risk check.
        SessionRiskCheck();
        /// Destructor.
        virtual ~SessionRiskCheck();

        /// Returns true if the new order passes the risk check.
        bool new_order(const Order*) override final;
        /// Returns true if the cancel passes the risk check.
        bool cancel_order(const Order*) override final;
        /// Returns true if the replace passes the risk check.
        bool replace_order(const Order* old_o_p, const Order* new_o_p) override final;

    protected:
        //TODO
    };

} }
