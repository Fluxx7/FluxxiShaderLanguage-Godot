#include "fsl_parser.h"
#include "../api/console_string.h"
#include "fsl_validator.h"

int32_t is_type(String identifier) {
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
    
    int32_t curr_index = 0;
    for (auto type_name : types) {
        if (identifier == type_name) {
            return curr_index;
        }
        curr_index++;
    }
    return -1;
}

TextureFormat token_to_tex_format(const Token* token) {
    switch (token->token_type) {
        case Token::TEXFORMAT_RGBA16F: return RGBA16F;
        case Token::TEXFORMAT_RGBA32F: return RGBA32F;
        default: return RGBA16F; // it should not be possible to reach here, but I don't know how to guarantee it
    }
}

std::optional<TextureType> token_to_tex_type(const Token* token) {
    switch (token->token_type) {
        case Token::TYPE_IMAGE2D: return TEXTURE2D;
        case Token::TYPE_IMAGE2DARRAY: return TEXTURE2DARRAY;
        default: return {};
    }
}

BufferFormat token_to_buf_format(const Token* token) {
    switch (token->token_type) {
        case Token::BUFFORMAT_STD140: return STD140;
        case Token::BUFFORMAT_STD430: return STD430;
        case Token::BUFFORMAT_VERTEX: return VERTEX;
        case Token::BUFFORMAT_INDEX: return INDEX;
        default: return STD140; // it should not be possible to reach here, but I don't know how to guarantee it
    }
}

std::optional<Ref<FileAccess>> load_file(String &path) {
    Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
    if (file == NULL) {
        print_error("Invalid file path: " + path);
        return {};
    }
    return file;
}

TypeRef FSLParser::_parse_type(TokenStream& stream) {
    TypeRef new_type;
    while (stream.peek().get_category() == Token::CATEGORY_SPECIFIER) {
        new_type.specifiers.push_back(stream.consume().get_token());
    } 
    auto& type_name = stream.peek();
    if (!stream.expect(Token::IDENTIFIER)) {
        print_fsl_err_expected_tok("type name", type_name);
    }
    new_type.type = type_name.get_token();
    auto array_scope = stream.descend_bracket(false);
    while (array_scope.has_value()) {
        new_type.array_dims.push_back(_parse_operation(std::move(*array_scope)));
        array_scope = stream.descend_bracket(false);
    }
	return new_type;
}

VariableDecl FSLParser::_parse_var_decl(TokenStream &&stream) {
    VariableDecl new_var;
    new_var.type = _parse_type(stream);
    auto& name_tok = stream.peek();
    if (!stream.expect(Token::IDENTIFIER)) {
        print_fsl_err_expected_tok_loc("valid identifier", "in variable declaration", name_tok);
        new_var.is_valid = false;
        return new_var;
    }
    new_var.name = name_tok.get_contents();
    if (!stream.at_end()) {
        print_fsl_err_unexpected_tok("variable declaration", stream.peek());
        new_var.is_valid = false;
        return new_var;
    }
	return new_var;
}

