#include "code_builder.h"

using namespace AST;

template <typename T>
void insert_all(HashSet<T>& dest, const HashSet<T>& source) {
    for (const auto& val : source) {
        dest.insert(val);
    }
}

template <typename T>
void insert_all_excluding(HashSet<T>& dest, const HashSet<T>& source, const HashSet<T>& exclusion_set) {
    for (const auto& val : source) {
        if (!exclusion_set.has(val)) {
            dest.insert(val);
        }
    }
}

FSLType typeref_to_fslType(const TypeRef &type_ref) {
    FSLPrimitive primitive_type = FLOAT;
    FSLVecSize vec_size = ONE;
    LocalVector<uint32_t> array_dims;
    const auto& type_token = type_ref.type;
    uint32_t type_index = 0;
    for (auto val : fsl_primitives) {
        if (type_token->contents == val) {
            primitive_type = (FSLPrimitive) type_index;
        } else {
            type_index++;
        }
    }
    uint32_t final_char_index = type_token->contents.length() - 1;
    String final_char = type_token->contents.substr(final_char_index);
    if (final_char.is_valid_int()) {
        uint32_t token_val = final_char.to_int();
        if (token_val > 4 || token_val == 0) {
            printvf("Invalid vector size %d", token_val);
        }
        vec_size = (FSLVecSize) token_val;
    }
    for(const auto& array_dim : type_ref.array_dims) {
        // TODO: handle this properly, this is not a safe set-up at all lmao
        array_dims.push_back(0);
    }
    if (array_dims.size() > 0) {
        return (FSLArray) {(FSLCoreType){primitive_type, vec_size}, array_dims};
    }
    return (FSLCoreType){primitive_type, vec_size};
}

CodeBuilder::CodeInfo CodeBuilder::gen_var_decl(const VariableDecl& var_decl) {
    ConsoleString var_decl_code;
    HashSet<StringName> var_decl_refs; 

    if (var_decl.annotations.has("specialization_constant")) {
        var_decl_code.add("layout(constant_id = ");
        uint32_t constant_id = 0;
        if (!var_decl.annotations["specialization_constant"].is_empty()) {
            constant_id = gen_operation_list(var_decl.annotations["specialization_constant"][0]).code.get_output().to_int();
        } else {
            while (used_constant_ids.has(constant_id)) {
                constant_id++;
            }
        }
        var_decl_code.add("%d) ", constant_id);
        used_constant_ids.insert(constant_id);
        spec_constants[var_decl.name] = {constant_id, 0};
        next_spec_const_name = var_decl.name;
        next_spec_const.constant_id = constant_id;
        last_op_spec = true;
    } else {
        last_op_spec = false;
    }
    
    for (const auto& specifier : var_decl.type.specifiers) {
        var_decl_code.add("%s ", specifier->contents);
    }
    var_decl_code.add("%s %s", to_original_type(var_decl.type.type->contents), var_decl.name);
    for (const auto& array_dim : var_decl.type.array_dims) {
        var_decl_code.add("[");
        if (!array_dim.operations.is_empty()) {
            auto [code, refs, _] = gen_operation_list(array_dim);
            var_decl_code.add("%s", code.get_output());
            insert_all(var_decl_refs, refs);
        }
        var_decl_code.add("]");
    }

	return {var_decl_code, var_decl_refs};
}

