#include "fsl_validator.h"
#include "ast_defs.h"
#include "fsl_operators.h"
#include "godot_fsl_defs.h"

using namespace AST;

void FSLValidator::log_redeclared_variable_error(const String &var_name, const DebugInfo &debug_info, const VariableDecl &orig_var_decl) {
	if (debug_info.file_name == orig_var_decl.debug_info.file_name) {
		log_validation_error("Redefinition of \"%s\", first declared at %d,%d", debug_info, FSLError::ERR_VAR_REDEFINITION, var_name, orig_var_decl.debug_info.start_row, orig_var_decl.debug_info.start_column);   
	} else {
		log_validation_error("Redefinition of \"%s\", first declared in file \"%s\" at %d,%d", debug_info, FSLError::ERR_VAR_REDEFINITION, var_name, orig_var_decl.debug_info.file_name, orig_var_decl.debug_info.start_row, orig_var_decl.debug_info.start_column);   
	}
}

void FSLValidator::log_redeclared_field_error(const String &field_name, const AST::DebugInfo &debug_info, const AST::VariableDecl &orig_field_decl) {
	log_validation_error("Redefinition of field \"%s\", first declared at %d,%d", debug_info, FSLError::ERR_FIELD_REDEFINITION, field_name, orig_field_decl.debug_info.start_row, orig_field_decl.debug_info.start_column);   
}

void FSLValidator::validate_field_decl(const VariableDecl &field_decl, HashMap<StringName, const AST::VariableDecl*>& members) {
	if (members.has(field_decl.name)) {
		log_redeclared_field_error(field_decl, *members[field_decl.name]);
	}
	if (!valid_types.has(field_decl.type.type)) {
		log_unknown_type_error(field_decl.type);
	}
	members[field_decl.name] = &field_decl;
}

void FSLValidator::validate_variable_annotations(const AST::VariableDecl &var_decl) {
}

void FSLValidator::validate_variable_decl(const VariableDecl &var_decl) {
	for (const auto& scope_info : call_stack.top()) {
		if (scope_info->local_vars.has(var_decl.name)) {
			log_redeclared_variable_error(var_decl, *scope_info->local_vars[var_decl.name]);
			break;
		}
	}
	if (!valid_types.has(var_decl.type.type)) {
		log_unknown_type_error(var_decl.type);
	}
	call_stack.top()[call_stack.top().size() - 1]->local_vars[var_decl.name] = &var_decl;
}

void FSLValidator::validate_operation_list(const OperationList &op_list) {
	enum OperandCategory {
		PLACE, // modifiable value with a place in memory
		CPLACE, // immutable value with a place in memory
		VALUE, // value with no place in memory
		NONE // anything else
	};
	OperandCategory last_op_category = NONE;
	const TypeRef* last_op_type = nullptr;

	
	for (const auto& oper : op_list.operations) {
		match(oper,  
			[&](const VariableDecl& var_decl) {
				validate_variable_decl(var_decl);
				last_op_category = PLACE;
				last_op_type = &var_decl.type;
			},  
			[&](const Operator& op) {
				if (op.symbol == "=") {
					if (last_op_category != PLACE) {
						if (last_op_category == CPLACE) {
							log_validation_error("Cannot assign to a const variable", op.debug_info, FSLError::ERR_ASSIGN_CPLACE);
						} else {	
							log_validation_error("Left operand must be a modifiable place", op.debug_info, FSLError::ERR_ASSIGN_NONPLACE);
						}
					}
					last_op_category = NONE;

				}
			},  
			[&](const Literal& literal) {
				last_op_category = VALUE;
			},  
			[&](const FieldAccess& field_access) {
				last_op_category = PLACE;
			},  
			[&](const ArrayIndex& arr_index) {
				last_op_category = PLACE;
			},  
			[&](const FuncCall& func_call) {
				last_op_category = VALUE;
			},  
			[&](const VariableRef& var_ref) {
				bool var_found = false;
				for (const auto& scope_info : call_stack.top()) {
					if (scope_info->local_vars.has(var_ref.name)) {
						var_found = true;
						const auto var_decl = scope_info->local_vars[var_ref.name];
						if (var_decl->type.specifiers.has(SPECIFIER_CONST) || var_decl->type.specifiers.has(SPECIFIER_IN)) {
							last_op_category = CPLACE;
						} else {
							last_op_category = PLACE;
						}
						last_op_type = &var_decl->type;
						break;
					}
				}
				if (!var_found) {
					log_validation_error("Unknown variable \"%s\" referenced", var_ref.debug_info, FSLError::ERR_UNKNOWN_VAR, var_ref.name);
				}
			},  
			[&](const OperationList& op_list) {
				validate_operation_list(op_list);
			},  
			[&](const UnknownOp& unknown_op) {
				// do nothing, this means an error occured in the parser and was already logged
			});
	}
}

