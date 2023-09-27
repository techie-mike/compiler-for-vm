#pragma once

#include <array>
#include <cinttypes>

namespace compiler {

#define OPCODE_LIST(ACTION) \
    ACTION( Add )           \
    ACTION( Constant )

enum class Opcode {
    NONE = 0,

#define CREATE_OPCODE(OPCODE) \
    OPCODE,

    OPCODE_LIST(CREATE_OPCODE)

#undef CREATE_OPCODE

    NUM_OPCODES
};

constexpr std::array<const char *const, static_cast<size_t>(Opcode::NUM_OPCODES)> OPCODE_NAME {
    "INVALID",

#define CREATE_NAMES(OPCODE) \
    #OPCODE,

    OPCODE_LIST(CREATE_NAMES)

#undef CREATE_NAMES
};

enum class Type {
    NONE = 0,
    INT32,
    UINT32,
    INT64,
    UINT64,
};

}
