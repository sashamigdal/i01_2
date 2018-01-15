#include <i01_oe/SimSession.hpp>

namespace i01 { namespace OE {

SimSession::SimSession(OrderManager *omp, const std::string& name_, const std::string& type_name)
    : OrderSession(omp, name_, type_name, true)
{
}


}}
