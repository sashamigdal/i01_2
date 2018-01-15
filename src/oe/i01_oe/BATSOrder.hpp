#pragma once

#include <i01_oe/BOE20Order.hpp>

namespace i01 { namespace OE {

class BATSOrder : public BOE20Order
{


public:
  BATSOrder(Instrument* const iinstrument,
            const Price iprice,
            const Size isize,
            const Side iside,
            const TimeInForce itif,
            const OrderType itype,
            OrderListener* ilistener,
            const UserData iuserdata = nullptr)
   : BOE20Order(iinstrument, iprice, isize, iside, itif, itype, ilistener, iuserdata)
  {}

  bool validate() const override final { return true; }
  const i01::core::MIC market() const override final { return i01::core::MICEnum::BATS; }
  BATSOrder* clone() const override final { return new BATSOrder(*this); }
};

}}
