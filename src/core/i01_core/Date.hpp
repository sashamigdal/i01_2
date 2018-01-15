#pragma once

#include <iostream>

#include <i01_core/Time.hpp>

namespace i01 { namespace core {

class Date {
public:
    Date();
    Date(std::uint64_t day);
    // Date(int day) : Date(static_cast<std::uint64_t>(day)) {}

    const std::uint64_t& date() const { return m_date; }

    // Why have a date class in the first place if this exists?
    //operator std::uint64_t() const { return m_date; }

    std::uint64_t yyyy() const;
    std::uint64_t mm() const;
    std::uint64_t dd() const;

    inline bool operator<(const Date &b)
    {
        return date() < b.date();
    }

    inline bool operator==(const Date &b)
    {
        return date() == b.date();
    }

    static Date today();
    static Date from_epoch(const time_t &t);
private:
    std::uint64_t m_date;
};

inline std::ostream & operator<<(std::ostream &os, const Date &d)
{
    return os << d.date();
}

}}
