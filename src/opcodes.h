#pragma once

#include <array>
#include <cinttypes>

namespace compiler {

//===================================================================

#define OPCODE_LIST(ACTION)                               \
    ACTION( Add         , BinaryOperation               ) \
    ACTION( Sub         , BinaryOperation               ) \
    ACTION( Mul         , BinaryOperation               ) \
    ACTION( Div         , BinaryOperation               ) \
    ACTION( Constant    , ConstantInst                  ) \
    ACTION( Start       , ControlProp<Inst>             ) \
    ACTION( End         , ControlProp<DynamicInputs>    ) \
    ACTION( If          , IfInst                        ) \
    ACTION( Jump        , JumpInst                      ) \
    ACTION( Region      , RegionInst                    ) \
    ACTION( Compare     , CompareInst                   ) \
    ACTION( Phi         , PhiInst                       ) \
    ACTION( Return      , ReturnInst                    ) \
    ACTION( Parameter   , ParameterInst                 ) \
    ACTION( Call        , CallInst                      )

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

//===================================================================

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

//===================================================================

#define TYPE_LIST(ACTION)           \
    ACTION( BOOL    , b     )       \
    ACTION( INT32   , i32   )       \
    ACTION( UINT32  , u32   )       \
    ACTION( INT64   , i64   )       \
    ACTION( UINT64  , u64   )

enum class Type {
    NONE = 0,

#define CREATE_TYPES(TYPE, ...) \
    TYPE,

    TYPE_LIST(CREATE_TYPES)

#undef CREATE_TYPES

    NUM_TYPES
};

constexpr std::array<const char *const, static_cast<size_t>(Type::NUM_TYPES)> TYPE_NAME {
    "",

#define CREATE_TYPES_STRING(TYPE, PRINT) \
    #PRINT,

    TYPE_LIST(CREATE_TYPES_STRING)

#undef CREATE_TYPES_STRING
};

#undef TYPE_LIST

}
