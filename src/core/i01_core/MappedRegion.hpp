#pragma once

#include <cstdint>
#include <string>
#include <sys/mman.h>

#include <i01_core/macro.hpp>

namespace i01 { namespace core {

    class MappedRegion {
    public:
        MappedRegion(const std::string& path, size_t size_ = 0, bool ro = false, bool map = true);
        virtual ~MappedRegion();
        int mmap(void * addr = nullptr, off_t offset = 0);
        int munmap();
        void close();
        int sync(bool async = false);
        int ftruncate(size_t size_);
        template <typename T>
        inline T* data() const { return reinterpret_cast<T*>(m_data); }
        inline size_t size() const { return m_size; }
        inline bool opened() const { return m_fd != -1; }
        inline bool mapped() const { return m_data != nullptr && m_data != MAP_FAILED; }
        inline bool readonly() const { return m_ro; }

    private:
        MappedRegion() = delete;
        MappedRegion(const MappedRegion&) = delete;
        MappedRegion& operator=(MappedRegion&) = delete;

        int open(size_t size_ = 0, bool ro = false, const char * path = nullptr);

        char *m_data;
        size_t m_size;
        bool m_ro;
        int m_fd;
        std::string m_file_path;

    };

    template <typename T>
    class MappedTypedRegion : public MappedRegion {
    public:
        inline T* data() const { return MappedRegion::data<T>(); }
        MappedTypedRegion(const std::string& path, bool ro = false, bool construct = false)
            : MappedRegion(path, sizeof(T), ro)
        {
            if (construct)
                new(MappedRegion::data<void>()) T;
        }
        virtual ~MappedTypedRegion()
        {
            data()->~T();
        }
    };

} }
