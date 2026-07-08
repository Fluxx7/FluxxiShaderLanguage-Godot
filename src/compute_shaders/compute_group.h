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
#include "compute_kernel.h"


class ComputeGroup : public RefCounted {
    GDCLASS(ComputeGroup, RefCounted)
private:
protected:
    HashMap<StringName, Ref<ComputeKernel>> kernels;
    
    HashMap<StringName, ResourceType> fsl_resources;
    HashMap<StringName, Ref<FSLBuffer>> fsl_buffers;
    HashMap<StringName, Ref<FSLTexture>> fsl_textures;
    RenderingDevice* group_rd;
    static void _bind_methods();
public:
    static Ref<ComputeGroup> make_new(RenderingDevice* rd);

    void print_info();

    void add_kernel(ComputeKernel::KernelInfo kernel_info, String kernel_source);
    Ref<ComputeKernel> get_kernel(StringName kernel_name);

    Ref<FSLBuffer> get_buffer(StringName buffer_name);
    Ref<FSLTexture> get_texture(StringName texture_name);

    void buffer_set_unsized_element_count(StringName buffer_name, uint32_t element_count);
    void buffer_bind_callback(StringName buffer_name, Callable callback);

    void texture_set_2d(StringName texture_name, uint32_t width, uint32_t height, Ref<Image> tex = nullptr);
    void texture_set_3d(StringName texture_name, uint32_t width, uint32_t height, uint32_t depth, TypedArray<Ref<Image>> images = {});
    void texture_bind_callback(StringName texture_name, Callable callback);

    void assign_resource(Ref<FSLResource> resource, StringName resource_name);
    void dispatch(StringName kernel_name, uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants = {});
};