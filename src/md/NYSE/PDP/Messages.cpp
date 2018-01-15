#include <i01_core/Time.hpp>
#include <i01_core/util.hpp>

#include <i01_md/NYSE/PDP/Messages.hpp>

namespace i01 { namespace MD { namespace NYSE { namespace PDP { namespace Messages {

using i01::core::operator<<;

SymbolArray replace_nulls(const Symbol &sym)
{
    SymbolArray newsym = sym.arr;
    for (auto i = 0U; i < newsym.size(); i++) {
        if (0 == newsym[i]) {
            newsym[i] = ' ';
        }
    }
    return newsym;
}

std::ostream & operator<<(std::ostream &os, const FullUpdate &msg)
{
    os << msg.security_index << ","
       << core::Timestamp{msg.source_time/1000L, (msg.source_time % 1000L)*1000000LL + msg.source_time_microseconds*1000L} << ",";
    return os << msg.symbol_seqnum << ","
              << (int) msg.source_session_id << ","
              << replace_nulls(msg.symbol) << ","
              << (int) msg.price_scale_code << ","
              << (char) msg.quote_condition << ","
              << (char) msg.trading_status << ","
              << msg.mpv;
}

std::ostream & operator<<(std::ostream &os, const FullUpdatePricePoint &msg)
{
    return os << msg.price_numerator << ","
              << msg.volume << ","
              << (char) msg.side;
}

std::ostream & operator<<(std::ostream &os, const DeltaUpdate &msg)
{
    os << msg.security_index << ","
       << core::Timestamp{msg.source_time/1000L, (msg.source_time % 1000L)*1000000LL + msg.source_time_microseconds*1000L} << ",";
    return os << msg.symbol_seqnum << ","
              << (int) msg.source_session_id << ","
              << (char) msg.quote_condition << ","
              << (char) msg.trading_status << ","
              << (int) msg.price_scale_code;
}

std::ostream & operator<<(std::ostream &os, const DeltaUpdatePricePoint &msg)
{
    return os << msg.price_numerator << ","
              << msg.volume << ","
              << msg.chg_qty << ","
              << msg.num_orders << ","
              << (char) msg.side << ","
              << (char) msg.reason_code << ","
              << msg.link_id1 << ","
              << msg.link_id2 << ","
              << msg.link_id3;
}

std::ostream & operator<<(std::ostream &os, const SequenceNumberReset &msg)
{
    return os << msg.next_seqnum;
}

std::ostream & operator<<(std::ostream &os, const SymbolIndexMapping &msg)
{
    return os << replace_nulls(msg.symbol) << ","
              << msg.security_index;
}

std::ostream & operator<<(std::ostream &os, const OpeningImbalance &msg)
{
    return os << replace_nulls(msg.symbol) << ","
              << (char) msg.stock_open_indicator << ","
              << (char) msg.imbalance_side << ","
              << (int) msg.price_scale_code << ","
              << msg.reference_price_numerator << ","
              << msg.imbalance_quantity << ","
              << msg.paired_quantity << ","
              << msg.clearing_price_numerator << ","
              << msg.source_time << ","
              << msg.ssr_filing_price_numerator;
}

std::ostream & operator<<(std::ostream &os, const ClosingImbalance &msg)
{
    return os << replace_nulls(msg.symbol) << ","
              << (char) msg.regulatory_imbalance_indicator << ","
              << (char) msg.imbalance_side << ","
              << (int) msg.price_scale_code << ","
              << msg.reference_price_numerator << ","
              << msg.imbalance_quantity << ","
              << msg.paired_quantity << ","
              << msg.continuous_book_clearing_price_numerator << ","
              << msg.closing_only_clearing_price_numerator << ","
              << msg.source_time;
}


}}}}}
