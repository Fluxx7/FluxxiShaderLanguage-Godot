#pragma once
#include "godot_imports.h"
#include "std_imports.h"
#include "fsl_lexer.h"
#include "../errors.h"


class FSLValidator {
protected:
    // TODO: implement a system that allows for both declaration of structs and tracking of specific information about types
    HashSet<StringName> valid_types; 
    struct FunctionOverload {
        const AST::FunctionDecl* decl;
        LocalVector<StringName> function_refs;
    };
    HashMap<StringName, LocalVector<FunctionOverload>> valid_funcs;
    struct ScopeInfo {
        HashMap<StringName, const AST::VariableDecl*> local_vars;
    };
    struct ScopeRef {
        ScopeInfo scope;
        FSLValidator& validator;
        explicit ScopeRef(FSLValidator& owner) : validator(owner) {
            owner.call_stack.top().push_back(&scope);
        }
        ~ScopeRef() { 
            auto& stack_frame = validator.call_stack.top();
            stack_frame.remove_at(stack_frame.find(&scope)); 
        }
        ScopeRef(const ScopeRef&) = delete;
        ScopeInfo* operator->() {
            return &scope;
        }
    };
    ScopeInfo global_scope;
    stack<LocalVector<ScopeInfo*>> call_stack;
    HashSet<StringName> kernel_names;
    const AST::fslAST* curr_ast;

    // builtins
    LocalVector<AST::VariableDecl> builtin_vars;
    LocalVector<AST::FunctionDecl> builtin_funcs;
    LocalVector<AST::TypeRef> builtin_types;

    LocalVector<FSLError> errors;

    void new_stack_frame() {
        call_stack.push({&global_scope});
    }

    void drop_stack_frame() {
        call_stack.pop();
    }

    template<typename... Ts>
    void log_validation_error(String error_msg, const AST::DebugInfo& debug_info, FSLError::ErrorType err_type, Ts... args) {
        errors.push_back({
            debug_info.file_name,
            debug_info.start_row,
            debug_info.start_column,
            debug_info.end_row,
            debug_info.end_column,
            FSLError::VALIDATOR,
            err_type,
            vformat(error_msg, args...)
        });
    }

    void log_unknown_type_error(StringName type_name, const AST::DebugInfo& debug_info) {
        log_validation_error("Unknown type \"%s\"", debug_info, FSLError::ERR_UNKNOWN_TYPE, type_name);
    }
    void log_unknown_type_error(const AST::TypeRef& unknown_type) {
        log_unknown_type_error(unknown_type.type, unknown_type.debug_info);
    }

    void log_redeclared_variable_error(const String& var_name, const AST::DebugInfo& debug_info, const AST::VariableDecl& orig_var_decl);
    void log_redeclared_variable_error(const AST::VariableDecl& var_decl, const AST::VariableDecl& orig_var_decl) {
        log_redeclared_variable_error(var_decl.name, var_decl.debug_info, orig_var_decl);
    }

    void log_redeclared_field_error(const String& field_name, const AST::DebugInfo& debug_info, const AST::VariableDecl& orig_field_decl);
    void log_redeclared_field_error(const AST::VariableDecl& field_decl, const AST::VariableDecl& orig_field_decl) {
        log_redeclared_variable_error(field_decl.name, field_decl.debug_info, orig_field_decl);
    }

    void validate_field_decl(const AST::VariableDecl& field_decl, HashMap<StringName, const AST::VariableDecl*>& members);
    void validate_variable_annotations(const AST::VariableDecl& var_decl);
    void validate_variable_decl(const AST::VariableDecl& var_decl);
    void validate_operation_list(const AST::OperationList& op_list);
    void validate_expression(const AST::Expression& expr);
    void validate_scope_inner(const AST::ScopeNode& scope);
    void validate_scope(const AST::ScopeNode& scope);
    void validate_function_call(const AST::FuncCall& func_call);
    void validate_function_decl(const AST::FunctionDecl& func_decl);
    void validate_buffer_annotations(const AST::BufferDef& buffer, const AST::DebugInfo& debug_info);
    LocalVector<FSLError> _validate_ast(const AST::fslAST& ast);
    FSLValidator();
public:
    static LocalVector<FSLError> validate_ast(const AST::fslAST& ast);
};