CodeBuilder::CodeInfo CodeBuilder::gen_operation(const Operation &operation, bool is_first) {
    ConsoleString operation_code;
    HashSet<StringName> operation_refs; 
    HashSet<StringName> operation_vars; 
    match(operation, 
        [&](VariableDecl var_decl) {
            if (!is_first) { // not sure if this can ever happen in valid code, but whatever lol
                operation_code.add(" ");
            }
            auto [code, refs, _] = gen_var_decl(var_decl);
            operation_code.add(code);
            operation_vars.insert(var_decl.name);

        },
        [&](Operator oper) {
            if (!is_first) {
                operation_code.add(" ");
            }
            operation_code.add("%s", oper.symbol);
            if (last_op_spec && oper.symbol != "=") {
                last_op_spec = false;
            }
        },
        [&](FuncCall func_call) {
            if (!is_first) {
                operation_code.add(" ");
            }
            operation_code.add("%s(", to_original_type(func_call.name));
            bool is_first_arg = true;
            for (const auto& arg : func_call.args) {
                if (is_first_arg) {
                    is_first_arg = false;
                } else {
                    operation_code.add(", ");
                }
                auto [code, refs, decls] = gen_operation_list(arg);
                operation_code.add("%s", code.get_output());
                insert_all_excluding(operation_refs, refs, operation_vars);
                insert_all(operation_vars, decls);
            }
            operation_code.add(")");
            insert_all(operation_refs, func_infos[func_call.name].refs);
            last_op_spec = false;
        },
        [&](VariableRef var_ref) {
            if (!is_first) {
                operation_code.add(" ");
            }
            operation_code.add("%s", var_ref.name);
            operation_refs.insert(var_ref.name);
            last_op_spec = false;
        },
        [&](FieldAccess field_access) {
            operation_code.add(".%s", field_access.field_name);
            last_op_spec = false;
        },
        [&](ArrayIndex array_index) {
            for (const auto& index : array_index.indices) {
                auto [code, refs, _] = gen_operation_list(index);
                operation_code.add("[%s]", code.get_output());
                insert_all(operation_refs, refs);
            }
            last_op_spec = false;
        },
        [&](Literal literal) {
            if (!is_first) {
                operation_code.add(" ");
            }
            operation_code.add("%s", literal.value);
            if (last_op_spec) {
                next_spec_const.value = literal.value.to_int();
                spec_constants[next_spec_const_name] = next_spec_const;
                last_op_spec = false;
                next_spec_const_name = "";
                next_spec_const = {0, 0};
            }
        },
        [&](OperationList op_list) {
            if (!is_first) {
                operation_code.add(" ");
            }
            auto [code, refs, _] = gen_operation_list(op_list);
            operation_code.add("(%s)", code.get_output());
            insert_all(operation_refs, refs);
            last_op_spec = false;
        },
        [&](UnknownOp error_op) {
            print_error(vformat("Error found in AST, code: %s", tokens_to_string(error_op.code)));
            last_op_spec = false;
        });
    return {operation_code, operation_refs, operation_vars};
}

CodeBuilder::CodeInfo CodeBuilder::gen_operation_list(const OperationList &op_list) {
    ConsoleString op_list_code;
    HashSet<StringName> op_list_refs; 
    HashSet<StringName> op_list_vars; 
    bool is_first = true; 
    for (const auto& operation : op_list.operations) {
        auto [code, refs, decls] = gen_operation(operation, is_first);
        op_list_code.add(code);
        if (is_first) {
            is_first = false;
        }
        insert_all_excluding(op_list_refs, refs, op_list_vars);
        insert_all(op_list_vars, decls);
    }
	return {op_list_code, op_list_refs, op_list_vars};
}

