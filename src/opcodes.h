#pragma once

#include <array>
#include <cinttypes>

namespace compiler {

#define OPCODE_LIST(ACTION) \
    ACTION( Add         , BinaryOperation ) \
    ACTION( Sub         , BinaryOperation ) \
    ACTION( Mul         , BinaryOperation ) \
    ACTION( Div         , BinaryOperation ) \
    ACTION( Constant    , ConstantInst    ) \
    ACTION( Start       , StartInst       ) \
    ACTION( If          , IfInst          ) \
    ACTION( Region      , RegionInst      ) \
    ACTION( Compare     , CompareInst     )


enum class Opcode {
    NONE = 0,

#define CREATE_OPCODE(OPCODE, ...) \
    OPCODE,

    OPCODE_LIST(CREATE_OPCODE)

#undef CREATE_OPCODE

    NUM_OPCODES
};

constexpr std::array<const char *const, static_cast<size_t>(Opcode::NUM_OPCODES)> OPCODE_NAME {
    "INVALID",

#define CREATE_NAMES(OPCODE, ...) \
    #OPCODE,

    OPCODE_LIST(CREATE_NAMES)

#undef CREATE_NAMES
};

enum class Type {
    NONE = 0,
    BOOL,
    INT32,
    UINT32,
    INT64,
    UINT64,
};

#define CC_LIST(ACTION) \
    ACTION( EQ ) \
    ACTION( NE ) \
    ACTION( GE ) \
    ACTION( GT ) \
    ACTION( LE ) \
    ACTION( LT )

enum class ConditionCode {
    NONE = 0,

#define CREATE_CC(CC) \
    CC,

    CC_LIST(CREATE_CC)

#undef CREATE_CC

    NUM_CC
    // Unsigned NOT SUPPORTED now
};

constexpr std::array<const char *const, static_cast<size_t>(ConditionCode::NUM_CC)> CC_NAME {
    "NONE",

#define CREATE_CC_STRING(CC) \
    #CC,

    CC_LIST(CREATE_CC_STRING)

#undef CREATE_CC_STRING
};

#undef CC_LIST

}
