#pragma once
#include "godot_imports.h"
#include "std_imports.h"


namespace FSL {

enum OperatorType {
    OP_INVERT,
    OP_BITAND,
    OP_BITOR,
    OP_XOR,
    OP_RSHIFT,
    OP_LSHIFT,

    OP_NOT,
    OP_LOGAND,
    OP_LOGOR,
    OP_LOGXOR,
    OP_LESSTHAN,
    OP_LESSEQUAL, 
    OP_EQUAL,
    OP_GREATEREQUAL, 
    OP_GREATERTHAN, 
    OP_NOTEQUAL,

    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,

    OP_TERNARY_SEPARATOR,
    OP_TERNARY_QUERY,
    OP_MODULO,

    OP_INCREMENT,
    OP_DECREMENT,
    OP_ASSIGN,
    OP_BITANDASSIGN,
    OP_BITORASSIGN,
    OP_XORASSIGN,
    OP_RSHIFTASSIGN,
    OP_LSHIFTASSIGN,
    OP_ADDASSIGN,
    OP_SUBTRACTASSIGN,
    OP_MULTIPLYASSIGN,
    OP_DIVIDEASSIGN,
    OP_MODULOASSIGN,
    OP_ERR
};

enum OperatorPriority : uint32_t {
    OPPRIO_NONE = 0,     // sentinel
    OPPRIO_ASSIGN,
    OPPRIO_TERNARY,      
    OPPRIO_LOGOR,        
    OPPRIO_LOGXOR,       
    OPPRIO_LOGAND,       
    OPPRIO_BITOR,        
    OPPRIO_BITXOR,       
    OPPRIO_BITAND,       
    OPPRIO_EQUALITY,     
    OPPRIO_RELATIONAL,   
    OPPRIO_SHIFT,        
    OPPRIO_ADDSUB,       
    OPPRIO_MULDIV,       
    OPPRIO_UNARY, 
    OPPRIO_POSTFIX,
    OPPRIO_COUNT
};


struct Operator {
    OperatorType type = OP_ERR;
    OperatorPriority priority = OPPRIO_NONE;
    bool right_assoc = false;
    bool is_pure = false;
};

const Operator* string_to_unary_op(const String& op_string);

const Operator* string_to_binary_op(const String& op_string);

}