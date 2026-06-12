#include "fsl_parser.h"

bool is_symbol(char c) {
    switch (c) {
        case '~': return true;
        case '!': return true;
        case '^': return true;
        case '&': return true;
        case '*': return true;
        case '(': return true;
        case ')': return true;
        case '/': return true;
        case ':': return true;
        case '-': return true;
        case '=': return true;
        case '|': return true;
        case '+': return true;
        case '<': return true;
        case '>': return true;
        case '?': return true;
        case ',': return true;
        case '.': return true;
        case '\'': return true;
        case '\"': return true;
        case ';': return true;
        case '%': return true;
        default: return false;
    }
}

uint32_t is_type(String identifier) {
    static const char *types[] = {
        "float",
        "int",
        "uint",
        "double",
        "bool",
        "float2",
        "float3",
        "float4",
        "int2",
        "int3",
        "int4",
        "uint2",
        "uint3",
        "uint4",
        "double2",
        "double3",
        "double4",
        "bool2",
        "bool3",
        "bool4"
    }; 
    
    uint32_t curr_index = 0;
    for (auto type_name : types) {
        if (identifier == type_name) {
            return curr_index;
        }
        curr_index++;
    }
    return -1;
}

uint32_t is_specifier(String identifier) {
    static const char *specifiers[] = {
        "in",
        "out",
        "inout",
        "const"
    }; 
    
    uint32_t curr_index = 0;
    for (auto specifier : specifiers) {
        if (identifier == specifier) {
            return curr_index;
        }
        curr_index++;
    }
    return -1;
}

uint32_t is_texture_format(String identifier) {
    static const char *texture_formats[] = {
        "rgba16f",
        "rgba32f"
    }; 
    
    uint32_t curr_index = 0;
    for (auto texture_format : texture_formats) {
        if (identifier == texture_format) {
            return curr_index;
        }
        curr_index++;
    }
    return -1;
}

uint32_t is_buffer_format(String identifier) {
    static const char *buffer_formats[] = {
        "std140",
        "std430"
    }; 
    
    uint32_t curr_index = 0;
    for (auto buffer_format : buffer_formats) {
        if (identifier == buffer_format) {
            return curr_index;
        }
        curr_index++;
    }
    return -1;
}

Token::TokenType get_token_type(String token_string) {
    if (token_string == "kernel") {
        return Token::KERNEL;
    }

    if (token_string == "layout") {
        return Token::LAYOUT;
    }

    if (is_specifier(token_string) != -1) {
        return Token::SPECIFIER;
    }

    if (is_type(token_string) != -1) {
        return Token::TYPE;
    }

    if (is_texture_format(token_string) != -1) {
        return Token::TEXTUREFORMAT;
    }

    if (is_buffer_format(token_string) != -1) {
        return Token::BUFFERFORMAT;
    }

    if (token_string.is_valid_int()) {
        return Token::INTEGER;
    }

    if (token_string.is_valid_float()) {
        return Token::NUMBER;
    }

    return Token::IDENTIFIER;
}

#define FLUSH_AND_APPEND(tok_type) \
    dump_token = 1; \
    next_tok = Token(); \
    next_tok.token_type = tok_type; \
    next_tok.contents = String() + curr_char; \
    push_tok = 1

