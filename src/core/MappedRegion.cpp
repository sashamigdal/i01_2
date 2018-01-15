#include <i01_core/MappedRegion.hpp>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

namespace i01 { namespace core {

    MappedRegion::MappedRegion(const std::string& path, size_t size_, bool ro, bool map) 
        : m_data(nullptr)
        , m_size(size_)
        , m_ro(ro)
        , m_fd(-1)
        , m_file_path(path)
    {
        open(size_, ro);
        if (map)
            mmap(NULL, 0);
    }

    MappedRegion::~MappedRegion()
    {
        munmap();
        if (opened()) {
            close();
        }
    }

    int MappedRegion::open(size_t size_, bool ro, const char * path)
    {
        if (!opened()) {
            if (path != nullptr)
                m_file_path = path;

            if ((m_fd = ::open(m_file_path.c_str(), (ro ? O_RDONLY : O_RDWR | O_CREAT), S_IRUSR | S_IWUSR)) < 0)
                goto err2;

            struct stat st;
            if (::fstat(m_fd, &st) != 0)
                goto err1;

            if (ro || (size_ == 0))
                size_ = static_cast<size_t>(st.st_size);

            if ((size_ > static_cast<size_t>(st.st_size))
                && (ftruncate(size_) != 0))
                goto err1;

            m_size = size_;
            m_ro = ro;
        }
        return 0;
err1:
        ::close(m_fd);
err2:
        if (errno == 0)
            return -1;
        else
            return -errno;
    }

    int MappedRegion::mmap(void * addr, off_t offset)
    {
        if (UNLIKELY(mapped()))
            return -1;

        int prot = 0;
        if (m_ro)
            prot = PROT_READ;
        else
            prot = PROT_READ | PROT_WRITE;

        m_data = static_cast<char *>(::mmap(addr, m_size - (unsigned long)offset, prot, (addr != nullptr ? MAP_FIXED : 0) | MAP_SHARED, m_fd, offset));
        if (UNLIKELY(m_data == MAP_FAILED))
            return -errno;

        return 0;
    }

    int MappedRegion::munmap()
    {
        int ret = -EINVAL;
        if (mapped()) {
            ret = ::munmap(m_data, m_size);
            m_data = nullptr;
        }
        return ret;
    }

    void MappedRegion::close()
    {
        ::close(m_fd);
        m_fd = -1;
    }

    int MappedRegion::sync(bool async)
    {
        return ::msync(m_data, m_size, async ? MS_ASYNC : MS_SYNC);
    }

    int MappedRegion::ftruncate(size_t size_)
    {
        return ::ftruncate(m_fd, static_cast<off_t>(size_));
    }

} }
