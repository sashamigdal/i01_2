#pragma once

#include <array>
#include <iosfwd>
#include <iterator>

namespace i01 { namespace core {

// compile time capable min
template<typename T>
constexpr const T& cmin(const T& a, const T& b) {
    return a < b ? a : b;
}

/// serialize a std::array
template<class T, std::size_t N>
std::ostream & operator<<(std::ostream &os, const std::array<T,N> & arr)
{
    copy(arr.begin(), arr.end(), std::ostream_iterator<T>(os));
    return os;
}

template <class T>
inline void i01_hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template<class T, std::size_t N>
struct array_hasher {
    std::size_t operator() (const std::array<T,N> &symbol) const {
        std::size_t seed = 0;
        for (std::size_t i = 0; i < N; i++) {
            std::uint8_t s = symbol[i];
            i01_hash_combine(seed, s);
        }
        return seed;
    }
};


}}
