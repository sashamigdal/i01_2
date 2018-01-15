#pragma once

#include <boost/noncopyable.hpp>

namespace i01 { namespace core {

class FileBase : private boost::noncopyable {
public:
    virtual ~FileBase() {}

    virtual int write(const char *, int len) = 0;
    virtual int writev(const struct iovec *, int iovlen) = 0;
};

} }
