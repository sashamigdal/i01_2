#pragma once

#include <cstdint>
#include <i01_core/macro.hpp>

namespace i01 {

extern const char g_IDENT[] SECTION(version);
extern const char g_COMMIT[41] SECTION(version);
extern const char g_VERSION[] SECTION(version);
extern const char g_BUILD_VERSION[] SECTION(version);
extern const std::uint64_t g_INTEGER_VERSION SECTION(version);

}
