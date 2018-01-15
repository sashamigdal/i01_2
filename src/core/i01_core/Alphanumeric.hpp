#pragma once
#include <i01_core/macro.hpp>
#include <i01_core/util.hpp>

#include <cstdint>
#include <array>
#include <type_traits>
#include <limits>
#include <cmath>

namespace i01 { namespace core {

/// Alphanumeric field type template for representing unsigned integers as
/// alphanumeric tokens.  For example, local order IDs as OUCH 4.2 OrderTokens.
/// @tparam OUT_WIDTH the width of the alphanumeric field (NOT null terminated!).
/// @tparam CASE_SENSITIVE use base 62 (true/default) or base 36 (false).
/// @tparam LEFT_PADDING use left padding (true/default) or right (false).
/// @tparam PADDING_CHAR character to use for padding.
template <int OUT_WIDTH, bool CASE_SENSITIVE = true, bool LEFT_PADDING = true, std::uint8_t PAD_CHAR = ' '>
union Alphanumeric {
    std::uint8_t raw[OUT_WIDTH];
    std::array<std::uint8_t, OUT_WIDTH> arr;

    /// Template function that returns the integer equivalent of the
    /// alphanumeric token.  This will work with any arithmetic return type.
    template <class T>
    typename std::enable_if<std::is_arithmetic<T>::value, T>::type
    get() const
    {
        static constexpr const std::uint8_t BASE = CASE_SENSITIVE ? 62 : 36;
        //static_assert( std::pow(62, OUT_WIDTH) < (double)std::numeric_limits<T>::max()
        //             , "Integral type T too small to fit 62^OUT_WIDTH.");

        T ret = 0;
        T multiplier = 1;
        for (int i = (LEFT_PADDING ? OUT_WIDTH - 1 : 0); LEFT_PADDING ? i >= 0 : i < OUT_WIDTH; LEFT_PADDING ? --i : ++i) {
            T inc = 0;
            if      ((raw[i] >= '0') && (raw[i] <= '9')) {
                inc = raw[i] - '0';
            } else if ((raw[i] >= 'A') && (raw[i] <= 'Z')) {
                inc = raw[i] - 'A' + 10;
            } else if ((raw[i] >= 'a') && (raw[i] <= 'z')) {
                inc = raw[i] - 'a' + (CASE_SENSITIVE ? 36 : 10);
            } else if (raw[i] == PAD_CHAR) {
                inc = 0;
            } else {
                ret = 0; // return 0 if invalid character detected.
                break;
            }

            ret += multiplier * inc;
            multiplier *= BASE;
        }
        return ret;
    }

    std::string get_string_copy() const
    {
        return (std::begin(arr), std::end(arr));
    }

    /// Template function that sets the alphanumeric token to the equivalent
    /// of the integer provided.  This will only work with unsigned types.
    /// Upon overflow, alphanumeric token is modified to garbage (or max
    /// possible value) and return value is false.
    template <class T>
    typename std::enable_if<std::is_unsigned<T>::value, bool>::type
    set(T n)
    {
        //static_assert( std::pow(62, OUT_WIDTH) > (double)std::numeric_limits<T>::max()
        //             , "Integral type T too big to fit in 62^OUT_WIDTH.");

        static constexpr const std::uint8_t BASE = CASE_SENSITIVE ? 62 : 36;
        static const uint8_t charset[62 + 1] = "0123456789"
                                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                               "abcdefghijklmnopqrstuvwxyz";
        for (int i = (LEFT_PADDING ? OUT_WIDTH - 1 : 0); LEFT_PADDING ? i >= 0 : i < OUT_WIDTH; LEFT_PADDING ? --i : ++i) {
            if ((n == 0) && (LEFT_PADDING ? (i < OUT_WIDTH - 1) : (i > 0))) {
                raw[i] = PAD_CHAR;
                continue;
            }
            raw[i] = charset[n % BASE];
            n = static_cast<T>(n - (n % BASE))/BASE;
        }
        if (UNLIKELY(n != 0)) {
            return false;
        }
        return true;
    }
};

template <int OUT_WIDTH, bool CASE_SENSITIVE = true, bool LEFT_PADDING = true, std::uint8_t PAD_CHAR = ' '>
std::ostream & operator<<(std::ostream &os, const Alphanumeric<OUT_WIDTH,CASE_SENSITIVE,LEFT_PADDING,PAD_CHAR> &a) {
    return os << a.arr;
}

} }