LocalVector<Token> FSLParser::_tokenize(String lexee) {
    uint char_index = 0;
    LocalVector<char> token_buffer = {};
    LocalVector<Token> tokens = {};
    Token next_tok;
    int dump_token = 0;
    int push_tok = 0;
    while (char_index < lexee.length()) {
        char curr_char = lexee[char_index++];
        switch (curr_char) {
            case ' ':
            case '\t':
                FLUSH_AND_APPEND(Token::WHITESPACE);
                break;
            case '\n':
                FLUSH_AND_APPEND(Token::NEWLINE);
                break;
            case '{':
                FLUSH_AND_APPEND(Token::LEFTBRACE);
                break;
            case '}':
                FLUSH_AND_APPEND(Token::RIGHTBRACE);
                break;
            case '#':
                FLUSH_AND_APPEND(Token::POUND);
                break;
            case '[':
                FLUSH_AND_APPEND(Token::LEFTBRACKET);
                break;
            case ']':
                FLUSH_AND_APPEND(Token::RIGHTBRACKET);
                break;
            default: 
                if (is_ascii_alphanumeric_char(curr_char) || curr_char == '_') {
                    token_buffer.push_back(curr_char);
                } else {
                    if (is_symbol(curr_char)) {
                        dump_token = 1;
                        next_tok = Token();
                        next_tok.token_type = Token::SYMBOL;
                        next_tok.contents = String() + curr_char;
                        push_tok = 1;
                    } else {
                        print_error(vformat("Unexpected character '%c' in file", curr_char));
                    }
                }
                break;

        }
        

        if (dump_token != 0) {
            if (token_buffer.size() > 0) {
                String token_string = "";
                for (auto tokchar : token_buffer) {
                    token_string += tokchar;
                }
                token_buffer.clear();
                Token new_tok;
                new_tok.contents = token_string;
                new_tok.token_type = get_token_type(token_string);
                tokens.push_back(new_tok);
            }
            dump_token = 0;
        }
        if (push_tok) {
            tokens.push_back(next_tok);
            push_tok = 0;
        }
    }
    return tokens;
}
#undef FLUSH_AND_APPEND

inline void _discard_whitespace(const LocalVector<Token>& tokens, uint32_t &token_index) {
    while (token_index < tokens.size() && tokens[token_index].token_type == Token::WHITESPACE) {
        token_index++;
    }
}

inline void _discard_whitespace_n(const LocalVector<Token>& tokens, uint32_t &token_index) {
    while (token_index < tokens.size() && (tokens[token_index].token_type == Token::WHITESPACE || tokens[token_index].token_type == Token::NEWLINE)) {
        token_index++;
    }
}

#define DISCARD_WHITESPACE _discard_whitespace(tokens, token_index)
#define DISCARD_WHITESPACE_N _discard_whitespace_n(tokens, token_index)

std::optional<VariableDecl> _parse_variable_decl(const LocalVector<Token>& tokens, uint32_t &token_index, bool arrays_allowed = false) {
    VariableDecl new_var_decl;
    DISCARD_WHITESPACE_N;

    const Token& token = tokens[token_index];
    switch (token.token_type) {
        case Token::TYPE: {
            new_var_decl.type.push_back(token);
        } break;
        case Token::SPECIFIER: {
            new_var_decl.type.push_back(token);
            token_index++;
            DISCARD_WHITESPACE_N;
            if (tokens[token_index].token_type != Token::TYPE) {
                print_error(vformat("Unexpected token \"%s\", expected type", tokens[token_index].contents));
                return {};
            }
            new_var_decl.type.push_back(tokens[token_index]);
        } break;
        default:
            print_error(vformat("Unexpected token \"%s\", expected type or specifier", token.contents));
            return {};
    }
    token_index++;
    DISCARD_WHITESPACE_N;

    if (tokens[token_index].token_type != Token::IDENTIFIER) {
        print_error(vformat("Unexpected token \"%s\", expected identifier", tokens[token_index].contents));
        return {};
    }
    new_var_decl.name = tokens[token_index++].contents;
    DISCARD_WHITESPACE;
    if (arrays_allowed) {
        while (tokens[token_index].token_type == Token::LEFTBRACKET) {
            new_var_decl.type.push_back(tokens[token_index++]);
            DISCARD_WHITESPACE;
            if (tokens[token_index].token_type == Token::INTEGER) {
                new_var_decl.type.push_back(tokens[token_index++]);
            }
            if (tokens[token_index].token_type != Token::RIGHTBRACKET) {
                print_error(vformat("Unexpected token \"%s\", expected \"]\"", tokens[token_index].contents));
                return {};
            }
            new_var_decl.type.push_back(tokens[token_index++]);
            DISCARD_WHITESPACE;
        }
    }

    return new_var_decl;
}

