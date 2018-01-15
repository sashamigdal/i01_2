#pragma once
#include <iosfwd>

#include <i01_oe/Order.hpp>

namespace i01 { namespace OE {

template<typename DataType>
class AperiodicDailySeries {
public:
    static_assert(std::is_pod<DataType>::value, "AperiodicDailySeries only support POD data");

    AperiodicDailySeries(const DataType &st1 = DataType{},
                         const DataType &st5 = DataType{},
                         const DataType &st20 = DataType{}) {
        m_data[0] = st1;
        m_data[1] = st5;
        m_data[2] = st20;
    }

    DataType five_day() const { return m_data[0]; }
    DataType ten_day() const { return m_data[1]; }
    DataType twenty_day() const { return m_data[2]; }

    bool operator==(const AperiodicDailySeries<DataType> &d) const {
        return m_data == d.m_data;
    }

    friend std::ostream & operator<<(std::ostream &os, const AperiodicDailySeries<DataType> &a) {
        return os << a.m_data[0] << "," << a.m_data[1] << "," << a.m_data[2];
    }

private:
    using Data = std::array<DataType,3>;

    Data m_data;
};

using ADV = AperiodicDailySeries<Quantity>;
using ADR = AperiodicDailySeries<Price>;

}}
