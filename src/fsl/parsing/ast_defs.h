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

inline DebugInfo from_token_debug_info(const TokenDebugInfo& tok_debug_info) {
    return {tok_debug_info.source_file, tok_debug_info.row, tok_debug_info.row, tok_debug_info.column, tok_debug_info.column + tok_debug_info.length};
}

struct FSLNode {
    DebugInfo debug_info;
    bool is_valid = true;
};

struct ScopeNode;
struct IfNode;
struct ElseNode;
struct ForNode;
struct WhileNode;
struct FunctionDecl;

typedef LocalVector<TokenTree> Statement;

struct OperationList;

typedef LocalVector<OperationList> Args;

struct TypeRef : FSLNode {
    LocalVector<FSLSpecifier> specifiers;
    StringName type;
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
    String code;
};

typedef sumtype<VariableDecl, Operator, Literal, FieldAccess, ArrayIndex, FuncCall, VariableRef, UnknownOp, OperationList> Operation;

struct OperationList : FSLNode {
    LocalVector<Operation> operations;
};

struct ReturnExpression : FSLNode {
    OperationList return_val;
};

struct StructDecl : FSLNode {
    StringName name;
    LocalVector<VariableDecl> fields;
    HashMap<StringName, Args> annotations;
};

typedef sumtype<OperationList, IfNode, ElseNode, ForNode, WhileNode, ScopeNode, FunctionDecl, ReturnExpression, StructDecl> Expression;

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

struct WhileNode : FSLNode {
    OperationList cond;
    ScopeNode body;
    bool is_do_while;
    bool is_scoped = true;
};

struct SwitchNode : FSLNode {
    OperationList operand;
    HashMap<String, ScopeNode> body;
};

struct FunctionDecl : FSLNode {
    StringName name;
    optional<TypeRef> return_type;
    LocalVector<VariableDecl> args;
    HashMap<StringName, Args> annotations;
    ScopeNode code;
};

struct TextureDef {
    TextureFormat format;
    TextureType type;
    HashMap<StringName, Args> annotations;
};

struct BufferDef {
    StringName buffer_name;
    BufferType buftype;
    BufferFormat layout;
    LocalVector<VariableDecl> fields;
    HashMap<StringName, Args> annotations;
};

struct UniformDef {
    VariableDecl uniform_decl;
    OperationList default_value;
};

struct ResourceNode : FSLNode {
    StringName name;
    sumtype<UniformDef, BufferDef, TextureDef> resource;
};

struct KernelNode : FSLNode {
    StringName name;
    LocalVector<VariableDecl> push_constants;
    HashMap<StringName, String> name_bindings; 
    HashMap<StringName, Args> annotations;

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
    LocalVector<GlobalDeclaration> contents;
    fslAST() = default;
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
    String output = "";
    for (const auto& specifier : type_ref.specifiers) {
        switch (specifier) {
            case SPECIFIER_IN:
                output += "in";
                break;
            case SPECIFIER_OUT:
                output += "out";
                break;
            case SPECIFIER_INOUT:
                output += "inout";
                break;
            case SPECIFIER_CONST:
                output += "const";
                break;
            case SPECIFIER_SHARED:
                output += "shared";
                break;
        }
    }
    output += type_ref.type;
    if (type_ref.array_dims.size() > 0) {
        // for (const auto& array_dim : type_ref.array_dims) {
        //     output += vformat("%s", tokens_to_string(array_dim));
        // }
    }
    return output;
}

}