CodeBuilder::CodeInfo CodeBuilder::gen_expression(const Expression &expression) {
    ConsoleString expr_code;
    HashSet<StringName> expr_refs; 
    HashSet<StringName> expr_vars;
    match(expression, 
        [&](OperationList op_expression) {
            auto [code, refs, decls] = gen_operation_list(op_expression);
            expr_code.add_line("%s;", code.get_output());
            expr_refs = std::move(refs);
            expr_vars = std::move(decls);
        },
        [&](IfNode if_node) {
            expr_code.add("if (");
            HashSet<StringName> if_decls;
            {
                auto [code, refs, decls] = gen_operation_list(if_node.cond);
                expr_code.add(code);
                insert_all(expr_refs, refs);
                if_decls = std::move(decls);
            }
            expr_code.add(") ");
            {
                CodeBuilder::CodeInfo info;
                if (if_node.is_scoped) {
                    info = gen_expression(if_node.body);
                } else {
                    info = gen_expression(if_node.body.body[0]);
                }
                auto& [code, refs, _] = info;
                expr_code.add_line(code);
                insert_all_excluding(expr_refs, refs, if_decls);
            }
        },
        [&](ElseNode else_node) {
            expr_code.add("else ");
            CodeBuilder::CodeInfo info;
            if (else_node.is_scoped) {
                info = gen_expression(else_node.body);
            } else {
                info = gen_expression(else_node.body.body[0]);
            }
            auto& [code, refs, _] = info;
            expr_code.add_line(code);
            insert_all(expr_refs, refs);
        },
        [&](ForNode for_node) {
            expr_code.add("for (");
            HashSet<StringName> for_decls;
            {
                auto [code, refs, decls] = gen_operation_list(for_node.init);
                expr_code.add(code);
                insert_all(expr_refs, refs);
                for_decls = std::move(decls);
            }
            expr_code.add("; ");
            {
                auto [code, refs, decls] = gen_operation_list(for_node.cond);
                expr_code.add(code);
                insert_all_excluding(expr_refs, refs, for_decls);
                insert_all(for_decls, decls);
            }
            expr_code.add("; ");
            {
                auto [code, refs, decls] = gen_operation_list(for_node.post);
                expr_code.add(code);
                insert_all_excluding(expr_refs, refs, for_decls);
                insert_all(for_decls, decls);
            }
            expr_code.add(") ");
            {
                CodeBuilder::CodeInfo info;
                if (for_node.is_scoped) {
                    info = gen_expression(for_node.body);
                } else {
                    info = gen_expression(for_node.body.body[0]);
                }
                auto& [code, refs, _] = info;
                expr_code.add_line(code);
                insert_all_excluding(expr_refs, refs, for_decls);
            }
        },
        [&](ScopeNode subblock) {
            expr_code.add_line("{");
            expr_code.indent();

            auto [code, refs, _] = gen_scope(subblock);
            expr_code.add(code);
            insert_all(expr_refs, refs);

            expr_code.unindent();
            expr_code.add("}");
        },
        [&](FunctionDecl func_decl) {
            for (const auto& specifier : func_decl.return_type.specifiers) {
                expr_code.add("%s ", specifier->contents);
            }
            expr_code.add("%s %s(", to_original_type(func_decl.return_type.type->contents), func_decl.name);
            bool is_first_arg = true;
            HashSet<StringName> arg_names;
            for (const auto& arg_decl : func_decl.args) {
                if (is_first_arg) {
                    is_first_arg = false;
                } else {
                    expr_code.add(", ");
                }
                auto [code, refs, _] = gen_var_decl(arg_decl);
                expr_code.add("%s", code.get_output());
                arg_names.insert(arg_decl.name);
            }
            expr_code.add_line(") ");
            auto [code, refs, _] = gen_expression(func_decl.code);
            expr_code.add_line(code);
            insert_all_excluding(expr_refs, refs, arg_names);
            func_infos[func_decl.name] = {expr_code, expr_refs};
        },
        [&](ReturnExpression ret_expr) {
            expr_code.add("return");
            if (!ret_expr.return_val.operations.is_empty()) {
                auto [code, refs, _] = gen_operation_list(ret_expr.return_val);
                expr_code.add(" %s", code.get_output());
                insert_all(expr_refs, refs);
            }
            expr_code.add_line(";");
        });
    return {expr_code, expr_refs, expr_vars};
}

CodeBuilder::CodeInfo CodeBuilder::gen_scope(const ScopeNode &block) {
    ConsoleString block_code;
    HashSet<StringName> block_refs; 
    HashSet<StringName> block_vars; 
    for (const auto &expr : block.body) {
        auto [code, refs, local_vars] = gen_expression(expr);
        block_code.add_line(code);
        insert_all_excluding(block_refs, refs, block_vars);
        insert_all(block_vars, local_vars);
    }
    return {block_code, block_refs};
}

