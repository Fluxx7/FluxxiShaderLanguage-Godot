#pragma once
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include <functional>
#include "../fsl_defs.h"
#include "utilities/slice.h"
#include <optional>

using namespace godot;

class TokenTree;

struct TokenDebugInfo {
    uint32_t row;
    uint32_t column;
    uint32_t length;
    String source_file;
};

struct Token {
    enum TokenType {
        // WHITESPACE TOKENS
        WHITESPACE,
        NEWLINE,

        // SYMBOLS
        SYMBOL_TILDE,
        SYMBOL_EXCLAMATION,
        SYMBOL_QUESTION,
        SYMBOL_CARAT,
        SYMBOL_AND,
        SYMBOL_STAR,

        SYMBOL_LEFTBRACE,
        SYMBOL_RIGHTBRACE,
        SYMBOL_LEFTBRACKET,
        SYMBOL_RIGHTBRACKET,
        SYMBOL_LEFTPAREN,
        SYMBOL_RIGHTPAREN,

        SYMBOL_POUND,
        SYMBOL_PERCENT,
        SYMBOL_SLASH,
        SYMBOL_BACKSLASH,
        SYMBOL_COLON,
        SYMBOL_DASH,
        SYMBOL_EQUALS,
        SYMBOL_OR,
        SYMBOL_PLUS,
        SYMBOL_LESSTHAN,
        SYMBOL_GREATERTHAN,
        SYMBOL_COMMA,
        SYMBOL_PERIOD,
        SYMBOL_SINGLEQUOTE,
        SYMBOL_DOUBLEQUOTE,
        SYMBOL_SEMICOLON,
        
        // KEYWORDS
        SPECIFIER_CONST,
        SPECIFIER_IN,
        SPECIFIER_OUT,
        SPECIFIER_INOUT,
        SPECIFIER_SHARED,
        
        KEYWORD_KERNEL,
        KEYWORD_LAYOUT,
        KEYWORD_IF,
        KEYWORD_ELSE,
        KEYWORD_FOR,
        KEYWORD_UNIFORM,
        KEYWORD_BUFFER,
        KEYWORD_RETURN,

        // COMPLEX TYPES
        TYPE_IMAGE2D,
        TYPE_IMAGE2DARRAY,

        // USEFUL CATEGORIES
        TEXFORMAT_RGBA16F,
        TEXFORMAT_RGBA32F,
        BUFFORMAT_VERTEX,
        BUFFORMAT_INDEX,
        BUFFORMAT_STD140,
        BUFFORMAT_STD430,

        // GENERIC TOKENS
        IDENTIFIER,
        INTEGER,
        NUMBER,

        // used by parser
        ERR_EOS,

        // used by lexer, should not ever reach parser
        ERR_TOKEN, 
    };
    enum TokenCategory {
        CATEGORY_WHITESPACE,
        CATEGORY_SYMBOL_OP,
        CATEGORY_SYMBOL_OTHER,
        CATEGORY_SPECIFIER,
        CATEGORY_GLOBAL_KEYWORD,
        CATEGORY_STATEMENT_KEYWORD,
        CATEGORY_COMPLEX_TYPE,
        CATEGORY_TEXTUREFORMAT,
        CATEGORY_BUFFERFORMAT,
        CATEGORY_IDENTIFIER,
        CATEGORY_NUMBER,
        CATEGORY_OTHER,
        CATEGORY_ERR,
    };
    String contents;
    TokenType token_type;
    TokenCategory category;
    bool has_leading_whitespace = false;
    TokenDebugInfo debug_info;
    Token() = default;
    Token(TokenType _token_type) : token_type(_token_type) {}
};

typedef std::optional<uint32_t> TokenIndex;

struct TokenScope {
    const Token* open = nullptr;
    const Token* close = nullptr;
    LocalVector<TokenTree> scope_contents;
    const LocalVector<const Token*> flatten() const;
    const void flatten(LocalVector<const Token*>& flat_tokens) const;
};

