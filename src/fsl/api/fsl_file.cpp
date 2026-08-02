#include "fsl_file.h"
#include "godot_cpp/classes/file_access.hpp"
#include "console_string.h"
using namespace AST;
void print_operation(const Operation& op, ConsoleString& output);
void print_expression(const Expression& expression, ConsoleString &output);
void print_resource_node(ResourceNode& resource_node, ConsoleString &output);
void print_buffer_def(BufferDef& buf_def, ConsoleString &output);
void print_texture_def(TextureDef& tex_def, ConsoleString &output);
void print_variable_decl(VariableDecl& var_decl, ConsoleString &output);
void print_scope_node(ScopeNode& block_node, ConsoleString &output);
void print_func_decl(FunctionDecl& func_decl, ConsoleString &output);

/******** STATIC METHODS *********/
void FSLFile::_bind_methods() {
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

/******** HELPERS *********/


void FSLFile::load_shader() {
    auto ast_out = FSLParser::get_ast(path);
    if (!ast_out.has_value()) {
        print_error(vformat("Failed to load shader file at \"%s\", check error messages for more detail", path));
        return;
    }
    currAst = std::move(*ast_out);
    compute_kernels = CodeBuilder::get_kernels(currAst);
}


void FSLFile::print_AST() {
    ConsoleString output;
    output.add_line("fslAST {");
    output.indent();
    for (GlobalDeclaration &decl : currAst.contents) {
        std::visit(overload{
            [&](Expression &expression)          { 
                print_expression(expression, output);
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
                output.add_line("Local threads [");
                output.indent();
                print_operation(kernel.local_x_threads, output);
                print_operation(kernel.local_y_threads, output);
                print_operation(kernel.local_z_threads, output);
                output.unindent();
                output.add_line("],");
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
    if (!type_ref.specifiers.is_empty()) {
        output.add_line("Specifiers: %s,", tokens_to_string(type_ref.specifiers));
    }
    output.add_line("Type: %s,", type_ref.type->contents);
    if (type_ref.array_dims.size() > 0) {
        output.add_line("Array dimensions {");
        output.indent();
        for (const auto& array_dim : type_ref.array_dims) {
            print_operation(array_dim, output);
        }
        output.unindent();
        output.add_line("},");
    }
    output.unindent();
    output.add_line("},");
}

void print_operation(const Operation &op, ConsoleString &output) {
    std::visit(overload{
        [&](VariableDecl var_decl) {
            print_variable_decl(var_decl, output);
        },
        [&](Operator oper) {
            output.add_line("Operator: %s,", oper.symbol);
        },
        [&](FuncCall func_call) {
            output.add_line("FuncCall {");
            output.indent();
            if (!func_call.is_valid) {
                output.add_line("Error");
                output.unindent();
                output.add_line("},");
                return;
            }
            output.add_line("Name: %s,", func_call.name);
            if (func_call.args.size() > 0) {
                output.add_line("Args {");
                output.indent();
                for (const auto& arg : func_call.args) {
                    print_operation(arg, output);
                }
                output.unindent();
                output.add_line("},");
            }
            output.unindent();
            output.add_line("},");
        },
        [&](VariableRef var_ref) {
            if (!var_ref.is_valid) {
                output.add_line("VariableRef: Error,");
                return;
            }
            output.add_line("VariableRef: %s,", var_ref.name);
        },
        [&](OperationList op_list) {
            output.add_line("OperationList {");
            output.indent();
            if (!op_list.is_valid) {
                output.add_line("Error");
                output.unindent();
                output.add_line("},");
                return;
            }
            for (const auto& operation : op_list.operations) {
                print_operation(operation, output);
            }
            output.unindent();
            output.add_line("},");
        }, 
        [&](FieldAccess field_access) {
            if (!field_access.is_valid) {
                output.add_line("FieldAccess: Error,");
                return;
            }
            output.add_line("FieldAccess: %s,", field_access.field_name);
        }, 
        [&](ArrayIndex array_index) {
            output.add_line("ArrayIndex {");
            output.indent();
            if (!array_index.is_valid) {
                output.add_line("Error");
                output.unindent();
                output.add_line("},");
                return;
            }
            output.add_line("Indices {");
            output.indent();
            for (const auto& index : array_index.indices) {
                print_operation(index, output);
            }
            output.unindent();
            output.add_line("},");
            output.unindent();
            output.add_line("},");
        }, 
        [&](Literal literal) {
            if (!literal.is_valid) {
                output.add_line("Literal: Error,");
                return;
            }
            output.add_line("Literal: %s,", literal.value);
        },
        [&](UnknownOp error_op) {
            output.add_line("Error {");
            output.indent();
            output.add_line("Code: %s", tokens_to_string(error_op.code));
            output.unindent();
            output.add_line("},");
        }
    }, op);
}

void print_expression(const Expression &expression, ConsoleString &output) {
	std::visit(overload{
        [&](Operation op_expression) {
            print_operation(op_expression, output);
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
            output.add_line("Condition {");
            output.indent();
            print_operation(if_node.cond, output);
            output.unindent();
            output.add_line("},");
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
            if (else_node.is_scoped) {
                output.add_line("Body {");
                output.indent();
                print_scope_node(else_node.body, output);
                output.unindent();
                output.add_line("},");
            } else {
                output.add("Body: ");
                print_expression(else_node.body.body[0], output);
            }
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
            output.add_line("Initial statement {");
            output.indent();
            print_operation(for_node.init, output);
            output.unindent();
            output.add_line("},");
            output.add_line("Condition {");
            output.indent();
            print_operation(for_node.cond, output);
            output.unindent();
            output.add_line("},");
            output.add_line("Post-check statement {");
            output.indent();
            print_operation(for_node.post, output);
            output.unindent();
            output.add_line("},");
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
        },
        [&](FunctionDecl func_decl) {
            print_func_decl(func_decl, output);
        },
        [&](ReturnExpression ret_expr) {
            output.add_line("ReturnExpression {");
            output.indent();
            if (!ret_expr.is_valid) {
                output.add_line("Error");
                output.unindent();
                output.add_line("},");
                return;
            }
            print_operation(ret_expr.return_val, output);
            output.unindent();
            output.add_line("},");
        }
    }, expression);
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
    if (!var_decl.annotations.is_empty()) {
        output.add_line("Annotations {");
        output.indent();
        for (const auto& [name, args] : var_decl.annotations) {
            output.add("Name: %s,", name);
            if (!args.is_empty()) {
                output.add_line(" Args: {");
                output.indent();
                for (const auto& arg : args) {
                    print_operation(arg, output);
                }
                output.unindent();
                output.add_line("},");
            }
        }
        output.unindent();
        output.add_line("},");
    }
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
        print_expression(expr, output);
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