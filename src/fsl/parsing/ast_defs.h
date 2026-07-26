#pragma once

#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/a_hash_map.hpp"
#include <variant>
#include "fsl/fsl_defs.h"
#include "token.h"
using namespace godot;

enum FSLExprType {
    EXPRTYPE_COMPTIME, // constant at FSL compile time
    EXPRTYPE_PIPELINE, // constant at pipeline compilation (primarily for specialization constants)
    EXPRTYPE_RUNTIME // not constant / constant only at runtime
};

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

struct TypeRef : FSLNode {
    LocalVector<const Token*> specifiers;
    const Token* type;
    LocalVector<OperationList> array_dims;
};

struct VariableDecl : FSLNode {
    StringName name;
    TypeRef type;
};

struct FuncCall : FSLNode {
    StringName name;
    LocalVector<OperationList> args;
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

typedef std::variant<VariableDecl, Operator, Literal, FieldAccess, ArrayIndex, FuncCall, VariableRef, UnknownOp, OperationList> Operation;

struct OperationList : FSLNode {
    LocalVector<Operation> operations;
};

struct ReturnExpression : FSLNode{
    OperationList return_val;
};

typedef std::variant<OperationList, IfNode, ElseNode, ForNode, ScopeNode, FunctionDecl, ReturnExpression> Expression;

struct ScopeNode : FSLNode {
    LocalVector<Expression> body;
};

struct IfNode : FSLNode {
    OperationList cond;
    ScopeNode body;
};

struct ElseNode : FSLNode {
    ScopeNode body;
};

struct ForNode : FSLNode {
    OperationList init;
    OperationList cond;
    OperationList post;
    ScopeNode body;
};

struct FunctionDecl : FSLNode {
    StringName name;
    TypeRef return_type;
    LocalVector<VariableDecl> args;
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
    std::variant<VariableDecl, BufferDef, TextureDef> resource;
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
    std::variant<KernelNode, ResourceNode, Expression, FunctionDecl> value;
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

inline FSLType tokens_to_fslType(const godot::LocalVector<Token> &tokens) {
    FSLPrimitive primitive_type = FLOAT;
    FSLVecSize vec_size = ONE;
    LocalVector<uint32_t> array_dims;
    uint32_t index = 0;
    while (index < tokens.size()) {
        switch (index){
            case 0: {
                uint32_t type_index = 0;
                for (auto val : fsl_primitives) {
                    if (tokens[0].contents == val) {
                        primitive_type = (FSLPrimitive) type_index;
                    } else {
                        type_index++;
                    }
                }
                uint32_t final_char_index = tokens[0].contents.length() - 1;
                String final_char = tokens[0].contents.substr(final_char_index);
                if (final_char.is_valid_int()) {
                    uint32_t token_val = final_char.to_int();
                    if (token_val > 4 || token_val == 0) {
                        printvf("Invalid vector size %d", token_val);
                        break;
                    }
                    vec_size = (FSLVecSize) token_val;
                }
            } break;
            default:
                if (tokens[index].token_type == Token::SYMBOL_LEFTBRACKET) {
                    index++;
                    if (tokens[index].token_type == Token::INTEGER) {
                        array_dims.push_back(tokens[index].contents.to_int());
                        index++;
                    } else {
                        array_dims.push_back(0);
                    }
                }
                break;
        }
        index++;
    }
    if (array_dims.size() > 0) {
        return (FSLArray) {(FSLCoreType){primitive_type, vec_size}, array_dims};
    }
    return (FSLCoreType){primitive_type, vec_size};
}

inline FSLType typeref_to_fslType(const TypeRef &type_ref) {
    FSLPrimitive primitive_type = FLOAT;
    FSLVecSize vec_size = ONE;
    LocalVector<uint32_t> array_dims;
    const auto& type_token = type_ref.type;
    uint32_t type_index = 0;
    for (auto val : fsl_primitives) {
        if (type_token->contents == val) {
            primitive_type = (FSLPrimitive) type_index;
        } else {
            type_index++;
        }
    }
    uint32_t final_char_index = type_token->contents.length() - 1;
    String final_char = type_token->contents.substr(final_char_index);
    if (final_char.is_valid_int()) {
        uint32_t token_val = final_char.to_int();
        if (token_val > 4 || token_val == 0) {
            printvf("Invalid vector size %d", token_val);
        }
        vec_size = (FSLVecSize) token_val;
    }
    // for(const auto& array_dim : type_ref.array_dims) {
    //     uint32_t index = 0;
    //     if (array_dim[index].get_type() == Token::SYMBOL_LEFTBRACKET) {
    //         index++;
    //         if (array_dim[index].get_type() == Token::INTEGER) {
    //             array_dims.push_back(array_dim[index].get_contents().to_int());
    //             index++;
    //         } else {
    //             array_dims.push_back(0);
    //         }
    //     }
    // }
    if (array_dims.size() > 0) {
        return (FSLArray) {(FSLCoreType){primitive_type, vec_size}, array_dims};
    }
    return (FSLCoreType){primitive_type, vec_size};
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