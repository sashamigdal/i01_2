#pragma once

#include <string>

#include <i01_core/AsyncMappedFile.hpp>

#include <i01_oe/BlotterReader.hpp>
#include <i01_oe/Types.hpp>

namespace i01 { namespace OE {

class FileBlotterReader : public BlotterReader {
public:
    FileBlotterReader(const std::string& filename = OE::DEFAULT_ORDERLOG_PATH);
    virtual ~FileBlotterReader() = default;

private:
    bool get_buffer(const char*&, std::uint64_t&);
    virtual void do_rewind() override final;
    virtual void do_next() override final;
    virtual bool do_at_end() override final;

private:
    core::AsyncMappedFile m_orderlog;
    const char * m_buf_start;
    const char * m_buf;
    std::uint64_t m_buffer_len;

};

}}
