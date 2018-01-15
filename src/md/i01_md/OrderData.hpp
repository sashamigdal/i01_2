#pragma once

#include <iosfwd>
#include <limits>
#include <tuple>
#include <utility>

namespace i01 { namespace MD {

using Price = std::uint64_t;
using Size = std::uint32_t;
using NumOrders = std::size_t;

constexpr double PRICE_DIVISOR_INV = 1e-4;
constexpr double PRICE_DIVISOR = 1e4;

template<typename T>
struct EmptyAsk {
    static constexpr T price = std::numeric_limits<T>::max();
};

template<typename T>
struct EmptyBid {
    static constexpr T price = 0;
};

inline constexpr double to_double(const Price &p) {
    return static_cast<double>(p) * PRICE_DIVISOR_INV;
}

inline constexpr Price to_fixed(const double &d) {
    return static_cast<Price>(d*PRICE_DIVISOR);
}

using Summary = std::tuple<Price, Size, NumOrders>;
using FullSummary = std::pair<Summary, Summary>;

struct L2Quote {
    Price price;
    Size size;
    NumOrders num_orders;
    L2Quote(const Price p, const Size s, const NumOrders n) : price(p),
                                                              size(s),
                                                              num_orders(n) {}
    L2Quote(const Summary &s) : price(std::get<0>(s)),
                                size(std::get<1>(s)),
                                num_orders(std::get<2>(s)) {}

    operator Summary() const { return Summary{price, size, num_orders}; }
    inline double price_as_double() const { return static_cast<double>(price)*PRICE_DIVISOR_INV; }
    bool operator==(const L2Quote &q) { return price == q.price && size == q.size && num_orders == q.num_orders; }
    bool operator!=(const L2Quote &q) { return !(*this == q); }
};

struct FullL2Quote {
    L2Quote bid;
    L2Quote ask;

    FullL2Quote() : bid{0,0,0}, ask{std::numeric_limits<Price>::max(),0,0} {}
    FullL2Quote(const L2Quote &b, const L2Quote &a) : bid(b),
                                                      ask(a) {}
    FullL2Quote(const FullSummary &s) : bid(s.first),
                                        ask(s.second) {}

    operator FullSummary() const { return FullSummary{ static_cast<Summary>(bid), static_cast<Summary>(ask) }; }
    bool operator==(const FullL2Quote &q) { return bid == q.bid && ask == q.ask; }
    bool operator!=(const FullL2Quote &q) { return !(*this == q);}
};

std::ostream & operator<<(std::ostream &, const Summary &);
std::ostream & operator<<(std::ostream &, const FullSummary &);

using TradePair = std::pair<Price, Size>;


}}
