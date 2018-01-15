#pragma once

#include <pcap.h>

#include <memory>
#include <boost/noncopyable.hpp>

namespace i01 { namespace core { namespace pcap {

class FileReaderImpl;
class FileReader : boost::noncopyable {
    std::shared_ptr<FileReaderImpl> m_impl_p;

    FileReader() = delete;
    FileReader(const FileReader&) = delete;

    std::shared_ptr<FileReaderImpl> get_impl() { return m_impl_p; }
public:
    static const size_t s_PCAP_ERRBUF_SIZE;
    FileReader(const std::string& path, char *errbuf);
    virtual ~FileReader();

    bool is_open() const;
    int next_ex(struct pcap_pkthdr **hp, const u_char **dp);
    const char * geterr() const;

    // TODO FIXME XXX: remove this hack by creating a FileWriter for
    // reorderpcapns to use instead!
    void *get_pcap_t_ptr();
};

} } }
