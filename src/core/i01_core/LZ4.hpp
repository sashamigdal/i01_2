#pragma once

#include <memory>
#include <boost/noncopyable.hpp>

namespace i01 { namespace core { namespace LZ4 {

class bad_magic_number : public std::runtime_error {
    std::uint32_t m_magic_number;
public:
    bad_magic_number(std::uint32_t magic)
      : std::runtime_error("bad LZ4 magic number")
      , m_magic_number(magic) {}
    std::uint32_t magic_number() { return m_magic_number; }
};

class FileReaderImpl;
class FileReader : boost::noncopyable {
    std::unique_ptr<FileReaderImpl> m_impl_p;

    FileReader() = delete;
    FileReader(const FileReader&) = delete;
public:
    FileReader(const std::string& path);
    virtual ~FileReader();

    bool is_open() const;
    ssize_t read(char * buf, size_t maxlen);
};

} } }