std::optional<LocalVector<Token>> _parse_scope(const LocalVector<Token>& tokens, uint32_t &token_index) {
    LocalVector<Token> out_code;
    
    bool in_scope = true;
    while (token_index < tokens.size() && in_scope) {
        const Token& token = tokens[token_index++];

        switch (token.token_type) {
            case Token::LEFTBRACE: {
                out_code.push_back(token);
                auto scope_code = _parse_scope(tokens, token_index);
                if (!scope_code.has_value()) return {};
                for (auto token : *scope_code) {
                    out_code.push_back(token);
                }
                out_code.push_back({"}", Token::RIGHTBRACE});
            } break;
            case Token::RIGHTBRACE:
                in_scope = false;
                break;
            case Token::SYMBOL: 
                if (tokens[token_index].contents == "/") {
                    if (tokens[token_index].contents == "/") {
                        while (tokens[token_index].token_type != Token::NEWLINE) token_index++;
                        break;
                    }
                    if (tokens[token_index].contents == "*") {
                        token_index++;
                        while (token_index < tokens.size()) {
                            if (tokens[token_index].contents == "*" && tokens[token_index + 1].contents == "/") {
                                token_index += 2;
                                break;
                            }
                            token_index++;
                        }
                        break;
                    }
                }
            case Token::IDENTIFIER:
            default:
                out_code.push_back(token);
                break;
        }
    }
    return out_code;
}

std::optional<CodeNode> _parse_code(const LocalVector<Token>& tokens, uint32_t &token_index) {
    CodeNode new_code;
    print_error("Code parsing not implemented");
    return new_code;
}

std::optional<KernelNode> _parse_kernel(const LocalVector<Token>& tokens, uint32_t &token_index, uint32_t entry_linenum) {
    if (tokens[token_index].token_type != Token::LEFTBRACKET) {
        print_error(vformat("Unexpected token \"%s\" after `kernel`, expected '['", tokens[token_index].contents));
        return {};
    }
    token_index++;
    KernelNode new_kernel = KernelNode();
    new_kernel.entrypoint_line = entry_linenum;


    new_kernel.local_x_threads = tokens[token_index++].contents.to_int();
    DISCARD_WHITESPACE_N;
    if (tokens[token_index].contents != ",") {
        print_error(vformat("Unexpected token \"%s\", expected \",\"", tokens[token_index].contents));
    }
    token_index ++;
    DISCARD_WHITESPACE_N;
    new_kernel.local_y_threads = tokens[token_index++].contents.to_int();
    DISCARD_WHITESPACE_N;
    if (tokens[token_index].contents != ",") {
        print_error(vformat("Unexpected token \"%s\", expected \",\"", tokens[token_index].contents));
    }
    token_index ++;
    DISCARD_WHITESPACE_N;
    new_kernel.local_z_threads = tokens[token_index++].contents.to_int();
    DISCARD_WHITESPACE_N;
    if (tokens[token_index].token_type != Token::RIGHTBRACKET) {
        print_error(vformat("Unexpected token \"%s\" after kernel size declaration, expected \"]\"", tokens[token_index].contents));
    }
    token_index ++;
    DISCARD_WHITESPACE_N;

    if (tokens[token_index].token_type == Token::IDENTIFIER) {
        new_kernel.name = tokens[token_index++].contents;
    } else {
        print_error("Expected identifier for kernel");
        return {};
    }

    DISCARD_WHITESPACE_N;
    if (tokens[token_index].contents != "(") {
        print_error("Expected \"(\" after kernel identifier");
        return {};
    }
    token_index++;
    DISCARD_WHITESPACE_N;
    while (tokens[token_index].contents != ")") {
        switch (tokens[token_index].token_type) {
            case Token::IDENTIFIER: {
                String name_binding_key = tokens[token_index++].contents;
                DISCARD_WHITESPACE_N;
                if (tokens[token_index].contents != ":") {
                    print_error(vformat("Unexpected token \"%s\", expected type, specifier, or name binding", name_binding_key));
                    return {};
                }
                token_index++;
                DISCARD_WHITESPACE_N;
                if (tokens[token_index].token_type != Token::IDENTIFIER) {
                    print_error(vformat("Unexpected token \"%s\", expected name to bind", tokens[token_index].contents));
                }
                new_kernel.name_bindings[name_binding_key] = tokens[token_index++].contents;
            } break;
            case Token::SPECIFIER: 
            case Token::TYPE: {
                auto var_decl = _parse_variable_decl(tokens, token_index);
                if (!var_decl.has_value()) return {};
                new_kernel.push_constants.push_back(*var_decl);
            } break;
            case Token::SYMBOL: 
                if (tokens[token_index].contents == "/") {
                    if (tokens[token_index + 1].contents == "/") {
                        while (tokens[token_index].token_type != Token::NEWLINE) token_index++;
                        break;
                    }
                    if (tokens[token_index + 1].contents == "*") {
                        token_index++;
                        while (token_index < tokens.size()) {
                            if (tokens[token_index].contents == "*" && tokens[token_index + 1].contents == "/") {
                                token_index += 2;
                                break;
                            }
                            token_index++;
                        }
                        break;
                    }
                }
            default:
                if (tokens[token_index].contents != ",") {
                    print_error(vformat("Unexpected token \"%s\" in kernel arguments", tokens[token_index].contents));
                    return {};
                }
                token_index++;
                break;
        }
        DISCARD_WHITESPACE_N;
    }
    token_index++;
    DISCARD_WHITESPACE_N;

    if (tokens[token_index].token_type != Token::LEFTBRACE) {
        print_error(vformat("Unexpected token \"" + tokens[token_index].contents + "\" at token index %d, expected '{'", token_index));
        return {};
    }
    token_index++;

    auto scope_code = _parse_scope(tokens, token_index);
    if (!scope_code.has_value()) return {};
    new_kernel.code = *scope_code;
    return new_kernel;
}

