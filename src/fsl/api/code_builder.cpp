#include "code_builder.h"

ConsoleString CodeBuilder::gen_statement(const Statement &statement, const HashMap<StringName, String> &renames) {
    ConsoleString statement_code;
    for (const auto& token : statement) {
        switch (token.get_type()){
            case Token::IDENTIFIER: {
                if (renames.has(token.get_contents())) {
                    statement_code.add(to_original_name(renames[token.get_contents()]));
                } else {
                    // if (resources.has(token.contents)) {
                    //     StringName resource_name = resources[token.contents].name;
                    //     if (!used_resources.has(resource_name)) {
                    //         used_resources.push_back(resource_name);
                    //     }
                    // }
                    statement_code.add(to_original_type(token.get_contents()));
                }
            } break;
            default:
                statement_code.add(token.get_contents());
        }
    }
    return statement_code;
}

ConsoleString CodeBuilder::gen_scope(const ScopeNode &block, const HashMap<StringName, String> &renames) {
    ConsoleString block_code;
    for (const auto &expr : block.body) {
        std::visit(overload{
            [&](Statement expression) {
                block_code.add(gen_statement(expression, renames));
                block_code.add_line(";");
            },
            [&](IfNode if_node) {
                block_code.add("if (");
                block_code.add(gen_statement(if_node.cond, renames));
                block_code.add(") {");
                block_code.indent();
                block_code.add(gen_scope(if_node.body, renames));
                block_code.unindent();
                block_code.add_line("}");
            },
            [&](ElseNode else_node) {
            },
            [&](ForNode for_node) {
                block_code.add("for (");
                block_code.add(gen_statement(for_node.init, renames));
                block_code.add("; ");
                block_code.add(gen_statement(for_node.cond, renames));
                block_code.add("; ");
                block_code.add(gen_statement(for_node.post, renames));
                block_code.add(") {");
                block_code.indent();
                block_code.add(gen_scope(for_node.body, renames));
                block_code.unindent();
                block_code.add_line("}");
            },
            [&](ScopeNode subblock) {
                block_code.add_line("{");
                block_code.indent();
                block_code.add(gen_scope(subblock, renames));
                block_code.unindent();
                block_code.add_line("}");
            }
        }, expr);
    }
    return block_code;
}

KernelDef CodeBuilder::gen_kernel(const KernelNode &kernel, const HashMap<StringName, ResourceNode> &resources) {
    ComputeKernel::KernelInfo kernel_info;
    ConsoleString kernel_code;
    kernel_code.add_line("layout(local_size_x = %d, local_size_y = %d, local_size_z = %d) in;", tokens_to_string(kernel.local_x_threads).to_int(), tokens_to_string(kernel.local_y_threads).to_int(), tokens_to_string(kernel.local_z_threads).to_int());
    kernel_info.kernel_name = kernel.name;
    kernel_info.local_invocations[0] = tokens_to_string(kernel.local_x_threads).to_int();
    kernel_info.local_invocations[1] = tokens_to_string(kernel.local_y_threads).to_int();
    kernel_info.local_invocations[2] = tokens_to_string(kernel.local_z_threads).to_int();

    LocalVector<StringName> used_resources;
    if (kernel.push_constants.size() > 0) {
        kernel_code.add_line("layout(push_constant) restrict readonly uniform PushConstants {");
        kernel_code.indent();
        uint32_t curr_index = 0;
        for (auto &push_constant : kernel.push_constants) {
            VariableInfo pc_info;
            pc_info.type = typeref_to_fslType(push_constant.type);
            kernel_code.add("%s %s", tokens_to_string(push_constant.type.type), push_constant.name);
            if (push_constant.type.array_dims.size() > 0) {
                for (const auto& array_dim : push_constant.type.array_dims) {
                    kernel_code.add("%s", tokens_to_string(array_dim));
                }
            }
            kernel_code.add_line(";");
            kernel_info.push_constants[push_constant.name] = pc_info;
        }
        kernel_code.unindent();
        kernel_code.add_line("};");
    }
    kernel_code.add_line("");
    kernel_code.add_line("void main() {");
    kernel_code.indent();
    kernel_code.add(gen_scope(kernel.code, kernel.name_bindings));
    kernel_code.unindent();
    kernel_code.add_line("}");
    ConsoleString resources_code;
    uint32_t next_binding = 0;
    uint32_t set = 0;
    resources_code.add(string_builder);
    for (auto &resource_name : used_resources) {
        auto [resource_info, resource_code] = gen_resource(resources[resource_name], set, next_binding++);
        kernel_info.bindings[resource_name] = resource_info;
        resources_code.add(resource_code);
    }
    resources_code.add(std::move(kernel_code));
    return {kernel_info, resources_code.get_output()};
}



Pair<ResourceInfo, String> CodeBuilder::gen_resource(const ResourceNode &resource, uint32_t set, uint32_t binding) {
    ResourceInfo res_info;
    res_info.set = set;
    res_info.binding = binding;
    String resource_code = vformat("\nlayout(set = 0, binding = %d, ", binding);
    std::visit(overload{
        [&](const BufferDef &buffer)          { 
            auto [buf_info, res_code] = buffer_to_glsl(resource.name, buffer);
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
        }
    }, resource.resource);
    return {res_info, resource_code};
}

CodeBuilder::CodeBuilder(const fslAST &ast) {
    string_builder.add_line("#version 450\n");
    HashMap<StringName, ResourceNode> resources;
    for (const GlobalDeclaration &decl : ast.contents) {
        std::visit(overload{
            [&](const Statement &statement) { 
                string_builder.add(gen_statement(statement));
            },
            [&](const KernelNode &kernel) { 
                auto kernel_def = gen_kernel(kernel, resources);
                kernel_defs[kernel.name] = kernel_def;
            },
            [&](const ResourceNode &resource) {
                resources[resource.name] = resource;
                std::visit(overload{
                    [&](const BufferDef &buffer) {
                        resources[buffer.buffer_name] = resource;
                        for (auto field : buffer.fields) {
                            resources[field.name] = resource;
                        }
                    },
                    [&](const TextureDef &texture) { },
                    [&](const VariableDecl &uniform)  {}
                }, resource.resource);
            },
            [&](const FunctionDecl &func) {
                
            }
        }, decl.value);
    }
}


HashMap<StringName, KernelDef> CodeBuilder::get_kernels(const fslAST &ast) {
    auto code_builder = CodeBuilder(ast);
	return code_builder.kernel_defs;
}
