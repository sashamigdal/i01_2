#include <cassert>
#include <string>

#include <i01_oe/FileBlotterReader.hpp>

namespace i01 { namespace OE {

FileBlotterReader::FileBlotterReader(const std::string& filename) :
    m_orderlog(filename, true),
    m_buf_start{nullptr},
    m_buf{nullptr},
    m_buffer_len{0}
{
    rewind();
}

bool FileBlotterReader::get_buffer(const char*& buf, std::uint64_t& len)
{
    auto ret = m_orderlog.as_readonly_buffer(buf, len);
    return ret >= 0;
}

void FileBlotterReader::do_rewind()
{
    m_buf = nullptr;
    if (!get_buffer(m_buf_start, m_buffer_len)) {
        std::cerr << "FileBlotterReader::rewind: could not read orderlog " << m_orderlog.path() << " as readonly buffer" << std::endl;
        return;
    }
    m_buf = m_buf_start;
}

bool FileBlotterReader::do_at_end()
{
    const char * buf;
    auto len = std::uint64_t{};
    if (!get_buffer(buf, len)) {
        if (m_buf_start != buf) {
            std::cerr << "FileBlotterRead::at_end: orderlog start changed! " << std::endl;
        }

        return true;
    }
    if (len < m_buffer_len) {
        return true;
    }
    m_buffer_len = len;
    // assert(m_buf_start == buf);
    // if (m_buf_start != buf) {
    //     std::cerr << "FileBlotterRead::next: orderlog start changed! " << std::endl;
    //     rewind();
    // }
    return m_buf == nullptr || m_buf >= (m_buf_start + m_buffer_len);
}

void FileBlotterReader::do_next()
{
    auto end_of_buffer = m_buf_start + m_buffer_len;

    if (!at_end()
        && m_buf != nullptr
        && m_buf_start != nullptr
        && m_buffer_len > 0) {
        auto ret = decode(m_buf, end_of_buffer - m_buf);
        if (ret < 0) {
            switch (static_cast<Errors>(-ret)) {
            case Errors::NO_HEADER_SIZE:
                std::cerr << "FileBlotterReader::next: not enough bytes in buffer for MessageHeader at " << m_buf - m_buf_start << " in " << m_orderlog.path() << std::endl;
                break;
            case Errors::NO_MSG_SIZE:
                std::cerr << "FileBlotterReader::next: not enough bytes for message body at " << m_buf - m_buf_start << " in " << m_orderlog.path() << std::endl;
                break;
            case Errors::NONE:
            default:
                break;
            }
            m_buf = nullptr;
        } else if (0 == ret) {
            std::cerr << "FileBlotterReader::next: decode return a read of 0 bytes at " << m_buf - m_buf_start << " in " << m_orderlog.path() << std::endl;
            m_buf = nullptr;
        } else {
            m_buf += ret;
        }
    }
}


}}