Operation FSLParser::_parse_op_segment(TokenStream &stream) {
    enum ExpectedOp {
        UNKNOWN,
        VARDECL,
        FUNCCALL,
        VARREF,
        OPER,
        LITERAL,
        FIELDACC,
        ARRINDEX,
        SUBOP
    };
    TokenStream check_stream = stream.trim();
    ExpectedOp op_type = UNKNOWN;

    // Determine operation type
    auto& first_token = check_stream.consume();
    switch (first_token.get_category()) {
        case Token::CATEGORY_SPECIFIER:
                op_type = VARDECL;
                break;
        case Token::CATEGORY_IDENTIFIER:{
            auto next_type = check_stream.peek().get_type();
            if (next_type == Token::IDENTIFIER) {
                op_type = VARDECL;
                break;
            }
            if (next_type == Token::SYMBOL_LEFTBRACKET) {
                check_stream.consume();
                while (check_stream.expect(Token::SYMBOL_LEFTBRACKET));
                if (check_stream.peek().get_type() == Token::IDENTIFIER) {
                    op_type = VARDECL;
                } else {
                    op_type = VARREF;
                }
                break;
            }
            if (next_type == Token::SYMBOL_LEFTPAREN && !check_stream.peek().has_leading_whitespace()) {
                op_type = FUNCCALL;
                break;
            }
            op_type = VARREF;
            break;
        }
        case Token::CATEGORY_SYMBOL_OP: {
            op_type = OPER;
            break;
        }
        case Token::CATEGORY_NUMBER:
            op_type = LITERAL;
            break;
        case Token::CATEGORY_SYMBOL_OTHER: {
            if (first_token.get_type() == Token::SYMBOL_PERIOD && !first_token.has_leading_whitespace()) {
                op_type = FIELDACC;
                break;
            }
            if (first_token.get_type() == Token::SYMBOL_LEFTPAREN) {
                op_type = SUBOP;
                break;
            }
            if (first_token.get_type() == Token::SYMBOL_LEFTBRACKET && !first_token.has_leading_whitespace()) {
                op_type = ARRINDEX;
                break;
            }
        }
        default:
            break;
    }

    // return associated node
    switch (op_type) {
        case VARDECL: {
            uint32_t var_decl_start = stream.get_index();
            uint32_t var_decl_len = 0;
            while (!stream.at_end()) {
                auto& token = stream.peek();
                switch (token.get_category()) {
                    case Token::CATEGORY_SPECIFIER:
                    case Token::CATEGORY_IDENTIFIER:
                        stream.consume();
                        var_decl_len++;
                        break;
                    default:
                        if (token.get_type() == Token::SYMBOL_LEFTBRACKET) {
                            stream.consume(); 
                            var_decl_len++;
                            break;
                        } else {
                            return _parse_var_decl(stream.get_slice(var_decl_start, var_decl_len));
                        }  
                }
            }
            return _parse_var_decl(stream.get_slice(var_decl_start, var_decl_len));
        } break;

        case FUNCCALL: {
            FuncCall new_func_call;
            new_func_call.name = stream.consume().get_contents();
            auto new_func_scope = stream.descend_paren();
            if (!new_func_scope.has_value()) {
                new_func_call.is_valid = false;
                return new_func_call;
            }
            auto& new_func_stream = *new_func_scope;
            while (!new_func_stream.at_end()) {
                Statement new_func_arg;
                uint32_t arg_start = new_func_stream.get_index();
                uint32_t arg_len = 0;
                new_func_stream.consume_until<Token::SYMBOL_COMMA>([&](){
                    arg_len++;
                });
                new_func_call.args.push_back(_parse_operation(new_func_stream.get_slice(arg_start, arg_len)));
            }
            return new_func_call;
        }

        case SUBOP: {
            auto sub_scope = stream.descend_paren();
            if (!sub_scope.has_value()) {
                OperationList fail_list;
                fail_list.is_valid = false;
                return fail_list;
            }
            return _parse_operation(std::move(*sub_scope));
        }

        case VARREF: {
            VariableRef var_ref;
            var_ref.name = stream.consume().get_contents();
            return var_ref;
        }

        case LITERAL: {
            // are any literals other than numerical literals needed for a shader language?
            Literal literal;
            literal.value += stream.consume().get_contents();
            while(!stream.at_end()) {
                auto& token = stream.peek();
                if ((token.get_type() == Token::SYMBOL_PERIOD || token.get_category() == Token::CATEGORY_NUMBER) && !token.has_leading_whitespace()) {
                    literal.value += stream.consume().get_contents();
                } else {
                    break;
                }
            }
            return literal;
        }

        case ARRINDEX: {
            ArrayIndex arr_index;
            auto index_scope = stream.descend_bracket();
            while (index_scope.has_value()) {
                arr_index.indices.push_back(_parse_operation(std::move(*index_scope)));
                index_scope = stream.descend_bracket();
            }
            return arr_index;
        }

        case OPER: {
            Operator op;
            String op_str = "";
            op_str += stream.consume().get_contents();
            while(!stream.at_end()) {
                if (stream.peek().get_category() == Token::CATEGORY_SYMBOL_OP && !stream.peek().has_leading_whitespace()) {
                    op_str += stream.consume().get_contents();
                } else {
                    break;
                }
            }
            op.symbol = op_str;
            return op;
        }

        case FIELDACC: {
            FieldAccess field_access;
            stream.consume();
            if(stream.peek().get_category() != Token::CATEGORY_IDENTIFIER) {
                field_access.is_valid = false;
                return field_access;
            }
            field_access.field_name = stream.consume().get_contents();
            return field_access;
        }



        default:
            UnknownOp fail_ref;
            fail_ref.is_valid = false;
            while(!stream.at_end()) {
                fail_ref.code.push_back(stream.consume());
            }
            return fail_ref;
    }
}

OperationList FSLParser::_parse_operation(TokenStream &&stream) {
    OperationList new_op_list;
    while (!stream.at_end()) {
        new_op_list.operations.push_back(_parse_op_segment(stream));
    }
	return new_op_list;
}

