#pragma once

#include "std_imports.h"
#include "godot_imports.h"
#include "fsl_lexer.h"


namespace AST {
    class FSLParser {
    protected:
        struct ParserContext {
            String file_path;
        };
        ParserContext current_context;
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
        template <typename... VarArgs>
        void printvf_err(const String& error_msg, const VarArgs... p_args ) {
            print_error(vformat(error_msg, p_args...));
        }

        template <typename... VarArgs>
        void print_fsl_err(const String& error_msg, uint32_t line_num, const VarArgs... p_args ) {
            print_error(vformat("%s, line %d: %s", current_context.file_path, line_num, vformat(error_msg, p_args...)));
        }

        template <typename... VarArgs>
        void print_fsl_err_unexpected_tok(const String& error_msg, const TokenTree& err_token, const VarArgs... p_args ) {
            print_error(vformat("%s %d,%d: Unexpected token \"%s\" in %s", err_token.get_debug_info().source_file, err_token.get_debug_info().row, err_token.get_debug_info().column, err_token.get_contents(), vformat(error_msg, p_args...)));
        }

        template <typename... VarArgs>
        void print_fsl_err_expected_tok(const String& expected_str, const TokenTree& err_token, const VarArgs... p_args ) {
            print_error(vformat("%s %d,%d: Unexpected token \"%s\", expected %s", err_token.get_debug_info().source_file, err_token.get_debug_info().row, err_token.get_debug_info().column, err_token.get_contents(), vformat(expected_str, p_args...)));
        }

        template <typename... VarArgs>
        void print_fsl_err_expected_tok_loc(const String& expected_str, const String& loc_string, const TokenTree& err_token, const VarArgs... p_args ) {
            print_error(vformat("%s %d,%d: Unexpected token \"%s\" %s, expected %s", err_token.get_debug_info().source_file, err_token.get_debug_info().row, err_token.get_debug_info().column, err_token.get_contents(), loc_string, vformat(expected_str, p_args...)));
        }

        TypeRef _parse_type(TokenStream& stream);
        VariableDecl _parse_var_decl(TokenStream&& stream);
        Args _parse_args(TokenStream&& stream);
        HashMap<StringName, Args> _parse_annotation_list(TokenStream& stream);

        Operation _parse_op_segment(TokenStream& stream);
        OperationList _parse_operation(TokenStream&& stream);
        Expression _parse_expression(TokenStream&& stream, bool is_global = false);

        IfNode _parse_if_statement(TokenStream&& stream);
        ElseNode _parse_else_statement(TokenStream&& stream);
        ForNode _parse_for_statement(TokenStream&& stream);

        VariableDecl _parse_var_decl(Slice<TokenTree> tokens) {
            return _parse_var_decl(tokens.get_stream());
        }

        OperationList _parse_operation(Slice<TokenTree> tokens) {
            return _parse_operation(tokens.get_stream());
        }

        Expression _parse_expression(Slice<TokenTree> tokens, bool is_global = false) {
            return _parse_expression(tokens.get_stream(), is_global);
        }

        IfNode _parse_if_statement(Slice<TokenTree> tokens) {
            return _parse_if_statement(tokens.get_stream());
        }

        ElseNode _parse_else_statement(Slice<TokenTree> tokens) {
            return _parse_else_statement(tokens.get_stream());
        }

        ForNode _parse_for_statement(Slice<TokenTree> tokens) {
            return _parse_for_statement(tokens.get_stream());
        }

        void _parse_bracket_scope(const TokenScope* scope, Statement& out_statement);
        void _parse_paren_scope(const TokenScope* scope, Statement& out_statement);
        ScopeNode _parse_brace_scope(TokenStream&& stream);

        FunctionDecl _parse_func_decl(Slice<TokenTree> tokens);

        void _parse_texture(const Token* layout_token, TokenStream& stream, ResourceNode& res_node);
        void _parse_buffer(const Token* layout_token, TokenStream& stream, ResourceNode& res_node);
        ResourceNode _parse_resource(Slice<TokenTree> tokens);

        KernelNode _parse_kernel(Slice<TokenTree> tokens);

        void _parse_file(fslAST &ast, Span<TokenTree> tokens);
        optional<MacroDef> _process_macro(const LocalVector<Token>& tokens, uint32_t &token_index);
        optional<LocalVector<Token>> _expand_macro(const LocalVector<Token>& tokens, uint32_t &token_index, const HashMap<StringName, MacroDef> &macros, const MacroDef &curr_macro);

        TokenScope _collect_scope(Span<Token> tokens, uint32_t& token_index);
        LocalVector<TokenTree> _collect_scopes(Span<Token> tokens);
    public:
        optional<String> error;
        HashMap<StringName, KernelNode> kernel_data;
        optional<LocalVector<Token>> _preprocess(String& path);
        
        static optional<fslAST> get_ast(String path);
    };
}