#pragma once

#include <string.h>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include <netinet/in.h>

#include <etherfabric/vi.h>
#include <etherfabric/pd.h>
#include <etherfabric/pio.h>
#include <etherfabric/memreg.h>

#include <i01_net/SFCInterface.hpp>

namespace std {
template <> struct hash< ::ef_filter_cookie>
{
    size_t operator()(const ::ef_filter_cookie& x) const
    { return std::hash<int>()(x.filter_id); }
};
template <> struct equal_to< ::ef_filter_cookie>
{
    bool operator()(const ::ef_filter_cookie& x, const ::ef_filter_cookie& y) const
    { return x.filter_id == y.filter_id; }
};
}

namespace i01 { namespace net {

class SFCFilterSpec {
public:
    enum class FilterIPProto : int {
        UDP = ::IPPROTO_UDP
      , TCP = ::IPPROTO_TCP
    };

    SFCFilterSpec()
    {
        ::ef_filter_spec_init(&m_fs, EF_FILTER_FLAG_NONE);
    }
    virtual ~SFCFilterSpec();

    ::ef_filter_spec& fs() { return m_fs; }
    ::ef_filter_cookie& fc() { return m_fc; }

    
protected:
    ::ef_filter_cookie m_fc;
    ::ef_filter_spec m_fs;
};

/// SFCVirtualInterface and SFCVISet inherit from this to provide a common
//  filtering API.
class SFCFilterable {
public:
    virtual bool apply_filter(SFCFilterSpec& fs) = 0;
    virtual bool remove_filter(SFCFilterSpec& fs) = 0;
};

class SFCFilterMulticastAll : public SFCFilterSpec {
public:
    SFCFilterMulticastAll()
        : SFCFilterSpec()
    {
        ::ef_filter_spec_set_multicast_all(&m_fs);
    }
};

class SFCFilterUnicastAll : public SFCFilterSpec {
public:
    SFCFilterUnicastAll()
        : SFCFilterSpec()
    {
        ::ef_filter_spec_set_unicast_all(&m_fs);
    }
};

class SFCFilterEthLocal : public SFCFilterSpec {
public:
    SFCFilterEthLocal(int vlan_id, const char mac[6])
        : SFCFilterSpec()
        , m_vlan_id(vlan_id)
        , m_mac(mac, mac + 6)
    {
        ::ef_filter_spec_set_eth_local(&m_fs, m_vlan_id, m_mac.c_str());
    }
protected:
    const int m_vlan_id;
    const std::string m_mac;
private:
    SFCFilterEthLocal() = delete;
};

class SFCFilterVlan : public SFCFilterSpec {
public:
    SFCFilterVlan(int vlan_id)
        : SFCFilterSpec()
        , m_vlan_id(vlan_id)
    {
        ::ef_filter_spec_set_vlan(&m_fs, m_vlan_id);
    }
protected:
    const int m_vlan_id;
private:
    SFCFilterVlan() = delete;
};

class SFCFilterIP4Full : public SFCFilterSpec {
public:
    SFCFilterIP4Full(
        const FilterIPProto protocol
      , const uint32_t localip_be32,  const int localport_be16
      , const uint32_t remoteip_be32, const int remoteport_be16)
        : SFCFilterSpec()
        , m_protocol(protocol)
        , m_localip_be32(localip_be32), m_localport_be16(localport_be16)
        , m_remoteip_be32(remoteip_be32), m_remoteport_be16(remoteport_be16)
    {
        ::ef_filter_spec_set_ip4_full(&m_fs, (int)m_protocol
            , m_localip_be32, m_localport_be16
            , m_remoteip_be32, m_remoteport_be16);
    }
protected:
    const FilterIPProto m_protocol;
    const int m_localip_be32;
    const int m_localport_be16;
    const int m_remoteip_be32;
    const int m_remoteport_be16;
private:
    SFCFilterIP4Full() = delete;
};

class SFCFilterIP4Local : public SFCFilterSpec {
public:
    SFCFilterIP4Local(
        const FilterIPProto protocol
      , const uint32_t localip_be32, const int localport_be16)
        : SFCFilterSpec()
        , m_protocol(protocol)
        , m_localip_be32(localip_be32), m_localport_be16(localport_be16)
    {
        ::ef_filter_spec_set_ip4_local(&m_fs, (int)m_protocol
            , m_localip_be32, m_localport_be16);
    }
protected:
    const FilterIPProto m_protocol;
    const int m_localip_be32;
    const int m_localport_be16;
private:
    SFCFilterIP4Local() = delete;
};

} }
