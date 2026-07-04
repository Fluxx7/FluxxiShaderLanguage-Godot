#include "compute_kernel.h"

/*******************************************************/
/******************* STATIC METHODS ********************/
/*******************************************************/


void ComputeKernel::_bind_methods() {
    BIND_ENUM_CONSTANT(RGBA16F);
    BIND_ENUM_CONSTANT(RGBA32F);
    BIND_ENUM_CONSTANT(UNIFORM);
    BIND_ENUM_CONSTANT(STORAGE);

    ClassDB::bind_method(D_METHOD("print_info"), &ComputeKernel::print_info);

    ClassDB::bind_method(D_METHOD("dispatch", "x_invocations", "y_invocations", "z_invocations", "push_constants"), &ComputeKernel::dispatch, DEFVAL((TypedDictionary<uint32_t, uint32_t>())));
    ClassDB::bind_method(D_METHOD("dispatch_workgroups", "x_workgroups", "y_workgroups", "z_workgroups", "push_constants"), &ComputeKernel::dispatch, DEFVAL((TypedDictionary<uint32_t, uint32_t>())));
    
    ClassDB::bind_method(D_METHOD("assign_resource", "resource", "resource_name"), &ComputeKernel::assign_resource);

    ClassDB::bind_method(D_METHOD("get_buffer", "buffer_name"), &ComputeKernel::get_buffer);
    ClassDB::bind_method(D_METHOD("get_texture", "texture_name"), &ComputeKernel::get_texture);
    ClassDB::bind_method(D_METHOD("buffer_bind_callback", "buffer_name", "callback"), &ComputeKernel::buffer_bind_callback);
    ClassDB::bind_method(D_METHOD("buffer_set_unsized_element_count", "buffer_name", "element_count"), &ComputeKernel::buffer_set_unsized_element_count);
    ClassDB::bind_method(D_METHOD("texture_bind_callback", "texture_name", "callback"), &ComputeKernel::texture_bind_callback);
    ClassDB::bind_method(D_METHOD("texture_set_2d", "texture_name", "width", "height", "tex"), &ComputeKernel::texture_set_2d, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("texture_set_3d", "texture_name", "width", "height", "depth", "tex"), &ComputeKernel::texture_set_3d, DEFVAL(nullptr));
}

Ref<ComputeKernel> ComputeKernel::make_new(String source, KernelInfo info, RenderingDevice *rd) {
    Ref<ComputeKernel> new_kernel = memnew(ComputeKernel);
    new_kernel->kernel_rd = rd;
    new_kernel->source = source;
    
    new_kernel->kernel_info = info;
    new_kernel->uniform_set = FSLUniformSet(rd);
    new_kernel->_init_resources();
    return new_kernel;
}


/*******************************************************/
/******************* HELPER METHODS ********************/
/*******************************************************/


void ComputeKernel::_generate_pipeline() {
    if (shader_comp.is_valid() && pipeline.is_valid() && kernel_rd->compute_pipeline_is_valid(pipeline)) {
        return;
    }
    if (shader_comp.is_valid()) {
        kernel_rd->free_rid(shader_comp);
        if (pipeline.is_valid() && kernel_rd->compute_pipeline_is_valid(pipeline)) {
            kernel_rd->free_rid(pipeline);
        }
    }
    Ref<RDShaderSource> shader_source = memnew(RDShaderSource());
    shader_source->set_language(RenderingDevice::ShaderLanguage::SHADER_LANGUAGE_GLSL);
    shader_source->set_stage_source(RenderingDevice::ShaderStage::SHADER_STAGE_COMPUTE, source);
    shader_spirv = kernel_rd->shader_compile_spirv_from_source(shader_source);

    if (shader_spirv->get_stage_compile_error(RenderingDevice::ShaderStage::SHADER_STAGE_COMPUTE) != "") {
        print_error(vformat("%s\nIn: %s", shader_spirv->get_stage_compile_error(RenderingDevice::ShaderStage::SHADER_STAGE_COMPUTE), source));
    }
    shader_comp = kernel_rd->shader_create_from_spirv(shader_spirv);
    pipeline = kernel_rd->compute_pipeline_create(shader_comp);
}

void ComputeKernel::_init_resources() {
    for (const auto &[resource_name, resource_info] : kernel_info.bindings) {
        const StringName& res_name = resource_name;
        std::visit(overload{
            [&](const BufferInfo &buf_info) {
                Ref<FSLBuffer> new_buf = FSLBuffer::new_buffer(kernel_rd, buf_info);
                fsl_buffers[res_name] = new_buf;
                fsl_resources[res_name] = RESTYPE_BUFFER;
                assign_resource(new_buf, res_name);
                unbound_resources.insert(res_name);
            },
            [&](const TextureInfo &tex_info) {
                Ref<FSLTexture> new_tex = FSLTexture::new_texture(kernel_rd, tex_info);
                fsl_textures[res_name] = new_tex;
                fsl_resources[res_name] = RESTYPE_TEXTURE;
                assign_resource(new_tex, res_name);
                unbound_resources.insert(res_name);
            },
            [&](const VariableInfo &var_info)  {
                print_error("\t\tHow the fuck");
            }
        }, resource_info.type_info);
    }
}