Expression FSLParser::_parse_expression(TokenStream &&stream, bool is_global) {
	switch(stream.peek().get_type()) {
        case Token::KEYWORD_IF: {
            stream.consume();
            uint32_t if_start = stream.get_index();
            uint32_t if_len = 0;
            while (!stream.at_end() && stream.peek().get_type() != Token::SYMBOL_LEFTBRACE && stream.peek().get_type() != Token::SYMBOL_SEMICOLON) {
                if_len++;
                stream.consume();
            }
            stream.consume();
            return _parse_if_statement(stream.get_slice(if_start, if_len));
        } break;
        case Token::KEYWORD_ELSE: {
            stream.consume();
            uint32_t else_start = stream.get_index();
            uint32_t else_len = 0;
            while (!stream.at_end() && stream.peek().get_type() != Token::SYMBOL_LEFTBRACE && stream.peek().get_type() != Token::SYMBOL_SEMICOLON) {
                else_len++;
                stream.consume();
            }
            stream.consume();
            return _parse_else_statement(stream.get_slice(else_start, else_len));
        } break;
        case Token::KEYWORD_FOR: {
            stream.consume();
            uint32_t for_start = stream.get_index();
            uint32_t for_len = 0;
            while (!stream.at_end() && stream.peek().get_type() != Token::SYMBOL_LEFTBRACE && stream.peek().get_type() != Token::SYMBOL_SEMICOLON) {
                for_len++;
                stream.consume();
            }
            stream.consume();
            return _parse_for_statement(stream.get_slice(for_start, for_len));
        } break;
        case Token::KEYWORD_RETURN: {
            ReturnExpression ret_expr;
            stream.consume();
            ret_expr.return_val = _parse_operation(stream.trim());
            return ret_expr;
        }
        case Token::SYMBOL_LEFTBRACE: {
            auto brace_scope = stream.descend_brace();
            return _parse_brace_scope(std::move(*brace_scope));
        } break;
        default:
            break;
    }
    uint32_t substream_start = stream.get_index();
    uint32_t substream_len = 0;
    bool is_function = false;
    while(!stream.at_end()) {
        switch(stream.peek().get_type()) {
            case Token::SYMBOL_LEFTBRACE:
                substream_len++;
                stream.consume();
                is_function = true;
                break;
            default:
                substream_len++;
                stream.consume();
                break;
        }
    }
    auto subslice = stream.get_slice(substream_start, substream_len);
    if (is_function) {
        return _parse_func_decl(subslice);
    } else {
        return _parse_operation(subslice);
    }
}

IfNode FSLParser::_parse_if_statement(TokenStream &&stream) {
    IfNode new_if;
    auto err_if = [&](){
        new_if.is_valid = false;
        return new_if;
    };
    
    auto args_scope = stream.descend_paren();
    if (!args_scope.has_value()) {
        print_fsl_err_expected_tok_loc("\'(\'", "after `if`", stream.peek());
        return err_if();
    }

    new_if.cond = _parse_operation(std::move(*args_scope));

    auto body_scope = stream.descend_brace();
    if (!body_scope.has_value()) {
        ScopeNode body_node;
        body_node.body.push_back(_parse_expression(stream.trim()));
        new_if.body = body_node;
        return new_if;
    }
    new_if.body = _parse_brace_scope(std::move(*body_scope));
	return new_if;
}

ElseNode FSLParser::_parse_else_statement(TokenStream &&stream) {
	ElseNode new_else;

    if (stream.expect(Token::KEYWORD_IF)) {
        ScopeNode body_node;
        uint32_t if_start = stream.get_index();
        uint32_t if_len = 0;
        while (!stream.at_end()) {
            stream.consume();
            if_len++;
        }
        body_node.body.push_back(_parse_if_statement(stream.get_slice(if_start, if_len)));
        new_else.body = body_node;
        return new_else;
    }

    auto body_scope = stream.descend_brace();
    if (!body_scope.has_value()) {
        ScopeNode body_node;
        body_node.body.push_back(_parse_expression(stream.trim()));
        new_else.body = body_node;
        return new_else;
    }
    new_else.body = _parse_brace_scope(std::move(*body_scope));
	return new_else;
}

ForNode FSLParser::_parse_for_statement(TokenStream &&stream) {
	ForNode new_for;
    auto err_for = [&](){
        new_for.is_valid = false;
        return new_for;
    };
    
    auto args_scope = stream.descend_paren();
    if (!args_scope.has_value()) {
        print_fsl_err_expected_tok_loc("\'(\'", "after `for`", stream.peek());
        return err_for();
    }
    {
        auto& args_stream = *args_scope;
        uint32_t statement_start = 0;
        uint32_t statement_len = 0;
        if (!args_stream.consume_until<Token::SYMBOL_SEMICOLON>([&](const Token * token) {
            statement_len++;
        })) {
            print_fsl_err_expected_tok_loc("\';\'", "in `for` statement", args_stream.peek());
            return err_for();
        }
        new_for.init = _parse_operation(args_stream.get_slice(statement_start, statement_len));

        statement_start = args_stream.get_index();
        statement_len = 0;
        if (!args_stream.consume_until<Token::SYMBOL_SEMICOLON>([&](const Token * token) {
            statement_len++;
        })) {
            print_fsl_err_expected_tok_loc("\';\'", "in `for` statement", args_stream.peek());
            return err_for();
        }
        new_for.cond = _parse_operation(args_stream.get_slice(statement_start, statement_len));

        statement_start = args_stream.get_index();
        statement_len = 0;
        while (!args_stream.at_end()) {
            statement_len++;
            args_stream.consume();
        }
        new_for.post = _parse_operation(args_stream.get_slice(statement_start, statement_len));
    }

    auto body_scope = stream.descend_brace();
    if (!body_scope.has_value()) {
        ScopeNode body_node;
        body_node.body.push_back(_parse_expression(stream.trim()));
        new_for.body = body_node;
        return new_for;
    }
    new_for.body = _parse_brace_scope(std::move(*body_scope));
	return new_for;
}

