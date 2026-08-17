#pragma once

#include "godot_imports.h"
#include "std_imports.h"

namespace FSL {

enum ExprType {
    EXPRTYPE_COMPTIME, // constant at FSL compile time
    EXPRTYPE_PIPELINE, // constant at pipeline compilation (primarily for specialization constants)
    EXPRTYPE_RUNTIME // not constant / constant only at runtime
};



struct Variable {

};
struct Operation {

};
struct Scope {

};
struct Expression {

};
struct SpecializationConstant {

};
struct Buffer {

};
struct Texture {

};
typedef sumtype<Buffer, Texture> Resource;

struct Function {

};

struct Kernel {
    struct UniformSet {
        HashMap<uint32_t, Resource> bindings;
    };
    StringName name;
    sumtype<uint32_t, StringName> local_size[3];
    LocalVector<SpecializationConstant> specialization_constants;
    HashMap<uint32_t, UniformSet> uniform_sets;
    HashMap<StringName, Resource> used_resources;
    HashMap<StringName, Variable> push_constants;
    Scope body;
};


typedef sumtype<Variable, Function, Kernel, Resource> GlobalExpression;

struct Program {
    LocalVector<GlobalExpression> contents;
};

}