#pragma once
#include "godot_imports.h"
#include "std_imports.h"

enum FSLOperatorType {
    OP_INVERT,
    OP_BITAND,
    OP_BITOR,
    OP_XOR,

    OP_NOT,
    OP_LOGAND,
    OP_LOGOR,
    OP_LESSTHAN,
    OP_LESSEQUAL, 
    OP_EQUAL,
    OP_EQUALGREATER, 
    OP_GREATERTHAN, 
    OP_NOTEQUAL,

    OP_MULTIPLY,
    OP_DIVIDE,
    OP_SUBTRACT,
    OP_ADD,

    OP_TERNARY_SEPARATOR,
    OP_TERNARY_QUERY,
    OP_MODULO,

    OP_ASSIGN,
    OP_MAX
};

enum FSLOperatorOperandCount {
    OPCOUNT_UNARY,
    OPCOUNT_BINARY,
    OPCOUNT_MAX
};

struct FSLOperator {
    FSLOperatorType type;
    FSLOperatorOperandCount op_count;
    bool is_const = false;
};

const FSLOperator* string_to_fsl_op(const String& op_string) {
    static const FSLOperator err_sentinel = {OP_MAX, OPCOUNT_MAX, false};
    static const FSLOperator fsl_operators[] = {
        {OP_INVERT, OPCOUNT_UNARY, true},
        {OP_BITAND, OPCOUNT_BINARY, true},
        {OP_BITOR, OPCOUNT_BINARY, true},
        {OP_XOR, OPCOUNT_BINARY, true},

        {OP_NOT, OPCOUNT_UNARY, true},
        {OP_LOGAND, OPCOUNT_BINARY, true},
        {OP_LOGOR, OPCOUNT_BINARY, true},
        {OP_LESSTHAN, OPCOUNT_BINARY, true},
        {OP_LESSEQUAL, OPCOUNT_BINARY, true},
        {OP_EQUAL, OPCOUNT_BINARY, true},
        {OP_EQUALGREATER, OPCOUNT_BINARY, true},
        {OP_GREATERTHAN, OPCOUNT_BINARY, true},
        {OP_NOTEQUAL, OPCOUNT_BINARY, true},

        {OP_ADD, OPCOUNT_BINARY, true},
        {OP_SUBTRACT, OPCOUNT_BINARY, true},
        {OP_MULTIPLY, OPCOUNT_BINARY, true},
        {OP_DIVIDE, OPCOUNT_BINARY, true},

        {OP_TERNARY_SEPARATOR, OPCOUNT_BINARY, true},
        {OP_TERNARY_QUERY, OPCOUNT_BINARY, true},
        {OP_MODULO, OPCOUNT_BINARY, true},

        {OP_ASSIGN, OPCOUNT_BINARY, true},
    };
    if (op_string.is_empty()) {
        return &err_sentinel;
    }
    switch (op_string[0]) {

    }
}