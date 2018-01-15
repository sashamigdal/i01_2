#pragma once

#include <type_traits>
#include <tuple>

#include <i01_oe/Order.hpp>

namespace i01 { namespace OE {

    class RiskCheck {
    public:
        /// Create a new risk check.
        RiskCheck();
        /// Destructor.
        virtual ~RiskCheck();

        /// Returns true if the new order passes the risk check.
        virtual bool new_order(const Order*) = 0;
        /// Returns true if the cancel passes the risk check.
        virtual bool cancel_order(const Order*) = 0;
        /// Returns true if the replace passes the risk check.
        virtual bool replace_order(const Order* old_, const Order* new_) = 0;

    protected:
    private:
        friend class OrderManager;
    };


} }
