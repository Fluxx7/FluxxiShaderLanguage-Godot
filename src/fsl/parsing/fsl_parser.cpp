#include "fsl_parser.h"
#include "../api/console_string.h"
#include "fsl_validator.h"
#include "fsl_resource.h"
using namespace AST;

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

optional<TextureType> token_to_tex_type(const Token* token) {
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

optional<Ref<FileAccess>> load_file(String &path) {
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
    while (stream.descend_bracket(false) == true) {
        new_type.array_dims.push_back(_parse_operation(stream.clip_and_ascend()));
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
    if (stream.expect(Token::SYMBOL_COLON)) {
        new_var.annotations = _parse_annotation_list(stream);
    } else if (!stream.at_end()) {
        print_fsl_err_unexpected_tok("variable declaration", stream.peek());
        new_var.is_valid = false;
        return new_var;
    }
	return new_var;
}

HashMap<StringName, Args> AST::FSLParser::_parse_annotation_list(TokenStream &stream) {
    HashMap<StringName, Args> annotations = {};
    while (!stream.at_end()) {
        if (stream.peek().get_type() != Token::IDENTIFIER) {
            print_fsl_err_expected_tok_loc("valid identifier", "in annotation list", stream.peek());
            return {};
        }
        StringName annotation_name = stream.consume().get_contents();

        if (stream.descend_paren()) {
            annotations[annotation_name] = _parse_args(stream.clip_and_ascend());
        } else {
            annotations[annotation_name] = Args();
        }
        if (!stream.at_end() && !stream.expect(Token::SYMBOL_COMMA)) {
            print_fsl_err_expected_tok_loc("','", "in annotation list", stream.peek());
            return {};
        }
    }
	return annotations;
}

Args AST::FSLParser::_parse_args(TokenStream &&stream) {
    Args new_args;
    while (!stream.at_end()) {
        Statement new_arg;
        stream.start_slice();
        stream.consume_until<Token::SYMBOL_COMMA>(false);
        new_args.push_back(_parse_operation(stream.get_slice()));
        stream.consume();
    }
	return new_args;
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
    TokenStream check_stream = stream.clip();
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
            if (next_type == Token::BRACKET_OPEN) {
                check_stream.consume();
                while (check_stream.expect(Token::BRACKET_OPEN));
                if (check_stream.peek().get_type() == Token::IDENTIFIER) {
                    op_type = VARDECL;
                } else {
                    op_type = VARREF;
                }
                break;
            }
            if (next_type == Token::PAREN_OPEN && !check_stream.peek().has_leading_whitespace()) {
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
        case Token::CATEGORY_LITERAL:
            op_type = LITERAL;
            break;
        case Token::CATEGORY_SYMBOL_OTHER: {
            if (first_token.get_type() == Token::SYMBOL_PERIOD) {
                if (first_token.has_leading_whitespace()) {
                    op_type = LITERAL;
                } else {
                    op_type = FIELDACC;
                }
                
                break;
            }
            if (first_token.get_type() == Token::PAREN_OPEN) {
                op_type = SUBOP;
                break;
            }
            if (first_token.get_type() == Token::BRACKET_OPEN && !first_token.has_leading_whitespace()) {
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
            stream.start_slice();
            while (!stream.at_end()) {
                auto& token = stream.peek();
                switch (token.get_category()) {
                    case Token::CATEGORY_SPECIFIER:
                    case Token::CATEGORY_IDENTIFIER:
                        stream.consume();
                        break;
                    default:
                        if (token.get_type() == Token::BRACKET_OPEN) {
                            stream.consume();
                            break;
                        } else {
                            if (stream.expect(Token::SYMBOL_COLON)) {
                                bool end = false;
                                while (!end && !stream.at_end()) {
                                    auto curr_token = stream.peek();
                                    switch (curr_token.get_type()) {
                                        case Token::IDENTIFIER:
                                        case Token::PAREN_OPEN:
                                        case Token::SYMBOL_COMMA:
                                            stream.consume();
                                            break;
                                        default: 
                                            end = true;
                                            break;
                                    }
                                }
                            }
                            return _parse_var_decl(stream.get_slice());
                        }  
                }
            }
            return _parse_var_decl(stream.get_slice());
        } break;

        case FUNCCALL: {
            FuncCall new_func_call;
            new_func_call.name = stream.consume().get_contents();
            if (!stream.descend_paren()) {
                new_func_call.is_valid = false;
                return new_func_call;
            }
            new_func_call.args = _parse_args(stream.clip_and_ascend());
            stream.ascend();
            return new_func_call;
        }

        case SUBOP: {
            if (!stream.descend_paren()) {
                OperationList fail_list;
                fail_list.is_valid = false;
                return fail_list;
            }
            return _parse_operation(stream.clip_and_ascend());
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
                if ((token.get_type() == Token::SYMBOL_PERIOD || token.get_category() == Token::CATEGORY_LITERAL) && !token.has_leading_whitespace()) {
                    literal.value += stream.consume().get_contents();
                } else {
                    if (literal.value[literal.value.length() - 1] == 'e' && (token.get_type() == Token::SYMBOL_DASH || token.get_type() == Token::SYMBOL_PLUS)) {
                        literal.value += stream.consume().get_contents();
                    } else {
                        break;
                    }
                }
            }
            return literal;
        }

        case ARRINDEX: {
            ArrayIndex arr_index;
            while (stream.descend_bracket()) {
                arr_index.indices.push_back(_parse_operation(stream.clip_and_ascend()));
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
            stream.start_slice();
            while (!stream.at_end() && stream.peek().get_type() != Token::BRACE_OPEN && stream.peek().get_type() != Token::SYMBOL_SEMICOLON) {
                stream.consume();
            }
            stream.expect(Token::BRACE_OPEN); // consumes the left brace if there is one
            return _parse_if_statement(stream.get_slice());
        } break;
        case Token::KEYWORD_ELSE: {
            stream.consume();
            stream.start_slice();
            while (!stream.at_end() && stream.peek().get_type() != Token::BRACE_OPEN && stream.peek().get_type() != Token::SYMBOL_SEMICOLON) {
                stream.consume();
            }
            stream.expect(Token::BRACE_OPEN);
            return _parse_else_statement(stream.get_slice());
        } break;
        case Token::KEYWORD_FOR: {
            stream.consume();
            stream.start_slice();
            while (!stream.at_end() && stream.peek().get_type() != Token::BRACE_OPEN && stream.peek().get_type() != Token::SYMBOL_SEMICOLON) {
                stream.consume();
            }
            stream.expect(Token::BRACE_OPEN);
            return _parse_for_statement(stream.get_slice());
        } break;
        case Token::KEYWORD_RETURN: {
            ReturnExpression ret_expr;
            stream.consume();
            ret_expr.return_val = _parse_operation(stream.clip());
            return ret_expr;
        }
        case Token::BRACE_OPEN: {
            stream.descend_brace();
            return _parse_brace_scope(stream.clip());
        } break;
        default:
            break;
    }
    stream.start_slice();
    bool is_function = false;
    while(!stream.at_end()) {
        switch(stream.peek().get_type()) {
            case Token::BRACE_OPEN:
                stream.consume();
                is_function = true;
                break;
            default:
                stream.consume();
                break;
        }
    }
    auto subslice = stream.get_slice();
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
    
    if (!stream.descend_paren()) {
        print_fsl_err_expected_tok_loc("\'(\'", "after `if`", stream.peek());
        return err_if();
    }

    new_if.cond = _parse_operation(stream.clip_and_ascend());

    if (!stream.descend_brace()) {
        ScopeNode body_node;
        body_node.body.push_back(_parse_expression(stream.clip()));
        new_if.body = body_node;
        new_if.is_scoped = false;
        return new_if;
    }
    new_if.body = _parse_brace_scope(std::move(stream));
	return new_if;
}

ElseNode FSLParser::_parse_else_statement(TokenStream &&stream) {
	ElseNode new_else;

    if (stream.expect(Token::KEYWORD_IF)) {
        ScopeNode body_node;
        body_node.body.push_back(_parse_if_statement(stream.clip()));
        new_else.body = body_node;
        new_else.is_scoped = false;
        return new_else;
    }

    if (!stream.descend_brace()) {
        ScopeNode body_node;
        body_node.body.push_back(_parse_expression(stream.clip()));
        new_else.body = body_node;
        new_else.is_scoped = false;
        return new_else;
    }
    new_else.body = _parse_brace_scope(std::move(stream));
	return new_else;
}

ForNode FSLParser::_parse_for_statement(TokenStream &&stream) {
	ForNode new_for;
    auto err_for = [&](){
        new_for.is_valid = false;
        return new_for;
    };
    
    if (!stream.descend_paren()) {
        print_fsl_err_expected_tok_loc("\'(\'", "after `for`", stream.peek());
        return err_for();
    }
    {
        uint32_t statement_start = 0;
        uint32_t statement_len = 0;
        stream.start_slice();
        if (!stream.consume_until<Token::SYMBOL_SEMICOLON>(false)) {
            print_fsl_err_expected_tok_loc("\';\'", "in `for` statement", stream.peek());
            return err_for();
        }
        new_for.init = _parse_operation(stream.get_slice());
        stream.consume();

        stream.start_slice();
        if (!stream.consume_until<Token::SYMBOL_SEMICOLON>(false)) {
            print_fsl_err_expected_tok_loc("\';\'", "in `for` statement", stream.peek());
            return err_for();
        }
        new_for.cond = _parse_operation(stream.get_slice());
        stream.consume();

        new_for.post = _parse_operation(stream.clip_and_ascend());
    }

    if (!stream.descend_brace()) {
        ScopeNode body_node;
        body_node.body.push_back(_parse_expression(stream.clip()));
        new_for.body = body_node;
        new_for.is_scoped = false;
        return new_for;
    }
    new_for.body = _parse_brace_scope(std::move(stream));
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
    uint32_t next_expression_start = stream.get_index();
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
            case Token::BRACE_OPEN:
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
        if (!stream.descend_paren()) {
            print_fsl_err_expected_tok_loc("\'(\'", "after function name", stream.peek());
            return err_func();
        }
        while(!stream.at_end()) {
            uint32_t tok_index = stream.get_index();

            auto& token = stream.consume();
            switch (token.get_type()) {
                case Token::IDENTIFIER:
                case Token::SPECIFIER_IN:
                case Token::SPECIFIER_OUT:
                case Token::SPECIFIER_INOUT:
                case Token::SPECIFIER_CONST: {
                    uint32_t var_decl_len = 1;

                    // it's ok if this terminates due to EOS
                    stream.consume_until<Token::SYMBOL_COMMA>([&](){ var_decl_len++; });
                    new_func.args.push_back(_parse_var_decl(stream.get_slice(tok_index, var_decl_len)));
                } break;
                default:
                    print_fsl_err_unexpected_tok("function arguments", stream.peek());
                    return err_func();
            }
        }
        stream.ascend();
    }

    {
        if (!stream.descend_brace()) {
            print_fsl_err_expected_tok_loc("\'{\'", "in function declaration", stream.peek());
            return err_func();
        }
        new_func.code = _parse_brace_scope(std::move(stream));
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

    if(!stream.descend_brace()) {
        print_fsl_err_expected_tok_loc("\'{\'", "in buffer declaration", stream.peek());
        return err_buffer();
    }
    {
        while(!stream.at_end()) {
            stream.start_slice();
            if (!stream.consume_until<Token::SYMBOL_SEMICOLON>(false)) {
                print_fsl_err_unexpected_tok("buffer declaration", stream.peek());
                return err_buffer();
            }
            new_buffer.fields.push_back(_parse_var_decl(stream.get_slice()));
            stream.consume();
        }
        stream.ascend();
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
        if (!stream.descend_paren()) {
            print_fsl_err_expected_tok_loc("'('", "after `layout`", stream.peek());
            new_resource.is_valid = false;
            return new_resource;
        }
        switch (stream.peek().get_category()) {
            case Token::CATEGORY_BUFFERFORMAT:
                res_type = RESTYPE_BUFFER;
                break;
            case Token::CATEGORY_TEXTUREFORMAT:
                res_type = RESTYPE_TEXTURE;
                break;
            default:
                print_fsl_err_unexpected_tok("resource layout", stream.peek());
                new_resource.is_valid = false;
                return new_resource;
        }
        layout_token = stream.consume().get_token();
        if (!stream.at_end()) {
            print_fsl_err_unexpected_tok("resource layout", stream.peek());
        }
        stream.ascend();
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

    if (!stream.descend_bracket(false)) {
        print_fsl_err_expected_tok_loc("kernel size", "in kernel declaration", stream.peek());
        return err_kernel();
    }
    {
        stream.start_slice();
        if(!stream.consume_until<Token::SYMBOL_COMMA>(false)) {
            print_fsl_err_expected_tok_loc(",", "in kernel size declaration", stream.peek());
            return err_kernel();
        }
        new_kernel.local_x_threads = _parse_operation(stream.get_slice());
        stream.consume();

        stream.start_slice();
        if(!stream.consume_until<Token::SYMBOL_COMMA>(false)) {
            print_fsl_err_expected_tok_loc(",", "in kernel size declaration", stream.peek());
            return err_kernel();
        }
        new_kernel.local_y_threads = _parse_operation(stream.get_slice());
        stream.consume();

        new_kernel.local_z_threads = _parse_operation(stream.clip_and_ascend());
    }

    auto& kernel_name = stream.peek();
    if (!stream.expect(Token::IDENTIFIER)) {
        print_fsl_err_expected_tok_loc("valid identifier", "in kernel declaration", kernel_name);
        return err_kernel();
    }
    new_kernel.name = kernel_name.get_contents();
    
    {
        if (!stream.descend_paren(false)) {
            print_fsl_err_expected_tok_loc("\'(\'", "after kernel identifier", stream.peek());
            return err_kernel();
        }
        while(!stream.at_end()) {
            uint32_t tok_index = stream.get_index();

            auto& token = stream.consume();
            switch (token.get_type()) {
                case Token::IDENTIFIER: {
                    if (stream.peek().get_type() == Token::SYMBOL_COLON) {
                        auto name_binding_key = token.get_contents();
                        stream.consume();
                        auto& bound_token = stream.peek();
                        if (!stream.expect(Token::IDENTIFIER)) {
                            print_fsl_err_expected_tok_loc("name to bind", "in kernel name binding", bound_token);
                            return err_kernel();
                        }
                        new_kernel.name_bindings[name_binding_key] = bound_token.get_contents();
                        if(!stream.expect(Token::SYMBOL_COMMA)) {
                            if (!stream.at_end()) {
                                print_fsl_err_expected_tok_loc("\',\'", "in kernel arguments", stream.peek());
                                return err_kernel();
                            }
                        }
                        break;
                    }
                    uint32_t var_decl_len = 1;

                    // it's ok if this terminates due to EOS
                    stream.consume_until<Token::SYMBOL_COMMA>([&](){ var_decl_len++; });
                    new_kernel.push_constants.push_back(_parse_var_decl(stream.get_slice(tok_index, var_decl_len)));
                } break;
                default:
                    print_fsl_err_unexpected_tok("kernel arguments", stream.peek());
                    return err_kernel();
            }
        }
        stream.ascend();
    }

    {
        if (!stream.descend_brace()) {
            print_fsl_err_expected_tok_loc("\'{\'", "in kernel declaration", stream.peek());
            return err_kernel();
        }
        new_kernel.code = _parse_brace_scope(std::move(stream));
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
                stream.start_slice();
                if (!stream.consume_until<Token::BRACE_OPEN>(true)) {
                    print_fsl_err("Unexpected end of file", stream.peek().get_debug_info().row);
                    return;
                }
                GlobalDeclaration new_global_decl;
                new_global_decl.value = _parse_kernel(stream.get_slice());
                ast.contents.push_back(new_global_decl);
            } break;
            case Token::KEYWORD_LAYOUT: {
                flush_expression();
                stream.consume();
                stream.start_slice();
                if (!stream.consume_until<Token::SYMBOL_SEMICOLON>(false)) {
                    print_fsl_err("Unexpected end of file", stream.peek().get_debug_info().row);
                    return;
                }
                GlobalDeclaration new_global_decl;
                new_global_decl.value = _parse_resource(stream.get_slice());
                stream.consume();
                ast.contents.push_back(new_global_decl);
            } break;    
            case Token::BRACE_OPEN:
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

void print_token_tree(const LocalVector<TokenTree>& tree, ConsoleString& output) {
    for (const auto& subtree : tree) {
        match(subtree.node, 
            [&](const Token* token) -> void{
            },
            [&](const TokenScope& scope) -> void {
                output.add_line("Scope %s", scope.open->contents);
                output.indent();
                print_token_tree(scope.scope_contents, output);
                output.unindent();
                output.add_line("%s", scope.close->contents);
            });
    }
}

TokenScope FSLParser::_collect_scope(Span<Token> tokens, uint32_t &token_index) {
    TokenScope out_scope;
    out_scope.open = &tokens[token_index++];
    while (token_index < tokens.size()) {
        auto& token = tokens[token_index];
        switch (token.token_type) {
            case Token::BRACE_OPEN:
            case Token::BRACKET_OPEN:
            case Token::PAREN_OPEN:
                out_scope.scope_contents.push_back(_collect_scope(tokens, token_index));
                break;
            case Token::BRACE_CLOSE:
            case Token::BRACKET_CLOSE:
            case Token::PAREN_CLOSE:
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
    out_scope.close = out_scope.eof_sentinel();
	return out_scope;
}

LocalVector<TokenTree> FSLParser::_collect_scopes(Span<Token> tokens) {
    LocalVector<TokenTree> out_tokens;
    uint32_t token_index = 0;
    while (token_index < tokens.size()) {
        auto& token = tokens[token_index];
        switch (token.token_type) {
            case Token::BRACE_OPEN:
            case Token::BRACKET_OPEN:
            case Token::PAREN_OPEN:
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

optional<fslAST> FSLParser::get_ast(String path) {
    auto ast = fslAST();

    FSLParser parser;
    auto parse_out = parser._preprocess(path);
    if (!parse_out.has_value()) {
        return {};
    }
    ast.tokens = std::move(*parse_out);

    LocalVector<TokenTree> tree = parser._collect_scopes(ast.tokens.span());
    parser._parse_file(ast, tree.span());

    if (!FSLValidator::validate_ast(ast)) {
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

optional<FSLParser::MacroDef> FSLParser::_process_macro(const LocalVector<Token>& tokens, uint32_t &token_index) {
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

optional<LocalVector<Token>> FSLParser::_expand_macro(const LocalVector<Token>& tokens, uint32_t &token_index, const HashMap<StringName, MacroDef> &macros, const MacroDef &curr_macro) {
    LocalVector<Token> pass_1_tokens;
    HashMap<StringName, LocalVector<Token>> args;
    
    if (curr_macro.args.size() > 0) {
        uint32_t arg_index = 0;
        if (tokens[token_index].contents != "(") {
            print_fsl_err_expected_tok_loc("\"(\"", "in macro usage", &tokens[token_index]);
        }
        token_index++;
        LocalVector<Token> curr_arg_tokens;
        uint32_t depth = 0;
        while (token_index < tokens.size()) {
            curr_arg_tokens.push_back(tokens[token_index++]);
            if (tokens[token_index].contents == ")") {
                if (depth == 0) {
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
                } else {
                    depth--;
                }
                break;
            }
            if (tokens[token_index].contents == "(") {
                depth++;
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

String reduce_file_path(const String& path) {
    LocalVector<String> path_builder;
    String next_segment = "";
    for (uint32_t char_index = 0; char_index < path.length(); char_index++) {
        const char& curr_char = path[char_index];
        if (curr_char == '/') {
            if (next_segment == "..") {
                path_builder.remove_at(path_builder.size() - 1);
            } else if (next_segment != ".") {
                path_builder.push_back(next_segment);
            }
            next_segment = "";
        } else {
            next_segment += curr_char;
        }
    }
    path_builder.push_back(next_segment);
    String clean_path = "";
    bool is_first = true;
    for (const auto& segment : path_builder) {
        if (!is_first) {
            clean_path += "/";
        } else {
            is_first = false;
        }
        clean_path += segment;
    }
    return clean_path;
}

optional<LocalVector<Token>> FSLParser::_preprocess(String &path) {
    String path_to_file = path.get_base_dir() + '/';
    auto main_file_opt = load_file(path);
    if (!main_file_opt.has_value()) {
        printvf_err("Failed to load file at \"%s\"", path);
        return {};
    }
    auto main_file = *main_file_opt;
    LocalVector<String> included_files = {path};
    FSLLexer lexer = FSLLexer();
    auto tokens_opt = lexer.tokenize(path, main_file->get_as_text());
    if (!tokens_opt.has_value()) {
        printvf_err("Failed to parse file at \"%s\"", path);
        return {};
    }
    LocalVector<Token> tokens = *tokens_opt;

    uint32_t token_index = 0;
    LocalVector<Token> out_tokens;
    HashMap<StringName, MacroDef> macros;
    stack<IfdefState> ifdef_state_stack;
    stack<String> curr_file_path;
    ifdef_state_stack.push(NONE);
    curr_file_path.push(path);
    current_context.file_path = path;
    
    while (token_index < tokens.size()) {
        const Token& token = tokens[token_index++];
        current_context.file_path = reduce_file_path(token.debug_info.source_file);
        path_to_file = current_context.file_path.get_base_dir() + "/";
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
                        String true_include_path = reduce_file_path(path_to_file + relative_include_path);
                        uint32_t token_insert_index = token_index;
                        auto included_file_opt = load_file(true_include_path);
                        if (!included_file_opt.has_value()) {
                            print_error(vformat("Failed to load file at \"%s\"", true_include_path));
                            return {};
                        }
                        auto included_file = *included_file_opt;
                        if (!included_files.has(true_include_path)) {
                            included_files.push_back(true_include_path);
                            auto include_opt = lexer.tokenize(true_include_path, included_file->get_as_text());
                            if (!include_opt.has_value()) {
                                printvf_err("Failed to parse file at \"%s\"", true_include_path);
                                return {};
                            }
                            for (auto &token : *include_opt) {
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
                        token_index++;
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
                if (ifdef_state_stack.top() != FALSE && ifdef_state_stack.top() != ELSEFALSE && !valid_directive) {
                    print_fsl_err("\"%s\" is not a valid preprocessor directive", tokens[token_index].debug_info.row, tokens[token_index].contents);
                    return {};
                }
            } break;
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
    if (ifdef_state_stack.size() != 1) {
        print_fsl_err("Expected `#endif`", tokens[token_index-1].debug_info.row);
        return {};
    }
    return out_tokens;
}
#pragma endregion