void FSLParser::_parse_bracket_scope(const TokenScope *scope, Statement &out_statement) {
	for (auto& token : scope->flatten()) {
        out_statement.push_back(token);
    }
}

void FSLParser::_parse_paren_scope(const TokenScope* scope, Statement& out_statement) {
    for (auto& token : scope->flatten()) {
        out_statement.push_back(token);
    }
}

ScopeNode FSLParser::_parse_brace_scope(TokenStream &&stream) {
    ScopeNode new_scope;
    auto err_scope = [&](){
        new_scope.is_valid = false;
        return new_scope;
    };
    uint32_t next_expression_start = 0;
    uint32_t next_expression_len = 0;
    auto flush_expression = [&](){
        if (next_expression_len != 0) {
            new_scope.body.push_back(_parse_expression(stream.get_slice(next_expression_start, next_expression_len)));
            next_expression_start = stream.get_index();
            next_expression_len = 0;
        }
    };
    auto push_expression = [&](){
        if (next_expression_len == 0) {
            next_expression_start = stream.get_index();
        }
        next_expression_len++;
    };

    while (!stream.at_end()) {
        switch(stream.peek().get_type()) {
            case Token::SYMBOL_LEFTBRACE:
                push_expression();
            case Token::SYMBOL_SEMICOLON:
                flush_expression();
                stream.consume();
                break;
            default:
                push_expression();
                stream.consume();
                break;
        }
    }
    flush_expression();
	return new_scope;

}

FunctionDecl FSLParser::_parse_func_decl(Slice<TokenTree> tokens) {
    FunctionDecl new_func;
    TokenStream stream = tokens.get_stream();
    auto err_func = [&]() {
        new_func.is_valid = false;
        return new_func;
    };

    new_func.return_type = _parse_type(stream);
    auto& name_token = stream.peek();
    if (!stream.expect(Token::IDENTIFIER)) {
        print_fsl_err_expected_tok_loc("valid identifier", "in function declaration", stream.peek());
        return err_func();
    }

    new_func.name = name_token.get_contents();

    {
        auto arg_scope = stream.descend_paren();
        if (!arg_scope.has_value()) {
            print_fsl_err_expected_tok_loc("\'(\'", "after function name", stream.peek());
            return err_func();
        }
        auto& arg_stream = *arg_scope;
        while(!arg_stream.at_end()) {
            uint32_t tok_index = arg_stream.get_index();

            auto& token = arg_stream.consume();
            switch (token.get_type()) {
                case Token::IDENTIFIER:
                case Token::SPECIFIER_IN:
                case Token::SPECIFIER_OUT:
                case Token::SPECIFIER_INOUT:
                case Token::SPECIFIER_CONST: {
                    uint32_t var_decl_len = 1;

                    // it's ok if this terminates due to EOS
                    arg_stream.consume_until<Token::SYMBOL_COMMA>([&](){ var_decl_len++; });
                    new_func.args.push_back(_parse_var_decl(arg_stream.get_slice(tok_index, var_decl_len)));
                } break;
                default:
                    print_fsl_err_unexpected_tok("function arguments", arg_stream.peek());
                    return err_func();
            }
        }
    }

    {
        auto body_scope = stream.descend_brace();
        if (!body_scope.has_value()) {
            print_fsl_err_expected_tok_loc("\'{\'", "in function declaration", stream.peek());
            return err_func();
        }
        new_func.code = _parse_brace_scope(std::move(*body_scope));
    }

	return new_func;
}

void FSLParser::_parse_texture(const Token *layout_token, TokenStream &stream, ResourceNode &res_node) {
	TextureDef new_texture;
    new_texture.format = token_to_tex_format(layout_token);
    auto err_texture = [&](){
        new_texture.is_valid = false;
        res_node.resource = new_texture;
        return;
    };

    if(!stream.expect(Token::KEYWORD_UNIFORM)) {
        print_fsl_err_expected_tok_loc("uniform", "in texture declaration", stream.peek());
        return err_texture();
    }

    auto tex_type_token = stream.consume().get_token();
    auto tex_type = token_to_tex_type(tex_type_token);
    if (!tex_type.has_value()) {
        print_fsl_err_expected_tok_loc("valid image type", "in texture declaration", stream.peek());
        return err_texture();
    }
    new_texture.type = *tex_type;

    auto tex_name = stream.peek();
    if(!stream.expect(Token::IDENTIFIER)) {
        print_fsl_err_expected_tok_loc("valid identifier for texture", "in texture declaration", stream.peek());
        return err_texture();
    }
    res_node.name = tex_name.get_contents();
    res_node.resource = new_texture;
}

