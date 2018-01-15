#include <unistd.h>
#include <string.h>

#include <i01_core/Environment.hpp>

extern char ** environ;

namespace i01 { namespace core {

Environment::size_type Environment::load()
{
    size_type n = 0;
    for (int i = 0; environ[i] != nullptr; ++i) {
        char * key_end = ::strchr(environ[i], '=');
        if (key_end == nullptr || key_end < environ[i])
            continue;
        std::string key(environ[i], (size_t)(key_end - environ[i]));
        char * val = ::getenv(key.c_str());
        if (val) {
            emplace(std::make_pair(key, std::string(val)));
            ++n;
        }
    }
    return n;
}

} }
