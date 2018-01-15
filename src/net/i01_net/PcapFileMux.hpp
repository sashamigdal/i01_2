#pragma once
#include <string.h>

#include <algorithm>
#include <queue>
#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#include <utility>

#include <boost/filesystem.hpp>

#include <i01_core/TimerListener.hpp>

#include <i01_net/IPAddress.hpp>
#include <i01_net/Pcap.hpp>

namespace i01 { namespace net {

using LatencyNS = decltype(core::Timestamp::tv_nsec);
using FileWithLatency = std::pair<std::string, LatencyNS>;
using FileWithLatencyContainer = std::set<FileWithLatency>;

class FileMuxBase {
public:
    using Timestamp = core::Timestamp;

    struct UDPPkt {
        std::uint32_t src_addr;
        std::uint16_t src_port;
        std::uint32_t addr;
        std::uint16_t port;
        std::uint8_t *buf;
        size_t len;
        Timestamp ts;
    };

    using TimerListener = core::TimerListener;
    struct TimerListenerEntry {
        TimerListener * listener;
        void * userdata;
        std::uint64_t iter;
    };

    using TimerListenerContainer = std::vector<TimerListenerEntry>;

public:
    FileMuxBase() = default;
    virtual ~FileMuxBase() = default;

    int read_packets(int num = -1);
    int read_packets_until(const Timestamp &ts);

    void register_timer(TimerListener *listener, void *userdata);

protected:
    void update_timers();

private:
    virtual bool read_next_packet() = 0;

protected:
    FileWithLatencyContainer m_filenames;
    TimerListenerContainer m_timer_listeners;

    Timestamp m_stop_ts;
    Timestamp m_last_ts;
    Timestamp m_timer_ts;
};

/// this is responsible for multiplexing multiple pcap files into a single timestamp ordered stream
template<typename ListenerType>
class PcapFileMux : public FileMuxBase, public UDPPktListener<PcapFileMux<ListenerType> > {
public:
    using UDPReader = pcap::UDPReader<PcapFileMux<ListenerType> >;

private:
    using ReaderEntry = std::pair<std::unique_ptr<UDPReader>, LatencyNS>;
    using ReaderContainer = std::vector<std::unique_ptr<ReaderEntry> >;
    struct PktEntry {
        ReaderEntry* reader;
        UDPPkt pkt;
        constexpr bool operator<(const PktEntry &p) const {
            return pkt.ts < p.pkt.ts;
        }
        constexpr bool operator>(const PktEntry &p) const {
            return pkt.ts > p.pkt.ts;
        }
    };

public:
    using PktQueue = std::priority_queue<PktEntry, std::vector<PktEntry>, std::greater<PktEntry> >;

public:
    PcapFileMux(const FileWithLatencyContainer &files, ListenerType *listener);
    PcapFileMux(const std::set<std::string>& files, ListenerType* listener, LatencyNS = 0);

    virtual ~PcapFileMux() = default;

    void handle_payload(std::uint32_t src_addr, std::uint16_t src_port, std::uint32_t addr, std::uint16_t port, std::uint8_t *buf, size_t len, const Timestamp *ts);

    typename PktQueue::size_type packets_enqueued() const { return m_queue.size(); }

    typename PktQueue::const_reference top() const { return m_queue.top(); }

    void listener(ListenerType *l) { m_listener = l; }

protected:
    virtual bool read_next_packet() override final ;

    int packets_from_reader(ReaderEntry* r);

    void initial_reader_load();

protected:
    ReaderContainer m_readers;
    bool m_handled;
    ReaderEntry* m_current_reader;
    ListenerType * m_listener;
    PktQueue m_queue;
};

template<typename LT>
PcapFileMux<LT>::PcapFileMux(const FileWithLatencyContainer& files, LT *l) :
    m_handled(false),
    m_listener(l)
{
    m_filenames = files;
    initial_reader_load();
}

template<typename LT>
PcapFileMux<LT>::PcapFileMux(const std::set<std::string>& files, LT *l, LatencyNS latency) :
    m_handled(false),
    m_listener(l)
{
    std::transform(files.begin(), files.end(), std::inserter(m_filenames, m_filenames.begin()), [latency](const std::string& s) -> FileWithLatency { return {s, latency};});
    initial_reader_load();
}

template<typename LT>
void PcapFileMux<LT>::initial_reader_load()
{
    for (auto f : m_filenames) {
        auto reader = std::unique_ptr<UDPReader>(new UDPReader(f.first, this));
        m_readers.emplace_back(std::unique_ptr<ReaderEntry>(new ReaderEntry{std::move(reader), f.second}));
        packets_from_reader(m_readers.back().get());
    }
}

template<typename LT>
int PcapFileMux<LT>::packets_from_reader(ReaderEntry* r)
{
    int num_packets = 1;
    int count = 0;

    m_current_reader = r;

    // read a packet at a time until we get one handled in our UDP callback
    m_handled = false;
    while (!m_handled) {
        int ret = m_current_reader->first->read_packets(num_packets);
        if (ret <= 0) {
            // std::cerr << "packet from reader returned 0 " << ip_addr_to_str(m_current_reader->last_channel().second) << std::endl;
            return count;
        }
        count += ret;
    }
    return count;
}

template<typename LT>
bool PcapFileMux<LT>::read_next_packet()
{
    assert(m_queue.size() <= m_readers.size());
    if (!m_queue.empty()) {
        const auto & pkte = m_queue.top();
        const UDPPkt & pkt = pkte.pkt;
        if (pkt.ts < m_last_ts) {
            std::cerr << "pcapfilemux: packet is before last event " << pkt.ts << " " << m_last_ts << " " << ip_addr_to_str(pkte.reader->first->last_channel().second) << std::endl;
        }
        m_last_ts = pkt.ts;

        core::Timestamp::last_event(m_last_ts);

        // need to call this before we dispatch the pkt since it could
        // trigger a timer callback for a time that is before the pkt
        // time
        update_timers();

        m_listener->handle_payload(pkt.src_addr, pkt.src_port, pkt.addr, pkt.port, pkt.buf, pkt.len, &pkt.ts);

        m_current_reader = pkte.reader;
        // pop before we read any new packets
        m_queue.pop();

        packets_from_reader(m_current_reader);

        return true;
    } else {
        return false;
    }
}



template<typename LT>
void PcapFileMux<LT>::handle_payload(std::uint32_t src_addr, std::uint16_t src_port, std::uint32_t addr, std::uint16_t port, std::uint8_t *buf, size_t len, const Timestamp *ts)
{
    m_queue.emplace(PktEntry{m_current_reader, UDPPkt{src_addr, src_port, addr, port, buf, len, *ts + Timestamp{0,m_current_reader->second}}});
    m_handled = true;
}

}}