void FSLParser::_parse_buffer(const Token *layout_token, TokenStream &stream, ResourceNode &res_node) {
    BufferDef new_buffer;
    new_buffer.layout = token_to_buf_format(layout_token);
    auto err_buffer = [&](){
        new_buffer.is_valid = false;
        res_node.resource = new_buffer;
        return;
    };

    switch (auto token = stream.consume(); token.get_type()) {
        case Token::KEYWORD_UNIFORM:
            new_buffer.buftype = UNIFORM;
            break;
        case Token::KEYWORD_BUFFER:
            new_buffer.buftype = STORAGE;
            break;
        default:
            print_fsl_err_expected_tok_loc("uniform or buffer", "in buffer declaration", token);
            return err_buffer();
    }

    auto buf_name = stream.peek();
    if(!stream.expect(Token::IDENTIFIER)) {
        print_fsl_err_expected_tok_loc("valid identifier for buffer", "in buffer declaration", stream.peek());
        return err_buffer();
    }
    res_node.name = buf_name.get_contents();

    auto buffer_body = stream.descend_brace();
    if(!buffer_body.has_value()) {
        print_fsl_err_expected_tok_loc("\'{\'", "in buffer declaration", stream.peek());
        return err_buffer();
    }
    {
        auto& buf_stream = *buffer_body;
        while(!buf_stream.at_end()) {
            uint32_t var_start = buf_stream.get_index();
            uint32_t var_len = 0;
            if (!buf_stream.consume_until<Token::SYMBOL_SEMICOLON>([&](){ var_len++; })) {
                print_fsl_err_unexpected_tok("buffer declaration", buf_stream.peek());
                return err_buffer();
            }
            new_buffer.fields.push_back(_parse_var_decl(buf_stream.get_slice(var_start, var_len)));
        }
    }
    if(!stream.expect_not(Token::IDENTIFIER)) {
        new_buffer.buffer_name = stream.consume().get_contents();
    }
    res_node.resource = new_buffer;
}

ResourceNode FSLParser::_parse_resource(Slice<TokenTree> tokens) {
    ResourceNode new_resource;
    TokenStream stream = tokens.get_stream();
    ResourceType res_type;
    const Token* layout_token;
    {
        auto layout_scope = stream.descend_paren();
        if (!layout_scope.has_value()) {
            print_fsl_err_expected_tok_loc("'('", "after `layout`", stream.peek());
            new_resource.is_valid = false;
            return new_resource;
        }
        TokenStream& layout_stream = *layout_scope;
        switch (layout_stream.peek().get_category()) {
            case Token::CATEGORY_BUFFERFORMAT:
                res_type = RESTYPE_BUFFER;
                break;
            case Token::CATEGORY_TEXTUREFORMAT:
                res_type = RESTYPE_TEXTURE;
                break;
            default:
                print_fsl_err_unexpected_tok("resource layout", layout_stream.peek());
                new_resource.is_valid = false;
                return new_resource;
        }
        layout_token = layout_stream.consume().get_token();
        if (!layout_stream.at_end()) {
            print_fsl_err_unexpected_tok("resource layout", layout_stream.peek());
        }
    }
    
    if (res_type == RESTYPE_TEXTURE) {
        _parse_texture(layout_token, stream, new_resource);
    } else {
        _parse_buffer(layout_token, stream, new_resource);
    }

    if (!stream.at_end()) {
        print_fsl_err_unexpected_tok("resource declaration", stream.peek());
        new_resource.is_valid = false;
        return new_resource;
    }
	return new_resource;
}

