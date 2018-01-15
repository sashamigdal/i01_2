#include <i01_core/util.hpp>
#include <i01_md/NASDAQ/ITCH50/Types.hpp>

namespace i01 { namespace MD { namespace NASDAQ { namespace ITCH50 { namespace Types {

std::ostream & operator<<(std::ostream &os, const Stock &s)
{
    using i01::core::operator<<;
    std::array<std::uint8_t,8> arr = s.arr;
    for (auto i = 0U; i < 8; i++) {
        if (arr[i] == '\0') {
            arr[i] = ' ';
        }
    }
    return os << arr;
}

std::ostream & operator<<(std::ostream &os, const ImbalanceDirection &s)
{
    switch (s) {
        case ImbalanceDirection::BUY_IMBALANCE:
            return os << "BUY_IMBALANCE";
        case ImbalanceDirection::SELL_IMBALANCE:
            return os << "SELL_IMBALANCE";
        case ImbalanceDirection::NO_IMBALANCE:
            return os << "NO_IMBALANCE";
        case ImbalanceDirection::INSUFFICIENT_ORDERS_TO_CALCULATE:
            return os << "INSUFFICIENT_ORDERS_TO_CALCULATE";
        default:
            return os << "UNKNOWN";
    }
}

std::ostream & operator<<(std::ostream &os, const CrossType& ct)
{
    switch (ct) {
    case CrossType::NASDAQ_OPENING_CROSS:
        os << "NASDAQ_OPENING_CROSS";
        break;
    case CrossType::NASDAQ_CLOSING_CROSS:
        os << "NASDAQ_CLOSING_CROSS";
        break;
    case CrossType::CROSS_FOR_IPO_AND_HALTED_PAUSED:
        os << "CROSS_FOR_IPO_AND_HALTED_PAUSED";
        break;
    case CrossType::NASDAQ_CROSS_NETWORK_INTRADAY_AND_POST_CLOSE:
        os << "NASDAQ_CROSS_NETWORK_INTRADAY_AND_POST_CLOSE";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const PriceVariationIndicator& pvi)
{
    switch (pvi) {
    case PriceVariationIndicator::LESS_THAN_1:
        os << "LESS_THAN_1";
        break;
    case PriceVariationIndicator::LESS_THAN_2:
        os << "LESS_THAN_2";
        break;
    case PriceVariationIndicator::LESS_THAN_3:
        os << "LESS_THAN_3";
        break;
    case PriceVariationIndicator::LESS_THAN_4:
        os << "LESS_THAN_4";
        break;
    case PriceVariationIndicator::LESS_THAN_5:
        os << "LESS_THAN_5";
        break;
    case PriceVariationIndicator::LESS_THAN_6:
        os << "LESS_THAN_6";
        break;
    case PriceVariationIndicator::LESS_THAN_7:
        os << "LESS_THAN_7";
        break;
    case PriceVariationIndicator::LESS_THAN_8:
        os << "LESS_THAN_8";
        break;
    case PriceVariationIndicator::LESS_THAN_9:
        os << "LESS_THAN_9";
        break;
    case PriceVariationIndicator::LESS_THAN_10:
        os << "LESS_THAN_10";
        break;
    case PriceVariationIndicator::LESS_THAN_20:
        os << "LESS_THAN_20";
        break;
    case PriceVariationIndicator::LESS_THAN_30:
        os << "LESS_THAN_30";
        break;
    case PriceVariationIndicator::GT_OR_EQ_30:
        os << "GT_OR_EQ_30";
        break;
    case PriceVariationIndicator::CANNOT_BE_CALCULATED:
        os << "CANNOT_BE_CALCULATED";
        break;
    default:
        break;
    }
    return os;
}

}}}}}
