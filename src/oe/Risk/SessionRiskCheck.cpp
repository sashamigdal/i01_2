
#include <i01_oe/Risk/SessionRiskCheck.hpp>

namespace i01 { namespace OE {

SessionRiskCheck::SessionRiskCheck()
{
}

SessionRiskCheck::~SessionRiskCheck()
{
}

bool SessionRiskCheck::new_order(const Order* o_p)
{
    (void)(o_p);
    //TODO
    return true;
}

bool SessionRiskCheck::cancel_order(const Order* o_p)
{
    (void)(o_p);
    //TODO
    return true;
}

bool SessionRiskCheck::replace_order(const Order* old_o_p, const Order* new_o_p)
{
    (void)(old_o_p);
    (void)(new_o_p);
    //TODO
    return true;
}

} }
