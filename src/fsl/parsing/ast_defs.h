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

struct ScopeNode;
struct IfNode;
struct ElseNode;
struct ForNode;

typedef LocalVector<TokenTree> Statement;

struct TypeRef {
    LocalVector<const Token*> type;
    LocalVector<Statement> array_dims;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct FuncCall {
    StringName func_name;
    LocalVector<Statement> args;
    bool is_valid = true;
};

struct PlaceValueOperation {
    Statement lhs;
    Statement op_tokens;
    Statement rhs;
    bool is_valid = true;
};


typedef std::variant<Statement, IfNode, ElseNode, ForNode, ScopeNode> Expression;

struct ScopeNode {
    LocalVector<Expression> body;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct IfNode {
    Statement cond;
    ScopeNode body;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct ElseNode {
    ScopeNode body;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct ForNode {
    Statement init;
    Statement cond;
    Statement post;
    ScopeNode body;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct VariableDecl {
    StringName name;
    TypeRef type;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct FunctionDecl {
    StringName name;
    TypeRef return_type;
    LocalVector<VariableDecl> args;
    ScopeNode code;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct TextureDef {
    TextureFormat format;
    TextureType type;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct BufferDef {
    StringName buffer_name;
    BufferType buftype;
    BufferFormat layout;
    LocalVector<VariableDecl> fields;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct ResourceNode {
    StringName name;
    std::variant<VariableDecl, BufferDef, TextureDef> resource;
    DebugInfo debug_info;
    bool is_valid = true;
};

struct KernelNode {
    StringName name;
    LocalVector<VariableDecl> push_constants;
    HashMap<StringName, String> name_bindings; 

    Statement local_x_threads;
    Statement local_y_threads;
    Statement local_z_threads;
    ScopeNode code;
    DebugInfo debug_info;
    bool is_valid = true;
};


struct GlobalDeclaration {
    std::variant<KernelNode, ResourceNode, Statement, FunctionDecl> value;
    DebugInfo debug_info;
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
    const auto& tokens = type_ref.type;
    uint32_t type_index = 0;
    for (auto val : fsl_primitives) {
        if (tokens[0]->contents == val) {
            primitive_type = (FSLPrimitive) type_index;
        } else {
            type_index++;
        }
    }
    uint32_t final_char_index = tokens[0]->contents.length() - 1;
    String final_char = tokens[0]->contents.substr(final_char_index);
    if (final_char.is_valid_int()) {
        uint32_t token_val = final_char.to_int();
        if (token_val > 4 || token_val == 0) {
            printvf("Invalid vector size %d", token_val);
        }
        vec_size = (FSLVecSize) token_val;
    }
    for(const auto& array_dim : type_ref.array_dims) {
        uint32_t index = 0;
        if (array_dim[index].get_type() == Token::SYMBOL_LEFTBRACKET) {
            index++;
            if (array_dim[index].get_type() == Token::INTEGER) {
                array_dims.push_back(array_dim[index].get_contents().to_int());
                index++;
            } else {
                array_dims.push_back(0);
            }
        }
    }
    if (array_dims.size() > 0) {
        return (FSLArray) {(FSLCoreType){primitive_type, vec_size}, array_dims};
    }
    return (FSLCoreType){primitive_type, vec_size};
}

inline String typeref_to_string(const TypeRef &type_ref) {
    String output = tokens_to_string(type_ref.type);
    if (type_ref.array_dims.size() > 0) {
        for (const auto& array_dim : type_ref.array_dims) {
            output += vformat("%s", tokens_to_string(array_dim));
        }
    }
    return output;
}