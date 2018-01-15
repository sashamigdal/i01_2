#pragma once

#include <i01_oe/Types.hpp>

namespace i01 { namespace OE {

class TradingFees {
public:
    TradingFees() = default;
    Price fee_from_fill(const Order *op, const Size size, const Price price) { return 0; }
};

}}