std::tuple<uint32_t, uint32_t, uint32_t> ComputeKernel::get_workgroups(uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations) {
	return std::make_tuple((x_invocations + (kernel_info.local_invocations[0] - (x_invocations % kernel_info.local_invocations[0])))/kernel_info.local_invocations[0], 
        (y_invocations + (kernel_info.local_invocations[1] - (y_invocations % kernel_info.local_invocations[1])))/kernel_info.local_invocations[1], 
        (z_invocations + (kernel_info.local_invocations[2] - (z_invocations % kernel_info.local_invocations[2])))/kernel_info.local_invocations[2]);
}



RID ComputeKernel::get_pipeline_rid() {
    _generate_pipeline();
	return pipeline;
}

RID ComputeKernel::get_uniform_set_rid() {
	return uniform_set.get_rid(shader_comp, 0);
}

PackedByteArray ComputeKernel::push_constants_to_bytes(TypedDictionary<StringName, Variant> push_constants) {
    if (kernel_info.push_constants.size() > 0) {
        PackedByteArray shader_pc_bytes = {};
        for (const auto& [pc_name, pc_info] : kernel_info.push_constants) {
            uint32_t pc_size_bytes = get_fsl_type_size(pc_info.type);
            PackedByteArray pc_byte_array;
            if (push_constants.has(pc_name)) {
                pc_byte_array = fsl_type_to_bytes(pc_info.type, push_constants[pc_name]);
                // printvf("input value %s, output %02x%02x%02x%02x", push_constants[pc_name].stringify(), pc_byte_array[3], pc_byte_array[2], pc_byte_array[1], pc_byte_array[0]);
            } else {
                Variant defval = get_fsl_default_value(pc_info.type);
                WARN_PRINT(vformat("No push constant assigned for \"%s\", using default value of %s", pc_name, defval));
                pc_byte_array = fsl_type_to_bytes(pc_info.type, defval);
            }
            
            shader_pc_bytes.append_array(pc_byte_array);
        }
        return shader_pc_bytes;
    }
	return PackedByteArray();
}

void ComputeKernel::_assign_resource(Ref<FSLResource> resource, uint32_t set, uint32_t binding) {
    uniform_set.bind_resource(resource, binding);
}


bool ComputeKernel::try_assign_resource(Ref<FSLResource> resource, StringName resource_name) {
    if (kernel_info.bindings.has(resource_name)) {
        if (unbound_resources.has(resource_name)) {
            unbound_resources.erase(resource_name);
        }
        if (resource->resource_type == RESTYPE_BUFFER) {
            fsl_buffers[resource_name] = resource;
        } else {
            fsl_textures[resource_name] = resource;
        }
        fsl_resources[resource_name] = resource->resource_type;
        _assign_resource(resource, kernel_info.bindings[resource_name].set, kernel_info.bindings[resource_name].binding);
        return true;
    } 
    return false;
}





/**************************************************/
/******************* GODOT API ********************/
/**************************************************/

void print_resource_info(const StringName& res_name, const ResourceInfo &res_info) {
    printvf("\t%s: set = %d, binding = %d", res_name, res_info.set, res_info.binding);
    std::visit(overload{
        [&](const BufferInfo &buf_info)          {
            String buffer_type = buf_info.type == UNIFORM ? "Uniform" : "Storage";
            String buffer_format = buf_info.format == STD140 ? "std140" : "std430";
            printvf("\t\t%s buffer with format %s", buffer_type, buffer_format);
            print_line("\t\tBuffer fields:");
            for (const auto& [name, value] : buf_info.fields) {
                printvf("\t\t\t%s: %s", name, fslType_to_string(value.type));
            }
        },
        [&](const TextureInfo &tex_info) {
            String tex_format = tex_info.format == RGBA16F ? "rgba16f" : "rgba32f";
            print_line(vformat("\t\tTexture with format %s", tex_format));
        },
        [&](const VariableInfo &var_info)  {
            print_line("\t\tHow the fuck");
        }
    }, res_info.type_info);
}

void ComputeKernel::print_info() {
    printvf("Local Invocations: %dx, %dy, %dz", kernel_info.local_invocations[0], kernel_info.local_invocations[1], kernel_info.local_invocations[2]);
    printvf("Push Constants: ");
    for (const auto& [name, value] : kernel_info.push_constants) {
        printvf("\t%s: %s", name, fslType_to_string(value.type));
    }
    print_line("Resources:");
    for (const auto& [name, resource] : kernel_info.bindings) {
        print_resource_info(name, resource);
    }
    printvf("Code:\n%s", source);
}

/********* RESOURCE API *********/

