#include "fsl_lexer.h"

Token::TokenCategory get_category(Token::TokenType tok_type) {
    switch (tok_type) {
        case Token::WHITESPACE:
        case Token::NEWLINE:
            return Token::CATEGORY_WHITESPACE;

        case Token::SYMBOL_COLON:
        case Token::SYMBOL_TILDE:
        case Token::SYMBOL_EXCLAMATION:
        case Token::SYMBOL_QUESTION:
        case Token::SYMBOL_CARAT:
        case Token::SYMBOL_AND:
        case Token::SYMBOL_STAR:
        case Token::SYMBOL_PERCENT:
        case Token::SYMBOL_SLASH:
        case Token::SYMBOL_DASH:
        case Token::SYMBOL_EQUALS:
        case Token::SYMBOL_OR:
        case Token::SYMBOL_PLUS:
        case Token::SYMBOL_LEFTCARAT:
        case Token::SYMBOL_RIGHTCARAT:
            return Token::CATEGORY_SYMBOL_OP;

        case Token::SYMBOL_COMMA:
        case Token::SYMBOL_PERIOD:
        case Token::SYMBOL_SINGLEQUOTE:
        case Token::SYMBOL_DOUBLEQUOTE:
        case Token::SYMBOL_SEMICOLON:
        case Token::BRACE_OPEN:
        case Token::BRACE_CLOSE:
        case Token::BRACKET_OPEN:
        case Token::BRACKET_CLOSE:
        case Token::PAREN_OPEN:
        case Token::PAREN_CLOSE:
        case Token::SYMBOL_BACKSLASH:
        case Token::SYMBOL_POUND:
            return Token::CATEGORY_SYMBOL_OTHER;

        case Token::SPECIFIER_CONST:
        case Token::SPECIFIER_IN:
        case Token::SPECIFIER_OUT:
        case Token::SPECIFIER_INOUT:
        case Token::SPECIFIER_SHARED:
        case Token::SPECIFIER_PRECISE:
            return Token::CATEGORY_SPECIFIER;

        
        case Token::KEYWORD_IF:
        case Token::KEYWORD_ELSE:
        case Token::KEYWORD_FOR:
        case Token::KEYWORD_RETURN:
        case Token::KEYWORD_DO:
        case Token::KEYWORD_SWITCH:
        case Token::KEYWORD_BREAK:
        case Token::KEYWORD_CASE:
        case Token::KEYWORD_DEFAULT:
            return Token::CATEGORY_STATEMENT_KEYWORD;

        case Token::KEYWORD_KERNEL:
        case Token::KEYWORD_LAYOUT:
        case Token::KEYWORD_UNIFORM:
        case Token::KEYWORD_BUFFER:
        case Token::KEYWORD_STRUCT:
        case Token::KEYWORD_VOID:
            return Token::CATEGORY_GLOBAL_KEYWORD;

        case Token::TYPE_IMAGE2D:
        case Token::TYPE_IMAGE2DARRAY:
            return Token::CATEGORY_TEXTURE_TYPE;
        case Token::TYPE_SAMPLER:
        case Token::TYPE_SAMPLER2DARRAY:
            return Token::CATEGORY_SAMPLER_TYPE;

        case Token::BUFFORMAT_STD140:
        case Token::BUFFORMAT_STD430:
            return Token::CATEGORY_BUFFERFORMAT;

        case Token::IDENTIFIER:
            return Token::CATEGORY_IDENTIFIER;

        case Token::INTEGER:
        case Token::NUMBER:
        case Token::LITERAL:
            return Token::CATEGORY_LITERAL;

        case Token::ERR_EOS:
        case Token::ERR_TOKEN: 
        default:
            return Token::CATEGORY_ERR;
    }
}

