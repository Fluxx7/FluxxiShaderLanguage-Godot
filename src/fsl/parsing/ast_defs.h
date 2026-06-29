#pragma once

#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/a_hash_map.hpp"
#include <variant>
#include "fsl/fsl_defs.h"

using namespace godot;



class Token {
public:
    enum TokenType {
        // WHITESPACE TOKENS
        WHITESPACE,
        NEWLINE,

        // SYMBOL TOKENS
        SYMBOL,
        LEFTBRACE,
        RIGHTBRACE,
        LEFTBRACKET,
        RIGHTBRACKET,
        POUND,

        // TEXT TOKENS
        TYPE,
        SPECIFIER,
        IDENTIFIER,
        INTEGER,
        NUMBER,
        KERNEL,
        LAYOUT,
        TEXTUREFORMAT,
        BUFFERFORMAT
    };
    String contents;
    TokenType token_type;
};

struct VariableDecl {
    StringName name;
    LocalVector<Token> type;
    String default_value = "";
};

struct CodeNode {
    LocalVector<Token> code;
};

struct TextureDef {
    TextureFormat format;
};

struct BufferDef {
    BufferType buftype;
    BufferFormat layout;
    LocalVector<VariableDecl> fields;
};

struct ResourceNode {
    StringName name;
    std::variant<VariableDecl, BufferDef, TextureDef> resource;
};

inline String tokens_to_string(const LocalVector<Token> &tokens) {
    String output = "";
    for (const auto &token : tokens) {
        output += token.contents;
    }
    return output;
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
                if (tokens[index].token_type == Token::LEFTBRACKET) {
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

struct KernelNode {
    StringName name;
    uint32_t entrypoint_line;
    LocalVector<VariableDecl> push_constants;
    HashMap<StringName, StringName> name_bindings; 

    uint32_t local_x_threads;
    uint32_t local_y_threads;
    uint32_t local_z_threads;
    LocalVector<Token> code;
};


struct GlobalDeclaration {
    std::variant<KernelNode, ResourceNode, CodeNode> value;
    uint32_t linenum;
};

class fslAST {
public:
    LocalVector<GlobalDeclaration> contents;
    fslAST() = default;
};