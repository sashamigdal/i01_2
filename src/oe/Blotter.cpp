#include <i01_oe/Blotter.hpp>

namespace i01 { namespace OE {

Blotter::Blotter(OrderManager& om) : m_om(om)
{
}

NullBlotter::NullBlotter(OrderManager& om) : Blotter(om)
{
}

}}
