#pragma once

#include <array>
#include <cinttypes>

namespace compiler {

enum class Opcode {
    NONE = 0,
    ADD,
    SUB,
    MUL,
    DIV,
    CONSTANT,
    NUM_OPCODES
};

constexpr std::array<const char *const, static_cast<size_t>(Opcode::NUM_OPCODES)> OPCODE_NAME {
    "NONE",
    "Add",
    "Sub",
    "Mul",
    "Div",
    "Constant"
};

enum class Type {
    NONE = 0,
    INT32,
    UINT32,
    INT64,
    UINT64,
};

}
