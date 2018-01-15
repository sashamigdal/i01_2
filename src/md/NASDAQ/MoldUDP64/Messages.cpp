#include <i01_core/util.hpp>
#include <i01_md/NASDAQ/MoldUDP64/Messages.hpp>

namespace i01 { namespace MD { namespace NASDAQ { namespace MoldUDP64 { namespace Messages {

using i01::core::operator<<;

std::ostream & operator<<(std::ostream &os, const DownstreamPacketHeader &msg)
{
    return os << msg.session << "," << msg.seqnum << "," << msg.msg_count;
}

}}}}}
