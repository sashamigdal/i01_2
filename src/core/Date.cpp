#include <time.h>

#include <i01_core/Date.hpp>

namespace i01 { namespace core {

Date::Date() : m_date(0)
{
}

Date::Date(std::uint64_t day) : m_date(day)
{
}

Date Date::today()
{
    return from_epoch(::time(NULL));
}

Date Date::from_epoch(const time_t &t)
{
    struct tm * tmp(::localtime(&t));
    return (tmp->tm_year + 1900)*10000 + ((tmp->tm_mon+1)*100) + tmp->tm_mday;
}

std::uint64_t Date::yyyy() const
{
    return m_date / 10000ULL;
}

std::uint64_t Date::mm() const
{
    return (m_date % 10000ULL) / 100ULL;
}

std::uint64_t Date::dd() const
{
    return m_date % 100ULL;
}

}}
