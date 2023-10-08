#pragma once

#include <cassert>

namespace compiler {

void PrintAssertionFailed(const char *cond, const char *file, int line, const char *function);

#ifndef NDEBUG
#define ASSERT(cond)                                                    \
    if (!(cond)) {                                                      \
        PrintAssertionFailed(#cond, __FILE__, __LINE__, __FUNCTION__);  \
    }
#else

#define ASSERT(cond) static_cast<void>(0)

#endif
}