Token::TokenType to_symbol(char c) {
    switch (c) {
        case '~': return Token::SYMBOL_TILDE;
        case '!': return Token::SYMBOL_EXCLAMATION;
        case '?': return Token::SYMBOL_QUESTION;
        case '^': return Token::SYMBOL_CARAT;
        case '&': return Token::SYMBOL_AND;
        case '*': return Token::SYMBOL_STAR;

        case '(': return Token::PAREN_OPEN;
        case ')': return Token::PAREN_CLOSE;
        case '{': return Token::BRACE_OPEN;
        case '}': return Token::BRACE_CLOSE;
        case '[': return Token::BRACKET_OPEN;
        case ']': return Token::BRACKET_CLOSE;

        case '#': return Token::SYMBOL_POUND;
        case '%': return Token::SYMBOL_PERCENT;
        case '/': return Token::SYMBOL_SLASH;
        case '\\': return Token::SYMBOL_BACKSLASH;
        case ':': return Token::SYMBOL_COLON;
        case '-': return Token::SYMBOL_DASH;
        case '=': return Token::SYMBOL_EQUALS;
        case '|': return Token::SYMBOL_OR;
        case '+': return Token::SYMBOL_PLUS;
        case '<': return Token::SYMBOL_LEFTCARAT;
        case '>': return Token::SYMBOL_RIGHTCARAT;
        
        case ',': return Token::SYMBOL_COMMA;
        case '.': return Token::SYMBOL_PERIOD;
        case '\'': return Token::SYMBOL_SINGLEQUOTE;
        case '\"': return Token::SYMBOL_DOUBLEQUOTE;
        case ';': return Token::SYMBOL_SEMICOLON;
        
        default: return Token::ERR_TOKEN;
    }
}

Token::TokenType get_token_type(String token_string) {
    static const HashMap<StringName, Token::TokenType> reserved_words = {
        {"kernel", Token::KEYWORD_KERNEL},
        {"layout", Token::KEYWORD_LAYOUT},

        {"for", Token::KEYWORD_FOR},
        {"if", Token::KEYWORD_IF},
        {"else", Token::KEYWORD_ELSE},
        {"while", Token::KEYWORD_WHILE},
        {"do", Token::KEYWORD_DO},
        {"switch", Token::KEYWORD_SWITCH},

        {"break", Token::KEYWORD_BREAK},
        {"continue", Token::KEYWORD_CONTINUE},
        {"uniform", Token::KEYWORD_UNIFORM},
        {"buffer", Token::KEYWORD_BUFFER},
        {"return", Token::KEYWORD_RETURN},
        {"struct", Token::KEYWORD_STRUCT},
        {"case", Token::KEYWORD_CASE},
        {"default", Token::KEYWORD_DEFAULT},
        {"void", Token::KEYWORD_VOID},

        {"in", Token::SPECIFIER_IN},
        {"out", Token::SPECIFIER_OUT},
        {"inout", Token::SPECIFIER_INOUT},
        {"const", Token::SPECIFIER_CONST},
        {"shared", Token::SPECIFIER_SHARED},
        {"precise", Token::SPECIFIER_PRECISE},

        {"true", Token::LITERAL},
        {"false", Token::LITERAL},

        {"std140", Token::BUFFORMAT_STD140},
        {"std430", Token::BUFFORMAT_STD430},
        {"rgba16f", Token::TEXFORMAT_RGBA16F},
        {"rgba32f", Token::TEXFORMAT_RGBA32F},
        
        {"image2D", Token::TYPE_IMAGE2D},
        {"image2DArray", Token::TYPE_IMAGE2DARRAY},
        {"sampler", Token::TYPE_SAMPLER},
        {"sampler2DArray", Token::TYPE_SAMPLER2DARRAY},
    };

    if (reserved_words.has(token_string)) {
        return reserved_words[token_string];
    }

    if (token_string.is_valid_int()) {
        return Token::INTEGER;
    }

    if (token_string.is_valid_float()) {
        return Token::NUMBER;
    }
    
    if (token_string.is_valid_ascii_identifier()) {
        return Token::IDENTIFIER;
    }
    
    return Token::LITERAL;
}



