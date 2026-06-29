#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/variant/typed_dictionary.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/a_hash_map.hpp"
#include "godot_cpp/classes/rd_shader_source.hpp"
#include "godot_cpp/classes/rd_shader_spirv.hpp"

#include <optional>
#include <variant>

#include "fsl/fsl_defs.h"
#include "shader_resource.h"

using namespace godot;



class ComputeKernel : public RefCounted {
    GDCLASS(ComputeKernel, RefCounted)
public:
    struct VariableInfo {
        FSLType type;
    };
    struct TextureInfo {
        TextureFormat format;
    };
    struct BufferInfo {
        BufferType type;
        BufferFormat format;
        HashMap<StringName, VariableInfo> fields;
    };
    struct ResourceInfo {
        uint32_t set;
        uint32_t binding;
        std::variant<BufferInfo, TextureInfo, VariableInfo> type_info;
    };
    struct KernelInfo {
        StringName kernel_name;
        HashMap<StringName, ResourceInfo> bindings;
        HashMap<StringName, VariableInfo> push_constants;
        uint32_t local_invocations[3];
    };

private:
	ComputeKernel() = default;
protected:
	static void _bind_methods();
    String source;
    KernelInfo kernel_info = {};
    void _init_resources();
    void _generate_pipeline();
    UniformSet uniform_set;
public:
    RenderingDevice* kernel_rd;

    RID shader_comp = RID();
    RID pipeline = RID();
    
    Ref<RDShaderSPIRV> shader_spirv;
    ShaderResourceStorage *resource_storage;
    HashSet<StringName> unbound_resources;

    ComputeKernel(String source, KernelInfo info, RenderingDevice *rd);
    void _assign_resource(GID uniform_id, uint32_t set, uint32_t binding);
    
    void print_info();

    void assign_resource(GID uniform_id, StringName resource_name);
    void dispatch(uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants = {});
    
    GID create_buffer(BufferType buffer_type, uint32_t size_bytes, PackedByteArray data = {});
    GID create_texture(uint32_t size_x, uint32_t size_y, TextureFormat texture_type = RGBA32F);

    void bind_texture_callback(GID texture_id, Callable callback);
    void set_texture(GID texture_id, uint32_t size_x, uint32_t size_y, Ref<Image> tex);

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
    ComputePlan() = default;
protected:
    enum ComputePlanItem {
        Shader,
        Barrier,
        Plan
    };
    struct ShaderDispatch {
        Ref<ComputeKernel> shader;
        uint32_t workgroups[3] = {1, 1, 1};
        TypedDictionary<StringName, Variant> push_constants = {};
    };
    static void _bind_methods();
    void bind(RenderingDevice *rd, int64_t compute_list);
    LocalVector<ShaderDispatch> dispatches;
    LocalVector<ShaderDispatch> temp_dispatches;
    LocalVector<Ref<ComputePlan>> compute_plans;
    LocalVector<Ref<ComputePlan>> temp_plans;
    LocalVector<Pair<ComputePlanItem, uint32_t>> compute_items;
    LocalVector<Pair<ComputePlanItem, uint32_t>> temp_items;
public:
    RenderingDevice* plan_rd;
    static Ref<ComputePlan> make_new(RenderingDevice* rd);
    ~ComputePlan() override = default;
    void add_plan(Ref<ComputePlan> subplan, bool is_temp = false);
    void dispatch();
    Ref<ComputePlan> add_barrier(bool is_temp = false);
    Ref<ComputePlan> add_kernel(Ref<ComputeKernel> kernel, uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants = {}, bool is_temp = false);
};