void FSLValidator::validate_expression(const Expression &expr) {
	match(expr, 
		[&](const OperationList& op_list) {
			validate_operation_list(op_list);
		}, 
		[&](const IfNode& if_node) {
			auto if_scope = ScopeRef(*this);
			validate_operation_list(if_node.cond);
			validate_scope_inner(if_node.body);
		}, 
		[&](const ForNode& for_node) {
			auto for_scope = ScopeRef(*this);
			validate_operation_list(for_node.init);
			validate_operation_list(for_node.cond);
			validate_operation_list(for_node.post);
			validate_scope_inner(for_node.body);
		}, 
		[&](const ElseNode& else_node) {
			validate_scope(else_node.body);
		}, 
		[&](const WhileNode& while_node) {
			auto while_scope = ScopeRef(*this);
			validate_operation_list(while_node.cond);
			validate_scope(while_node.body);
		},
		[&](const ScopeNode& scope) {
			validate_scope(scope);
		}, 
		[&](const FunctionDecl& func_decl) {
			validate_function_decl(func_decl);
		}, 
		[&](const StructDecl& struct_decl) {
			auto struct_scope = ScopeRef(*this);
			HashMap<StringName, const VariableDecl*> members;
			for (const auto& field : struct_decl.fields) {
				if (field.type.type == struct_decl.name) {
					log_validation_error("Struct \"%s\" cannot contain a field of itself", field.debug_info, FSLError::ERR_RECURSIVE_STRUCT);
				} else {
					validate_field_decl(field, members);
				}
			}
			valid_types.insert(struct_decl.name);
		},
		[&](const ReturnExpression& ret_expr) {
			validate_operation_list(ret_expr.return_val);
		});
}

void FSLValidator::validate_scope_inner(const AST::ScopeNode &scope) {
	for (const auto& expr : scope.body) {
		validate_expression(expr);
	}
}

void FSLValidator::validate_scope(const ScopeNode &scope) {
	auto scope_scope = ScopeRef(*this);
	validate_scope_inner(scope);
}

void FSLValidator::validate_function_call(const AST::FuncCall &func_call) {
}

void FSLValidator::validate_function_decl(const AST::FunctionDecl &func_decl) {
	auto func_scope = ScopeRef(*this);
	for (const auto& arg : func_decl.args) {
		validate_variable_decl(arg);
	}
	validate_scope_inner(func_decl.code);
}