std::optional<ResourceNode> _parse_buffer(const LocalVector<Token>& tokens, uint32_t &token_index) {
    ResourceNode new_buffer;
    BufferDef buf_def;
    if (tokens[token_index].contents == "std140") {
        buf_def.layout = BufferDef::STD140;
    } else if (tokens[token_index].contents == "std430") {
        buf_def.layout = BufferDef::STD430;
    }
    token_index++;
    if (tokens[token_index].contents != ")") {
        print_error(vformat("Unexpected token \"%s\" after buffer format, expected \")\"", tokens[token_index].contents));
        return {};
    }
    token_index++;
    DISCARD_WHITESPACE_N;

    if (tokens[token_index].contents == "uniform") {
        buf_def.buftype = BufferDef::UNIFORM;
    } else if (tokens[token_index].contents == "buffer") {
        buf_def.buftype = BufferDef::STORAGE;
    } else {
        print_error(vformat("Unexpected token \"%s\" in buffer declaration, expected \"uniform\" or \"buffer\"", tokens[token_index].contents));
        return {};
    }
    token_index++;
    DISCARD_WHITESPACE_N;

    if (tokens[token_index].token_type != Token::IDENTIFIER) {
        print_error(vformat("Unexpected token \"%s\" in buffer declaration, expected identifier", tokens[token_index].contents));
        return {};
    }
    new_buffer.name = tokens[token_index].contents;
    token_index++;
    DISCARD_WHITESPACE_N;
    if (tokens[token_index].token_type != Token::LEFTBRACE) {
        print_error(vformat("Unexpected token \"%s\" after buffer identifier, expected \"{\"", tokens[token_index].contents));
        return {};
    }
    token_index++;
    while (tokens[token_index].token_type != Token::RIGHTBRACE) {
        auto var_decl = _parse_variable_decl(tokens, token_index, true);
        if (!var_decl.has_value()) return {};
        buf_def.fields.push_back(*var_decl);
        if (tokens[token_index].contents != ";") {
            print_error(vformat("Unexpected token \"%s\" after buffer field, expected \";\"", tokens[token_index].contents));
        }
        token_index++;
        DISCARD_WHITESPACE_N;
    }
    token_index++;
    DISCARD_WHITESPACE_N;
    if (tokens[token_index].contents != ";") {
        print_error(vformat("Unexpected token \"%s\" after buffer declaration, expected \";\"", tokens[token_index].contents));
    }
    token_index++;
    new_buffer.resource = buf_def;
    return new_buffer;
}

