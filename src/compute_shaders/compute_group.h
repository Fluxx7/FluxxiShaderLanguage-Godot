#pragma once


#include "std_imports.h"
#include "godot_imports.h"

#include "godot_cpp/variant/typed_dictionary.hpp"
#include "godot_cpp/classes/rd_shader_source.hpp"
#include "godot_cpp/classes/rd_shader_spirv.hpp"

#include "fsl/fsl_defs.h"
#include "compute_kernel.h"


class ComputeGroup : public RefCounted {
    GDCLASS(ComputeGroup, RefCounted)
private:
protected:
    HashMap<StringName, Ref<ComputeKernel>> kernels;
    
    HashMap<StringName, Ref<FSLResource>> fsl_resources;
    RenderingDevice* group_rd;
    static void _bind_methods();

    template <typename T>
    Ref<T> _get_resource_typed(const StringName &res_name) const {
        const Ref<FSLResource> *res = fsl_resources.getptr(res_name);
        ERR_FAIL_NULL_V_MSG(res, Ref<T>(), vformat("ComputeGroup has no resource named \'%s\'", res_name));
        Ref<T> typed_res = *res;
        ERR_FAIL_COND_V_MSG(typed_res.is_null(), Ref<T>(), vformat("Resource \'%s\' is a %s, not a %s", (*res)->get_class(), T::get_class_static()));
        return typed_res;
    }
public:
    static Ref<ComputeGroup> make_new(RenderingDevice* rd);

    void print_info();

    void add_kernel(ComputeKernel::KernelInfo kernel_info, String kernel_source);
    Ref<ComputeKernel> get_kernel(StringName kernel_name);

    Ref<FSLStorageBuffer> get_storage_buffer(const StringName &buffer_name);
    Ref<FSLUniformBuffer> get_uniform_buffer(const StringName &buffer_name);
    Ref<FSLVertexBuffer> get_vertex_buffer(const StringName &buffer_name);
    Ref<FSLIndexBuffer> get_index_buffer(const StringName &buffer_name);

    Ref<FSLTexture2D> get_texture_2d(const StringName &texture_name);
    Ref<FSLTexture2DArray> get_texture_2d_array(const StringName &texture_name);

    Ref<FSLResource> get_resource(const StringName &res_name);

    void assign_resource(Ref<FSLResource> resource, StringName resource_name);
    void dispatch(StringName kernel_name, uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants = {});
};