void ComputeKernel::assign_resource(Ref<FSLResource> resource, StringName resource_name) {
    if (kernel_info.bindings.has(resource_name)) {
        if (unbound_resources.has(resource_name)) {
            unbound_resources.erase(resource_name);
        }
        if (resource->resource_type == RESTYPE_BUFFER) {
            fsl_buffers[resource_name] = resource;
        } else {
            fsl_textures[resource_name] = resource;
        }
        fsl_resources[resource_name] = resource->resource_type;
        _assign_resource(resource, kernel_info.bindings[resource_name].set, kernel_info.bindings[resource_name].binding);
        return;
    } 
    WARN_PRINT(vformat("Compute shader \"%s\" has no resource named \"%s\"", kernel_info.kernel_name, resource_name));
}




Ref<FSLBuffer> ComputeKernel::get_buffer(StringName buffer_name) {
    if (!fsl_buffers.has(buffer_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no buffer named \"%s\"", buffer_name));
    }
	return fsl_buffers[buffer_name];
}

Ref<FSLTexture> ComputeKernel::get_texture(StringName texture_name) {
    if (!fsl_textures.has(texture_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no texture named \"%s\"", texture_name));
    }
	return fsl_textures[texture_name];
}

void ComputeKernel::buffer_set_unsized_element_count(StringName buffer_name, uint32_t element_count) {
    if (!fsl_buffers.has(buffer_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no buffer named \"%s\"", buffer_name));
        return;
    }
    fsl_buffers[buffer_name]->set_unsized_element_count(element_count);
}

void ComputeKernel::buffer_bind_callback(StringName buffer_name, Callable callback) {
    if (!fsl_buffers.has(buffer_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no buffer named \"%s\"", buffer_name));
        return;
    }
    fsl_buffers[buffer_name]->bind_callback(callback);
}

void ComputeKernel::texture_set_2d(StringName texture_name, uint32_t width, uint32_t height, Ref<Image> tex) {
    if (!fsl_textures.has(texture_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no texture named \"%s\"", texture_name));
        return;
    }
    fsl_textures[texture_name]->set_2d_texture(width, height, tex);
}

void ComputeKernel::texture_set_3d(StringName texture_name, uint32_t width, uint32_t height, uint32_t depth, Ref<Image> tex) {
    if (!fsl_textures.has(texture_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no texture named \"%s\"", texture_name));
        return;
    }
    fsl_textures[texture_name]->set_3d_texture(width, height, depth, tex);
}

void ComputeKernel::texture_bind_callback(StringName texture_name, Callable callback) {
    if (!fsl_textures.has(texture_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no texture named \"%s\"", texture_name));
        return;
    }
    fsl_textures[texture_name]->bind_callback(callback);
}

/********* KERNEL API *********/

void ComputeKernel::dispatch(uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants) {
    // for (const auto& resource_name : unbound_resources) {
    //     WARN_PRINT_ONCE(vformat("No binding provided for resource \"%s\" in shader \"%s\"", resource_name, kernel_info.kernel_name));
    // }
    _generate_pipeline();
    RID uniform_set_rid = uniform_set.get_rid(shader_comp);

    int64_t computeList = kernel_rd->compute_list_begin();
    kernel_rd->compute_list_bind_compute_pipeline(computeList, pipeline);
    
    if (kernel_info.push_constants.size() > 0) {
        PackedByteArray shader_pc_bytes = push_constants_to_bytes(push_constants);
        kernel_rd->compute_list_set_push_constant(computeList, shader_pc_bytes, shader_pc_bytes.size());
    }

    auto workgroups = get_workgroups(x_invocations, y_invocations, z_invocations);
    kernel_rd->compute_list_bind_uniform_set(computeList, uniform_set_rid, 0);
    kernel_rd->compute_list_dispatch(computeList, std::get<0>(workgroups), std::get<1>(workgroups), std::get<2>(workgroups));
    kernel_rd->compute_list_end();
}

void ComputeKernel::dispatch_workgroups(uint32_t x_workgroups, uint32_t y_workgroups, uint32_t z_workgroups, TypedDictionary<StringName, Variant> push_constants) {
    _generate_pipeline();
    RID uniform_set_rid = uniform_set.get_rid(shader_comp);

    int64_t computeList = kernel_rd->compute_list_begin();
    kernel_rd->compute_list_bind_compute_pipeline(computeList, pipeline);
    
    if (kernel_info.push_constants.size() > 0) {
        PackedByteArray shader_pc_bytes = push_constants_to_bytes(push_constants);
        kernel_rd->compute_list_set_push_constant(computeList, shader_pc_bytes, shader_pc_bytes.size());
    }

    kernel_rd->compute_list_bind_uniform_set(computeList, uniform_set_rid, 0);
    kernel_rd->compute_list_dispatch(computeList, x_workgroups, y_workgroups, z_workgroups);
    kernel_rd->compute_list_end();
}