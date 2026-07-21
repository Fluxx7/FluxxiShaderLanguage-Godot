#include "fsl_file.h"
#include "godot_cpp/classes/file_access.hpp"
#include "console_string.h"

void print_shader_info(fslAST &currAst);
Pair<ResourceInfo, String> gen_resource(ResourceNode &resource, uint32_t set, uint32_t binding, ConsoleString& code_builder);

void print_resource_node(ResourceNode& resource_node, ConsoleString &output);
void print_buffer_def(BufferDef& buf_def, ConsoleString &output);
void print_texture_def(TextureDef& tex_def, ConsoleString &output);
void print_variable_decl(VariableDecl& var_decl, ConsoleString &output);
void print_scope_node(ScopeNode& block_node, ConsoleString &output);
void print_func_decl(FunctionDecl& func_decl, ConsoleString &output);

/******** STATIC METHODS *********/
void FSLFile::_bind_methods() {
    ClassDB::bind_method(D_METHOD("test"), &FSLFile::test);
    ClassDB::bind_method(D_METHOD("print_AST"), &FSLFile::print_AST);
    ClassDB::bind_method(D_METHOD("get_kernel_source", "kernel_name"), &FSLFile::get_kernel_source);
    ClassDB::bind_method(D_METHOD("get_kernel", "kernel_name", "rendering_device"), &FSLFile::get_kernel, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("get_kernel_group", "rendering_device"), &FSLFile::get_kernel_group, DEFVAL(nullptr));
    ClassDB::bind_static_method("FSLFile", D_METHOD("from_file", "file_path"), &FSLFile::from_file);
}

Ref<FSLFile> FSLFile::from_file(String file_path) {
	return Ref<FSLFile>(memnew(FSLFile(file_path)));
}

/******** CONSTRUCTORS *********/

FSLFile::FSLFile(String file_path) {
    path = file_path;
    load_shader();
}

/******** PUBLIC METHODS *********/

String FSLFile::get_kernel_source(StringName kernel_name) {
    if (compute_kernels.find(kernel_name) != compute_kernels.end()) {
        return compute_kernels[kernel_name].source;
    }
    print_error(vformat("Could not find kernel with name \"%s\"", kernel_name));
	return String();
}

Ref<ComputeKernel> FSLFile::get_kernel(StringName kernel_name, RenderingDevice *rd = nullptr) {
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, Ref<ComputeKernel>());
        }
    }
    if (compute_kernels.find(kernel_name) != compute_kernels.end()) {
        auto new_kernel = ComputeKernel::make_new(compute_kernels[kernel_name].source, compute_kernels[kernel_name].info, rd);
        return new_kernel;
    }
    print_error(vformat("Could not find kernel with name \"%s\"", kernel_name));
	return Ref<ComputeKernel>();
}

Ref<ComputeGroup> FSLFile::get_kernel_group(RenderingDevice *rd) {
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, Ref<ComputeGroup>());
        }
    }
    Ref<ComputeGroup> compute_group = ComputeGroup::make_new(rd);
    for (const auto& [kernel_name, kernel_def] : compute_kernels) {
        compute_group->add_kernel(kernel_def.info, kernel_def.source);
    }
	return compute_group;
}

void FSLFile::test() {
	print_shader_info(currAst);
}

/******** HELPERS *********/

void _print_scope(ScopeNode &block) {
    for (const auto &expr : block.body) {
        std::visit(overload{
            [&](Statement expression) {
                printvf("%s;", tokens_to_string(expression));
            },
            [&](IfNode if_node) {
                printvf("If statement");
            },
            [&](ElseNode else_node) {
                printvf("Else statement");
            },
            [&](ForNode for_node) {
                printvf("For statement");
            },
            [&](ScopeNode subblock) {
                printvf("{");
                _print_scope(subblock);
                printvf("}");
            }
        }, expr);
    }
}

void _print_resource(ResourceNode &resource) {
    print_line(vformat("\nResource %s:", resource.name));
    std::visit(overload{
        [&](BufferDef &buffer)          { 
            printvf("\t%s", get_buffer_desc(buffer.buftype, buffer.layout));
            print_line("\tBuffer fields:");
            for (auto field : buffer.fields) {
                print_line(vformat("\t\tName: %s, type: %s", field.name, typeref_to_string(field.type)));
            }
        },
        [&](TextureDef &texture) { 
            String tex_format = texture.format == RGBA16F ? "rgba16f" : "rgba32f";
            print_line(vformat("\tTexture of type %s with format %s", tex_to_glsl_name(texture.type), tex_format));
        },
        [&](VariableDecl &uniform)  {
            print_line("How the fuck");
        }
    }, resource.resource);
}

void FSLFile::load_shader() {
    auto ast_out = FSLParser::get_ast(path);
    if (!ast_out.has_value()) {
        print_error(vformat("Failed to load shader file at \"%s\", check error messages for more detail", path));
        return;
    }
    currAst = std::move(*ast_out);
    compute_kernels = CodeBuilder::get_kernels(currAst);
}

