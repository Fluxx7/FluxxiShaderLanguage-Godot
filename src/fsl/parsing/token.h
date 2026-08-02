#pragma once
#include "std_imports.h"
#include "godot_imports.h"
#include "../fsl_defs.h"
#include "utilities/slice.h"

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

        BRACE_OPEN,
        BRACE_CLOSE,
        BRACKET_OPEN,
        BRACKET_CLOSE,
        PAREN_OPEN,
        PAREN_CLOSE,

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
        KEYWORD_WHILE,
        KEYWORD_UNIFORM,
        KEYWORD_BUFFER,
        KEYWORD_RETURN,
        KEYWORD_CONTINUE,
        KEYWORD_BREAK,
        

        // OPAQUE TYPES
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
        LITERAL,

        // used by parser
        ERR_EOS,

        // used by lexer, should not ever reach parser
        ERR_TOKEN, 
    };
    enum TokenCategory {
        CATEGORY_WHITESPACE = 1,
        CATEGORY_SYMBOL_OP = 2,
        CATEGORY_SYMBOL_OTHER = 4,
        CATEGORY_SPECIFIER = 8,
        CATEGORY_GLOBAL_KEYWORD = 16,
        CATEGORY_STATEMENT_KEYWORD = 32,
        CATEGORY_OPAQUE_TYPE = 64,
        CATEGORY_TEXTUREFORMAT = 128,
        CATEGORY_BUFFERFORMAT = 256,
        CATEGORY_IDENTIFIER = 512,
        CATEGORY_LITERAL = 1024,
        CATEGORY_OTHER = 2048,
        CATEGORY_ERR = 4096,
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
    const Token* eof_sentinel() const;
    const LocalVector<const Token*> flatten() const;
    const void flatten(LocalVector<const Token*>& flat_tokens) const;
};

class TokenTree {
public:
    sumtype<const Token*, TokenScope> node;
    TokenTree() = default;
    TokenTree(const Token* token) { node = token; }
    TokenTree(const TokenScope& scope) { node = scope; }
    TokenTree(TokenScope&& scope) { node = scope; }
    Token::TokenType get_type() const {
        return match(node,
            [&](const Token* token) {
                return token->token_type;
            },
            [&](const TokenScope& scope) {
                return scope.open->token_type;
            });
    }
    Token::TokenCategory get_category() const {
        return match(node,
            [&](const Token* token) {
                return token->category;
            },
            [&](const TokenScope& scope) {
                return scope.open->category;
            });
    }
    TokenDebugInfo get_debug_info() const {
        return match(node,
            [&](const Token* token) {
                return token->debug_info;
            },
            [&](const TokenScope& scope) {
                return scope.open->debug_info;
            });
    }
    String get_contents() const {
        return match(node,
            [&](const Token* token) {
                return token->contents;
            },
            [&](const TokenScope& scope) {
                return scope.open->contents;
            });
    }
    const Token* get_token() const {
        return match(node,
            [&](const Token* token) {
                return token;
            },
            [&](const TokenScope& scope) {
                return scope.open;
            });
    }
    bool has_leading_whitespace() const {
        return match(node,
            [&](const Token* token) {
                return token->has_leading_whitespace;
            },
            [&](const TokenScope& scope) {
                return scope.open->has_leading_whitespace;
            });
    }
    void flatten(LocalVector<const Token*>& tokens) const;
    const LocalVector<const Token*> flatten() const;
};



class TokenStream {
protected:
    class SliceBuilder {
    protected:
        uint32_t start;
        uint32_t length = 0;
    public:
        SliceBuilder(uint32_t _start) : start(_start) {};
        Slice<TokenTree> get_slice(const TokenStream& parent) const {
            return parent.get_slice(start, length);
        }
        TokenStream get_stream(const TokenStream& parent) const {
            return parent.get_slice(start, length).get_stream();
        }
        void add() {
            length++;
        }
        void reset(uint32_t _start) {
            start = _start;
            length = 0;
        }
    };
    stack<Stream<TokenTree>> streams;
    const TokenTree& eof_sentinel() const;
    TokenDebugInfo last_debug_info;
    SliceBuilder builder = SliceBuilder(0);
    const TokenTree* _consume();
    template<typename T>
    bool _expect(const T& lhs, const T& rhs) {
        if (lhs == rhs) {
            _consume();
            return true;
        }
        return false;
    }
    template<typename T>
    bool _expect_not(const T& lhs, const T& rhs) {
        if (lhs != rhs) {
            _consume();
            return true;
        }
        return false;
    }
public:
    
    operator const TokenTree&() {
        return peek();
    }
    operator const TokenTree*() {
        return &peek();
    }

    bool ok() const { return streams.top().ok(); }
    bool at_end() const { return streams.top().at_end(); }
    void reset() { 
        while (streams.size() > 1) {
            streams.pop();
        }
        streams.top().reset();
    }

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

    bool descend(Token::TokenType scope_in, Token::TokenType scope_out, bool allow_leading_whitespace = true);
    
    bool descend_paren(bool allow_leading_whitespace = true);
    bool descend_brace(bool allow_leading_whitespace = true);
    bool descend_bracket(bool allow_leading_whitespace = true);

    void ascend() {
        if (streams.size() > 1) {
            streams.pop();
        }
    }

    uint32_t get_index() { return streams.top().get_index(); };

    void start_slice() {
        builder.reset(get_index());
    }

    Slice<TokenTree> get_slice() const {
        return builder.get_slice(*this);
    }

    TokenStream get_stream() const {
        return builder.get_stream(*this);
    }

    Slice<TokenTree> get_slice(uint32_t _start, uint32_t len) const;

    TokenStream clip() const {
        return TokenStream(streams.top(), streams.top().get_index());
    }

    TokenStream clip_and_ascend() {
        auto out = TokenStream(streams.top(), streams.top().get_index());
        ascend();
        return out;
    }

    template<Token::TokenType end_tok>
    bool consume_until(bool consume_last = true) {
        while (!at_end()) {
            switch (peek().get_type()) {
                case end_tok: {
                    if (consume_last) consume();
                    return true;
                } break;
                default: 
                    consume();
                    break;
            }
        }
        return false;
    }

    template<Token::TokenType end_tok>
    bool consume_until(std::function<void(const Token*)> func, bool consume_last = true) {
        while (!at_end()) {
            switch (peek().get_type()) {
                case end_tok: {
                    if (consume_last) consume();
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
    bool consume_until(std::function<void()> func, bool consume_last = true) {
        while (!at_end()) {
            auto& tok = peek();
            switch (tok.get_type()) {
                case end_tok: {
                    if (consume_last) consume();
                    return true;
                } break;
                default: 
                    consume();
                    func();
                    break;
            }
        }
        return false;
    }

    TokenStream() = default;
    TokenStream(Stream<TokenTree> source) {
        streams.push(source);
    }
    TokenStream(Stream<TokenTree> source, uint32_t start) {
        streams.push(Stream<TokenTree>(source, start));
    }
    template<typename T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, Stream<TokenTree>>>>
    TokenStream(T&& _root) { 
        streams.push(Stream<TokenTree>(std::forward<T>(_root)));
    }
    template<typename T, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, Stream<TokenTree>>>>
    TokenStream(T&& _root, uint32_t start) { 
        streams.push(Stream<TokenTree>(std::forward<T>(_root), start));
    }

    TokenStream(Span<TokenTree> _source, uint32_t start, uint32_t _end)  {
        streams.push(Stream<TokenTree>(_source, start, _end));
    } 
};