optional<LocalVector<Token>> FSLLexer::tokenize(String file_name, String lexee_base) {
    uint32_t char_index = 0;
    LocalVector<char> token_buffer = {};
    LocalVector<Token> tokens = {};
    auto lexee_clean = clean(lexee_base);
    if (!lexee_clean.has_value()) {
        return {};
    }
    String lexee = *lexee_clean;
    Token next_tok;
    uint32_t linenum = 0;
    uint32_t column = 0;
    int dump_token = 0;
    int push_tok = 0;
    bool next_has_leading_whitespace = false;
    char32_t curr_char = ' ';
    auto flush_and_append = [&](Token::TokenType tok_type) {
        dump_token = 1;
        next_tok = Token();
        next_tok.token_type = tok_type;
        next_tok.category = get_category(tok_type);
        next_tok.contents = String() + curr_char;
        push_tok = 1;
    };
    auto dump_token_buffer = [&]() {
        if (token_buffer.size() > 0) {
            String token_string = "";
            for (auto tokchar : token_buffer) {
                token_string += tokchar;
            }
            token_buffer.clear();
            Token new_tok;
            new_tok.contents = token_string;
            new_tok.has_leading_whitespace = next_has_leading_whitespace;
            new_tok.debug_info.row = linenum;
            new_tok.debug_info.column = column - token_string.length();
            new_tok.debug_info.length = token_string.length();
            new_tok.debug_info.source_file = file_name;
            auto new_tok_type = get_token_type(token_string);
            new_tok.token_type = new_tok_type;
            new_tok.category = get_category(new_tok_type);
            tokens.push_back(new_tok);
            next_has_leading_whitespace = false;
        }
    };
    while (char_index < lexee.length()) {
        curr_char = lexee[char_index++];

        switch (curr_char) {
            case ' ':
            case '\t':
                flush_and_append(Token::WHITESPACE);
                break;
            case '\r':
            case '\n':
                flush_and_append(Token::NEWLINE);
                break;
            default: 
                if (is_ascii_alphanumeric_char(curr_char) || curr_char == '_') {
                    token_buffer.push_back(curr_char);
                } else {
                    if (Token::TokenType symb_type = to_symbol(curr_char); symb_type != Token::ERR_TOKEN) {
                        flush_and_append(symb_type);
                    } else {
                        print_error(vformat("Unexpected character '%c' in file \"%s\"", curr_char, file_name));
                    }
                }
                break;
        }
        

        if (dump_token != 0) {
            dump_token_buffer();
            dump_token = 0;
        }
        if (push_tok) {
            next_tok.debug_info.row = linenum;
            next_tok.debug_info.column = column;
            next_tok.debug_info.length = 1;
            next_tok.debug_info.source_file = file_name;
            next_tok.has_leading_whitespace = next_has_leading_whitespace;
            switch (next_tok.token_type) {
                case Token::NEWLINE:
                    linenum++;
                    column = 0;
                    next_has_leading_whitespace = true;
                    break;
                case Token::WHITESPACE:
                    next_has_leading_whitespace = true;
                    column++;
                    break;
                default:
                    next_has_leading_whitespace = false;
                    column++;
                    break;
            }
            tokens.push_back(next_tok);
            push_tok = 0;
        } else {
            column++;
        }
    }
    dump_token_buffer();    
    return tokens;
}

optional<String> FSLLexer::clean(const String &source) {
    String cleaned = "";
    uint32_t char_index = 0;
    enum CleanState {
        NORMAL,
        MAYBE_COMMENT,
        LINE_COMMENT,
        BLOCK_COMMENT,
        MAYBE_ENDBLOCK
    };
    CleanState state = NORMAL;
    while (char_index < source.length()) {
        const char32_t& curr_char = source[char_index];
        switch (state) {
            case NORMAL: {
                switch (curr_char) {
                    case '/': {
                        state = MAYBE_COMMENT;
                        break;
                    }  
                    default:
                        cleaned += curr_char;
                        break;
                }
            } break;
            case MAYBE_COMMENT: {
                switch (curr_char) {
                    case '/': {
                        state = LINE_COMMENT;
                        break;
                    }
                    case '*': {
                        state = BLOCK_COMMENT;
                        break;
                    }
                    default:
                        state = NORMAL;
                        cleaned += '/';
                        cleaned += curr_char;
                }
            } break;
            case LINE_COMMENT: {
                if (curr_char == '\n' || curr_char == '\r') {
                    cleaned += curr_char;
                    state = NORMAL;
                }
            } break;
            case BLOCK_COMMENT: {
                if (curr_char == '*') {
                    state = MAYBE_ENDBLOCK;
                    break;
                }
                if (curr_char == '\n' || curr_char == '\r') {
                    cleaned += curr_char;
                }
            } break;
            case MAYBE_ENDBLOCK: {
                if (curr_char == '/') {
                    state = NORMAL;
                    cleaned += ' ';
                    break;
                }
            } break;
        }
        char_index++;
    }
    if (state == BLOCK_COMMENT || state == MAYBE_ENDBLOCK) {
        print_error("Unclosed block comment, expected a \"*/\" before end of file");
        return {};
    }
	return cleaned;
}
