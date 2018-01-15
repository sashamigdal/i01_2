#pragma once

#include <string>
#include <typeinfo>

#include <i01_core/MIC.hpp>
#include <i01_core/Time.hpp>

#include <i01_md/DecoderEvents.hpp>
#include <i01_md/NASDAQ/MoldUDP64/Types.hpp>
#include <i01_md/RawListener.hpp>


namespace i01 { namespace MD {

class SeqnumListener : public RawListener<SeqnumListener> {
public:
    using Timestamp = core::Timestamp;

public:
    SeqnumListener(const core::MIC& mic, const std::string& name);
    virtual ~SeqnumListener() = default;

    // this enable_if stuff is because I couldn't keep this template
    // from being used instead of the overrides any other way...
    template<typename MT, typename...Args>
    typename std::enable_if<!std::is_same<MT, StartOfPktMsg>::value
    && !std::is_same<MT, EndOfPktMsg>::value
    && !std::is_same<MT, GapMsg>::value
    && !std::is_same<MT, TimeoutMsg>::value >::type
    on_raw_msg(const Timestamp&, const MT& msg, Args...) {
        // std::cerr << "SeqnumListener " << typeid(msg).name() << std::endl;
    }


    void on_raw_msg(const Timestamp& ts, const StartOfPktMsg&, std::uint64_t seqnum, std::uint32_t index= 0) {
        //     std::cerr << ts << "," << mic() << "," << name() << "," << index << "," << seqnum << std::endl;
        m_seqnum = seqnum;
    }

    void on_raw_msg(const Timestamp& ts, const EndOfPktMsg&, std::uint64_t seqnum, std::uint32_t index = 0) {
        for (;m_seqnum <= seqnum ; m_seqnum++) {
            do_on_seqnum_event(ts, m_seqnum, index);
            // std::cerr << ts << "," << mic() << "," << name() << "," << index << "," << m_seqnum << std::endl;
        }
    }

    void on_raw_msg(const Timestamp &ts, const GapMsg &, std::uint32_t addr, std::uint16_t port, std::uint8_t unit, std::uint64_t expected, std::uint64_t received, const Timestamp &last_ts) {
        // std::cerr << "GAP," << ts << "," << mic() << "," << name() << "," << addr << "," << port << "," << (int) unit << "," << expected << "," << received << "," << last_ts << std::endl;
        do_on_gap_event(ts, addr, port, unit, expected, received, last_ts);
    }

    void on_raw_msg(const Timestamp &ts, const GapMsg &, const NASDAQ::MoldUDP64::Types::Session &session, std::uint64_t expected, std::uint64_t received, const Timestamp &last_ts) {
        do_on_gap_event(ts, 0, 0, 0, expected, received, last_ts);
    }

    void on_raw_msg(const Timestamp& ts, const TimeoutMsg&, bool started, const std::string& name_, int unit_index, const Timestamp& last_ts) {
        do_on_timeout_event(ts, started, name_, static_cast<std::uint8_t>(unit_index), last_ts);
        // std::cerr << "TIMEOUT," << ts << "," << mic() << "," << name() << "," << (int) started << "," << name_ << "," << unit_index << "," << last_ts << std::endl;
    }

    core::MIC mic() const { return m_mic; }
    std::string name() const { return m_name; }

private:
    virtual void do_on_seqnum_event(const Timestamp& ts, std::uint64_t seqnum, std::uint32_t index) = 0;
    virtual void do_on_gap_event(const Timestamp& ts, std::uint32_t addr, std::uint16_t port,
                                 std::uint8_t unit, std::uint64_t expected, std::uint64_t received,
                                 const Timestamp& last_ts) = 0;
    virtual void do_on_timeout_event(const Timestamp& ts, bool started, const std::string& name, std::uint8_t unit, const Timestamp& last_ts) = 0;

private:
    core::MIC m_mic;
    std::string m_name;
    std::uint64_t m_seqnum;
};


}}
