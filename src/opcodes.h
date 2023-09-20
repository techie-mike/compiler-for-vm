#pragma once

#include <cinttypes>

namespace compiler {

enum class Opcode {
    NONE = 0,
    ADD,
    SUB,
    MUL,
    DIV,
};

enum class Type {
    NONE = 0,
    INT32,
    UINT32,
    INT64,
    UINT64,
};

}
