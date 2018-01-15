#pragma once

#include <vector>

namespace i01 { namespace OE {

class BlotterReaderListener;

class BlotterReader {
public:
    enum class Errors {
        NONE = 0,
            NO_HEADER_SIZE,
            NO_MSG_SIZE,
            };
    using ListenerContainer = std::vector<BlotterReaderListener*>;

public:
    template<typename...Listeners>
    void register_listeners(Listeners&&...listeners) {
        auto l = ListenerContainer{std::forward<Listeners>(listeners)...};

        m_replay_listeners.insert(m_replay_listeners.end(), l.begin(), l.end());
    }

    // void register_listener(BlotterReaderListener *brl);

    std::int32_t decode(const char *buf, std::uint64_t len);

    void rewind();
    bool at_end();
    void next();
    void replay();

protected:
    template<typename MemberFnPtr, typename...Args>
    void notify(MemberFnPtr mfp, Args&&...args) {
        if (m_replay_listeners.size()) {
            for (auto& l : m_replay_listeners) {
                (l->*mfp)(std::forward<Args>(args)...);
            }
        }
    }

private:
    virtual void do_rewind() = 0;
    virtual void do_next() = 0;
    virtual bool do_at_end() = 0;

protected:
    ListenerContainer m_replay_listeners;

};

}}
