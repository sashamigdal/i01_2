#pragma once

#include <i01_oe/Order.hpp>

namespace i01 { namespace OE {

class NYSEOrder : public Order
{
public:
  NYSEOrder(Instrument* const iinstrument,
            const Price iprice,
            const Size isize,
            const Side iside,
            const TimeInForce itif,
            const OrderType itype,
            OrderListener* ilistener,
            const UserData iuserdata = nullptr)
      : Order(iinstrument, iprice, isize, iside, itif, itype, ilistener, iuserdata)
  {}

  bool validate() const override final { return true; }
  const i01::core::MIC market() const override final { return i01::core::MICEnum::XNYS; }
  NYSEOrder* clone() const override final { return new NYSEOrder(*this); }
};

}}
