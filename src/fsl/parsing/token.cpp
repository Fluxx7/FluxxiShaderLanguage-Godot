#include "token.h"

const TokenTree& TokenStream::eof_sentinel() const {
    static Token eos_tok = Token(Token::ERR_EOS);
    static const TokenTree sentinel = TokenTree(&eos_tok);
    eos_tok.debug_info = last_debug_info;
    return sentinel;
}

const TokenTree &TokenStream::peek() const {
	return at_end() ? eof_sentinel() : source[index];
}

const TokenTree &TokenStream::consume() {
    if (at_end()) {
        errored = true;
        return eof_sentinel();
    }
    last_debug_info = peek().get_debug_info();
    return source[index++];
}

bool TokenStream::expect(Token::TokenType tok_type) {
    if (errored || (peek().get_type() != tok_type)) {
        return false;
    }
    index++;
    last_debug_info = peek().get_debug_info();
	return true;
}

bool TokenStream::expect(Token::TokenCategory tok_category) {
	if (errored || (peek().get_category() != tok_category)) {
        return false;
    }
    index++;
    last_debug_info = peek().get_debug_info();
	return true;
}

bool TokenStream::expect(String tok_contents) {
	if (errored || (peek().get_contents() != tok_contents)) {
        return false;
    }
    index++;
    last_debug_info = peek().get_debug_info();
	return true;
}

bool TokenStream::expect_not(Token::TokenType tok_type) {
	if (errored || (peek().get_type() == tok_type)) {
        return false;
    }
    index++;
    last_debug_info = peek().get_debug_info();
	return true;
}

const TokenScope *TokenStream::expect_scope(Token::TokenType scope_in, Token::TokenType scope_out) {
    if (const TokenScope* pval = std::get_if<TokenScope>(&peek().node); !errored && (pval && pval->open->token_type == scope_in && pval->close->token_type == scope_out)) {
        index++;
        last_debug_info = peek().get_debug_info();
        return pval;
    }
	return nullptr;
}

const TokenScope *TokenStream::expect_scope_paren() {
	return expect_scope(Token::SYMBOL_LEFTPAREN, Token::SYMBOL_RIGHTPAREN);
}

const TokenScope *TokenStream::expect_scope_brace() {
	return expect_scope(Token::SYMBOL_LEFTBRACE, Token::SYMBOL_RIGHTBRACE);
}

const TokenScope *TokenStream::expect_scope_bracket() {
	return expect_scope(Token::SYMBOL_LEFTBRACKET, Token::SYMBOL_RIGHTBRACKET);
}

std::optional<TokenStream> TokenStream::descend(Token::TokenType scope_in, Token::TokenType scope_out, bool allow_leading_whitespace) {
    if (const TokenScope* pval = std::get_if<TokenScope>(&peek().node); !errored && (pval && pval->open->token_type == scope_in && pval->close->token_type == scope_out)) {
        if (!allow_leading_whitespace && pval->open->has_leading_whitespace) {
            return {};
        }
        index++;
        last_debug_info = peek().get_debug_info();
        return TokenStream(pval->scope_contents.span(), 0, pval->scope_contents.size());
    }
	return {};
}

std::optional<TokenStream> TokenStream::descend_paren(bool allow_leading_whitespace) {
	return descend(Token::SYMBOL_LEFTPAREN, Token::SYMBOL_RIGHTPAREN, allow_leading_whitespace);
}

std::optional<TokenStream> TokenStream::descend_brace(bool allow_leading_whitespace) {
	return descend(Token::SYMBOL_LEFTBRACE, Token::SYMBOL_RIGHTBRACE, allow_leading_whitespace);
}

std::optional<TokenStream> TokenStream::descend_bracket(bool allow_leading_whitespace) {
	return descend(Token::SYMBOL_LEFTBRACKET, Token::SYMBOL_RIGHTBRACKET, allow_leading_whitespace);
}

Slice<TokenTree> TokenStream::get_slice(uint32_t _start, uint32_t len) {
	return Slice<TokenTree>::make_slice(source, _start, len);
}

// *************************
// ******* TOKENTREE *******
// *************************

void TokenTree::flatten(LocalVector<const Token*> &tokens) const {
    std::visit(overload{
        [&](const Token* token) {
            tokens.push_back(token);
        },
        [&](const TokenScope& scope) {
            scope.flatten(tokens);
        },
    }, node);
}

const LocalVector<const Token*> TokenTree::flatten() const {
    LocalVector<const Token*> flat_tokens;
    flatten(flat_tokens);
    return flat_tokens;
}

// **************************
// ******* TOKENSCOPE *******
// **************************

const LocalVector<const Token*> TokenScope::flatten() const {
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