std::optional<ResourceNode> _parse_texture(const LocalVector<Token>& tokens, uint32_t &token_index) {
    ResourceNode new_texture;
    TextureDef tex_def;
    if (tokens[token_index].contents == "rgba16f") {
        tex_def.format = TextureDef::RGBA16F;
    } else if (tokens[token_index].contents == "rgba32f") {
        tex_def.format = TextureDef::RGBA16F;
    }
    token_index++;
    if (tokens[token_index].contents != ")") {
        print_error(vformat("Unexpected token \"%s\" after texture format, expected \")\"", tokens[token_index].contents));
        return {};
    }
    token_index++;
    DISCARD_WHITESPACE_N;

    if (tokens[token_index].contents != "uniform") {
        print_error(vformat("Unexpected token \"%s\" in texture declaration, expected \"uniform\"", tokens[token_index].contents));
        return {};
    }
    token_index++;
    DISCARD_WHITESPACE_N;

    // TODO: support texture types other than image2D
    token_index++;
    DISCARD_WHITESPACE_N;

    if (tokens[token_index].token_type != Token::IDENTIFIER) {
        print_error(vformat("Unexpected token \"%s\" in texture declaration, expected identifier", tokens[token_index].contents));
        return {};
    }
    new_texture.name = tokens[token_index].contents;
    token_index++;
    DISCARD_WHITESPACE_N;
    if (tokens[token_index].contents != ";") {
        print_error(vformat("Unexpected token \"%s\" after texture declaration, expected \";\"", tokens[token_index].contents));
        return {};
    }
    token_index++;
    new_texture.resource = tex_def;
    return new_texture;
}


std::optional<ResourceNode> _parse_resource(const LocalVector<Token>& tokens, uint32_t &token_index) {
    ResourceNode new_resource;
    if (tokens[token_index].contents != "(") {
        print_error(vformat("Unexpected token \"%s\" after `layout`, expected \"(\"", tokens[token_index].contents));
        return {};
    }
    token_index++;
    DISCARD_WHITESPACE_N;
    switch (tokens[token_index].token_type) {
        case Token::BUFFERFORMAT: {
            return _parse_buffer(tokens, token_index);
        } break;
        case Token::TEXTUREFORMAT: {
            return _parse_texture(tokens, token_index);
        } break;
        default:
            print_error(vformat("Unexpected token \"%s\" in `layout`, expected format specifier", tokens[token_index].contents));
            return {};
    }
}

#define DUMP_CODE_NODE \
    if (!shared_code.is_empty()) { \
        ast.contents.push_back({(CodeNode){shared_code}, last_code_linenum});\
        shared_code.clear();\
    }\
    last_code_linenum = out_linenum


auto load_file(String &path) {
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file == NULL) {
        print_error("Invalid file path: " + path);
    }
    return file;
}

enum IfdefState {
    NONE,
    TRUE,
    FALSE,
    ELSEFALSE,
    ELSETRUE
};

struct MacroDef {
    LocalVector<StringName> args;
    LocalVector<Token> body;
};

