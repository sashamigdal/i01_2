#pragma once

#include <vector>


template<typename HeaderType>
struct HeaderWrapper {
    void set_length(std::uint16_t len) {}

    HeaderType m_header;
};

template<typename HeaderType>
HeaderWrapper<HeaderType> make_header_wrapper(const HeaderType &ht) {
    return HeaderWrapper<HeaderType>(ht);
}

struct Pkt {
    std::vector<std::uint8_t> m_buf;
};


template<typename HeaderType, typename MsgType>
Pkt make(HeaderWrapper<HeaderType> header_wrapper, const std::vector<MsgType> & msg)
{
    Pkt pkt;
    auto msg_size = sizeof(MsgType)*msg.size() + sizeof(HeaderType);
    std::uint16_t len = static_cast<std::uint16_t>(msg_size);
    pkt.m_buf = std::vector<std::uint8_t>(len);
    // HeaderWrapper<HeaderType> header_wrapper;
    header_wrapper.set_length(len);
    // HeaderType ph{(PktSize)len, Types::DeliveryFlag::ORIGINAL, 1, sn, 0,0};

    HeaderType *phb = reinterpret_cast<HeaderType *>(pkt.m_buf.data());
    *phb = header_wrapper.m_header;
    for (auto i = 0U; i < msg.size(); i++) {
        MsgType *msg_p = reinterpret_cast<MsgType *>(pkt.m_buf.data() + sizeof(HeaderType) + i*sizeof(MsgType));
        *msg_p = msg[i];
    }

    return pkt;
}

template<typename HeaderType, typename MsgType>
Pkt make(HeaderWrapper<HeaderType> header_wrapper, const MsgType & msg)
{
    const std::vector<MsgType> vec = {msg};
    return make(header_wrapper, vec);
}

template<typename HeaderType, typename MsgType, typename AppendType>
Pkt make(HeaderWrapper<HeaderType> header_wrapper, const MsgType & msg, const AppendType &app)
{
    Pkt pkt;
    auto msg_size = sizeof(MsgType) + sizeof(HeaderType) + sizeof(AppendType);
    std::uint16_t len = static_cast<std::uint16_t>(msg_size);
    pkt.m_buf = std::vector<std::uint8_t>(len);
    header_wrapper.set_length(len);

    HeaderType *phb = reinterpret_cast<HeaderType *>(pkt.m_buf.data());
    *phb = header_wrapper.m_header;
    MsgType *msg_p = reinterpret_cast<MsgType *>(pkt.m_buf.data() + sizeof(HeaderType));
    *msg_p = msg;

    auto app_p = reinterpret_cast<AppendType *>(pkt.m_buf.data() + sizeof(HeaderType) + sizeof(MsgType));
    *app_p = app;

    return pkt;
}
