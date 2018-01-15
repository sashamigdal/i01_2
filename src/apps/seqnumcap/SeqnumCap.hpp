#pragma once

#include <iosfwd>

#include <i01_core/AsyncMappedFile.hpp>
#include <i01_core/Lock.hpp>
#include <i01_core/Time.hpp>

#include <i01_md/SeqnumListener.hpp>


namespace SeqnumCap {


enum class EventType : std::uint8_t {
    UNKNOWN = 0,
    SEQNUM = 1,
    GAP = 2,
    TIMEOUT = 3,
    };
std::ostream& operator<<(std::ostream& os, const EventType& e);

struct FileHeader {
    std::uint32_t magic_number;
    std::uint32_t version;
    std::uint8_t mic;
    std::array<std::uint8_t,15> feed_name;
} __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const FileHeader& h);

struct EventHeader {
    EventType type;
    std::uint64_t time_s;
    std::uint64_t time_ns;
    std::uint32_t length;
} __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const EventHeader& h);

struct Event {
    EventHeader header;
    std::uint8_t data[];
} __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const Event& h);

struct SeqnumEvent {
    EventHeader header;
    std::uint32_t unit;
    std::uint64_t seqnum;
} __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const SeqnumEvent& h);

struct GapEvent {
    EventHeader header;
    std::uint32_t addr;
    std::uint16_t port;
    std::uint8_t unit;
    std::uint64_t expected;
    std::uint64_t received;
    std::uint64_t last_time_s;
    std::uint64_t last_time_ns;
} __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const GapEvent& h);

struct TimeoutEvent {
    EventHeader header;
    std::uint8_t unit;
    std::uint8_t started;
    std::uint64_t last_time_s;
    std::uint64_t last_time_ns;
} __attribute__((packed));
std::ostream& operator<<(std::ostream& os, const TimeoutEvent& h);

class SeqnumWriter : public i01::MD::SeqnumListener {
public:
    static const std::uint32_t MAGIC_NUMBER = 0xEEAD0BF0;
    static const std::uint32_t VERSION_NUMBER = 0x01;

    using Mutex = i01::core::SpinMutex;
    using MIC = i01::core::MIC;
public:
    SeqnumWriter(const MIC& mic, const std::string& name, const std::string& path);
    virtual ~SeqnumWriter() = default;

private:
    virtual void do_on_seqnum_event(const Timestamp& ts, std::uint64_t seqnum, std::uint32_t index) override final;
    virtual void do_on_gap_event(const Timestamp& ts, std::uint32_t addr, std::uint16_t port,
                                 std::uint8_t unit, std::uint64_t expected, std::uint64_t received,
                                 const Timestamp& last_ts) override final;
    virtual void do_on_timeout_event(const Timestamp& ts, bool started, const std::string& name, std::uint8_t unit, const Timestamp& last_ts) override final;

    int write_header(const MIC& mic, const std::string& fn);
    FileHeader read_header();

    std::string conforming_name(const std::string& str);
private:
    Mutex m_mutex;
    i01::core::AsyncMappedFile m_file;
};

class SeqnumReader {
public:
    class const_iterator {
    public:
        const_iterator(const char * buf, std::uint64_t len);
        bool operator!=(const const_iterator& o) const;
        const Event * operator*() const;
        const const_iterator& operator++();
    private:
        const char * m_buf;
        const char * m_end_of_buf;
        std::uint64_t m_len;
    };

public:
    SeqnumReader(const std::string& path);

    FileHeader header() const { return m_header; }

    const_iterator begin() const;
    const_iterator end() const;

private:
    i01::core::AsyncMappedFile m_file;
    const char * m_buf;
    std::uint64_t m_len;
    FileHeader m_header;
};

}
