#include <arpa/inet.h>

#include <i01_core/util.hpp>
#include <i01_net/IPAddress.hpp>

#include "SeqnumCap.hpp"
namespace SeqnumCap {

SeqnumWriter::SeqnumWriter(const MIC& mic_, const std::string& fname, const std::string& path)
    : SeqnumListener(mic_, fname),
      m_file(path)
{
    if (0 == m_file.size()) {
        if (m_file.capacity() < sizeof(FileHeader)) {
            throw std::runtime_error("SeqnumWriter: no space in file to write header");
        }
        if (write_header(mic(), conforming_name(name())) < 0) {
            throw std::runtime_error("SeqnumWriter: could not write header to file");
        }
    } else {
        if (m_file.size() < sizeof(FileHeader)) {
            throw std::runtime_error("SeqnumWriter: existing file too small to contain header");
        }

        auto fh = read_header();
        auto fn = std::string{};
        for (auto i = 0U; i < sizeof(fh.feed_name); i++) {
            if (fh.feed_name[i]) {
                fn.push_back(fh.feed_name[i]);
            }
        }
        if (fh.mic != mic().index() || conforming_name(name()) != fn) {
            throw std::runtime_error("SeqnumWriter: existing file does not match MIC or feed name");
        }
    }
}

std::string SeqnumWriter::conforming_name(const std::string &str)
{
    std::string res;
    for (auto i = 0U; i < std::min(str.size(), sizeof(FileHeader::feed_name)); i++) {
        res.push_back(str[i]);
    }
    return res;
}

auto SeqnumWriter::read_header() -> FileHeader
{
    const char * buf;
    std::uint64_t len;

    if (m_file.as_readonly_buffer(buf, len) < 0) {
        return FileHeader{};
    }

    auto p = reinterpret_cast<const FileHeader *>(buf);
    return *p;
}

int SeqnumWriter::write_header(const MIC &amic, const std::string &fn)
{
    FileHeader fh;
    fh.magic_number = MAGIC_NUMBER;
    fh.version = VERSION_NUMBER;
    fh.mic = static_cast<std::uint8_t>(amic.index());
    fh.feed_name.fill(' ');
    auto cn = conforming_name(name());
    for (auto i = 0U; i < sizeof(fh.feed_name); i++) {
        if (i < cn.size()) {
            fh.feed_name[i] = cn[i];
        }
    }

    return m_file.write((const char *)&fh, sizeof(FileHeader));
}

void SeqnumWriter::do_on_seqnum_event(const Timestamp& ts, std::uint64_t seqnum, std::uint32_t index)
{
    Mutex::scoped_lock lock(m_mutex);
    SeqnumEvent evt{};

    evt.header.type = EventType::SEQNUM;
    evt.header.time_s = ts.tv_sec;
    evt.header.time_ns = ts.tv_nsec;
    evt.header.length = sizeof(SeqnumEvent);
    evt.unit = index;
    evt.seqnum = seqnum;

    if (m_file.write((const char*)&evt, evt.header.length) < 0) {
        std::cerr << "SeqnumWriter: could not write seqnum event to file " << ts << " " << mic() << " " << name() << " " << seqnum << " " << index << std::endl;
    }
 }

void SeqnumWriter::do_on_gap_event(const Timestamp& ts, std::uint32_t addr, std::uint16_t port,
                                   std::uint8_t unit, std::uint64_t expected, std::uint64_t received,
                                   const Timestamp& last_ts)
{
    Mutex::scoped_lock lock(m_mutex);
    GapEvent evt{};

    evt.header.type = EventType::GAP;
    evt.header.time_s = ts.tv_sec;
    evt.header.time_ns = ts.tv_nsec;
    evt.header.length = sizeof(GapEvent);
    evt.addr = addr;
    evt.port = port;
    evt.unit = unit;
    evt.expected = expected;
    evt.received = received;
    evt.last_time_s = last_ts.tv_sec;
    evt.last_time_ns = last_ts.tv_nsec;

    if (m_file.write((const char *)&evt, sizeof(evt)) < 0) {
        std::cerr << "SeqnumWriter: could not write gap event to file " << ts << " " << mic() << " "
                  << name() << " " << addr << " " << port << " " << (int) unit << " " << expected << " "
                  << received << " " << last_ts;
    }
}

void SeqnumWriter::do_on_timeout_event(const Timestamp& ts, bool started, const std::string& aname,
                                       std::uint8_t unit, const Timestamp& last_ts)
{
    Mutex::scoped_lock lock(m_mutex);
    TimeoutEvent evt{};

    evt.header.type = EventType::TIMEOUT;
    evt.header.time_s = ts.tv_sec;
    evt.header.time_ns = ts.tv_nsec;
    evt.header.length = sizeof(TimeoutEvent);
    evt.unit = unit;
    evt.started = started;
    evt.last_time_s = last_ts.tv_sec;
    evt.last_time_ns = last_ts.tv_nsec;

    if (m_file.write((const char*) &evt, sizeof(evt)) < 0) {
        std::cerr << "SeqnumWriter: could not write timeout event to file " << ts << " " << mic() << " "
                  << name() << " " << (int) started << " " << aname << " " << (int) unit << " "
                  << last_ts;
    }
}

SeqnumReader::SeqnumReader(const std::string& path) :
    m_file(path, true),
    m_buf{nullptr},
    m_len{0},
    m_header{}
{
    if (m_file.as_readonly_buffer(m_buf, m_len) < 0) {
        throw std::runtime_error("SeqnumReader: could not read file " + path);
    }

    if (m_len >= sizeof(FileHeader)) {
        m_header = *reinterpret_cast<const FileHeader*>(m_buf);
        m_buf += sizeof(FileHeader);
        m_len -= sizeof(FileHeader);
    } else {
        // this is not a valid file
        m_buf = nullptr;
        m_len = 0;
    }
}

auto SeqnumReader::begin() const -> const_iterator
{
    return const_iterator(m_buf, m_len);
}

auto SeqnumReader::end() const -> const_iterator
{
    if (m_len > 0) {
        return const_iterator(m_buf + m_len, 0);
    } else {
        return const_iterator(nullptr, 0);
    }
}

SeqnumReader::const_iterator::const_iterator(const char *buf, std::uint64_t len) :
    m_buf(buf),
    m_end_of_buf(nullptr),
    m_len(len)
{
    if (m_len) {
        m_end_of_buf = m_buf + m_len;
    }
}

bool SeqnumReader::const_iterator::operator!=(const const_iterator& o) const
{
    return m_buf != o.m_buf;
}

const Event * SeqnumReader::const_iterator::operator*() const
{
    auto ptr = reinterpret_cast<const Event *>(m_buf);
    return ptr;
}

auto SeqnumReader::const_iterator::operator++() -> const const_iterator&
{
    auto ptr = reinterpret_cast<const Event*>(m_buf);
    if (ptr->header.length + m_buf <= m_end_of_buf) {
        m_buf += ptr->header.length;
    } else {
        m_buf = m_end_of_buf;
    }
    return *this;
}

std::ostream& operator<<(std::ostream& os, const FileHeader& h)
{
    using i01::core::operator<<;
    auto mic = i01::core::MIC::clone(h.mic);
    os << "Version " << h.version << "," << mic << "," << h.feed_name;
    return os;
}

std::ostream& operator<<(std::ostream& os, const EventHeader& h)
{
    return os << h.type << "," << i01::core::Timestamp(h.time_s,h.time_ns) << "," << h.length;
}

std::ostream& operator<<(std::ostream& os, const EventType& e)
{
    switch (e) {
    case EventType::UNKNOWN:
        os << "UNKNOWN";
        break;
    case EventType::SEQNUM:
        os << "SEQNUM";
        break;
    case EventType::GAP:
        os << "GAP";
        break;
    case EventType::TIMEOUT:
        os << "TIMEOUT";
        break;
    default:
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Event& e)
{
    switch (e.header.type) {
    case EventType::SEQNUM:
        {
            auto p = reinterpret_cast<const SeqnumEvent *>(&e);
            os << *p;
        }
        break;
    case EventType::GAP:
        {
            auto p = reinterpret_cast<const GapEvent *>(&e);
            os << *p;
        }
        break;
    case EventType::TIMEOUT:
        {
            auto p = reinterpret_cast<const TimeoutEvent *>(&e);
            os << *p;
        }
        break;
    case EventType::UNKNOWN:
    default:
        os << e.header;
        break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const SeqnumEvent& h)
{
    return os << h.header << ","
              << (int) h.unit << ","
              << h.seqnum;
}

std::ostream& operator<<(std::ostream& os, const GapEvent& h)
{
    return os << h.header << ","
              << i01::net::ip_addr_to_str(ntohl(h.addr)) << ":"
              << ntohs(h.port) << ","
              << (int) h.unit << ","
              << h.expected << ","
              << h.received << ","
              << i01::core::Timestamp(h.last_time_s, h.last_time_ns);
}

std::ostream& operator<<(std::ostream& os, const TimeoutEvent& h)
{
    return os << h.header << ","
              << (int) h.unit << ","
              << (int) h.started << ","
              << i01::core::Timestamp(h.last_time_s, h.last_time_ns);
}
}