class TokenTree {
public:
    std::variant<const Token*, TokenScope> node;
    TokenTree() = default;
    TokenTree(const Token* token) { node = token; }
    TokenTree(const TokenScope& scope) { node = scope; }
    TokenTree(TokenScope&& scope) { node = scope; }
    Token::TokenType get_type() const {
        if(const Token* const* token = std::get_if<const Token*>(&node)) {
            return (*token)->token_type;
        } else {
            const TokenScope& scope = std::get<TokenScope>(node);
            return scope.open->token_type;
        }
    }
    Token::TokenCategory get_category() const {
        if(const Token* const* token = std::get_if<const Token*>(&node)) {
            return (*token)->category;
        } else {
            const TokenScope& scope = std::get<TokenScope>(node);
            return scope.open->category;
        }
    }
    TokenDebugInfo get_debug_info() const {
        if(const Token* const* token = std::get_if<const Token*>(&node)) {
            return (*token)->debug_info;
        } else {
            const TokenScope& scope = std::get<TokenScope>(node);
            return scope.open->debug_info;
        }
    }
    String get_contents() const {
        if(const Token* const* token = std::get_if<const Token*>(&node)) {
            return (*token)->contents;
        } else {
            const TokenScope& scope = std::get<TokenScope>(node);
            return scope.open->contents;
        }
    }
    const Token* get_token() const {
        if (const Token* const* tok = std::get_if<const Token*>(&node)) {
            return *tok;
        } else {
            const TokenScope& scope = std::get<TokenScope>(node);
            return scope.open;
        }
    }
    bool has_leading_whitespace() const {
        if (const Token* const* tok = std::get_if<const Token*>(&node)) {
            return (*tok)->has_leading_whitespace;
        } else {
            const TokenScope& scope = std::get<TokenScope>(node);
            return scope.open->has_leading_whitespace;
        }
    }
    void flatten(LocalVector<const Token*>& tokens) const;
    const LocalVector<const Token*> flatten() const;
};



class TokenStream : Stream<TokenTree> {
protected:
    const TokenTree& eof_sentinel() const;
    TokenDebugInfo last_debug_info;
public:
    operator const TokenTree&() {
        return peek();
    }
    operator const TokenTree*() {
        return &peek();
    }

    bool ok() const { return Stream<TokenTree>::ok(); }
    bool at_end() const { return Stream<TokenTree>::at_end(); }
    void reset() { index = start; errored = false; }

    const TokenTree& peek() const;
    const TokenTree& consume();

    bool expect(Token::TokenType tok_type);
    bool expect(Token::TokenCategory tok_category);
    bool expect(String tok_contents);
    bool expect_not(Token::TokenType tok_type);

    const TokenScope* expect_scope(Token::TokenType scope_in, Token::TokenType scope_out);
    const TokenScope* expect_scope_paren();
    const TokenScope* expect_scope_brace();
    const TokenScope* expect_scope_bracket();

    std::optional<TokenStream> descend(Token::TokenType scope_in, Token::TokenType scope_out, bool allow_leading_whitespace = true);
    
    std::optional<TokenStream> descend_paren(bool allow_leading_whitespace = true);
    std::optional<TokenStream> descend_brace(bool allow_leading_whitespace = true);
    std::optional<TokenStream> descend_bracket(bool allow_leading_whitespace = true);
    uint32_t get_index() { return index; };

    Slice<TokenTree> get_slice(uint32_t _start, uint32_t len);
    TokenStream trim() const {
        return TokenStream(source, index, end);
    }

    template<Token::TokenType end_tok>
    bool consume_until(std::function<void(const Token*)> func) {
        while (!at_end()) {
            switch (peek().get_type()) {
                case end_tok: {
                    consume();
                    return true;
                } break;
                default: 
                    func(consume().get_token());
                    break;
            }
        }
        return false;
    }

    template<Token::TokenType end_tok>
    bool consume_until(std::function<void()> func) {
        while (!at_end()) {
            auto& tok = consume();
            switch (tok.get_type()) {
                case end_tok: {
                    return true;
                } break;
                default: 
                    func();
                    break;
            }
        }
        return false;
    }

    TokenStream() = default;
    TokenStream(Stream<TokenTree>&& _root) : Stream<TokenTree>(std::move(_root)) { }

    TokenStream(Span<TokenTree> _source, uint32_t start, uint32_t _end) : Stream<TokenTree>(_source, start, _end) { }
};