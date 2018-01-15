#pragma once

#include <i01_oe/Types.hpp>

namespace i01 { namespace OE {

class Order;

class OrderListener
{
public:
  OrderListener() {}
  virtual ~OrderListener() {}

  virtual void on_order_acknowledged(const Order*) {}
  virtual void on_order_fill(const Order*, const Size, const Price, const Dollars fee) {}
  virtual void on_order_cancel(const Order*, const Size) {}
  virtual void on_order_cancel_rejected(const Order*) {}
  virtual void on_order_cancel_replaced(const Order*) {}
  virtual void on_order_rejected(const Order*) {}
};

}}