KernelNode FSLParser::_parse_kernel(Slice<TokenTree> tokens) {
    KernelNode new_kernel;
    TokenStream stream = tokens.get_stream();
    auto err_kernel = [&](){
        new_kernel.is_valid = false;
        return new_kernel;
    };

    auto layout_scope = stream.descend_bracket(false);
    if (!layout_scope.has_value()) {
        print_fsl_err_expected_tok_loc("kernel size", "in kernel declaration", stream.peek());
        return err_kernel();
    }
    {
        auto& layout_stream = *layout_scope;
        uint32_t next_thread_size_start = layout_stream.get_index();
        uint32_t next_thread_size_len = 0;
        if(!layout_stream.consume_until<Token::SYMBOL_COMMA>([&](){ 
            next_thread_size_len++;
        })) {
            print_fsl_err_expected_tok_loc(",", "in kernel size declaration", layout_stream.peek());
            return err_kernel();
        }
        new_kernel.local_x_threads = _parse_operation(layout_stream.get_slice(next_thread_size_start, next_thread_size_len));

        next_thread_size_start = layout_stream.get_index();
        next_thread_size_len = 0;
        if(!layout_stream.consume_until<Token::SYMBOL_COMMA>([&](){ 
            next_thread_size_len++;
        })) {
            print_fsl_err_expected_tok_loc(",", "in kernel size declaration", layout_stream.peek());
            return err_kernel();
        }
        new_kernel.local_y_threads = _parse_operation(layout_stream.get_slice(next_thread_size_start, next_thread_size_len));
        new_kernel.local_z_threads = _parse_operation(layout_stream.trim());
    }

    auto& kernel_name = stream.peek();
    if (!stream.expect(Token::IDENTIFIER)) {
        print_fsl_err_expected_tok_loc("valid identifier", "in kernel declaration", kernel_name);
        return err_kernel();
    }
    new_kernel.name = kernel_name.get_contents();
    
    {
        auto push_constant_scope = stream.descend_paren(false);
        if (!push_constant_scope.has_value()) {
            print_fsl_err_expected_tok_loc("\'(\'", "after kernel identifier", stream.peek());
            return err_kernel();
        }
        auto& pc_stream = *push_constant_scope;
        while(!pc_stream.at_end()) {
            uint32_t tok_index = pc_stream.get_index();

            auto& token = pc_stream.consume();
            switch (token.get_type()) {
                case Token::IDENTIFIER: {
                    if (pc_stream.peek().get_type() == Token::SYMBOL_COLON) {
                        auto name_binding_key = token.get_contents();
                        pc_stream.consume();
                        auto& bound_token = pc_stream.peek();
                        if (!pc_stream.expect(Token::IDENTIFIER)) {
                            print_fsl_err_expected_tok_loc("name to bind", "in kernel name binding", bound_token);
                            return err_kernel();
                        }
                        new_kernel.name_bindings[name_binding_key] = bound_token.get_contents();
                        if(!pc_stream.expect(Token::SYMBOL_COMMA)) {
                            if (!pc_stream.at_end()) {
                                print_fsl_err_expected_tok_loc("\',\'", "in kernel arguments", pc_stream.peek());
                                return err_kernel();
                            }
                        }
                        break;
                    }
                    uint32_t var_decl_len = 1;

                    // it's ok if this terminates due to EOS
                    pc_stream.consume_until<Token::SYMBOL_COMMA>([&](){ var_decl_len++; });
                    new_kernel.push_constants.push_back(_parse_var_decl(pc_stream.get_slice(tok_index, var_decl_len)));
                } break;
                default:
                    print_fsl_err_unexpected_tok("kernel arguments", pc_stream.peek());
                    return err_kernel();
            }
            
        }
    }

    {
        auto body_scope = stream.descend_brace();
        if (!body_scope.has_value()) {
            print_fsl_err_expected_tok_loc("\'{\'", "in kernel declaration", stream.peek());
            return err_kernel();
        }
        new_kernel.code = _parse_brace_scope(std::move(*body_scope));
    }

	return new_kernel;
}

void FSLParser::_parse_file(fslAST &ast, Span<TokenTree> tokens) {
    auto stream = TokenStream(tokens, 0, tokens.size());
    uint32_t next_expression_start = 0;
    uint32_t next_expression_len = 0;
    auto flush_expression = [&](){
        if (next_expression_len != 0) {
            GlobalDeclaration new_global_decl;
            new_global_decl.value = _parse_expression(stream.get_slice(next_expression_start, next_expression_len));
            ast.contents.push_back(new_global_decl);
            next_expression_start = stream.get_index();
            next_expression_len = 0;
        }
    };
    auto push_expression = [&](){
        if (next_expression_len == 0) {
            next_expression_start = stream.get_index();
        }
        next_expression_len++;
    };

    while (!stream.at_end()) {
        auto& token = stream.peek();
        switch(token.get_type()) {
            case Token::KEYWORD_KERNEL: {
                flush_expression();
                stream.consume();
                auto kernel_start = stream.get_index();
                auto kernel_len = 0;
                if (!stream.consume_until<Token::SYMBOL_LEFTBRACE>([&](){ kernel_len++; })) {
                    print_fsl_err("Unexpected end of file", stream.peek().get_debug_info().row);
                    return;
                }
                kernel_len++;
                GlobalDeclaration new_global_decl;
                new_global_decl.value = _parse_kernel(Slice<TokenTree>::make_slice(tokens, kernel_start, kernel_len));
                ast.contents.push_back(new_global_decl);
            } break;
            case Token::KEYWORD_LAYOUT: {
                flush_expression();
                stream.consume();
                auto resource_start = stream.get_index();
                auto resource_len = 0;
                while(stream.expect_not(Token::SYMBOL_SEMICOLON)) {
                    resource_len++;
                }
                if (!stream.expect(Token::SYMBOL_SEMICOLON)) {
                    print_fsl_err("Unexpected end of file", stream.peek().get_debug_info().row);
                    return;
                }
                GlobalDeclaration new_global_decl;
                new_global_decl.value = _parse_resource(Slice<TokenTree>::make_slice(tokens, resource_start, resource_len));
                ast.contents.push_back(new_global_decl);
            } break;    
            case Token::SYMBOL_LEFTBRACE:
                push_expression();
            case Token::SYMBOL_SEMICOLON:
                flush_expression();
                stream.consume();
                break;
            default:
                push_expression();
                stream.consume();
                break;
        }
    }
    flush_expression();
}

