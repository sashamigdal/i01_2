#pragma once

#ifdef STRINGIFY
#undef STRINGIFY
#endif
/// Replace identifier with string literal of its name.
#define STRINGIFY(x) STRINGIFY2(x)
/// \internal Helper macro for STRINGIFY macro.
#define STRINGIFY2(x) #x

/// Mark the expression x as likely to be (boolean) true.
#define LIKELY(x)           __builtin_expect(!!(x), 1)

/// Mark the expression x as unlikely to be (boolean) true.
#define UNLIKELY(x)         __builtin_expect(!!(x), 0)

/// Prefetch in preparation to read.
//  rw and locality default to 0 and 3 in __builtin_prefetch(x,rw,locality).
#define PREFETCH(x)         __builtin_prefetch(x)

/// Prefetch a range of cache lines to read, default stride is 4x cache line size.
inline void PREFETCH_RANGE(char* data, int len) \
{ for (int i = 0; i < len - (len % 256); i+=256) PREFETCH(data + i); }

/// Prefetch in preparation to read and write.
//  locality defaults to 3 in __builtin_prefetch(x,rw,locality).
#define PREFETCHW(x)        __builtin_prefetch(x,1)

/// Prefetch a range of cache lines to read and write, default stride is 4x cache line size.
inline void PREFETCHW_RANGE(char* data, int len) \
{ for (int i = 0; i < len - (len % 256); i+=256) PREFETCHW(data + i); }

/// Declare section (e.g. data, bss, etc.) in binary to put the variable.
#define SECTION(s)          __attribute__((__section__(#s)))

/// Declare function as unlikely to be called, code paths leading to it
//  can be similarly marked unlikely, which can avoid littering the code with
//  unlikely()s.
#define I01_COLD            __attribute__((__cold__))

/// Declare function as having no side effects except return value, and return
//  value depends only on parameters and globals.
#define I01_PURE            __attribute__((pure))

/// Declare variable as unused to suppress relevant warnings.
#define I01_UNUSED          __attribute__((unused))

/// Pack type or variable in memory, disabling some optimizations.
#define I01_PACKED          __attribute__((packed))

/// Mark a function as having no return value to suppress relevant warnings.
#define I01_NORETURN        __attribute__((noreturn))

/// Mark a function as being deprecated. (Commented due to -Wextra.)
#define I01_DEPRECATED      /* __attribute__((deprecated)) */

/// Assert at compile-time if sizeof(name) != size.
#define I01_ASSERT_SIZE(name, size)                                         \
    static_assert(sizeof(name) == size,                                     \
        "Type " STRINGIFY(name) " "                                         \
        "has incorrect size (" STRINGIFY(sizeof(name)) "!=" STRINGIFY(size) ") "                       \
        "in " __FILE__ ":" STRINGIFY(__LINE__) ".");

#ifndef I01_API
#   if __GNUC__ >= 4
/// Set default visibility.
#       define I01_API __attribute__((visibility("default")))
#   else
#       define I01_API
#   endif
#endif

// // From http://nadeausoftware.com/articles/2012/10/c_c_tip_how_detect_compiler_name_and_version_using_compiler_predefined_macros
// #if defined(__clang__)
//     /* Clang/LLVM. ---------------------------------------------- */
// #elif defined(__ICC) || defined(__INTEL_COMPILER)
//     /* Intel ICC/ICPC. ------------------------------------------ */
// #elif (defined(__GNUC__) || defined(__GNUG__)) && !(defined(__clang__) || defined(__INTEL_COMPILER))
//     /* GNU GCC/G++. --------------------------------------------- */
// #elif defined(__HP_cc) || defined(__HP_aCC)
//     /* Hewlett-Packard C/aC++. ---------------------------------- */
// #elif defined(__IBMC__) || defined(__IBMCPP__)
//     /* IBM XL C/C++. -------------------------------------------- */
// #elif defined(_MSC_VER)
//     /* Microsoft Visual Studio. --------------------------------- */
// #elif defined(__PGI)
//     /* Portland Group PGCC/PGCPP. ------------------------------- */
// #elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
//     /* Oracle Solaris Studio. ----------------------------------- */
// #endif
