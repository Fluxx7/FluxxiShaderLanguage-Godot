#include "compute_group.h"
#include "init_helpers.h"

void ComputeGroup::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_kernel", "kernel_name"), &ComputeGroup::get_kernel);

    ClassDB::bind_method(D_METHOD("print_info"), &ComputeGroup::print_info);

    ClassDB::bind_method(D_METHOD("assign_resource", "resource", "resource_name"), &ComputeGroup::assign_resource);

    ClassDB::bind_method(D_METHOD("dispatch", "kernel_name", "x_invocations", "y_invocations", "z_invocations", "push_constants"), &ComputeGroup::dispatch, DEFVAL((TypedDictionary<uint32_t, uint32_t>())));

    ClassDB::bind_method(D_METHOD("get_storage_buffer", "buffer_name"), &ComputeGroup::get_storage_buffer);
    ClassDB::bind_method(D_METHOD("get_uniform_buffer", "buffer_name"), &ComputeGroup::get_uniform_buffer);
    ClassDB::bind_method(D_METHOD("get_vertex_buffer", "buffer_name"), &ComputeGroup::get_vertex_buffer);
    ClassDB::bind_method(D_METHOD("get_index_buffer", "buffer_name"), &ComputeGroup::get_index_buffer);

    ClassDB::bind_method(D_METHOD("get_texture_2d", "texture_name"), &ComputeGroup::get_texture_2d);
    ClassDB::bind_method(D_METHOD("get_texture_2d_array", "texture_name"), &ComputeGroup::get_texture_2d_array);

    ClassDB::bind_method(D_METHOD("get_resource", "resource_name"), &ComputeGroup::get_resource);
}

Ref<ComputeGroup> ComputeGroup::make_new(RenderingDevice *rd) {
    Ref<ComputeGroup> new_group = memnew(ComputeGroup);
    new_group->group_rd = rd;
	return new_group;
}

void ComputeGroup::print_info() {
    for (const auto &[kernel_name, kernel] : kernels) {
        printvf("Kernel %s:", kernel_name);
        kernel->print_info();
    }
}

void ComputeGroup::add_kernel(ComputeKernel::KernelInfo kernel_info, String kernel_source) {
    auto new_kernel = ComputeKernel::make_new(kernel_source, kernel_info, group_rd);
    for (const auto &[resource_name, resource_info] : kernel_info.bindings) {
        if (!fsl_resources.has(resource_name)) {
            fsl_resources[resource_name] = init_resource(resource_info, group_rd);
        }
        new_kernel->assign_resource(fsl_resources[resource_name], resource_name);
    }
    kernels[kernel_info.kernel_name] = new_kernel;
}

Ref<ComputeKernel> ComputeGroup::get_kernel(StringName kernel_name) {
	if (kernels.has(kernel_name)) {
        return kernels[kernel_name];
    }
    WARN_PRINT(vformat("No kernel named \"%s\" in ComputeGroup", kernel_name));
    return Ref<ComputeKernel>();
}

void ComputeGroup::assign_resource(Ref<FSLResource> resource, StringName resource_name) {
	for (const auto& [kernel_name, kernel] : kernels) {
        kernel->try_assign_resource(resource, resource_name);
    }
}

void ComputeGroup::dispatch(StringName kernel_name, uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants) {
    if (kernels.has(kernel_name)) {
        kernels[kernel_name]->dispatch(x_invocations, y_invocations, z_invocations, push_constants);
        return;
    }
    WARN_PRINT(vformat("No kernel named \"%s\" in ComputeGroup", kernel_name));
    return;
}



Ref<FSLStorageBuffer> ComputeGroup::get_storage_buffer(const StringName &buffer_name) {
	return _get_resource_typed<FSLStorageBuffer>(buffer_name);
}

Ref<FSLUniformBuffer> ComputeGroup::get_uniform_buffer(const StringName &buffer_name) {
	return _get_resource_typed<FSLUniformBuffer>(buffer_name);
}

Ref<FSLVertexBuffer> ComputeGroup::get_vertex_buffer(const StringName &buffer_name) {
	return _get_resource_typed<FSLVertexBuffer>(buffer_name);
}

Ref<FSLIndexBuffer> ComputeGroup::get_index_buffer(const StringName &buffer_name) {
	return _get_resource_typed<FSLIndexBuffer>(buffer_name);
}

// -- TEXTURES --

Ref<FSLTexture2D> ComputeGroup::get_texture_2d(const StringName &texture_name) {
	return _get_resource_typed<FSLTexture2D>(texture_name);
}

Ref<FSLTexture2DArray> ComputeGroup::get_texture_2d_array(const StringName &texture_name) {
	return _get_resource_typed<FSLTexture2DArray>(texture_name);
}

// -- GENERIC --

Ref<FSLResource> ComputeGroup::get_resource(const StringName &res_name) {
	const Ref<FSLResource> *res = fsl_resources.getptr(res_name);
    ERR_FAIL_NULL_V_MSG(res, Ref<FSLResource>(), vformat("Kernel has no resource named \'%s\'", res_name));
    return *res;
}