void print_shader_info(fslAST &currAst) {
    for (GlobalDeclaration &decl : currAst.contents) {
        std::visit(overload{
            [&](Statement &statement)          { 
                print_line("\nShared code:");
                printvf("%s;", tokens_to_string(statement));
            },
            [&](KernelNode &kernel)      { 
                print_line("\nKernel " + kernel.name + ":");
                print_line(vformat("Local threads: %d, %d, %d", tokens_to_string(kernel.local_x_threads).to_int(), tokens_to_string(kernel.local_y_threads).to_int(), tokens_to_string(kernel.local_z_threads).to_int()));
                if (kernel.name_bindings.size() > 0) {
                    print_line("Name bindings:");
                    for (auto [bound_name, orig_name] : kernel.name_bindings) {
                        print_line(vformat("\tBound name: %s, original name: %s", bound_name, orig_name));
                    }
                }
                if (kernel.push_constants.size() > 0) {
                    print_line("Push constants:");
                    for (auto &push_constant : kernel.push_constants) {
                        print_line(vformat("\tName: %s, type: %s", push_constant.name, typeref_to_string(push_constant.type)));
                    }
                }
                printvf("Code:");
                _print_scope(kernel.code);
            },
            [&](ResourceNode &resource)  {
                _print_resource(resource);
            },
            [&](FunctionDecl &func) {

            }
        }, decl.value);
    }
}


void FSLFile::print_AST() {
    ConsoleString output;
    output.add_line("fslAST {");
    output.indent();
    for (GlobalDeclaration &decl : currAst.contents) {
        std::visit(overload{
            [&](Statement &statement)          { 
                output.add_line("Statement: %s", tokens_to_string(statement));
            },
            [&](KernelNode &kernel)      { 
                output.add_line("KernelNode {");
                output.indent();
                if (!kernel.is_valid) {
                    output.add_line("Error");
                    output.unindent();
                    output.add_line("},");
                    return;
                }
                output.add_line("Name: %s,", kernel.name);
                output.add_line("Local threads: [%d, %d, %d],", tokens_to_string(kernel.local_x_threads).to_int(), tokens_to_string(kernel.local_y_threads).to_int(), tokens_to_string(kernel.local_z_threads).to_int());
                if (kernel.name_bindings.size() > 0) {
                    output.add_line("Name bindings {");
                    output.indent();
                    for (auto [bound_name, orig_name] : kernel.name_bindings) {
                        output.add_line("Bound name: %s, original name: %s,", bound_name, orig_name);
                    }
                    output.unindent();
                    output.add_line("},");
                }
                if (kernel.push_constants.size() > 0) {
                    output.add_line("Push constants {");
                    output.indent();
                    for (auto &push_constant : kernel.push_constants) {
                        print_variable_decl(push_constant, output);
                    }
                    output.unindent();
                    output.add_line("},");
                }
                output.add_line("Code {");
                output.indent();
                String next_str = "";
                print_scope_node(kernel.code, output);
                output.unindent();
                output.add_line("},");
                output.unindent();
                output.add_line("},");
            },
            [&](ResourceNode &resource)  {
                print_resource_node(resource, output);
            },
            [&](FunctionDecl &func) {
                
            }
        }, decl.value);
    }
    output.unindent();
    output.add_line("}");
    output.print();
}

void print_type(TypeRef &type_ref, ConsoleString &output) {
    output.add_line("TypeRef {");
    output.indent();
    if (!type_ref.is_valid) {
        output.add_line("Error");
        output.unindent();
        output.add_line("},");
        return;
    }
    output.add_line("Type: %s,", tokens_to_string(type_ref.type));
    if (type_ref.array_dims.size() > 0) {
        output.add_line("Array dimensions {");
        output.indent();
        for (const auto& array_dim : type_ref.array_dims) {
            if (array_dim.is_empty()) {
                output.add_line("unsized,");
            } else {
                output.add_line("%s,", tokens_to_string(array_dim));
            }
        }
        output.unindent();
        output.add_line("},");
    }
    output.unindent();
    output.add_line("},");
}

void print_resource_node(ResourceNode &resource_node, ConsoleString &output) {
    output.add_line("ResourceNode {");
    output.indent();
    if (!resource_node.is_valid) {
        output.add_line("Error");
        output.unindent();
        output.add_line("},");
        return;
    }
    output.add_line("Name: %s,", resource_node.name);
    std::visit(overload{
        [&](BufferDef &buffer)          { 
            print_buffer_def(buffer, output);
        },
        [&](TextureDef &texture) { 
            print_texture_def(texture, output);
        },
        [&](VariableDecl &uniform)  {
            print_variable_decl(uniform, output);
        }
    }, resource_node.resource);
    output.unindent();
    output.add_line("},");
}

