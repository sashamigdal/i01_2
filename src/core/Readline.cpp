#include <i01_core/Readline.hpp>

namespace i01 { namespace core { namespace readline {


std::string readline(const std::string& prompt) {
    return ::readline(prompt.c_str());
}

void add_history(const std::string& str) {
    ::add_history(str.c_str());
}


}}}