std::optional<MacroDef> _process_macro(const LocalVector<Token>& tokens, uint32_t &token_index) {
    MacroDef new_macro;
    DISCARD_WHITESPACE;
    if (tokens[token_index].contents == "(") {
        token_index++;
        while (token_index < tokens.size()) {
            DISCARD_WHITESPACE;
            if (tokens[token_index].contents == "\\") {
                token_index++;
                DISCARD_WHITESPACE;
            }
            if (tokens[token_index].token_type == Token::NEWLINE) {
                print_error("Incomplete macro definition");
                return {};
            }
            if (tokens[token_index].token_type != Token::IDENTIFIER) {
                print_error(vformat("Unexpected token \"%s\" in `define`, expected argument name", tokens[token_index].contents));
                return {};
            }
            new_macro.args.push_back(tokens[token_index++].contents);
            DISCARD_WHITESPACE;
            if (tokens[token_index].contents == ")") {
                token_index++;
                break;
            }
            if (tokens[token_index].contents != ",") {
                print_error(vformat("Unexpected token \"%s\" in `define`, expected \",\"", tokens[token_index].contents));
                return {};
            } 
            token_index++;
        }  
        token_index++;
        DISCARD_WHITESPACE;
    }

    while (tokens[token_index].token_type != Token::NEWLINE && token_index < tokens.size()) {
        if (tokens[token_index].contents == "\\") {
            token_index++;
        }
        new_macro.body.push_back(tokens[token_index++]);
    }

    return new_macro;
}

std::optional<LocalVector<Token>> _expand_macro(const LocalVector<Token>& tokens, uint32_t &token_index, const HashMap<StringName, MacroDef> &macros, const MacroDef &curr_macro) {
    LocalVector<Token> pass_1_tokens;
    HashMap<StringName, LocalVector<Token>> args;
    
    
    if (curr_macro.args.size() > 0) {
        uint32_t arg_index = 0;
        if (tokens[token_index].contents != "(") {
            print_error(vformat("Unexpected token \"%s\" in macro usage, expected \"(\"", tokens[token_index].contents));
        }
        token_index++;
        LocalVector<Token> curr_arg_tokens;
        while (token_index < tokens.size()) {
            curr_arg_tokens.push_back(tokens[token_index++]);
            if (tokens[token_index].contents == ")") {
                if (curr_arg_tokens.size() > 0) {
                    args[curr_macro.args[arg_index]] = curr_arg_tokens;
                    String debug_string = "";
                    for (auto &token : curr_arg_tokens) {
                        debug_string += token.contents;
                    }
                    arg_index++;
                    curr_arg_tokens.clear();
                    
                }
                if (arg_index < curr_macro.args.size()) {
                    print_error(vformat("Too few arguments provided for macro, expected %d but was given %d", curr_macro.args.size(), arg_index));
                    return {};
                } else if (arg_index > curr_macro.args.size()) {
                    print_error(vformat("Too many arguments provided for macro, expected %d but was given %d", curr_macro.args.size(), arg_index));
                    return {};
                }
                break;
            }
            if (tokens[token_index].contents == ",") {
                if (curr_arg_tokens.size() > 0) {
                    args[curr_macro.args[arg_index]] = curr_arg_tokens;
                    String debug_string = "";
                    for (auto &token : curr_arg_tokens) {
                        debug_string += token.contents;
                    }
                    arg_index++;
                    curr_arg_tokens.clear();
                    token_index++;
                } else {
                    print_error("Unexpected \",\", expected argument value");
                    return {};
                }  
            }
        }  
        token_index++;
    }
    for (const auto& token : curr_macro.body) {
        if (args.has(token.contents)) {
            for (auto &arg_token : args[token.contents]) {
                pass_1_tokens.push_back(arg_token);
            }
        } else {
            pass_1_tokens.push_back(token);
        }
    }

    LocalVector<Token> out_tokens;
    uint32_t macro_token_index = 0;
    while (macro_token_index < pass_1_tokens.size()) {
        const Token& token = pass_1_tokens[macro_token_index++];
        if (macros.has(token.contents)) {
            auto macro_tokens = _expand_macro(pass_1_tokens, macro_token_index, macros, macros[token.contents]);
            if (!macro_tokens.has_value()) {
                return {};
            }
            for (auto &def_tok : *macro_tokens) {
                pass_1_tokens.push_back(def_tok);
            }
        } else {
            out_tokens.push_back(token);
        }
    }

    return out_tokens;
}

