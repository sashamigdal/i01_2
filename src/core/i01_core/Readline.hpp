#include <string>

#include <readline/readline.h>
#include <readline/history.h>

namespace i01 { namespace core { namespace readline {

std::string readline(const std::string& prompt);
void add_history(const std::string& str);

}}}
