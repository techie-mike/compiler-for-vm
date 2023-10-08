#include "utils.h"
#include "errno.h"
#include "iostream"
#include "cstring"

namespace compiler {

void PrintAssertionFailed(const char *cond, const char *file, int line, const char *function) {
    int errnum = errno;
    std::cerr << "ASSERTION FAILED: " << cond << std::endl;
    std::cerr << "IN " << file << ":" << std::dec << line << ": " << function << std::endl;
    if (errnum != 0) {
        std::cerr << "ERRNO: " << errnum << " (" << std::strerror(errnum) << ")" << std::endl;
    }
}

}
