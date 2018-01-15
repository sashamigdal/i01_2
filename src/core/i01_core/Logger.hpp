#pragma once

#include <stddef.h>

namespace i01 { namespace core {

class Logger {
public:
    Logger();
    virtual ~Logger();
};

class LoggerSink {
public:
    virtual bool write(const char* buf, int len) = 0;
    virtual bool writev(struct iovec* iovs, int iovlen) = 0;
};



} }
