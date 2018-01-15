#pragma once

#include <string>

#include <boost/noncopyable.hpp>

#include <i01_core/Lock.hpp>
#include <i01_core/MappedRegion.hpp>
#include <i01_core/TimerListener.hpp>
#include <i01_core/FileBase.hpp>

namespace i01 { namespace core {

class AsyncMappedFile : public TimerListener
                      , public FileBase {
    AsyncMappedFile() = delete;

public:
    AsyncMappedFile(const std::string& path, bool readonly = false);
    virtual ~AsyncMappedFile();

    virtual void on_timer(
            const core::Timestamp&
          , void * userdata
          , std::uint64_t iter) override final;

    int as_readonly_buffer(const char *& buf, std::uint64_t& len) const;

    virtual int write(const char *buf, int len) override final;
    virtual int writev(const struct iovec *, int iovlen) override final;
    std::uint64_t size() const { return *m_sizemap.data(); }
    std::uint64_t capacity() const { return m_filemap.size(); }

    std::string path() const { return m_path; }

private:
    void maintain(bool aggressive = false);

private:
    std::string m_path;
    bool m_readonly;
    RecursiveMutex m_maint_mutex;

    MappedRegion m_filemap;
    MappedTypedRegion<std::uint64_t> m_sizemap;
};

} }
