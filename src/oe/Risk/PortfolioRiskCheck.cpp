#include <i01_oe/Risk/PortfolioRiskCheck.hpp>

namespace i01 { namespace OE {

PortfolioRiskCheck::PortfolioRiskCheck()
{
}

PortfolioRiskCheck::~PortfolioRiskCheck()
{
}

bool PortfolioRiskCheck::new_order(const Order* o_p)
{
    (void)(o_p);
    //TODO
    return true;
}

bool PortfolioRiskCheck::cancel_order(const Order* o_p)
{
    (void)(o_p);
    //TODO
    return true;
}

bool PortfolioRiskCheck::replace_order(const Order* old_o_p, const Order* new_o_p)
{
    (void)(old_o_p);
    (void)(new_o_p);
    //TODO
    return true;
}

} }
