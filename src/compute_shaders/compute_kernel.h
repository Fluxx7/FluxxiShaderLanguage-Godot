#pragma once

#include "std_imports.h"
#include "godot_imports.h"

#include "godot_cpp/variant/typed_dictionary.hpp"
#include "godot_cpp/classes/rd_shader_source.hpp"
#include "godot_cpp/classes/rd_shader_spirv.hpp"

#include "fsl/fsl_defs.h"
#include "resources/fsl_buffer.h"
#include "resources/fsl_texture.h"

using namespace godot;



class ComputeKernel : public RefCounted {
    GDCLASS(ComputeKernel, RefCounted)
public:
    struct SpecializationConstant {
        uint32_t constant_id;
        Variant value;
    };
    struct KernelInfo {
        StringName kernel_name;
        HashMap<StringName, ResourceInfo> bindings;
        HashMap<StringName, VariableInfo> push_constants;
        HashMap<StringName, SpecializationConstant> specialization_constants;
        sumtype<uint32_t, StringName> local_invocations[3];
    };

private:
protected:
	static void _bind_methods();

    String source;
    KernelInfo kernel_info = {};

    void _init_resources();
    void _generate_pipeline();

    AHashMap<StringName, Ref<FSLResource>> fsl_resources;
    FSLUniformSet uniform_set;


    template <typename T>
    Ref<T> _get_resource_typed(const StringName &res_name) const {
        const Ref<FSLResource> *res = fsl_resources.getptr(res_name);
        ERR_FAIL_NULL_V_MSG(res, Ref<T>(), vformat("Kernel has no resource named \'%s\'", res_name));
        Ref<T> typed_res = *res;
        ERR_FAIL_COND_V_MSG(typed_res.is_null(), Ref<T>(), vformat("Resource \'%s\' is a %s, not a %s", res_name, (*res)->get_class(), T::get_class_static()));
        return typed_res;
    }
    uint32_t get_min_workgroup_count(sumtype<uint32_t, StringName> local_invocation_var, uint32_t requested_invocations);
public:
    ComputeKernel();
    RenderingDevice* kernel_rd;

    RID shader_comp = RID();
    RID pipeline = RID();
    
    Ref<RDShaderSPIRV> shader_spirv;

    static Ref<ComputeKernel> make_new(String source, KernelInfo info, RenderingDevice* rd = nullptr);

    void _assign_resource(Ref<FSLResource> resource, uint32_t set, uint32_t binding);
    
    void print_info();

    void assign_resource(Ref<FSLResource> resource, StringName resource_name);
    bool try_assign_resource(Ref<FSLResource> resource, StringName resource_name);

    void dispatch(uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants = {});
    void dispatch_workgroups(uint32_t x_workgroups, uint32_t y_workgroups, uint32_t z_workgroups, TypedDictionary<StringName, Variant> push_constants = {});
    void dispatch_indirect(Ref<FSLBuffer> command_buffer, uint32_t offset, TypedDictionary<StringName, Variant> push_constants);
    
    std::tuple<uint32_t, uint32_t, uint32_t> get_workgroups(uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations);
    
    Variant get_specialization_constant_value(StringName spec_constant);
    void set_specialization_constant(StringName spec_constant, Variant value);
    bool try_set_specialization_constant(StringName spec_constant, Variant value);

    Ref<FSLStorageBuffer> get_storage_buffer(const StringName &buffer_name);
    Ref<FSLUniformBuffer> get_uniform_buffer(const StringName &buffer_name);
    Ref<FSLVertexBuffer> get_vertex_buffer(const StringName &buffer_name);
    Ref<FSLIndexBuffer> get_index_buffer(const StringName &buffer_name);

    Ref<FSLTexture2D> get_texture_2d(const StringName &texture_name);
    Ref<FSLTexture2DArray> get_texture_2d_array(const StringName &texture_name);

    Ref<FSLResource> get_resource(const StringName &res_name);

    

    RID get_pipeline_rid();
    RID get_uniform_set_rid();
    PackedByteArray push_constants_to_bytes(TypedDictionary<StringName, Variant> push_constants);

    
    

	~ComputeKernel() override {
        if (shader_comp.is_valid()) {
            kernel_rd->free_rid(shader_comp);
            if (kernel_rd->compute_pipeline_is_valid(pipeline)) {
                kernel_rd->free_rid(pipeline);
            }
        }
    };
};

class ComputePlan : public RefCounted {
    GDCLASS(ComputePlan, RefCounted)
private:
protected:
    enum ComputePlanItem {
        Shader,
        Barrier,
        Plan
    };
    struct ShaderDispatch {
        struct DirectDispatch {
            uint32_t x, y, z;
        };
        struct IndirectDispatch {
            Ref<FSLBuffer> command_buffer;
            uint32_t offset;
        };
        Ref<ComputeKernel> shader;
        sumtype<DirectDispatch, IndirectDispatch> dispatch_target = DirectDispatch{1, 1, 1};
        TypedDictionary<StringName, Variant> push_constants = {};
    };
    static void _bind_methods();
    void bind(int64_t compute_list);
    LocalVector<ShaderDispatch> dispatches;
    LocalVector<ShaderDispatch> temp_dispatches;
    LocalVector<Ref<ComputePlan>> compute_plans;
    LocalVector<Ref<ComputePlan>> temp_plans;
    LocalVector<Pair<ComputePlanItem, uint32_t>> compute_items;
    LocalVector<Pair<ComputePlanItem, uint32_t>> temp_items;
    void _dispatch_shader(ShaderDispatch &shader_dispatch, int64_t compute_list);
public:
    RenderingDevice* plan_rd;
    static Ref<ComputePlan> make_new(RenderingDevice* rd);
    ComputePlan();
    ~ComputePlan() override = default;
    void add_plan(Ref<ComputePlan> subplan, bool is_temp = false);
    void dispatch();
    Ref<ComputePlan> add_barrier(bool is_temp = false);
    Ref<ComputePlan> add_kernel(Ref<ComputeKernel> kernel, uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants = {}, bool is_temp = false);
    Ref<ComputePlan> add_kernel_workgroups(Ref<ComputeKernel> kernel, uint32_t x_workgroups, uint32_t y_workgroups, uint32_t z_workgroups, TypedDictionary<StringName, Variant> push_constants = {}, bool is_temp = false);
    Ref<ComputePlan> add_kernel_indirect(Ref<ComputeKernel> kernel, Ref<FSLBuffer> command_buffer, uint32_t offset, TypedDictionary<StringName, Variant> push_constants = {}, bool is_temp = false);
};