void print_token_tree(LocalVector<TokenTree>& tree, ConsoleString& output) {
    for (auto& subtree : tree) {
        std::visit(overload{
            [&](const Token*& token) {
            },
            [&](TokenScope& scope) {
                output.add_line("Scope %s", scope.open->contents);
                output.indent();
                print_token_tree(scope.scope_contents, output);
                output.unindent();
                output.add_line("%s", scope.close->contents);
            }
        }, subtree.node);
    }
}

TokenScope FSLParser::_collect_scope(Span<Token> tokens, uint32_t &token_index) {
    TokenScope out_scope;
    out_scope.open = &tokens[token_index++];
    while (token_index < tokens.size()) {
        auto& token = tokens[token_index];
        switch (token.token_type) {
            case Token::SYMBOL_LEFTBRACE:
            case Token::SYMBOL_LEFTBRACKET:
            case Token::SYMBOL_LEFTPAREN:
                out_scope.scope_contents.push_back(_collect_scope(tokens, token_index));
                break;
            case Token::SYMBOL_RIGHTBRACE:
            case Token::SYMBOL_RIGHTBRACKET:
            case Token::SYMBOL_RIGHTPAREN:
                out_scope.close = &token;
                token_index++;
                return out_scope;
            case Token::WHITESPACE:
            case Token::NEWLINE:
                token_index++;
                break;
            default:
                out_scope.scope_contents.push_back(&token);
                token_index++;
                break;
        }
    }
    print_fsl_err("Unexpected end of file", tokens[token_index - 1].debug_info.row);
	return out_scope;
}

LocalVector<TokenTree> FSLParser::_collect_scopes(Span<Token> tokens) {
    LocalVector<TokenTree> out_tokens;
    uint32_t token_index = 0;
    while (token_index < tokens.size()) {
        auto& token = tokens[token_index];
        switch (token.token_type) {
            case Token::SYMBOL_LEFTBRACE:
            case Token::SYMBOL_LEFTBRACKET:
            case Token::SYMBOL_LEFTPAREN:
                out_tokens.push_back(_collect_scope(tokens, token_index));
                break;
            case Token::WHITESPACE:
            case Token::NEWLINE:
                token_index++;
                break;
            default:
                out_tokens.push_back(&token);
                token_index++;
                break;
        }
    }
	return out_tokens;
}

std::optional<fslAST> FSLParser::get_ast(String path) {
    auto ast = fslAST();

    FSLParser parser;
    auto parse_out = parser._preprocess(path);
    if (!parse_out.has_value()) {
        return {};
    }
    ast.tokens = std::move(*parse_out);

    LocalVector<TokenTree> tree = parser._collect_scopes(ast.tokens.span());
    parser._parse_file(ast, tree.span());

    FSLValidator validator;
    if (!validator.validate_ast(ast)) {
        return {};
    }
	return std::move(ast);
}

// ************************************
// *********** PREPROCESSOR ***********
// ************************************

#pragma region preprocessor

inline void _discard_whitespace(const LocalVector<Token>& tokens, uint32_t &token_index) {
    while (token_index < tokens.size() && tokens[token_index].token_type == Token::WHITESPACE) {
        token_index++;
    }
}

#define DISCARD_WHITESPACE _discard_whitespace(tokens, token_index)

std::optional<FSLParser::MacroDef> FSLParser::_process_macro(const LocalVector<Token>& tokens, uint32_t &token_index) {
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
                print_fsl_err("Incomplete macro definition", tokens[token_index].debug_info.row);
                return {};
            }
            if (!tokens[token_index].contents.is_valid_ascii_identifier()) {
                print_fsl_err_expected_tok_loc("argument name", "in `define`", &tokens[token_index]);
                return {};
            }
            new_macro.args.push_back(tokens[token_index++].contents);
            DISCARD_WHITESPACE;
            if (tokens[token_index].contents == ")") {
                token_index++;
                break;
            }
            if (tokens[token_index].contents != ",") {
                print_fsl_err_expected_tok_loc("\",\"", "in `define`", &tokens[token_index]);
                return {};
            } 
            token_index++;
        }  
        token_index++;
        DISCARD_WHITESPACE;
    }

    while (token_index < tokens.size() && tokens[token_index].token_type != Token::NEWLINE) {
        if (tokens[token_index].contents == "\\") {
            token_index++;
        }
        new_macro.body.push_back(tokens[token_index++]);
    }

    return new_macro;
}

