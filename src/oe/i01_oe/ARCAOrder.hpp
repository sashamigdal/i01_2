#pragma once

#include <i01_oe/Order.hpp>

namespace i01 { namespace OE {

class ARCAOrder : public Order
{
public:
  ARCAOrder(Instrument* const iinstrument,
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
  const i01::core::MIC market() const override final { return i01::core::MICEnum::ARCX; }
  ARCAOrder* clone() const override final { return new ARCAOrder(*this); }
};

}}
