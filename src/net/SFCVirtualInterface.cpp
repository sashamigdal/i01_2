#include <stdexcept>
#include <algorithm>

#include <unistd.h>

#include <i01_core/macro.hpp>
#include <i01_core/Time.hpp>
#include <i01_net/SFCVirtualInterface.hpp>

namespace i01 { namespace net {

// TODO FIXME XXX: figure out what is causing this warning...
#if !defined(__INTEL_COMPILER) && __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
SFCVirtualInterface::SFCVirtualInterface(
          SFCInterface& intf
        , int evq_capacity_
        , int rxq_capacity_
        , int txq_capacity_
        , SFCVirtualInterface* evq_opt
        , bool hw_timestamping)
    : m_intf(intf)
    , m_vi()
    , m_mr()
    , m_pio()
    , m_sfc_mac()
    , m_sfc_mtu(0)
    , m_pktbufs_p(nullptr)
    , m_pktbufs_lifo_size(0)
    , m_rxq_cushion(8)
    , m_hw_timestamping(hw_timestamping)
{
    if (::ef_vi_alloc_from_pd(&m_vi
                             , m_intf.dh()
                             , &m_intf.pd()
                             , m_intf.dh()
                             , evq_capacity_
                             , rxq_capacity_
                             , txq_capacity_
                             , ( evq_opt == nullptr
                                 ? nullptr
                                 : &evq_opt->vi() )
                             , ( evq_opt == nullptr
                                 ? -1
                                 : evq_opt->m_intf.dh() )
                             , (m_hw_timestamping ? EF_VI_RX_TIMESTAMPS : EF_VI_FLAGS_DEFAULT)
                             ) < 0)
    {
        perror("ef_vi_alloc_from_pd");
        throw std::runtime_error("ef_vi_alloc_from_pd failed.");
    }

    init();
}

SFCVirtualInterface::SFCVirtualInterface(
          SFCVISet& vi_set
        , int index_in_vi_set
        , int evq_capacity_
        , int rxq_capacity_
        , int txq_capacity_
        , SFCVirtualInterface* evq_opt
        , bool hw_timestamping)
    : m_intf(vi_set.interface())
    , m_index_in_vi_set(index_in_vi_set)
    , m_vi()
    , m_mr()
    , m_pio()
    , m_sfc_mac()
    , m_sfc_mtu(0)
    , m_pktbufs_p(nullptr)
    , m_pktbufs_lifo_size(0)
    , m_rxq_cushion(8)
    , m_hw_timestamping(hw_timestamping)
{
    if (::ef_vi_alloc_from_set( &m_vi
                              , m_intf.dh()
                              , &vi_set.vi_set()
                              , vi_set.interface().dh()
                              , m_index_in_vi_set
                              , evq_capacity_
                              , rxq_capacity_
                              , txq_capacity_
                              , ( evq_opt == nullptr
                                  ? nullptr
                                  : &evq_opt->vi() )
                              , ( evq_opt == nullptr
                                  ? -1
                                  : evq_opt->m_intf.dh() )
                              , (m_hw_timestamping ? EF_VI_RX_TIMESTAMPS : EF_VI_FLAGS_DEFAULT)
                              ) < 0)
    {
        perror("ef_vi_alloc_from_set");
        throw std::runtime_error("ef_vi_alloc_from_set failed.");
    }

    init();
}
#if !defined(__INTEL_COMPILER) && __GNUC__
#pragma GCC diagnostic pop
#endif

SFCVirtualInterface::~SFCVirtualInterface()
{
    if (::ef_vi_flush(&m_vi, m_intf.dh())==0)
    {
        for (int i = 0; i < pbs_capacity(); ++i)
        { put_pktbuf(get_pktbuf(i)); }
        ::ef_memreg_free(&m_mr, m_intf.dh());
        if (m_pktbufs_p)
        {
            ::free(&m_pktbufs_p);
        }
    } else { throw std::runtime_error("ef_vi_flush failed."); }
    ::ef_vi_free(&m_vi, m_intf.dh());
}

void SFCVirtualInterface::init()
{
    if (::ef_vi_get_mac(&m_vi, m_intf.dh(), m_sfc_mac) < 0)
    {
        perror("ef_vi_get_mac");
        throw std::runtime_error("ef_vi_get_mac failed.");
    }
    m_sfc_mtu = ::ef_vi_mtu(&m_vi, m_intf.dh());
    m_receive_prefix_len = ::ef_vi_receive_prefix_len(&m_vi);
    m_evq_capacity = ::ef_eventq_capacity(&m_vi);
    m_rxq_capacity = ::ef_vi_receive_capacity(&m_vi);
    m_txq_capacity = ::ef_vi_transmit_capacity(&m_vi);

    /* TODO FIXME: use get_hugepage_region with GHR_FALLBACK
     *             since these pages will be locked in physical memory anyhow,
     *             TLB misses shouldn't be an issue (with hugepages) */
    if (::posix_memalign( (void **)&m_pktbufs_p
                        , SFCPktBuf::alloc_size()
                        , pbs_capacity() * SFCPktBuf::alloc_size() ) != 0)
    {
        perror("posix_memalign");
        throw std::runtime_error("posix_memalign failed.");
    }

    if (::ef_memreg_alloc( &m_mr
                         , m_intf.dh()
                         , &m_intf.pd()
                         , m_intf.dh()
                         , m_pktbufs_p
                         , pbs_capacity() * static_cast<int>(SFCPktBuf::alloc_size())
                         ) < 0)
    {
        throw std::runtime_error("ef_memreg_alloc failed.");
    }

    m_pktbufs_lifo.reserve(pbs_capacity());
    for (int i = 0; i < pbs_capacity(); ++i)
    {
        /* TODO: reference counting, possibly use shared_ptr. */
        SFCPktBuf * pb_p = get_pktbuf(i);
        pb_p->m_pod.m_vi_owner = this;
        pb_p->m_pod.m_addr = ::ef_memreg_dma_addr( &m_mr, i * static_cast<int>(SFCPktBuf::alloc_size()) );
        pb_p->m_pod.m_addr += SFCPktBuf::prefix_size();
        pb_p->m_pod.m_pool_id = i;
        pb_p->m_pod.m_data_size = 0; /* XXX: redundant with put_pktbuf() */
        put_pktbuf(pb_p);
    }

    maintain_rxq();
}

bool SFCVirtualInterface::apply_filter(SFCFilterSpec& fs)
{
    return (::ef_vi_filter_add(&m_vi, m_intf.dh(), &fs.fs(), &fs.fc()) >= 0);
}

bool SFCVirtualInterface::remove_filter(SFCFilterSpec& fs)
{
    return (::ef_vi_filter_del(&m_vi, m_intf.dh(), &fs.fc()) >= 0);
}

SFCPktBuf* SFCVirtualInterface::get_pktbuf(int id)
{
    /* TODO: reference counting, possibly use shared_ptr. */
    if (m_pktbufs_p && id >= 0 && id < pbs_capacity())
    {
        return reinterpret_cast<SFCPktBuf*>(
            reinterpret_cast<char*>(m_pktbufs_p) + id * SFCPktBuf::alloc_size() );
    } else { return nullptr; }
}

SFCPktBuf* SFCVirtualInterface::get_pktbuf()
{
    SFCPktBuf* ret = nullptr;
    /* TODO: reference counting, possibly use shared_ptr. */
    if (m_pktbufs_lifo.pop(ret) && ret)
    {
        --m_pktbufs_lifo_size;
        ret->m_pod.m_data_size = 0;
    }
    return ret;
}

bool SFCVirtualInterface::put_pktbuf(SFCPktBuf* pb_p)
{
    /* TODO: reference counting, possibly use shared_ptr. */
    if ( ((char*)pb_p >= (char*)m_pktbufs_p)
      && ((char*)pb_p < ((char*)m_pktbufs_p + pbs_capacity() * SFCPktBuf::alloc_size()))
      && ((std::uintptr_t)pb_p % SFCPktBuf::alloc_size() == 0)
       )
    {
        if (m_pktbufs_lifo.bounded_push(pb_p))
        {
            ++m_pktbufs_lifo_size;
            return true;
        }
    }
    return false;
}

void SFCVirtualInterface::maintain_rxq()
{
    /* TODO: add statistics collection */
    int rxfill = ::ef_vi_receive_fill_level(&m_vi);
    int rxpbneeded = rxq_capacity() - rxfill;

    if ( (m_pktbufs_lifo_size < m_rxq_cushion)
       ||(rxpbneeded < m_rxq_cushion)
         /* Receive queue size is within m_rxq_cushion of capacity. */
        )
    {
        m_rxq_cushion = 8;
        return; /* depletion not severe enough to warrant pushing */
    } else if (rxpbneeded < std::max(16, rxq_capacity() / 5)) {
        m_rxq_cushion = 16; /* <20% depleted */
    } else if (rxpbneeded < std::max(32, rxq_capacity() / 4)) {
        m_rxq_cushion = 32; /* 20% < depletion < 25% */
    } else if (rxpbneeded < std::max(64, rxq_capacity() / 3)) {
        m_rxq_cushion = 64; /* 25% < depletion < 33% */
    } else if (rxpbneeded < std::max(128, rxq_capacity() / 2)) {
        m_rxq_cushion = 128; /* 33% < depletion < 50% */
    } else { /* serious depletion > 50% */
        m_rxq_cushion = std::max(128, rxq_capacity() / 10);
    }

    fill_rxq(m_rxq_cushion);
}

int SFCVirtualInterface::fill_rxq(int n)
{
    /* Remember that m_rxq_cushion is always bounded from above by the number of
     * SFCPktBufs available due to get_pktbuf()'s behavior. */
    int ret = 0;
    for (int i = 0; i < m_rxq_cushion; ++i)
    {
        SFCPktBuf * pb_p = get_pktbuf();
        if ( pb_p
          && ef_vi_receive_init(&m_vi
              , pb_p->addr() + SFCPktBuf::prefix_size()
              , pb_p->pool_id()) == 0 /* will be -EAGAIN if rxq full */
           )
        {
            ++ret;
        } else { break; }
    }
    if (ret > 0)
    { ef_vi_receive_push(&m_vi); }
    return ret;
}

int SFCVirtualInterface::poll(const int max_evs)
{
    ef_event evs[max_evs];
    int n_ev = -1;
    n_ev = ef_eventq_poll(&m_vi, evs, static_cast<int>(sizeof(evs) / sizeof(evs[0])));
    if (n_ev > 0)
    {
        Timestamp swts( Timestamp::now() );
        for (int i = 0; i < n_ev; ++i)
        {
            switch(EF_EVENT_TYPE(evs[i])) {
              case EF_EVENT_TYPE_RX: {
                bool sop = EF_EVENT_RX_SOP(evs[i]);
                bool cont = EF_EVENT_RX_CONT(evs[i]);
                bool I01_UNUSED jumbo = (sop == 0 || cont != 0);
                SFCPktBuf * pb_p = get_pktbuf(EF_EVENT_RX_RQ_ID(evs[i]));
                if (pb_p && pb_p->vi_owner())
                {
                    Timestamp hwts;
                    int rxtsn = -ENODATA;
                    unsigned timesync_flags = 0;
                    if (m_hw_timestamping)
                        rxtsn = ::ef_vi_receive_get_timestamp_with_sync_flags(
                              &m_vi
                            , pb_p->data()
                            , static_cast<i01::core::POSIXTimeSpec*>(&hwts)
                            , &timesync_flags);
                    bool hwts_clock_set = timesync_flags | EF_VI_SYNC_FLAG_CLOCK_SET;
                    bool hwts_clock_in_sync = timesync_flags | EF_VI_SYNC_FLAG_CLOCK_IN_SYNC;
                    char * buf = pb_p->data()
                               + pb_p->vi_owner()->m_receive_prefix_len;
                    int len = EF_EVENT_RX_BYTES(evs[i])
                            - pb_p->vi_owner()->m_receive_prefix_len;
                    PREFETCH_RANGE(buf, len);
                    // TODO FIXME: add jumbo frame support.
                    if (UNLIKELY(rxtsn < 0)) 
                        hwts = {0,0};
                    on_rx_cb(buf, len, swts, hwts, hwts_clock_set, hwts_clock_in_sync);
                    pb_p->vi_owner()->put_pktbuf(pb_p);
                } else { /* TODO FIXME: id out of bounds error... */ }
              } break;
              case EF_EVENT_TYPE_TX: {
                ef_request_id ids[EF_VI_TRANSMIT_BATCH];
                int n = ef_vi_transmit_unbundle(&m_vi, &evs[i], ids);
                int qid = EF_EVENT_TX_Q_ID(evs[i]);
                for (int j = 0; j < n; ++j)
                {
                    SFCPktBuf * pb_p = get_pktbuf(ids[j]);
                    char * buf = pb_p->data()
                               + pb_p->vi_owner()->m_receive_prefix_len;
                    int len = EF_EVENT_RX_BYTES(evs[i])
                            - pb_p->vi_owner()->m_receive_prefix_len;
                    on_tx_cb(qid, buf, len, swts);
                    pb_p->vi_owner()->put_pktbuf(pb_p);
                }
              } break;
              case EF_EVENT_TYPE_RX_DISCARD: {
                bool sop = EF_EVENT_RX_DISCARD_SOP(evs[i]);
                bool cont = EF_EVENT_RX_DISCARD_CONT(evs[i]);
                bool I01_UNUSED jumbo = (sop == 0 || cont != 0);
                unsigned subtype = EF_EVENT_RX_DISCARD_TYPE(evs[i]);
                SFCPktBuf * pb_p = get_pktbuf(EF_EVENT_RX_RQ_ID(evs[i]));
                if (pb_p && pb_p->vi_owner())
                {
                    Timestamp hwts;
                    int rxtsn = -ENODATA;
                    unsigned timesync_flags = 0;
                    if (m_hw_timestamping)
                        rxtsn = ::ef_vi_receive_get_timestamp_with_sync_flags(
                              &m_vi
                            , pb_p->data()
                            , static_cast<i01::core::POSIXTimeSpec*>(&hwts)
                            , &timesync_flags);
                    bool hwts_clock_set = timesync_flags | EF_VI_SYNC_FLAG_CLOCK_SET;
                    bool hwts_clock_in_sync = timesync_flags | EF_VI_SYNC_FLAG_CLOCK_IN_SYNC;
                    char * buf = pb_p->data()
                               + pb_p->vi_owner()->m_receive_prefix_len;
                    int len = EF_EVENT_RX_DISCARD_BYTES(evs[i])
                            - pb_p->vi_owner()->m_receive_prefix_len;
                    // TODO FIXME: add jumbo frame support.
                    if (UNLIKELY(rxtsn < 0)) 
                        hwts = {0,0};
                    /* TODO FIXME: log? config-based allow crc errored pkts? etc. */
                    on_rx_discard_cb(buf, len, subtype, swts, hwts, hwts_clock_set, hwts_clock_in_sync);
                    pb_p->vi_owner()->put_pktbuf(pb_p);
                } else { /* TODO FIXME: id out of bounds error... */ }
              } break;
              case EF_EVENT_TYPE_TX_ERROR: {
                unsigned subtype I01_UNUSED = EF_EVENT_TX_ERROR_TYPE(evs[i]);
              } break;
              case EF_EVENT_TYPE_RX_NO_DESC_TRUNC: {
                unsigned qid I01_UNUSED = EF_EVENT_RX_NO_DESC_TRUNC_Q_ID(evs[i]);
                /* TODO FIXME: log dropped packet and refill rxq */
                maintain_rxq();
              } break;
              case EF_EVENT_TYPE_SW: {
                unsigned data I01_UNUSED = EF_EVENT_SW_DATA_MASK & EF_EVENT_SW_DATA(evs[i]);
                /* TODO FIXME: add timers? other events? */
              } break;
              case EF_EVENT_TYPE_OFLOW: {
                /* TODO FIXME: log? empty event queue more aggressively? */
              } break;
              case EF_EVENT_TYPE_TX_WITH_TIMESTAMP: {
                ef_request_id ids[EF_VI_TRANSMIT_BATCH];
                int n = ef_vi_transmit_unbundle(&m_vi, &evs[i], ids);
                int qid = EF_EVENT_TX_Q_ID(evs[i]);
                Timestamp hwts( EF_EVENT_TX_WITH_TIMESTAMP_SEC(evs[i])
                              , EF_EVENT_TX_WITH_TIMESTAMP_NSEC(evs[i]));
                unsigned timesync_flags = EF_EVENT_TX_WITH_TIMESTAMP_SYNC_FLAGS(evs[i]);
                bool hwts_clock_set = timesync_flags | EF_VI_SYNC_FLAG_CLOCK_SET;
                bool hwts_clock_in_sync = timesync_flags | EF_VI_SYNC_FLAG_CLOCK_IN_SYNC;
                for (int j = 0; j < n; ++j)
                {
                    SFCPktBuf * pb_p = get_pktbuf(ids[j]);
                    char * buf = pb_p->data()
                               + pb_p->vi_owner()->m_receive_prefix_len;
                    int len = EF_EVENT_RX_BYTES(evs[i])
                            - pb_p->vi_owner()->m_receive_prefix_len;
                    on_tx_cb(qid, buf, len, swts, hwts, hwts_clock_set, hwts_clock_in_sync);
                    pb_p->vi_owner()->put_pktbuf(pb_p);
                }
              } break;
              case EF_EVENT_TYPE_RX_PACKED_STREAM: {
                  // TODO
              } break;
#if defined(EF_EVENT_TYPE_RX_PACKED_STREAM_DISCARD)
              case EF_EVENT_TYPE_RX_PACKED_STREAM_DISCARD: {
#else
              case (EF_EVENT_TYPE_RX_PACKED_STREAM + 1): {
#endif
                  // TODO: possibly get rid of this?
              } break;
              default: { 
                /* TODO FIXME: log error; unknown event type */
              } break;
            }
        }
        on_poll_complete_cb(n_ev);
    }
    return n_ev;
}

} }
