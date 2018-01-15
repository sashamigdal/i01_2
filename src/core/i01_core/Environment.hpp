#pragma once

#include <string>
#include <map>

namespace i01 { namespace core {

typedef std::map<std::string, std::string> EnvironmentBase;
class Environment : public EnvironmentBase {
    public:
        using EnvironmentBase::EnvironmentBase;

        size_type load();
};

} }