void FSLValidator::validate_buffer_annotations(const AST::BufferDef &buffer, const DebugInfo& debug_info) {
	enum ValidationState {
		START,
		IS_VERTEX,
		IS_INDEX,
		IS_INDIRECT
	};
	ValidationState state = START;
	for (const auto& [name, args] : buffer.annotations) {
		switch (is_valid_buffer_annotation(name)) {
			case 1: // godot_vertex
				switch (state) {
					case START:
						if (buffer.buftype == UNIFORM) {
							log_validation_error("Uniform buffers cannot be vertex buffers", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						} 
						state = IS_VERTEX;
						break;
					case IS_VERTEX:
						// can't happen unless parser is broken
						break;
					case IS_INDEX:
						log_validation_error("A buffer cannot be both a vertex and index buffer", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						break;
					case IS_INDIRECT:
						log_validation_error("Vertex buffers cannot be used as indirect dispatch/draw command buffers", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						break;
				}
				break;
			case 2: // godot_index
				switch (state) {
					case START:
						if (buffer.buftype == UNIFORM) {
							log_validation_error("Uniform buffers cannot be index buffers", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						}
						state = IS_INDEX;
						break;
					case IS_VERTEX:
						log_validation_error("A buffer cannot be both a vertex and index buffer", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						break;
					case IS_INDEX:
						// can't happen unless parser is broken
						break;
					case IS_INDIRECT:
						log_validation_error("Index buffers cannot be used as indirect dispatch/draw command buffers", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						break;
				}
				break;
			case 3: // indirect
				switch (state) {
					case START:
						if (buffer.buftype == UNIFORM) {
							log_validation_error("Uniform buffers cannot be used as indirect dispatch/draw command buffers", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						}
						state = IS_INDIRECT;
						break;
					case IS_VERTEX:
						log_validation_error("Vertex buffers cannot be used as indirect dispatch/draw command buffers", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						break;
					case IS_INDEX:
						log_validation_error("Index buffers cannot be used as indirect dispatch/draw command buffers", debug_info, FSLError::ERR_INVALID_ANNOTATION);
						break;
					case IS_INDIRECT:
						// can't happen unless parser is broken
						break;
				}
				break;
			default:
				log_validation_error("\"%s\" is not a valid annotation for buffers", debug_info, FSLError::ERR_UNKNOWN_ANNOTATION, name);
				break;
		}
	}
	
}

LocalVector<FSLError> FSLValidator::_validate_ast(const fslAST &ast) {
	// these are kinda cheap hacks that give me a fast way to safely insert buffers and textures into the global variable list.
	// in the future, these should be removed and replaced with either separate lists tracking resources or a more advanced bookkeeping method
	static VariableDecl buffer_var = {DebugInfo(), true, "", {DebugInfo(), true, {}, "buffer", {}}, {}};
	static VariableDecl texture_var = {DebugInfo(), true, "", {DebugInfo(), true, {}, "texture", {}}, {}};
	static HashMap<StringName, VariableDecl> bindables = {
		{"GlobalInvocationID", {DebugInfo(), true, "", {DebugInfo(), true, {SPECIFIER_CONST}, "uint3", {}}, {}}},
        {"LocalInvocationID", {DebugInfo(), true, "", {DebugInfo(), true, {SPECIFIER_CONST}, "uint3", {}}, {}}},
        {"LocalInvocationIndex", {DebugInfo(), true, "", {DebugInfo(), true, {SPECIFIER_CONST}, "uint", {}}, {}}},
        {"WorkGroupID", {DebugInfo(), true, "", {DebugInfo(), true, {SPECIFIER_CONST}, "uint3", {}}, {}}},
        {"WorkGroupSize", {DebugInfo(), true, "", {DebugInfo(), true, {SPECIFIER_CONST}, "uint3", {}}, {}}},
        {"NumWorkGroups", {DebugInfo(), true, "", {DebugInfo(), true, {SPECIFIER_CONST}, "uint3", {}}, {}}}
	};
	HashSet<StringName> resource_names;
	call_stack.push({&global_scope}); 
	curr_ast = &ast;
	for (const auto& entry : ast.contents) {
		match(entry.value, 
			[&](const KernelNode& kernel) {
				if (kernel_names.has(kernel.name)) {
					log_validation_error("Redefinition of kernel \"%s\"", kernel.debug_info, FSLError::ERR_KERNEL_REDEFINITION, kernel.name);
				} else {
					kernel_names.insert(kernel.name);
				}
				auto kernel_scope = ScopeRef(*this);
				for (const auto& [name_binding, bound_var] : kernel.name_bindings) {
					bool conflict = false;
					for (const auto& scope_info : call_stack.top()) {
						if (scope_info->local_vars.has(name_binding)) {
							log_redeclared_variable_error(name_binding, kernel.debug_info, *scope_info->local_vars[name_binding]);
							conflict = true;
							break;
						}
					}
					if (!bindables.has(bound_var)) {
						log_validation_error("Expected a valid built-in name, found \"%s\"", kernel.debug_info, FSLError::ERR_INVALID_NAME_BINDING, bound_var);
						break;
					}
					if (conflict) return;
					kernel_scope->local_vars[name_binding] = &bindables[bound_var];
				}
				for (const auto& push_constant : kernel.push_constants) {
					validate_variable_decl(push_constant);
					kernel_scope->local_vars[push_constant.name] = &push_constant;
				}
				validate_scope_inner(kernel.code);
			}, 
			[&](const ResourceNode& resource) {
				if (resource_names.has(resource.name)) {
					log_validation_error("Redeclaration of resource name \"%s\"", resource.debug_info, FSLError::ERR_RESOURCE_REDEFINITION, resource.name);
				} else {
					resource_names.insert(resource.name);
				}
				match(resource.resource, 
					[&](const TextureDef& texture) {
						if (global_scope.local_vars.has(resource.name)) {
								log_redeclared_variable_error(resource.name, resource.debug_info, *global_scope.local_vars[resource.name]);
						} else {
							global_scope.local_vars[resource.name] = &texture_var;
						}
					}, 
					[&](const BufferDef& buffer) {
						bool is_named = false;
						if (!buffer.buffer_name.is_empty()) {
							if (global_scope.local_vars.has(buffer.buffer_name)) {
								log_redeclared_variable_error(buffer.buffer_name, resource.debug_info, *global_scope.local_vars[buffer.buffer_name]);
							} else {
								// TODO: once structs are implemented, declare buffers type as a struct matching the buffer layout
								global_scope.local_vars[buffer.buffer_name] = &buffer_var;
							}
							is_named = true;
						} 
						if (!buffer.annotations.is_empty()) {
							validate_buffer_annotations(buffer, resource.debug_info);
						}
						// 0 = no unsized field yet, 1 = unsized field was the last field found, 2 = unsized field was found and is not the last member of the buffer
						uint32_t has_unsized_field = 0;
						HashMap<StringName, const AST::VariableDecl*> members;
						for (const auto& field : buffer.fields ) {
							if (has_unsized_field == 1) {
								has_unsized_field = 2;
								log_validation_error("Unsized array field must be final member of buffer", resource.debug_info, FSLError::ERR_UNSIZED_FIELD_NOT_LAST);
							}
							if (is_named) {
								validate_field_decl(field, members);
							} else {
								validate_variable_decl(field);
							}
							
							for (const auto& array_dim : field.type.array_dims) {
								if (array_dim.operations.is_empty()) {
									has_unsized_field = 1;
								}
							}
						}
					

					}, 
					[&](const UniformDef& uniform) {
						validate_variable_decl(uniform.uniform_decl);
					});
			}, 
			[&](const Expression& expression) {
				validate_expression(expression);
			});
	}
	return errors;
}

FSLValidator::FSLValidator() {
	static const String type_names[] = {
        "f16",
        "f32",
        "f64",
        "u8",
        "u16",
        "u32",
        "u64",
        "i8",
        "i16",
        "i32",
        "i64",
        "bool"
    };
	for (const auto& type : type_names) {
		valid_types.insert(type);
		for (uint32_t i : {2, 3, 4}) {
			valid_types.insert(vformat("%sx%d", type, i));
			auto type_string = String(type);
			if (type_string == "f16" || type_string == "f32" || type_string == "f64") {
				for (uint32_t j : {2, 3, 4}) {
					valid_types.insert(vformat("%sx%dx%d", type, i, j));
				}
			}
		}
	}
}

LocalVector<FSLError> FSLValidator::validate_ast(const fslAST &ast) {
	auto validator = FSLValidator();
	return validator._validate_ast(ast);
}