void print_buffer_def(BufferDef &buf_def, ConsoleString &output) {
    output.add_line("BufferDef {");
    output.indent();
    if (!buf_def.is_valid) {
        output.add_line("Error");
        output.unindent();
        output.add_line("},");
        return;
    }
    if (!buf_def.buffer_name.is_empty()) {
        output.add_line("Local name: %s,", buf_def.buffer_name);
    }
    output.add_line("Type: %s,", bufferType_to_string(buf_def.buftype));
    output.add_line("Layout: %s,", bufferFormat_to_string(buf_def.layout));
    output.add_line("Buffer fields {");
    output.indent();
    for (auto field : buf_def.fields) {
        print_variable_decl(field, output);
    }
    output.unindent();
    output.add_line("},");
    output.unindent();
    output.add_line("},");
}

void print_texture_def(TextureDef& tex_def, ConsoleString &output) {
    output.add_line("TextureDef {");
    output.indent();
    if (!tex_def.is_valid) {
        output.add_line("Error");
        output.unindent();
        output.add_line("},");
        return;
    }
    output.add_line("Type: %s,", textureType_to_string(tex_def.type));
    output.add_line("Format: %s,", textureFormat_to_string(tex_def.format));
    output.unindent();
    output.add_line("},");
}

void print_variable_decl(VariableDecl& var_decl, ConsoleString &output) {
    output.add_line("VariableDecl {");
    output.indent();
    if (!var_decl.is_valid) {
        output.add_line("Error");
        output.unindent();
        output.add_line("},");
        return;
    }
    output.add_line("Name: %s,", var_decl.name);
    output.add_line("Type: {");
    output.indent();
    print_type(var_decl.type, output);
    output.unindent();
    output.add_line("},");
    output.unindent();
    output.add_line("},");
}

void print_scope_node(ScopeNode& block_node, ConsoleString &output) {
    output.add_line("ScopeNode {");
    output.indent();
    if (!block_node.is_valid) {
        output.add_line("Error");
        output.unindent();
        output.add_line("},");
        return;
    }
    for (const auto &expr : block_node.body) {
        std::visit(overload{
            [&](Statement expression) {
                output.add_line("Statement: %s", tokens_to_string(expression));
            },
            [&](IfNode if_node) {
                output.add_line("IfNode {");
                output.indent();
                if (!if_node.is_valid) {
                    output.add_line("Error");
                    output.unindent();
                    output.add_line("},");
                    return;
                }
                output.add_line("Condition: %s,", tokens_to_string(if_node.cond));
                output.add_line("Body {");
                output.indent();
                print_scope_node(if_node.body, output);
                output.unindent();
                output.add_line("},");
                output.unindent();
                output.add_line("},");
                
            },
            [&](ElseNode else_node) {
                output.add_line("ElseNode {");
                output.indent();
                if (!else_node.is_valid) {
                    output.add_line("Error");
                    output.unindent();
                    output.add_line("},");
                    return;
                }
                output.add_line("Body {");
                output.indent();
                print_scope_node(else_node.body, output);
                output.unindent();
                output.add_line("},");
                output.unindent();
                output.add_line("},");
            },
            [&](ForNode for_node) {
                output.add_line("ForNode {");
                output.indent();
                if (!for_node.is_valid) {
                    output.add_line("Error");
                    output.unindent();
                    output.add_line("},");
                    return;
                }
                output.add_line("Initial statement: %s,", tokens_to_string(for_node.init));
                output.add_line("Condition: %s,", tokens_to_string(for_node.cond));
                output.add_line("Post-check statement: %s,", tokens_to_string(for_node.post));
                output.add_line("Body {");
                output.indent();
                print_scope_node(for_node.body, output);
                output.unindent();
                output.add_line("},");
                output.unindent();
                output.add_line("},");
            },
            [&](ScopeNode subblock) {
                print_scope_node(subblock, output);
            }
        }, expr);
    }
    output.unindent();
    output.add_line("},");
}

void print_func_decl(FunctionDecl &func_decl, ConsoleString &output) {
    output.add_line("FunctionDecl {");
    output.indent();
    if (!func_decl.is_valid) {
        output.add_line("Error");
        output.unindent();
        output.add_line("},");
        return;
    }
    output.add_line("Name: %s,", func_decl.name);
    output.add_line("Return type {");
    output.indent();
    print_type(func_decl.return_type, output);
    output.unindent();
    output.add_line("},");
    if (func_decl.args.size() > 0) {
        output.add_line("Arguments {");
        output.indent();
        for (auto &arg : func_decl.args) {
            print_variable_decl(arg, output);
        }
        output.unindent();
        output.add_line("},");
    }
    output.add_line("Code {");
    output.indent();
    String next_str = "";
    print_scope_node(func_decl.code, output);
    output.unindent();
    output.add_line("},");
    output.unindent();
    output.add_line("},");
}