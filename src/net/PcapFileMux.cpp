#include <i01_net/PcapFileMux.hpp>

namespace i01 { namespace net {

void FileMuxBase::register_timer(TimerListener *tl, void *userdata)
{
    m_timer_listeners.emplace_back(TimerListenerEntry{tl, userdata, 0});
}

int FileMuxBase::read_packets(int num)
{
    // auto it = m_queue.begin();
    auto count = 0;
    // read until we count off enough pkts or we hit the stop time
    while ((m_stop_ts.tv_sec == 0 && (num < 0 || count < num))
           || (m_last_ts < m_stop_ts)) {
        count++;
        if (!read_next_packet()) {
            break;
        }
    }
    return count;
}

int FileMuxBase::read_packets_until(const Timestamp &ts)
{
    m_stop_ts = ts;
    return read_packets(-1);
}

void FileMuxBase::update_timers()
{
    // call the timer listeners if necessary ... thsi needs to happen before we dispatch the pkt since we could've had a gap in the data... so would need to fire timers in the interim...

    if (m_timer_ts.tv_sec == 0) {
        m_timer_ts = m_last_ts;
    } else {
        auto diff = m_last_ts - m_timer_ts;

        if (diff.tv_sec >= 1) {
            // we have to fire timer at least once
            while (diff.tv_sec-- >= 1) {
                // it's been at least a second since last timer ..
                // we need to hit any timers that occurred in between ...
                m_timer_ts.tv_sec++;
                core::Timestamp::last_event(m_timer_ts);
                for (auto t : m_timer_listeners) {
                    // i think it's always iter==1, since we are never going to miss a read on the timer
                    t.listener->on_timer(m_timer_ts, t.userdata, 1);
                }
            }
            core::Timestamp::last_event(m_last_ts);
        }
    }
}

}}
