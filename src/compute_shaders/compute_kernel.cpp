#include "compute_kernel.h"
#include "init_helpers.h"
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
    ClassDB::bind_method(D_METHOD("dispatch_workgroups", "x_workgroups", "y_workgroups", "z_workgroups", "push_constants"), &ComputeKernel::dispatch_workgroups, DEFVAL((TypedDictionary<uint32_t, uint32_t>())));
    
    ClassDB::bind_method(D_METHOD("assign_resource", "resource", "resource_name"), &ComputeKernel::assign_resource);

    ClassDB::bind_method(D_METHOD("get_storage_buffer", "buffer_name"), &ComputeKernel::get_storage_buffer);
    ClassDB::bind_method(D_METHOD("get_uniform_buffer", "buffer_name"), &ComputeKernel::get_uniform_buffer);
    ClassDB::bind_method(D_METHOD("get_vertex_buffer", "buffer_name"), &ComputeKernel::get_vertex_buffer);
    ClassDB::bind_method(D_METHOD("get_index_buffer", "buffer_name"), &ComputeKernel::get_index_buffer);

    ClassDB::bind_method(D_METHOD("get_texture_2d", "texture_name"), &ComputeKernel::get_texture_2d);
    ClassDB::bind_method(D_METHOD("get_texture_2d_array", "texture_name"), &ComputeKernel::get_texture_2d_array);

    ClassDB::bind_method(D_METHOD("get_resource", "resource_name"), &ComputeKernel::get_resource);


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
        assign_resource(init_resource(resource_info, kernel_rd), resource_name);
    }
}

uint32_t get_min_workgroup_count(uint32_t local_invocations, uint32_t requested_invocations) {
    uint32_t def_workgroups = requested_invocations / local_invocations;
    uint32_t remaining_invocations = requested_invocations % local_invocations;
    return remaining_invocations > 0 ? def_workgroups + 1 : def_workgroups;
}

std::tuple<uint32_t, uint32_t, uint32_t> ComputeKernel::get_workgroups(uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations) {
	return std::make_tuple(
        get_min_workgroup_count(kernel_info.local_invocations[0], x_invocations), 
        get_min_workgroup_count(kernel_info.local_invocations[1], y_invocations), 
        get_min_workgroup_count(kernel_info.local_invocations[2], z_invocations));
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
        fsl_resources[resource_name] = resource;
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
            printvf("\t\t%s", get_buffer_desc(buf_info.type, buf_info.format));
            print_line("\t\tBuffer fields:");
            for (const auto& [name, value] : buf_info.fields) {
                printvf("\t\t\t%s: %s", name, fslType_to_string(value.type));
            }
        },
        [&](const TextureInfo &tex_info) {
            printvf("\t\t%s with format %s", textureType_to_string(tex_info.type), textureFormat_to_string(tex_info.format));
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
        fsl_resources[resource_name] = resource;
        _assign_resource(resource, kernel_info.bindings[resource_name].set, kernel_info.bindings[resource_name].binding);
        return;
    } 
    WARN_PRINT(vformat("Compute shader \"%s\" has no resource named \"%s\"", kernel_info.kernel_name, resource_name));
}

// -- BUFFERS --

Ref<FSLStorageBuffer> ComputeKernel::get_storage_buffer(const StringName &buffer_name) {
	return _get_resource_typed<FSLStorageBuffer>(buffer_name);
}

Ref<FSLUniformBuffer> ComputeKernel::get_uniform_buffer(const StringName &buffer_name) {
	return _get_resource_typed<FSLUniformBuffer>(buffer_name);
}

Ref<FSLVertexBuffer> ComputeKernel::get_vertex_buffer(const StringName &buffer_name) {
	return _get_resource_typed<FSLVertexBuffer>(buffer_name);
}

Ref<FSLIndexBuffer> ComputeKernel::get_index_buffer(const StringName &buffer_name) {
	return _get_resource_typed<FSLIndexBuffer>(buffer_name);
}

// -- TEXTURES --

Ref<FSLTexture2D> ComputeKernel::get_texture_2d(const StringName &texture_name) {
	return _get_resource_typed<FSLTexture2D>(texture_name);
}

Ref<FSLTexture2DArray> ComputeKernel::get_texture_2d_array(const StringName &texture_name) {
	return _get_resource_typed<FSLTexture2DArray>(texture_name);
}

// -- GENERIC --

Ref<FSLResource> ComputeKernel::get_resource(const StringName &res_name) {
	const Ref<FSLResource> *res = fsl_resources.getptr(res_name);
    ERR_FAIL_NULL_V_MSG(res, Ref<FSLResource>(), vformat("Kernel has no resource named \'%s\'", res_name));
    return *res;
}

/********* KERNEL API *********/

void ComputeKernel::dispatch(uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants) {
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