std::optional<LocalVector<Token>> FSLParser::_expand_macro(const LocalVector<Token>& tokens, uint32_t &token_index, const HashMap<StringName, MacroDef> &macros, const MacroDef &curr_macro) {
    LocalVector<Token> pass_1_tokens;
    HashMap<StringName, LocalVector<Token>> args;
    
    if (curr_macro.args.size() > 0) {
        uint32_t arg_index = 0;
        if (tokens[token_index].contents != "(") {
            print_fsl_err_expected_tok_loc("\"(\"", "in macro usage", &tokens[token_index]);
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
                    print_fsl_err("Too few arguments provided for macro, expected %d but was given %d", tokens[token_index].debug_info.row, curr_macro.args.size(), arg_index);
                    return {};
                } 
                break;
            }
            if (tokens[token_index].contents == ",") {
                if (curr_arg_tokens.size() > 0) {
                    args[curr_macro.args[arg_index]] = curr_arg_tokens;
                    arg_index++;
                    if (arg_index >= curr_macro.args.size()) {
                        print_fsl_err("Too many arguments provided for macro, expected %d", tokens[token_index].debug_info.row, curr_macro.args.size());
                        return {};
                    }
                    curr_arg_tokens.clear();
                    token_index++;
                } else {
                    print_fsl_err_expected_tok("argument value", &tokens[token_index]);
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
    auto main_file_opt = load_file(path);
    if (!main_file_opt.has_value()) {
        print_error("Failed to load file at \"%s\"", path);
        return {};
    }
    auto main_file = *main_file_opt;
    LocalVector<Ref<FileAccess>> included_files = {main_file};
    FSLLexer lexer = FSLLexer();
    LocalVector<Token> tokens = lexer.tokenize(path.get_file() ,main_file->get_as_text());

    uint32_t token_index = 0;
    LocalVector<Token> out_tokens;
    HashMap<StringName, MacroDef> macros;
    std::stack<IfdefState> ifdef_state_stack;
    ifdef_state_stack.push(NONE);
    current_context.file_name = path.get_file();
    
    while (token_index < tokens.size()) {
        const Token& token = tokens[token_index++];
        switch (token.token_type) {
            case Token::SYMBOL_POUND: {
                bool valid_directive = false;
                if (ifdef_state_stack.top() != FALSE && ifdef_state_stack.top() != ELSEFALSE) {
                    if (tokens[token_index].contents == "include") {
                        valid_directive = true;
                        token_index++;
                        DISCARD_WHITESPACE;
                        if (tokens[token_index].contents != "\"") {
                            print_fsl_err_expected_tok_loc("\"", "in include directive", &tokens[token_index]);
                            return {};
                        }
                        token_index++;
                        String relative_include_path = "";
                        while (token_index < tokens.size() && tokens[token_index].contents != "\"") {
                            if (tokens[token_index].token_type == Token::NEWLINE) {
                                print_fsl_err("Incomplete include directive", tokens[token_index].debug_info.row);
                                return {};
                            }
                            relative_include_path += tokens[token_index].contents;
                            token_index++;
                        }
                        token_index++;
                        DISCARD_WHITESPACE;
                        if (tokens[token_index].token_type != Token::NEWLINE) {
                            print_fsl_err_expected_tok_loc("newline", "in include directive", &tokens[token_index]);
                            return {};
                        }
                        token_index++;
                        String true_include_path = path_to_file + relative_include_path;
                        uint32_t token_insert_index = token_index;
                        auto included_file_opt = load_file(true_include_path);
                        if (!included_file_opt.has_value()) {
                            print_error("Failed to load file at \"%s\"", true_include_path);
                            return {};
                        }
                        auto included_file = *included_file_opt;
                        if (!included_files.has(included_file)) {
                            included_files.push_back(included_file);
                            for (auto &token : lexer.tokenize(relative_include_path.get_file(), included_file->get_as_text())) {
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
                            print_fsl_err_expected_tok_loc("identifier", "in `define`", &tokens[token_index]);
                            return {};
                        }
                        String define_name = tokens[token_index++].contents;
                        if (macros.has(define_name)) {
                            print_fsl_err("Redefinition of \"%s\" at line %d, use `undef` first to change the definition", tokens[token_index].debug_info.row, tokens[token_index].contents);
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
                            print_fsl_err_expected_tok_loc("newline", "after `undef` directive", &tokens[token_index]);
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
                            print_fsl_err_expected_tok_loc("newline", "after `ifdef` directive", &tokens[token_index]);
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
                        print_fsl_err("Unexpected `else` directive", tokens[token_index].debug_info.row);
                        return {};
                    }
                    while(tokens[token_index++].token_type != Token::NEWLINE);
                    break;
                }
                if (tokens[token_index].contents == "endif") {
                    valid_directive = true;
                    if (ifdef_state_stack.top() != NONE) {
                        ifdef_state_stack.pop();
                    } else {
                        print_fsl_err("Unexpected `endif` directive", tokens[token_index].debug_info.row);
                        return {};
                    }
                    while(tokens[token_index++].token_type != Token::NEWLINE);
                    break;
                }
                if (!valid_directive) {
                    print_fsl_err("\"%s\" is not a valid preprocessor directive", tokens[token_index].debug_info.row, tokens[token_index].contents);
                    return {};
                }
            } break;
            case Token::SYMBOL_SLASH: 
                if (ifdef_state_stack.top() != FALSE && ifdef_state_stack.top() != ELSEFALSE) {
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
#pragma endregion

