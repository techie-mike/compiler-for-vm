#pragma once

#include <cassert>

namespace compiler {

void PrintAssertionFailed(const char *cond, const char *file, int line, const char *function);

#ifndef NDEBUG
#define ASSERT(cond)                                                    \
    if (!(cond)) {                                                      \
        PrintAssertionFailed(#cond, __FILE__, __LINE__, __FUNCTION__);  \
        std::abort();                                                   \
    }

#define UNREACHABLE()                                                   \
    __builtin_unreachable();                                            \

#else

#define ASSERT(cond) ;
#define UNREACHABLE() ;

#endif
}
