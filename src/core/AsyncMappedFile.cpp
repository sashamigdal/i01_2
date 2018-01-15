#include <string.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <atomic>

#include <i01_core/AsyncMappedFile.hpp>

namespace i01 { namespace core {

namespace {
/// Map 1 terabyte of virtual memory, the maximum log file size.
const size_t MAX_LOGFILE_SIZE = 1ULL << 40;
}

AsyncMappedFile::AsyncMappedFile(const std::string& path_, bool readonly)
    : m_path(path_)
    , m_readonly(readonly)
    , m_maint_mutex()
    , m_filemap(path_, MAX_LOGFILE_SIZE, readonly, true)
    , m_sizemap(path_ + ".offset", readonly, false)
{
    LockGuard<RecursiveMutex> lock(m_maint_mutex);
    // Best effort madvise:
    ::madvise(m_filemap.data<void>(), MAX_LOGFILE_SIZE, MADV_SEQUENTIAL);
}

AsyncMappedFile::~AsyncMappedFile()
{
    LockGuard<RecursiveMutex> lock(m_maint_mutex);
    if (m_sizemap.mapped() && !m_filemap.readonly())
        if (m_filemap.ftruncate(*m_sizemap.data()) != 0)
            throw std::runtime_error(std::string("Truncate failed: ") + strerror(errno));
}

void AsyncMappedFile::on_timer(
        const core::Timestamp&
      , void * userdata
      , std::uint64_t iter)
{
    if (UNLIKELY(userdata != this)) {
        // This should never happen...
        throw std::runtime_error("AsyncMappedFile userdata<->this mismatch.");
    }

    if (iter < 2) {
        maintain();
    } else {
        maintain(true);
    }
}

// We don't call this "read" as that implies reading from a file
// descriptor, and I don't want to keep state on where the last read
// bytes are...
int AsyncMappedFile::as_readonly_buffer(const char*& buf, std::uint64_t& len) const
{
    if (UNLIKELY(!m_filemap.mapped() || !m_sizemap.mapped()
                 || !m_filemap.opened() || !m_sizemap.opened())) {
        return -EBADF;
    }
    buf = m_filemap.data<char>();

    len = *m_sizemap.data();

    return 0;
}

int AsyncMappedFile::write(const char *buf, int len)
{
    if (UNLIKELY(!m_filemap.mapped() || !m_sizemap.mapped()))
        return -EBADF;
    if (UNLIKELY(len <= 0))
        return -1;

    std::uint64_t off = __sync_fetch_and_add(m_sizemap.data(), (std::uint64_t)len);
    if ((std::int64_t)off + len < (std::int64_t)m_filemap.size()) {
        ::memcpy(m_filemap.data<char>() + off, buf, len);
        return len;
    } else {
        return -ENOMEM;
    }

    return -EINVAL;
}

int AsyncMappedFile::writev(const struct iovec *iov, int iovlen)
{
    int n = 0;
    for (int i = 0; i < iovlen; ++i) {
        int m = write((const char *)iov[i].iov_base, (int)iov[i].iov_len);
        if (m >= 0) n += m;
        else return n;
    }
    return n;
}

namespace {
    const int PAGE_SIZE = ::getpagesize();
    const int LOW_REFRESH_RATE = 4 * 1024 * PAGE_SIZE; // 16MB
    const int HIGH_REFRESH_RATE = 16 * 1024 * PAGE_SIZE; // 64MB
}

void AsyncMappedFile::maintain(bool aggressive)
{
    LockGuard<RecursiveMutex> lock(m_maint_mutex);

    if (m_readonly) {
        return;
    }

    int refresh_rate = aggressive ? HIGH_REFRESH_RATE : LOW_REFRESH_RATE;
    if (m_filemap.mapped() && m_sizemap.mapped()) {
        // TODO: lock ahead, unlock behind.
        // TODO: switch to mapping 128MB at a time, three-buffer sequence.
        ::madvise(m_filemap.data<char>() + *m_sizemap.data(), refresh_rate, MADV_SEQUENTIAL);
    }
}

} }