KernelDef CodeBuilder::gen_kernel(const KernelNode &kernel) {
    ComputeKernel::KernelInfo kernel_info;
    ConsoleString kernel_code;
    ConsoleString x_threads = gen_operation_list(kernel.local_x_threads).code;
    ConsoleString y_threads = gen_operation_list(kernel.local_y_threads).code;
    ConsoleString z_threads = gen_operation_list(kernel.local_z_threads).code;
    kernel_code.add("layout(local_size_x");
#ifdef SPECIALIZATION_CONSTANTS_WORKGROUPS_USEABLE
    if (spec_constants.has(x_threads.get_output())) {
        auto spec_constant = spec_constants[x_threads.get_output()];
        kernel_code.add("_id = %d, local_size_x = %d, ", spec_constant.constant_id, spec_constant.value);
        kernel_info.local_invocations[0] = x_threads.get_output();
    } else {
        kernel_code.add(" = %s, ", x_threads.get_output());
        kernel_info.local_invocations[0] = (uint32_t) x_threads.get_output().to_int();
    }

    kernel_code.add("local_size_y");
    if (spec_constants.has(y_threads.get_output())) {
        auto spec_constant = spec_constants[y_threads.get_output()];
        kernel_code.add("_id = %d, local_size_y = %d, ", spec_constant.constant_id, spec_constant.value);
        kernel_info.local_invocations[1] = y_threads.get_output();
    } else {
        kernel_code.add(" = %s, ", y_threads.get_output());
        kernel_info.local_invocations[1] = (uint32_t) y_threads.get_output().to_int();
    }

    kernel_code.add("local_size_z");
    if (spec_constants.has(z_threads.get_output())) {
        auto spec_constant = spec_constants[z_threads.get_output()];
        kernel_code.add_line("_id = %d, local_size_z = %d) in;", spec_constant.constant_id, spec_constant.value);
        kernel_info.local_invocations[2] = z_threads.get_output();
    } else {
        kernel_code.add_line(" = %s) in;", z_threads.get_output());
        kernel_info.local_invocations[2] = (uint32_t) z_threads.get_output().to_int();
    }
#else
    if (spec_constants.has(x_threads.get_output())) {
        WARN_PRINT_ONCE(vformat("Kernel \"%s\", local size x: Specialization-constant-sized workgroups are not currently supported by Godot on all APIs, so this feature is disabled. The default value will be used instead", kernel.name));
        auto spec_constant = spec_constants[x_threads.get_output()];
        kernel_code.add(" = %d, ", spec_constant.value);
        kernel_info.local_invocations[0] = spec_constant.value;
    } else {
        kernel_code.add(" = %s, ", x_threads.get_output());
        kernel_info.local_invocations[0] = (uint32_t) x_threads.get_output().to_int();
    }
    

    kernel_code.add("local_size_y");
    if (spec_constants.has(y_threads.get_output())) {
        WARN_PRINT_ONCE(vformat("Kernel \"%s\", local size y: Specialization-constant-sized workgroups are not currently supported by Godot on all APIs, so this feature is disabled. The default value will be used instead", kernel.name));
        auto spec_constant = spec_constants[y_threads.get_output()];
        kernel_code.add(" = %d, ", spec_constant.value);
        kernel_info.local_invocations[1] = spec_constant.value;
    } else {
        kernel_code.add(" = %s, ", y_threads.get_output());
        kernel_info.local_invocations[1] = (uint32_t) y_threads.get_output().to_int();
    }


    kernel_code.add("local_size_z");
    if (spec_constants.has(z_threads.get_output())) {
        WARN_PRINT_ONCE(vformat("Kernel \"%s\", local size z: Specialization-constant-sized workgroups are not currently supported by Godot on all APIs, so this feature is disabled. The default value will be used instead", kernel.name));
        auto spec_constant = spec_constants[z_threads.get_output()];
        kernel_code.add_line(" = %d) in;", spec_constant.value);
        kernel_info.local_invocations[2] = spec_constant.value;
    } else {
        kernel_code.add_line(" = %s) in;", z_threads.get_output());
        kernel_info.local_invocations[2] = (uint32_t) z_threads.get_output().to_int();
    }

#endif
    // kernel_code.add_line("layout(local_size_x = %s, local_size_y = %s, local_size_z = %s) in;", x_threads.get_output(), y_threads.get_output(), z_threads.get_output());
    kernel_info.kernel_name = kernel.name;
    HashSet<StringName> local_identifiers;
    if (kernel.push_constants.size() > 0) {
        kernel_code.add_line("layout(push_constant) restrict readonly uniform PushConstants {");
        kernel_code.indent();
        uint32_t curr_index = 0;
        for (auto &push_constant : kernel.push_constants) {
            auto var_info = gen_var_decl(push_constant);
            kernel_code.add(var_info.code);
            VariableInfo pc_info;
            pc_info.type = typeref_to_fslType(push_constant.type);
            kernel_code.add_line(";");
            kernel_info.push_constants[push_constant.name] = pc_info;
            local_identifiers.insert(push_constant.name);
        }
        kernel_code.unindent();
        kernel_code.add_line("};");
    }
    kernel_code.add_line("");
    kernel_code.add_line("void main() {");
    kernel_code.indent();
    for (const auto& [name, bound_name] : kernel.name_bindings) {
        local_identifiers.insert(name);
        kernel_code.add_line(gen_binding_code(name, bound_name));
    }
    auto [code, refs, _] = gen_scope(kernel.code);
    kernel_code.add(code);
    kernel_code.unindent();
    kernel_code.add_line("}");
    ConsoleString resources_code;
    uint32_t next_binding = 0;
    uint32_t set = 0;
    resources_code.add(string_builder);
    for (auto &ref_name : refs) {
        if (!local_identifiers.has(ref_name) && !global_vars.has(ref_name)) {
            if (resources.has(ref_name)) {
                auto [resource_info, resource_code] = gen_resource(resources[ref_name], set, next_binding++);
                kernel_info.bindings[resources[ref_name].name] = resource_info;
                resources_code.add(resource_code);
            }
        } 
    }
    resources_code.add(std::move(kernel_code));
    kernel_info.specialization_constants = spec_constants;
    return {kernel_info, resources_code.get_output()};
}

