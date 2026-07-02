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
    static void _bind_methods();
public:
    void _assign_resource(Ref<FSLResource> resource, uint32_t set, uint32_t binding);
    
    void print_info();

    void assign_resource(Ref<FSLResource> resource, StringName resource_name);
    void dispatch(StringName kernel_name, uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants = {});
};