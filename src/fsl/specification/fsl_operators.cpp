#include "fsl_operators.h"

namespace FSL {

const Operator* _err_sentinel() {
    static const Operator err_sentinel = {OP_ERR, OPPRIO_NONE};
    return &err_sentinel; 
}

const Operator* string_to_unary_op(const String& op_string) {
    
    static const HashMap<String, Operator> unary_ops = {
        {"~", {OP_INVERT, OPPRIO_UNARY, true, true}},
        {"!", {OP_NOT, OPPRIO_UNARY, true, true}},
        {"+", {OP_ADD, OPPRIO_UNARY, true, true}},
        {"-", {OP_SUBTRACT, OPPRIO_UNARY, true, true}},
        {"++", {OP_INCREMENT, OPPRIO_UNARY, true, false}},
        {"--", {OP_DECREMENT, OPPRIO_UNARY, true, false}},
    };
    if (unary_ops.has(op_string)) {
        return &unary_ops[op_string];
    }
    return _err_sentinel();
}

const Operator* string_to_binary_op(const String& op_string) {
    static const HashMap<String, Operator> unary_ops = {

    };
    if (unary_ops.has(op_string)) {
        return &unary_ops[op_string];
    }
    return _err_sentinel();
}

}