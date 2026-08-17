#include "token.h"

// TODO: have last_debug_info reflect scope.close when consume() hits a scope or when ascending from a scope while leaving _consume unchanged 

const TokenTree& TokenStream::eof_sentinel() const {
    static Token eos_tok = Token(Token::ERR_EOS);
    static const TokenTree sentinel = TokenTree(&eos_tok);
    eos_tok.debug_info = last_debug_info;
    return sentinel;
}

const TokenTree *TokenStream::_consume() {
    builder.add();
    last_debug_info = peek().get_debug_info();
    auto out = streams.top().consume();
    return out;
}

const TokenTree &TokenStream::peek() const {
    auto peek_tok = streams.top().peek();
	return peek_tok == nullptr ? eof_sentinel() : *peek_tok;
}

const TokenTree &TokenStream::consume() {
    if (streams.top().peek() == nullptr) {
        return eof_sentinel();
    }
    auto& out = *_consume();
    // if (out.)
    return out;
}

bool TokenStream::expect(Token::TokenType tok_type) {
    return _expect(peek().get_type(), tok_type);
}

bool TokenStream::expect(Token::TokenCategory tok_category) {
    return _expect(peek().get_category(), tok_category);
}

bool TokenStream::expect(String tok_contents) {
    return _expect(peek().get_contents(), tok_contents);
}

bool TokenStream::expect_not(Token::TokenType tok_type) {
    return _expect_not(peek().get_type(), tok_type);
}

const TokenScope *TokenStream::expect_scope(Token::TokenType scope_in, Token::TokenType scope_out) {
    return match(peek().node,
            [&](const Token* token) -> const TokenScope* {
                return nullptr;
            },
            [&](const TokenScope& scope) -> const TokenScope* {
                if (scope.open->token_type == scope_in && scope.close->token_type == scope_out) {
                    _consume();
                    return &scope;
                } else {
                    return nullptr;
                }
            });
}

const TokenScope *TokenStream::expect_scope_paren() {
	return expect_scope(Token::PAREN_OPEN, Token::PAREN_CLOSE);
}

const TokenScope *TokenStream::expect_scope_brace() {
	return expect_scope(Token::BRACE_OPEN, Token::BRACE_CLOSE);
}

const TokenScope *TokenStream::expect_scope_bracket() {
	return expect_scope(Token::BRACKET_OPEN, Token::BRACKET_CLOSE);
}

bool TokenStream::descend(Token::TokenType scope_in, Token::TokenType scope_out, bool allow_leading_whitespace) {
    auto p_scope = expect_scope(scope_in, scope_out);
    if (p_scope != nullptr) {
        if (allow_leading_whitespace || !p_scope->open->has_leading_whitespace) {
             streams.push(Stream<TokenTree>(p_scope->scope_contents.span(), 0, p_scope->scope_contents.size()));
             return true;
        }
    }
    return false;
}

bool TokenStream::descend_paren(bool allow_leading_whitespace) {
	return descend(Token::PAREN_OPEN, Token::PAREN_CLOSE, allow_leading_whitespace);
}

bool TokenStream::descend_brace(bool allow_leading_whitespace) {
	return descend(Token::BRACE_OPEN, Token::BRACE_CLOSE, allow_leading_whitespace);
}

bool TokenStream::descend_bracket(bool allow_leading_whitespace) {
	return descend(Token::BRACKET_OPEN, Token::BRACKET_CLOSE, allow_leading_whitespace);
}

Slice<TokenTree> TokenStream::get_slice(uint32_t _start, uint32_t len) const {
	return Slice<TokenTree>::make_slice(streams.top().get_source(), _start, len);
}

// *************************
// ******* TOKENTREE *******
// *************************

void TokenTree::flatten(LocalVector<const Token*> &tokens) const {
    match(node,
        [&](const Token* token) {
            tokens.push_back(token);
        },
        [&](const TokenScope& scope) {
            scope.flatten(tokens);
        });
}

const LocalVector<const Token*> TokenTree::flatten() const {
    LocalVector<const Token*> flat_tokens;
    flatten(flat_tokens);
    return flat_tokens;
}

// **************************
// ******* TOKENSCOPE *******
// **************************

const Token* TokenScope::eof_sentinel() const {
	static Token eos_tok = Token(Token::ERR_EOS);
    return &eos_tok;
}

const LocalVector<const Token *> TokenScope::flatten() const {
	LocalVector<const Token*> flat_tokens;
    flatten(flat_tokens);
    return flat_tokens;
}

const void TokenScope::flatten(LocalVector<const Token*> &flat_tokens) const {
    flat_tokens.push_back(open);
    for (auto& val : scope_contents) {
        val.flatten(flat_tokens);
    }
    flat_tokens.push_back(close);
}