std::optional<LocalVector<Token>> FSLParser::_preprocess(String &path) {
    String path_to_file = path.get_base_dir() + '/';
    auto main_file = load_file(path);
    LocalVector<Ref<FileAccess>> included_files = {main_file};
    LocalVector<Token> tokens = _tokenize(main_file->get_as_text());

    uint32_t token_index = 0;
    LocalVector<Token> out_tokens;
    HashMap<StringName, MacroDef> macros;
    std::stack<IfdefState> ifdef_state_stack;
    ifdef_state_stack.push(NONE);
    
    while (token_index < tokens.size()) {
        const Token& token = tokens[token_index++];
        switch (token.token_type) {
            case Token::POUND: {
                bool valid_directive = false;
                if (ifdef_state_stack.top() != FALSE && ifdef_state_stack.top() != ELSEFALSE) {
                    if (tokens[token_index].contents == "include") {
                        valid_directive = true;
                        token_index++;
                        DISCARD_WHITESPACE;
                        if (tokens[token_index].contents != "\"") {
                            print_error(vformat("Unexpected token \"%s\" in include directive, expected \"", tokens[token_index].contents));
                            return {};
                        }
                        token_index++;
                        String relative_include_path = "";
                        while (tokens[token_index].contents != "\"" && token_index < tokens.size()) {
                            if (tokens[token_index].token_type == Token::NEWLINE) {
                                print_error("Incomplete include directive");
                                return {};
                            }
                            relative_include_path += tokens[token_index].contents;
                            token_index++;
                        }
                        token_index++;
                        DISCARD_WHITESPACE;
                        if (tokens[token_index].token_type != Token::NEWLINE) {
                            print_error(vformat("Unexpected token \"%s\" in include directive, expected newline", tokens[token_index].contents));
                            return {};
                        }
                        token_index++;
                        String true_include_path = path_to_file + relative_include_path;
                        uint32_t token_insert_index = token_index;
                        auto included_file = load_file(true_include_path);
                        if (!included_files.has(included_file)) {
                            included_files.push_back(included_file);
                            for (auto &token : _tokenize(included_file->get_as_text())) {
                                tokens.insert(token_insert_index++, token);
                            }
                        }
                        break;
                    }
                    if (tokens[token_index].contents == "define") {
                        valid_directive = true;
                        token_index++;
                        DISCARD_WHITESPACE;
                        if (tokens[token_index].token_type != Token::IDENTIFIER) {
                            print_error(vformat("Unexpected token \"%s\" in `define`, expected identifier", tokens[token_index].contents));
                            return {};
                        }
                        String define_name = tokens[token_index++].contents;
                        if (macros.has(define_name)) {
                            print_error(vformat("Redefinition of \"%s\", use `undef` first to change the definition", tokens[token_index].contents));
                            return {};
                        }
                        auto macro_def = _process_macro(tokens, token_index);
                        if (!macro_def.has_value()) {
                            return {};
                        }
                        macros[define_name] = *macro_def;
                    }
                    if (tokens[token_index].contents == "undef") {
                        valid_directive = true;
                        token_index++;
                        DISCARD_WHITESPACE;
                        if (macros.has(tokens[token_index].contents)) {
                            macros.erase(tokens[token_index].contents);
                        }
                        DISCARD_WHITESPACE;
                        if (tokens[token_index].token_type != Token::NEWLINE) {
                            print_error(vformat("Unexpected token \"%s\" after `undef` directive, expected newline", tokens[token_index].contents));
                            return {};
                        }
                        break;
                    }
                    if (tokens[token_index].contents == "ifdef") {
                        valid_directive = true;
                        token_index++;
                        DISCARD_WHITESPACE;
                        if (macros.has(tokens[token_index].contents)) {
                            ifdef_state_stack.push(TRUE);
                        } else {
                            ifdef_state_stack.push(FALSE);
                        }
                        token_index++;
                        DISCARD_WHITESPACE;
                        if (tokens[token_index].token_type != Token::NEWLINE) {
                            print_error(vformat("Unexpected token \"%s\" after `ifdef` directive, expected newline", tokens[token_index].contents));
                            return {};
                        }
                        token_index++;
                        break;
                    }
                }
                if (tokens[token_index].contents == "else") {
                    valid_directive = true;
                    if (ifdef_state_stack.top() == TRUE) {
                        ifdef_state_stack.top() = ELSEFALSE;
                    } else if (ifdef_state_stack.top() == FALSE) {
                        ifdef_state_stack.top() = ELSETRUE;
                    } else {
                        print_error("Unexpected `else` directive");
                    }
                    while(tokens[token_index++].token_type != Token::NEWLINE);
                    break;
                }
                if (tokens[token_index].contents == "endif") {
                    valid_directive = true;
                    if (ifdef_state_stack.top() != NONE) {
                        ifdef_state_stack.pop();
                    } else {
                        print_error("Unexpected `endif` directive");
                    }
                    while(tokens[token_index++].token_type != Token::NEWLINE);
                    break;
                }
                if (!valid_directive) {
                    print_error(vformat("\"%s\" is not a valid preprocessor directive", tokens[token_index].contents));
                    return {};
                }
            } break;
            case Token::SYMBOL: 
                if (ifdef_state_stack.top() != FALSE && ifdef_state_stack.top() != ELSEFALSE) {
                    if (token.contents == "/") {
                        if (tokens[token_index].contents == "/") {
                            while (tokens[token_index].token_type != Token::NEWLINE) token_index++;
                            break;
                        }
                        if (tokens[token_index].contents == "*") {
                            token_index++;
                            while (token_index < tokens.size()) {
                                if (tokens[token_index].contents == "*" && tokens[token_index + 1].contents == "/") {
                                    token_index += 2;
                                    break;
                                }
                                token_index++;
                            }
                            break;
                        }
                    }
                }
            default:
                if (ifdef_state_stack.top() != FALSE && ifdef_state_stack.top() != ELSEFALSE) {
                    if (macros.has(token.contents)) {
                        auto macro_tokens = _expand_macro(tokens, token_index, macros, macros[token.contents]);
                        if (!macro_tokens.has_value()) {
                            return {};
                        }
                        for (auto &def_tok : *macro_tokens) {
                            out_tokens.push_back(def_tok);
                        }
                    } else {
                        out_tokens.push_back(token);
                    }
                }
                break;
        }
    }
    return out_tokens;
}

