#pragma once

#include "godot_imports.h"

namespace FSL {

enum ExprType {
    EXPRTYPE_COMPTIME, // constant at FSL compile time
    EXPRTYPE_PIPELINE, // constant at pipeline compilation (primarily for specialization constants)
    EXPRTYPE_RUNTIME // not constant / constant only at runtime
};


struct Operation {

};
struct Scope {

};
struct Expression {

};
struct Kernel {

};
struct Buffer {

};
struct Texture {

};
struct Function {

};
struct Program {
};

}