#include "pcap-int.h"

#include <endian.h>

#include <iostream>
#include <string>
#include <locale>

#include <i01_core/LZ4.hpp>
#include <i01_core/Pcap.hpp>
#include <i01_core/MappedRing.hpp>


extern "C" {
// pcap-common.c:
extern int linktype_to_dlt(int linktype);
}

namespace i01 { namespace core { namespace pcap {

typedef i01::core::LZ4::FileReader LZ4Reader;

// See https://github.com/the-tcpdump-group/libpcap/blob/master/sf-pcap.c
static const std::uint32_t TCPDUMP_MAGIC = 0xA1B2C3D4;
static const std::uint32_t NSEC_TCPDUMP_MAGIC = 0xA1B23C4D;

const size_t FileReader::s_PCAP_ERRBUF_SIZE = PCAP_ERRBUF_SIZE;

class FileReaderImpl : public std::enable_shared_from_this<FileReaderImpl> {
    enum {
        FILETYPE_UNKNOWN = 0
      , FILETYPE_PCAP // little endian pcap-us
      , FILETYPE_PCAPNS // little endian pcap-ns
      , FILETYPE_PCAP_LZ4 // little endian pcap-us.lz4
      , FILETYPE_PCAPNS_LZ4 // little endian pcap-ns.lz4
    } m_filetype;
    LZ4Reader *m_lz4reader;
    pcap_t *m_pcap_p;
    pcap_file_header m_pcap_sf_fhdr;
    char *m_errbuf;

    static int pcapnslz4_read(pcap_t *p, int cnt, pcap_handler callback, u_char *user)
    {
        if (!p || !callback) {
            ::snprintf(p->errbuf, sizeof(p->errbuf), "pcapnslz4_read invalid arguments");
            return -1;
        }
        auto *fri = ((FileReaderImpl*)(p->priv));
        if (!fri || fri->m_pcap_p != p) {
            ::snprintf(p->errbuf, sizeof(p->errbuf), "priv uninitialized in pcapnslz4_read");
            return -1;
        }
        // see pcap_next_ex in pcap.c, we can't reuse pcap_offline_read
        // directly as read_op because p->rfile is null, so the return value
        // is 0 instead of -2 at eof...
        int status = ::pcap_offline_read(p, cnt, callback, user);
        if (status == 0)
            return -2;
        else
            return status;
    }

    static int pcapnslz4_next_packet(pcap_t *p, struct pcap_pkthdr *hdr, u_char **data)
    {
        if (!p || !hdr) {
            ::snprintf(p->errbuf, sizeof(p->errbuf), "pcapnslz4_next_packet invalid arguments");
            return -1;
        }
        auto *fri = ((FileReaderImpl*)(p->priv));
        if (!fri || fri->m_pcap_p != p) {
            ::snprintf(p->errbuf, sizeof(p->errbuf), "priv uninitialized in pcapnslz4_next_packet");
            return -1;
        }
        return fri->next_packet_(hdr, data);
    }

    static void pcapnslz4_cleanup(pcap_t *p)
    {
        if (!p)
            return;
        if (!p->priv)
            return;
        ((FileReaderImpl *)(p->priv))->m_pcap_p = nullptr;
    }

    // return 1 = EOF
    //        0 = success
    //       -1 = error
    int next_packet_(struct pcap_pkthdr *hdr, u_char **data)
    {
        if (!m_lz4reader || !m_lz4reader->is_open()) {
            ::snprintf(m_pcap_p->errbuf, sizeof(m_pcap_p->errbuf), "lz4 reader not open.");
            return -1;
        }
        pcap_sf_pkthdr sf_hdr;
        ssize_t n = m_lz4reader->read((char*)&sf_hdr, sizeof(pcap_sf_pkthdr));
        if (n == sizeof(pcap_sf_pkthdr))
        {
            hdr->caplen = sf_hdr.caplen;
            hdr->len = sf_hdr.len;
            hdr->ts.tv_sec = sf_hdr.ts.tv_sec;
            switch (m_filetype) {
                case FILETYPE_PCAP:
                case FILETYPE_PCAP_LZ4:
                    hdr->ts.tv_usec = sf_hdr.ts.tv_usec * 1000;
                    break;
                case FILETYPE_PCAPNS:
                case FILETYPE_PCAPNS_LZ4:
                    hdr->ts.tv_usec = sf_hdr.ts.tv_usec;
                    break;
                case FILETYPE_UNKNOWN:
                default:
                    return -1;
            }
            if (hdr->caplen > MAXIMUM_SNAPLEN) {
                ::snprintf(m_pcap_p->errbuf, PCAP_ERRBUF_SIZE, "bogus savefile header");
                return -1;
            }
            if (hdr->caplen > (std::uint32_t)m_pcap_p->bufsize) {
                ::snprintf(m_pcap_p->errbuf, PCAP_ERRBUF_SIZE, "oversized caplen");
                return -1;
            }
            n = m_lz4reader->read((char*)m_pcap_p->buffer, hdr->caplen);
            if (n > 0 && n == hdr->caplen) {
                *data = &m_pcap_p->buffer[0];
                return 0;
            } else if (n > 0) {
                ::snprintf(m_pcap_p->errbuf, PCAP_ERRBUF_SIZE, "truncated savefile");
                return -1;
            } else if (n == hdr->caplen) {
                return 1; // caplen=0 at EOF...
            } else {
                ::snprintf(m_pcap_p->errbuf, PCAP_ERRBUF_SIZE, "error reading data from lz4 file");
                return -1;
            }
        } else if (n == 0) {
            return 1; // EOF
        } else {
            ::snprintf(m_pcap_p->errbuf, sizeof(m_pcap_p->errbuf), "error reading header from lz4 file.");
            return -1;
        }

        ::snprintf(m_pcap_p->errbuf, sizeof(m_pcap_p->errbuf), "unspecified error in next_packet_.");
        return -1;
    }
public:

    FileReaderImpl(const std::string& path, char * errbuf)
        : m_filetype(FILETYPE_UNKNOWN)
        , m_lz4reader(nullptr)
        , m_pcap_p(nullptr)
        , m_errbuf(errbuf)
    {
        int tstamp_precision = PCAP_TSTAMP_PRECISION_NANO;
        if (path.empty())
            throw std::runtime_error("missing path.");
        // lz4 compressed
        try {
            m_lz4reader = new LZ4Reader(path);
            ssize_t n = m_lz4reader->read((char *)&m_pcap_sf_fhdr, sizeof(m_pcap_sf_fhdr));
            if (sizeof(m_pcap_sf_fhdr) == n) {
                // NB: does not support "swapped" (big endian)
                if (m_pcap_sf_fhdr.magic == NSEC_TCPDUMP_MAGIC) {
                    m_filetype = FILETYPE_PCAPNS_LZ4;
                    tstamp_precision = PCAP_TSTAMP_PRECISION_NANO;
                } else if (m_pcap_sf_fhdr.magic == TCPDUMP_MAGIC) {
                    m_filetype = FILETYPE_PCAP_LZ4;
                    tstamp_precision = PCAP_TSTAMP_PRECISION_MICRO;
                } else {
                    throw std::runtime_error("bag magic number inside lz4 file");
                }
            } else {
                std::cerr << n << std::endl;
                throw std::runtime_error("could not read magic number from inside lz4 file");
            }
            if (m_pcap_sf_fhdr.version_major < PCAP_VERSION_MAJOR)
                throw std::runtime_error("pcap savefile format version too old.");
        } catch (LZ4::bad_magic_number& e) {
            delete m_lz4reader;
            m_lz4reader = nullptr;
            m_filetype = FILETYPE_PCAPNS;
        }

        switch (m_filetype) {
        case FILETYPE_PCAP_LZ4:
        case FILETYPE_PCAPNS_LZ4: {
            std::string sf("(savefile)");
            m_pcap_p = ::pcap_create_common(sf.c_str(), m_errbuf, 0);
            if (!m_pcap_p)
                throw std::runtime_error(m_errbuf);
            if (!m_pcap_p->priv)
                m_pcap_p->priv = this;
            else
                throw std::runtime_error("unexpected error setting up pcap_t");
            m_pcap_p->swapped = 0;
            m_pcap_p->version_major = m_pcap_sf_fhdr.version_major;
            m_pcap_p->version_minor = m_pcap_sf_fhdr.version_minor;
            m_pcap_p->tzoff = m_pcap_sf_fhdr.thiszone;
            m_pcap_p->snapshot = m_pcap_sf_fhdr.snaplen;
            m_pcap_p->linktype = ::linktype_to_dlt(m_pcap_sf_fhdr.linktype & 0x03FFFFFF);
            m_pcap_p->linktype_ext = m_pcap_sf_fhdr.linktype & 0xFC000000;
            m_pcap_p->opt.tstamp_precision = tstamp_precision;
            m_pcap_p->read_op = pcapnslz4_read;
            m_pcap_p->next_packet_op = &FileReaderImpl::pcapnslz4_next_packet;
            m_pcap_p->cleanup_op = &FileReaderImpl::pcapnslz4_cleanup;
            if (!m_pcap_p->buffer) {
                m_pcap_p->buffer = (u_char*)::malloc(MAXIMUM_SNAPLEN);
                if (m_pcap_p->buffer)
                    m_pcap_p->bufsize = MAXIMUM_SNAPLEN;
            }
            m_pcap_p->activated = 1;
        } break;
        case FILETYPE_PCAP:
        case FILETYPE_PCAPNS: {
            // open uncompressed
            m_pcap_p = ::pcap_open_offline_with_tstamp_precision(path.c_str(), PCAP_TSTAMP_PRECISION_NANO, m_errbuf);
            if (!m_pcap_p)
                throw std::runtime_error(m_errbuf);
        } break;
        case FILETYPE_UNKNOWN:
        default:
                throw std::runtime_error("could not identify file type.");
        }

    }

    virtual ~FileReaderImpl()
    {
        if (m_pcap_p) {
            pcap_close(m_pcap_p);
            m_pcap_p = nullptr;
        }
        if (m_lz4reader) {
            delete m_lz4reader;
            m_lz4reader = nullptr;
        }
    }

    bool opened() const { return m_pcap_p && (m_lz4reader ? m_lz4reader->is_open() : true); }

    int next_ex(struct pcap_pkthdr **hp, const u_char **dp)
    { return ::pcap_next_ex(m_pcap_p, hp, dp); }

    const char * geterr() { return ::pcap_geterr(m_pcap_p); }

    pcap_t *get_pcap_t_ptr() { return m_pcap_p; }
};


FileReader::FileReader(const std::string& path, char *errbuf)
    : m_impl_p(new FileReaderImpl(path, errbuf))
{
}

FileReader::~FileReader()
{
}

bool FileReader::is_open() const { return m_impl_p ? m_impl_p->opened() : false; }

int FileReader::next_ex(struct pcap_pkthdr **hp, const u_char **dp)
{
    return m_impl_p ? m_impl_p->next_ex(hp, dp) : -1;
}

const char * FileReader::geterr() const { return m_impl_p ? m_impl_p->geterr() : nullptr; }

void * FileReader::get_pcap_t_ptr()
{
    return m_impl_p ? m_impl_p->get_pcap_t_ptr() : nullptr;
}

} } }