void FSLParser::_parse_file(fslAST &ast, const LocalVector<Token> &tokens, uint32_t &out_linenum, uint32_t &last_code_linenum) {
    uint32_t token_index = 0;
    while (token_index < tokens.size()) {
        const Token& token = tokens[token_index++];

        switch (token.token_type) {
            case Token::NEWLINE:
                out_linenum++;
                shared_code.push_back(token);
                break;
            case Token::LAYOUT: {
                if (!shared_code.is_empty()) { 
                    ast.contents.push_back({(CodeNode){shared_code}, last_code_linenum});
                    shared_code.clear();
                }
                last_code_linenum = out_linenum;
                auto out = _parse_resource(tokens, token_index);
                if (!out.has_value()) return;
                ast.contents.push_back({*out, out_linenum});
                DISCARD_WHITESPACE_N;
            } break;
            case Token::KERNEL: {
                DUMP_CODE_NODE;
                auto out = _parse_kernel(tokens, token_index, out_linenum);
                if (!out.has_value()) return;
                ast.contents.push_back({*out, out_linenum});
                DISCARD_WHITESPACE_N;
            } break;
            case Token::IDENTIFIER:
            default:
                shared_code.push_back(token);
                break;
        }
    }
    
}

std::optional<fslAST> FSLParser::get_ast(String path) {
    auto ast = fslAST();
    uint32_t out_linenum = 0;
    uint32_t code_linenum = 0;

    FSLParser parser;
    auto parse_out = parser._preprocess(path);
    if (!parse_out.has_value()) {
        return {};
    }
    const LocalVector<Token> tokens = *parse_out;

    // String debug_text = "";
    // for (const auto &token : tokens) {
    //     debug_text += token.contents;
    // }
    // print_line(debug_text);
    
    parser._parse_file(ast, tokens, out_linenum, code_linenum);
	return ast;
}
