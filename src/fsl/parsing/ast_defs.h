#pragma once

#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include <variant>

using namespace godot;

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

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
    enum Format {
        RGBA32F,
        RGBA16F
    };
    Format format;
};

struct BufferDef {
    enum BufferType {
        UNIFORM,
        STORAGE
    };
    enum Layout {
        STD140,
        STD430
    };
    BufferType buftype;
    Layout layout;
    LocalVector<VariableDecl> fields;
};

struct ResourceNode {
    StringName name;
    std::variant<VariableDecl, BufferDef, TextureDef> resource;
};

String tokens_to_string(const LocalVector<Token> &tokens) {
    String output = "";
    for (const auto &token : tokens) {
        output += token.contents;
    }
    return output;
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