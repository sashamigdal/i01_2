#pragma once

#include <list>
#include <map>
#include <string>

#include <i01_core/MIC.hpp>
#include <i01_core/TimerListener.hpp>

#include <i01_net/EventPoller.hpp>
#include <i01_net/UDPSocket.hpp>


namespace i01 { namespace MD {

class MDEventPoller {
public:
    struct EventPollerConfigEntry {
        core::MIC mic;
        std::string feed_name;
        std::string unit_name;
    };

    using EventPollerConfig = std::map<std::string, std::list<EventPollerConfigEntry> >;
    using MulticastPollerContainer = std::map<std::string, net::EpollEventPoller *>;

public:
    MDEventPoller();
    ~MDEventPoller();

    void init(const core::Config::storage_type& cfg);

    void shutdown();

    net::EpollEventPoller* get_poller(const std::string& name) const;
    net::EpollEventPoller* get_sys_poller() const { return m_sys_poller; }

    template<typename DecoderContainer, typename FeedStateContainer>
    void init_feed_event_pollers(DecoderContainer& dec,
                                 FeedStateContainer& fs);

    MulticastPollerContainer::const_iterator begin() const { return m_pollers.begin(); }
    MulticastPollerContainer::const_iterator end() const { return m_pollers.end(); }

    EventPollerConfig get_config() const { return m_epc; }

    friend std::ostream& operator<<(std::ostream&, const EventPollerConfig&);

private:
    EventPollerConfig get_event_poller_config(const core::Config::storage_type&) const;

    void create_event_pollers_by_name(const std::vector<std::string>&);

    template<typename FS>
    void add_feed_to_event_pollers(const EventPollerConfig& epc, net::SocketListener* sl, FS& feed_state);


private:
    EventPollerConfig m_epc;
    net::EpollEventPoller* m_sys_poller;
    core::Timestamp m_timer_interval;
    core::Timestamp m_timer_start;
    MulticastPollerContainer m_pollers;
};

template<typename DecoderContainer, typename FeedStateContainer>
void MDEventPoller::init_feed_event_pollers(DecoderContainer& decoders, FeedStateContainer& fs)
{
    // there may be multiple feeds per decoder... e.g., OB & IMB for XNYS PDP
    for (const auto& f : fs) {
        const auto& mic = f.first.first;
        const auto& feed_name = f.first.second;
        if (nullptr == f.second) {
            std::cerr << "MDEventPoller: init_feed_event_pollers: do not have FeedState cfg for " << f.first.first << ":" << f.first.second << std::endl;
            continue;
        }

        auto it = decoders.find(mic);
        if (it == decoders.end()) {
            std::cerr << "MDEventPoller: init_feed_event_pollers: could not find decoder for feed state " << mic << ":" << feed_name << std::endl;
            continue;
        }

        add_feed_to_event_pollers(m_epc, it->second, *f.second);

        // add timers
        if (m_sys_poller) {
            for (const auto& u : *f.second) {
                m_sys_poller->add_timer(*static_cast<core::TimerListener *>(it->second), u.state_ptr.get(), m_timer_start, m_timer_interval);
            }
        } else {
            std::cerr << "MDEventPoller: sys poller does not exist, can not add timers for " << mic << " " << feed_name << std::endl;
        }
    }
}

template<typename FS>
void MDEventPoller::add_feed_to_event_pollers(const EventPollerConfig& epc, net::SocketListener* sl, FS& feed_state)
{
    for (const auto& e: epc) {
        // there should be an event poller with this name already
        if (m_pollers.find(e.first) == m_pollers.end()) {
            continue;
        }

        // now create and add all relevant sockets that use this poller that come from this feed_state
        for (const auto& entry : e.second) {
            if (entry.mic == feed_state.mic() && entry.feed_name == feed_state.name()) {
                auto unit_info = feed_state.get_unit(entry.unit_name);
                if (unit_info) {
                    // for each address in the unit, create the socket
                    for (const auto& a: unit_info->addresses) {
                        auto udpsocket = net::UDPSocket::create_multicast_socket(unit_info->local_interface, a);
                        // transfer ownership of the fd to fd.. so
                        // udpsocket will not close it when it goes
                        // out of scope.
                        auto fd = udpsocket.fd().transfer_ownership();
                        if (fd < 0) {
                            std::cerr << "MDEventPoller: add_feed_to_event_pollers: " << feed_state.mic() << " " << feed_state.name() << ": could not create UDP socket for local ip " << unit_info->local_interface << " and group " << a << std::endl;
                            continue;
                        }

                        auto edptr = net::EventUserData{unit_info->state_ptr.get()};

                        // if (m_verbose_level) {
                        // std::cerr << "MDEventPoller adding feed " << feed_state.name() << ":" << sl << " to poller " << e.first << std::endl;
                        // }
                        // add to the event poller
                        m_pollers[e.first]->add_socket(*sl, edptr, fd);
                    }
                }
            }
        }
    }
}


}}
