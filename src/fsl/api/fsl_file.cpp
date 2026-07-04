#include "fsl_file.h"
#include "godot_cpp/classes/file_access.hpp"
 
void print_shader_info(fslAST &currAst);
Pair<ResourceInfo, String> gen_resource(ResourceNode &resource, uint32_t set, uint32_t binding);

String to_original_type(String identifier) {
    static const char *changed_types[] = {
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
    static const char *original_types[] = {
        "vec2",
        "vec3",
        "vec4",
        "ivec2",
        "ivec3",
        "ivec4",
        "uvec2",
        "uvec3",
        "uvec4",
        "dvec2",
        "dvec3",
        "dvec4",
        "bvec2",
        "bvec3",
        "bvec4"
    }; 
    uint32_t curr_index = 0;
    for (auto type_name : changed_types) {
        if (identifier == type_name) {
            return original_types[curr_index];
        }
        curr_index++;
    }
    return identifier;
}

String to_original_name(const String &name) {
    static const char *custom_names[] = {
        "GlobalInvocationID"
    }; 
    static const char *original_names[] = {
        "gl_GlobalInvocationID"
    }; 
    uint32_t index;
    for (auto custom_name : custom_names) {
        if (name == custom_name) {
            return original_names[index];
        }
        index++;
    }
    return name;
}

/******** STATIC METHODS *********/
void FSLFile::_bind_methods() {
    ClassDB::bind_method(D_METHOD("test"), &FSLFile::test);
    ClassDB::bind_method(D_METHOD("get_kernel_source", "kernel_name"), &FSLFile::get_kernel_source);
    ClassDB::bind_method(D_METHOD("get_kernel", "kernel_name", "rendering_device"), &FSLFile::get_kernel, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("get_kernel_group", "rendering_device"), &FSLFile::get_kernel_group, DEFVAL(nullptr));
    ClassDB::bind_static_method("FSLFile", D_METHOD("from_file", "file_path"), &FSLFile::from_file);
}

Ref<FSLFile> FSLFile::from_file(String file_path) {
	return Ref<FSLFile>(memnew(FSLFile(file_path)));
}

/******** CONSTRUCTORS *********/

FSLFile::FSLFile() {
    kernel_sources = HashMap<StringName, String>();
}

FSLFile::FSLFile(String file_path) {
    kernel_sources = HashMap<StringName, String>();
    path = file_path;
    load_shader();
}

/******** PUBLIC METHODS *********/

String FSLFile::get_kernel_source(StringName kernel_name) {
    if (kernel_sources.find(kernel_name) != kernel_sources.end()) {
        return kernel_sources[kernel_name];
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
    if (kernel_sources.find(kernel_name) != kernel_sources.end()) {
        auto new_kernel = ComputeKernel::make_new(kernel_sources[kernel_name], compute_kernels[kernel_name], rd);
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
    for (const auto& [kernel_name, kernel_info] : compute_kernels) {
        compute_group->add_kernel(kernel_info, kernel_sources[kernel_name]);
    }
	return compute_group;
}

void FSLFile::test() {
    print_shader_info(currAst);
}

/******** HELPERS *********/

Pair<ComputeKernel::KernelInfo, String> gen_kernel(KernelNode &kernel, HashMap<StringName, ResourceNode> &resources) {
    ComputeKernel::KernelInfo kernel_info;
    String kernel_code = vformat("\nlayout(local_size_x = %d, local_size_y = %d, local_size_z = %d) in;\n", kernel.local_x_threads, kernel.local_y_threads, kernel.local_z_threads);
    kernel_info.kernel_name = kernel.name;
    kernel_info.local_invocations[0] = kernel.local_x_threads;
    kernel_info.local_invocations[1] = kernel.local_y_threads;
    kernel_info.local_invocations[2] = kernel.local_z_threads;

    LocalVector<StringName> used_resources;
    if (kernel.push_constants.size() > 0) {
        kernel_code += "\nlayout(push_constant) restrict readonly uniform PushConstants {\n";
        uint32_t curr_index = 0;
        for (auto &push_constant : kernel.push_constants) {
            VariableInfo pc_info;
            pc_info.type = tokens_to_fslType(push_constant.type);
            kernel_code += vformat("\t%s %s;\n", tokens_to_string(push_constant.type), push_constant.name);
            kernel_info.push_constants[push_constant.name] = pc_info;
        }
        kernel_code += "};\n";
    }

    kernel_code += "\nvoid main() {";
    for (const auto &token : kernel.code) {
        switch (token.token_type) {
            case Token::IDENTIFIER: {
                if (kernel.name_bindings.has(token.contents)) {
                    kernel_code += to_original_name(kernel.name_bindings[token.contents]);
                } else {
                    if (resources.has(token.contents)) {
                        StringName resource_name = resources[token.contents].name;
                        if (!used_resources.has(resource_name)) {
                            used_resources.push_back(resource_name);
                        }
                    }
                    kernel_code += token.contents;
                }
            } break;
            case Token::TYPE: {
                kernel_code += to_original_type(token.contents);
            } break;
            default:
                kernel_code += token.contents;
        }
    }
    kernel_code += "}\n";
    String resources_code = "";
    uint32_t next_binding = 0;
    uint32_t set = 0;
    for (auto &resource_name : used_resources) {
        auto [resource_info, resource_code] = gen_resource(resources[resource_name], set, next_binding++);
        kernel_info.bindings[resource_name] = resource_info;
        resources_code += resource_code;
    }
    return {kernel_info, resources_code + kernel_code};
}

Pair<ResourceInfo, String> gen_resource(ResourceNode &resource, uint32_t set, uint32_t binding) {
    ResourceInfo res_info;
    res_info.set = set;
    res_info.binding = binding;
    String resource_code = vformat("\nlayout(set = 0, binding = %d, ", binding);
    std::visit(overload{
        [&](BufferDef &buffer)          { 
            BufferInfo buf_info;
            buf_info.type = buffer.buftype;
            buf_info.format = buffer.layout;
            String buffer_type = buffer.buftype == UNIFORM ? "uniform" : "buffer";
            String buffer_layout = buffer.layout == STD140 ? "std140" : "std430";
            resource_code += vformat("%s) %s restrict %s {\n", buffer_layout, buffer_type, resource.name);
            for (auto field : buffer.fields) {
                VariableInfo field_info;
                String type_string = "";
                String postname_string = "";
                for (const auto &token : field.type) {
                    switch (token.token_type) {
                        case Token::SPECIFIER:
                            type_string += token.contents;
                            break;
                        case Token::TYPE: {
                            type_string += to_original_type(token.contents);
                        } break;
                        default:
                            postname_string += token.contents;
                            break;
                    }
                }
                field_info.type = tokens_to_fslType(field.type);
                if (const FSLArray* fsl_array = std::get_if<FSLArray>(&field_info.type)) {
                    for (const auto& dimension : fsl_array->dimensions) {
                        if (dimension == 0) {
                            buf_info.has_unsized_field = true;
                        }
                    }
                }
                buf_info.fields[field.name] = field_info;
                resource_code += vformat("\t%s %s%s;\n", type_string, field.name, postname_string);
            }
            resource_code += "};\n";
            res_info.type_info = buf_info;
        },
        [&](TextureDef &texture) { 
            TextureInfo tex_info;
            tex_info.format = texture.format;
            String tex_format = texture.format == RGBA16F ? "rgba16f" : "rgba32f";
            resource_code += vformat("%s) restrict uniform image2D %s;\n", tex_format, resource.name);
            res_info.type_info = tex_info;
        },
        [&](VariableDecl &uniform)  {
            VariableInfo var_info;
            res_info.type_info = var_info;
        }
    }, resource.resource);
    return {res_info, resource_code};
}

void _print_resource(ResourceNode &resource) {
    print_line(vformat("\nResource %s:", resource.name));
    std::visit(overload{
        [&](BufferDef &buffer)          { 
            String buffer_type = buffer.buftype == UNIFORM ? "Uniform" : "Storage";
            String buffer_layout = buffer.layout == STD140 ? "std140" : "std430";
            print_line(vformat("\t%s buffer with format %s", buffer_type, buffer_layout));
            print_line("\tBuffer fields:");
            for (auto field : buffer.fields) {
                print_line(vformat("\t\tName: %s, type: %s", field.name, tokens_to_string(field.type)));
            }
        },
        [&](TextureDef &texture) { 
            String tex_format = texture.format == RGBA16F ? "rgba16f" : "rgba32f";
            print_line(vformat("\tTexture with format %s", tex_format));
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
    currAst = *ast_out;
    String shared_code = "#version 450\n";
    HashMap<StringName, ResourceNode> resources;
    for (GlobalDeclaration &decl : currAst.contents) {
        std::visit(overload{
            [&](CodeNode &code) { 
                shared_code += "\n";
                for (const auto &token : code.code) {
                    switch (token.token_type) {
                        case Token::TYPE: {
                            shared_code += to_original_type(token.contents);
                        } break;
                        default:
                            shared_code += token.contents;
                            break;
                    }
                }
            },
            [&](KernelNode &kernel) { 
                auto [kernel_info, kernel_source] = gen_kernel(kernel, resources);
                kernel_sources[kernel.name] = shared_code + kernel_source;
                compute_kernels[kernel.name] = kernel_info;
            },
            [&](ResourceNode &resource) {
                resources[resource.name] = resource;
                std::visit(overload{
                    [&](BufferDef &buffer) {
                        for (auto field : buffer.fields) {
                            resources[field.name] = resource;
                        }
                    },
                    [&](TextureDef &texture) { },
                    [&](VariableDecl &uniform)  {}
                }, resource.resource);
            }
        }, decl.value);
    }
}

void print_shader_info(fslAST &currAst) {
    for (GlobalDeclaration &decl : currAst.contents) {
        std::visit(overload{
            [&](CodeNode &code)          { 
                print_line("\nShared code:");
                print_line(vformat("Line number %d", decl.linenum));
                print_line(tokens_to_string(code.code)); 
            },
            [&](KernelNode &kernel)      { 
                print_line("\nKernel " + kernel.name + ":");
                print_line(vformat("Entrypoint line: %d", kernel.entrypoint_line));
                print_line(vformat("Local threads: %d, %d, %d", kernel.local_x_threads, kernel.local_y_threads, kernel.local_z_threads));
                if (kernel.name_bindings.size() > 0) {
                    print_line("Name bindings:");
                    for (auto [bound_name, orig_name] : kernel.name_bindings) {
                        print_line(vformat("\tBound name: %s, original name: %s", bound_name, orig_name));
                    }
                }
                if (kernel.push_constants.size() > 0) {
                    print_line("Push constants:");
                    for (auto &push_constant : kernel.push_constants) {
                        print_line(vformat("\tName: %s, type: %s", push_constant.name, tokens_to_string(push_constant.type)));
                    }
                }
                print_line(vformat("Code: %s", tokens_to_string(kernel.code)));
            },
            [&](ResourceNode &resource)  {
                _print_resource(resource);
            }
        }, decl.value);
    }
}