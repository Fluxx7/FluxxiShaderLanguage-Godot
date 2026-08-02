#pragma once
#include "godot_imports.h"
#include "std_imports.h"

#include "fsl/fsl_defs.h"
#include "token.h"

namespace AST {

struct DebugInfo {
    String file_name;

    uint32_t start_row;
    uint32_t end_row;

    uint32_t start_column;
    uint32_t end_column;
};

struct FSLNode {
    DebugInfo debug_info;
    bool is_valid = true;
};

struct ScopeNode;
struct IfNode;
struct ElseNode;
struct ForNode;
struct FunctionDecl;

typedef LocalVector<TokenTree> Statement;

struct OperationList;

typedef LocalVector<OperationList> Args;

struct TypeRef : FSLNode {
    LocalVector<const Token*> specifiers;
    const Token* type;
    LocalVector<OperationList> array_dims;
};

struct VariableDecl : FSLNode {
    StringName name;
    TypeRef type;
    HashMap<StringName, Args> annotations;
};

struct FuncCall : FSLNode {
    StringName name;
    Args args;
};

struct Operator : FSLNode {
    String symbol;
};

struct VariableRef : FSLNode {
    StringName name;
};

struct Literal : FSLNode {
    String value;
};

struct FieldAccess : FSLNode {
    StringName field_name;
};

struct ArrayIndex : FSLNode {
    LocalVector<OperationList> indices;
};

struct UnknownOp : FSLNode {
    Statement code;
};

typedef sumtype<VariableDecl, Operator, Literal, FieldAccess, ArrayIndex, FuncCall, VariableRef, UnknownOp, OperationList> Operation;

struct OperationList : FSLNode {
    LocalVector<Operation> operations;
};

struct ReturnExpression : FSLNode{
    OperationList return_val;
};

typedef sumtype<OperationList, IfNode, ElseNode, ForNode, ScopeNode, FunctionDecl, ReturnExpression> Expression;

struct ScopeNode : FSLNode {
    LocalVector<Expression> body;
};

struct IfNode : FSLNode {
    OperationList cond;
    ScopeNode body;
    bool is_scoped = true;
};

struct ElseNode : FSLNode {
    ScopeNode body;
    bool is_scoped = true;
};

struct ForNode : FSLNode {
    OperationList init;
    OperationList cond;
    OperationList post;
    ScopeNode body;
    bool is_scoped = true;
};

struct FunctionDecl : FSLNode {
    StringName name;
    TypeRef return_type;
    LocalVector<VariableDecl> args;
    HashMap<StringName, Args> annotations;
    ScopeNode code;
};

struct TextureDef : FSLNode {
    TextureFormat format;
    TextureType type;
};

struct BufferDef : FSLNode {
    StringName buffer_name;
    BufferType buftype;
    BufferFormat layout;
    LocalVector<VariableDecl> fields;
};

struct ResourceNode : FSLNode {
    StringName name;
    sumtype<VariableDecl, BufferDef, TextureDef> resource;
};

struct KernelNode : FSLNode {
    StringName name;
    LocalVector<VariableDecl> push_constants;
    HashMap<StringName, String> name_bindings; 

    OperationList local_x_threads;
    OperationList local_y_threads;
    OperationList local_z_threads;
    ScopeNode code;
};


struct GlobalDeclaration : FSLNode {
    sumtype<KernelNode, ResourceNode, Expression> value;
};


class fslAST {
public:
    StringName filename;
    LocalVector<Token> tokens;
    LocalVector<GlobalDeclaration> contents;
    fslAST() = default;
    fslAST& operator=(fslAST&& rhs) {
        filename = rhs.filename;
        tokens = std::move(rhs.tokens);
        contents = std::move(rhs.contents);
        return *this;
    }
    fslAST(const fslAST&) = delete;
    fslAST(fslAST&&) = default;
};

inline String tokens_to_string(const LocalVector<Token> &tokens) {
    String output = "";
    for (const auto &token : tokens) {
        output += token.contents;
    }
    return output;
}

inline String tokens_to_string(const LocalVector<const Token*> &tokens) {
    String output = "";
    for (const auto &token : tokens) {
        output += token->contents;
    }
    return output;
}

inline String tokens_to_string(const LocalVector<TokenTree> &token_tree) {
    LocalVector<const Token*> output;
    for (const auto &token : token_tree) {
        token.flatten(output);
    }
    return tokens_to_string(output);
}

inline String tokens_to_string(const LocalVector<TokenTree*> &token_tree) {
    LocalVector<const Token*> output;
    for (const auto &token : token_tree) {
        token->flatten(output);
    }
    return tokens_to_string(output);
}

inline String typeref_to_string(const TypeRef &type_ref) {
    String output = tokens_to_string(type_ref.specifiers);
    output += type_ref.type->contents;
    if (type_ref.array_dims.size() > 0) {
        // for (const auto& array_dim : type_ref.array_dims) {
        //     output += vformat("%s", tokens_to_string(array_dim));
        // }
    }
    return output;
}

}