Pair<BufferInfo, String> CodeBuilder::gen_buffer(const String &buffer_name, const BufferDef &buffer) {
	BufferInfo buf_info;
    String res_code = "";
    buf_info.type = buffer.buftype;
    buf_info.format = buffer.layout;
    String buffer_type = buffer.buftype == UNIFORM ? "uniform" : "buffer";
    String buffer_layout = buffer.layout == STD140 ? "std140" : "std430";
    res_code += vformat("%s) %s restrict %s {\n", buffer_layout, buffer_type, buffer_name);
    uint32_t curr_offset = 0;
    for (auto field : buffer.fields) {
        BufferFieldInfo field_info;
        String type_string = "";
        String postname_string = "";
        for (const auto &token : field.type.specifiers) {
            type_string += token->contents;
        }
        type_string += to_original_type(field.type.type->contents);
        for (const auto &array_dim : field.type.array_dims) {
            postname_string += "[";
            if (!array_dim.operations.is_empty()) {
                postname_string += gen_operation_list(array_dim).code.get_output();
            }
            postname_string += "]";
        }
        field_info.type = typeref_to_fslType(field.type);
        if (const FSLArray* fsl_array = get_if<FSLArray>(&field_info.type)) {
            for (const auto& dimension : fsl_array->dimensions) {
                if (dimension == 0) {
                    buf_info.has_unsized_field = true;
                }
                field_info.dimensions.push_back(dimension);
            }
        }
        field_info.offset = curr_offset;
        curr_offset += get_fsl_type_alignment(field_info.type, 1, buffer.layout);

        buf_info.fields[field.name] = field_info;
        res_code += vformat("\t%s %s%s;\n", type_string, field.name, postname_string);
    }
    if (!(buffer.buffer_name.length() == 0)) {
        res_code += vformat("} %s;\n",  buffer.buffer_name);
    } else {
        res_code += "};\n";
    }
	return {buf_info, res_code};
}

Pair<ResourceInfo, String> CodeBuilder::gen_resource(const ResourceNode &resource, uint32_t set, uint32_t binding) {
    ResourceInfo res_info;
    res_info.set = set;
    res_info.binding = binding;
    String resource_code = vformat("\nlayout(set = 0, binding = %d, ", binding);
    match(resource.resource, 
        [&](const BufferDef &buffer)          { 
            auto [buf_info, res_code] = gen_buffer(resource.name, buffer);
            resource_code += res_code;
            res_info.type_info = buf_info;
        },
        [&](const TextureDef &texture) { 
            auto [tex_info, res_code] = texture_to_glsl(resource.name, texture);
            resource_code += res_code;
            res_info.type_info = tex_info;
        },
        [&](const VariableDecl &uniform)  {
            VariableInfo var_info;
            res_info.type_info = var_info;
        });
    return {res_info, resource_code};
}

CodeBuilder::CodeBuilder(const fslAST &ast) {
    string_builder.add_line("#version 450\n");
    // collect resources first so that declaration order doesn't matter
    for (const GlobalDeclaration &decl : ast.contents) {
        if (const ResourceNode* p_resource = get_if<ResourceNode>(&decl.value)) {
            const ResourceNode& resource = *p_resource;
            resources[resource.name] = resource;
            match(resource.resource, 
                [&](const BufferDef &buffer) {
                    resources[buffer.buffer_name] = resource;
                    for (auto field : buffer.fields) {
                        resources[field.name] = resource;
                    }
                },
                [&](const TextureDef &texture) { },
                [&](const VariableDecl &uniform)  {});
        }
    }

    for (const GlobalDeclaration &decl : ast.contents) {
        match (decl.value, 
            [&](const Expression &expression) { 
                auto [code, _, decls] = gen_expression(expression);
                string_builder.add(code);
                insert_all(global_vars, decls);
            },
            [&](const KernelNode &kernel) { 
                auto kernel_def = gen_kernel(kernel);
                kernel_defs[kernel.name] = kernel_def;
            },
            [&](const ResourceNode &resource) {
            });
    }
}

HashMap<StringName, KernelDef> CodeBuilder::get_kernels(const fslAST &ast) {
    auto code_builder = CodeBuilder(ast);
	return code_builder.kernel_defs;
}
