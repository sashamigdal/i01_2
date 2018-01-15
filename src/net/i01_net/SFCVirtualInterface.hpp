#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <atomic>

#include <boost/lockfree/stack.hpp>

#include <netinet/in.h>
#include <etherfabric/vi.h>
#include <etherfabric/pd.h>
#include <etherfabric/pio.h>
#include <etherfabric/memreg.h>

#include <i01_core/macro.hpp>

#include <i01_core/Time.hpp>
#include <i01_net/SFCInterface.hpp>
#include <i01_net/SFCFilterSpec.hpp>
#include <i01_net/SFCVISet.hpp>
#include <i01_net/SFCPktBuf.hpp>

namespace i01 { namespace net {

class SFCVirtualInterface : public SFCFilterable {
public:
    typedef i01::core::Timestamp Timestamp;

    enum class FilterIPProto : int {
        UDP = ::IPPROTO_UDP
      , TCP = ::IPPROTO_TCP
    };

    /// Create a new VI for use with the interface `intf`.
    explicit SFCVirtualInterface( SFCInterface& intf
                                , int evq_capacity_ = -1
                                , int rxq_capacity_ = -1
                                , int txq_capacity_ = -1
                                , SFCVirtualInterface* evq_opt = nullptr
                                , bool hw_timestamping = true);
    explicit SFCVirtualInterface( SFCVISet& vi_set
                                , int index_in_vi_set = -1
                                , int evq_capacity_ = -1
                                , int rxq_capacity_ = -1
                                , int txq_capacity_ = -1
                                , SFCVirtualInterface* evq_opt = nullptr
                                , bool hw_timestamping = true);
    virtual ~SFCVirtualInterface();

    /* SFCFilterable: */
    bool apply_filter(SFCFilterSpec& fs) override final;
    bool remove_filter(SFCFilterSpec& fs) override final;

    /* Capacities: */
    int evq_capacity() const { return m_evq_capacity; }
    int rxq_capacity() const { return m_rxq_capacity; }
    int txq_capacity() const { return m_txq_capacity; }
    /// Number of `SFCPktBuf`s allocated.
    int pbs_capacity() const { return m_rxq_capacity + m_txq_capacity; }

    /* Runtime API: */
    /// Poll up to `max_evs` events from evq.  2 < max_evs < 64.
    int poll(const int max_evs = 16);

    /* Callbacks: */
    /// Received packet callback.
    //  The data buffer will contain the complete packet, including all headers
    //  (Ethernet, IP, UDP/TCP, etc.).
    virtual void on_rx_cb(const char * data, int len, const Timestamp& swts, const Timestamp& hwts = {0,0}, bool hwts_clock_set = false, bool hwts_clock_in_sync = false) = 0;
    /// Received discarded packet callback.
    //  A received packet was discarded (and why).
    virtual void on_rx_discard_cb(const char * data, int len, unsigned discard_type, const Timestamp& swts, const Timestamp& hwts = {0,0}, bool hwts_clock_set = false, bool hwts_clock_in_sync = false) {}
    //  TODO FIXME: support sending packets!
    /// Sent packet callback.
    //  This is called after a packet has left the wire.
    virtual void on_tx_cb(int qid, const char * data, int len, const Timestamp& swts, const Timestamp& hwts = {0,0}, bool hwts_clock_set = false, bool hwts_clock_in_sync = false) {}
    /// Optional on_final_event_cb, called after all polled events have been
    //  processed.  This is useful to, for example, ensure that all available
    //  packets in the event queue have been processed before taking action.
    virtual void on_poll_complete_cb(int num_events) {}

private:
    ::ef_vi&            vi() { return m_vi; }
    ::ef_memreg&        mr() { return m_mr; }
    ::ef_pio&           pio() { return m_pio; }

    void                init();
    /// \internal Returns the `SFCPktBuf` at index `id`.
    SFCPktBuf*          get_pktbuf(int id);
    /// \internal Returns a reclaimed `SFCPktBuf`.
    SFCPktBuf*          get_pktbuf();
    /// \internal Reclaims `SFCPktBuf` at `pb`.
    bool                put_pktbuf(SFCPktBuf* pb_p);
    /// \internal Returns `SFCPktBuf`s to receive queue via
    //`ef_vi_receive_{init,push}` as needed if receive queue is low.
    void                maintain_rxq();
    /// \internal Adds up to `n` SFCPktBufs to rxq, returns number of SFCPktBufs
    //  actually pushed onto rxq.
    int                 fill_rxq(int n);

    SFCInterface&                     m_intf;
    int                               m_index_in_vi_set;
    struct ::ef_vi                    m_vi;
    struct ::ef_memreg                m_mr;
    struct ::ef_pio                   m_pio;

    char                              m_sfc_mac[7];
    unsigned                          m_sfc_mtu;
    int                               m_receive_prefix_len;
    int                               m_evq_capacity;
    int                               m_rxq_capacity;
    int                               m_txq_capacity;

    /// \internal Pointer to start of region containing `SFCPktBuf`s for this VI.
    SFCPktBuf*                        m_pktbufs_p;
    /// \internal Reclamation stack for freed/available `SFCPktBuf`s.
    boost::lockfree::stack<SFCPktBuf*> m_pktbufs_lifo;
    /// \internal Number of unused `SFCPktBuf`s available.
    std::atomic<int>                  m_pktbufs_lifo_size;
    /// \internal Next rxq maintenance will try to add at least this many
    //  `SFCPktBuf`s.from `m_pktbufs_lifo` to the receive queue.
    int                               m_rxq_cushion;
    /// \internal Is hardware timestamping enabled?
    bool                              m_hw_timestamping;

private:
    SFCVirtualInterface() = delete;
    SFCVirtualInterface(const SFCVirtualInterface&) = delete;
    SFCVirtualInterface& operator=(const SFCVirtualInterface&) = delete;
};

} }
