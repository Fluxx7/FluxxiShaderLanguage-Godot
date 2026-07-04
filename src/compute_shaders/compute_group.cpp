#include "compute_group.h"

void ComputeGroup::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_kernel", "kernel_name"), &ComputeGroup::get_kernel);

    ClassDB::bind_method(D_METHOD("print_info"), &ComputeGroup::print_info);

    ClassDB::bind_method(D_METHOD("assign_resource", "resource", "resource_name"), &ComputeGroup::assign_resource);

    ClassDB::bind_method(D_METHOD("dispatch", "kernel_name", "x_invocations", "y_invocations", "z_invocations", "push_constants"), &ComputeGroup::dispatch, DEFVAL((TypedDictionary<uint32_t, uint32_t>())));

    ClassDB::bind_method(D_METHOD("get_buffer", "buffer_name"), &ComputeGroup::get_buffer);
    ClassDB::bind_method(D_METHOD("get_texture", "texture_name"), &ComputeGroup::get_texture);
    ClassDB::bind_method(D_METHOD("buffer_bind_callback", "buffer_name", "callback"), &ComputeGroup::buffer_bind_callback);
    ClassDB::bind_method(D_METHOD("buffer_set_unsized_element_count", "buffer_name", "element_count"), &ComputeGroup::buffer_set_unsized_element_count);
    ClassDB::bind_method(D_METHOD("texture_bind_callback", "texture_name", "callback"), &ComputeGroup::texture_bind_callback);
    ClassDB::bind_method(D_METHOD("texture_set_2d", "texture_name", "width", "height", "tex"), &ComputeGroup::texture_set_2d, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("texture_set_3d", "texture_name", "width", "height", "depth", "tex"), &ComputeGroup::texture_set_3d, DEFVAL(nullptr));
}

Ref<ComputeGroup> ComputeGroup::make_new(RenderingDevice *rd) {
    Ref<ComputeGroup> new_group = memnew(ComputeGroup);
    new_group->group_rd = rd;
	return new_group;
}

void ComputeGroup::print_info() {
}

void ComputeGroup::add_kernel(ComputeKernel::KernelInfo kernel_info, String kernel_source) {
    auto new_kernel = ComputeKernel::make_new(kernel_source, kernel_info, group_rd);
    for (const auto &[resource_name, resource_info] : kernel_info.bindings) {
        const StringName& res_name = resource_name;
        if (fsl_resources.has(res_name)) {
            switch (fsl_resources[res_name]) {
                case RESTYPE_BUFFER: new_kernel->assign_resource(fsl_buffers[res_name], res_name); break;
                case RESTYPE_TEXTURE: new_kernel->assign_resource(fsl_textures[res_name], res_name); break;
            }
            continue;
        }
        std::visit(overload{
            [&](const BufferInfo &buf_info) {
                Ref<FSLBuffer> new_buf = FSLBuffer::new_buffer(group_rd, buf_info);
                fsl_buffers[res_name] = new_buf;
                fsl_resources[res_name] = RESTYPE_BUFFER;
                new_kernel->assign_resource(new_buf, res_name);
            },
            [&](const TextureInfo &tex_info) {
                Ref<FSLTexture> new_tex = FSLTexture::new_texture(group_rd, tex_info);
                fsl_textures[res_name] = new_tex;
                fsl_resources[res_name] = RESTYPE_TEXTURE;
                new_kernel->assign_resource(new_tex, res_name);
            },
            [&](const VariableInfo &var_info)  {
                print_error("\t\tHow the fuck");
            }
        }, resource_info.type_info);
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




Ref<FSLBuffer> ComputeGroup::get_buffer(StringName buffer_name) {
    if (!fsl_buffers.has(buffer_name)) {
        ERR_PRINT_ONCE(vformat("ComputeGroup has no buffer named \"%s\"", buffer_name));
    }
	return fsl_buffers[buffer_name];
}

Ref<FSLTexture> ComputeGroup::get_texture(StringName texture_name) {
    if (!fsl_textures.has(texture_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no texture named \"%s\"", texture_name));
    }
	return fsl_textures[texture_name];
}

void ComputeGroup::buffer_set_unsized_element_count(StringName buffer_name, uint32_t element_count) {
    if (!fsl_buffers.has(buffer_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no buffer named \"%s\"", buffer_name));
        return;
    }
    fsl_buffers[buffer_name]->set_unsized_element_count(element_count);
}

void ComputeGroup::buffer_bind_callback(StringName buffer_name, Callable callback) {
    if (!fsl_buffers.has(buffer_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no buffer named \"%s\"", buffer_name));
        return;
    }
    fsl_buffers[buffer_name]->bind_callback(callback);
}

void ComputeGroup::texture_set_2d(StringName texture_name, uint32_t width, uint32_t height, Ref<Image> tex) {
    if (!fsl_textures.has(texture_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no texture named \"%s\"", texture_name));
        return;
    }
    fsl_textures[texture_name]->set_2d_texture(width, height, tex);
}

void ComputeGroup::texture_set_3d(StringName texture_name, uint32_t width, uint32_t height, uint32_t depth, Ref<Image> tex) {
    if (!fsl_textures.has(texture_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no texture named \"%s\"", texture_name));
        return;
    }
    fsl_textures[texture_name]->set_3d_texture(width, height, depth, tex);
}

void ComputeGroup::texture_bind_callback(StringName texture_name, Callable callback) {
    if (!fsl_textures.has(texture_name)) {
        ERR_PRINT_ONCE(vformat("Kernel has no texture named \"%s\"", texture_name));
        return;
    }
    fsl_textures[texture_name]->bind_callback(callback);
}