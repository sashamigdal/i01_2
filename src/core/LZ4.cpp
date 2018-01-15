#include <string.h>

#include <stdexcept>
#include <cstdint>
#include <iostream>

#include <lz4.h>
#include <lz4frame.h>

#include <i01_core/macro.hpp>

#include <i01_core/LZ4.hpp>

#include <i01_core/MappedRegion.hpp>


namespace i01 { namespace core { namespace LZ4 {

class FrameDecompressionCtx {
    LZ4F_decompressionContext_t m_dctx_p;
public:
    FrameDecompressionCtx() : m_dctx_p(nullptr) {
        auto err = LZ4F_createDecompressionContext(&m_dctx_p, LZ4F_VERSION);
        if (LZ4F_isError(err)) {
            if (m_dctx_p != nullptr)
                LZ4F_freeDecompressionContext(m_dctx_p);
            throw std::runtime_error(LZ4F_getErrorName(err));
        }
    }
    virtual ~FrameDecompressionCtx()
    {
        auto err = LZ4F_freeDecompressionContext(m_dctx_p);
        if (LZ4F_isError(err))
            std::cerr << "LZ4::~FrameDecompressionCtx: " << LZ4F_getErrorName(err) << std::endl;
    }

    LZ4F_decompressionContext_t get() { return m_dctx_p; }
};

class FileReaderImpl : protected MappedRegion {
    FrameDecompressionCtx m_ctx;
    size_t m_compressed_offset;
    size_t m_decompressed_bytes;
    size_t m_next_frame_size;
    bool m_ready;
public:
    FileReaderImpl(const std::string& path)
        : MappedRegion(path, 0, /* ro = */ true)
        , m_ctx()
        , m_compressed_offset(0)
        , m_decompressed_bytes(0)
        , m_next_frame_size(0)
        , m_ready(false)
    {
        if (!mapped())
            throw std::runtime_error("could not map LZ4 file");

        std::uint32_t magic_number = *data<std::uint32_t>();
        if (magic_number != 0x184D2204)
            throw bad_magic_number(magic_number);

        //LZ4F_frameInfo_t info;
        //::memset(&info, 0, sizeof(info));
        //size_t n = sizeof(magic_number);
        //size_t m = 0;
        //m_next_frame_size = LZ4F_decompress(m_ctx.get(), nullptr, &m, data<const void*>(), &n, nullptr);
        //if (LZ4F_isError(m_next_frame_size))
        //    throw std::runtime_error(LZ4F_getErrorName(m_next_frame_size));
        //m_next_frame_size = LZ4F_getFrameInfo(m_ctx.get(), &info, data<const char*>(), &n);
        //if (LZ4F_isError(m_next_frame_size))
        //    throw std::runtime_error(LZ4F_getErrorName(m_next_frame_size));
        //m_compressed_offset += n;
        m_ready = true;
    }

    virtual ~FileReaderImpl()
    {
    }

    bool opened() const { return m_ready && mapped(); }

    ssize_t read(char * buf, size_t maxlen)
    {
        if (m_compressed_offset >= size())
            return 0;
        if (!opened() || LZ4F_isError(m_next_frame_size)) {
            return -1;
        }
        size_t n = 0;
        while (n < maxlen) {
            size_t sn = size() - m_compressed_offset;
            // optimization:
            if (m_next_frame_size > 0 && m_next_frame_size < size() - m_compressed_offset)
                sn = m_next_frame_size;

            size_t dn = maxlen - n;
            m_next_frame_size = LZ4F_decompress( m_ctx.get()
                                               , &buf[n], &dn
                                               , data<const char>() + m_compressed_offset, &sn
                                               , nullptr);
            n += dn;
            if (LZ4F_isError(m_next_frame_size)) {
                if (n > 0) {
                    return n;
                } else {
                    return -1;
                }
            }
            if (dn == 0 || sn == 0) {
                return n;
            }
            if (sn > 0)
                m_compressed_offset += sn;
            m_decompressed_bytes += dn;
        }
        return n;
    }

    size_t compressed_offset() const { return m_compressed_offset; }
    size_t decompressed_bytes() const { return m_decompressed_bytes; }
};

FileReader::FileReader(const std::string& path)
    : m_impl_p(new FileReaderImpl(path))
{
}

FileReader::~FileReader()
{
}

bool FileReader::is_open() const { return m_impl_p->opened(); }

ssize_t FileReader::read(char * buf, size_t maxlen)
{
    return m_impl_p->read(buf, maxlen);
}

} } }
