#include "compute_kernel.h"

/******** STATIC METHODS *********/
void ComputePlan::_bind_methods() {
    ClassDB::bind_static_method("ComputePlan", D_METHOD("make_new", "rendering_device"), &ComputePlan::make_new);
    ClassDB::bind_method(D_METHOD("add_barrier", "is_temp"), &ComputePlan::add_barrier, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("add_kernel", "kernel", "x_invocations", "y_invocations", "z_invocations", "push_constants", "is_temp"), &ComputePlan::add_kernel, DEFVAL((TypedDictionary<uint32_t, uint32_t>())), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("add_plan", "subplan", "is_temp"), &ComputePlan::add_plan, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("dispatch"), &ComputePlan::dispatch);
}

void ComputePlan::bind(RenderingDevice *rd, int64_t compute_list) {
    for (const auto& [item_type, group_index] : compute_items) {
        if (item_type == Barrier) {
            rd->compute_list_add_barrier(compute_list);
            continue;
        }
        if (item_type == Plan) {
            compute_plans[group_index]->bind(rd, compute_list);
            continue;
        }
        ShaderDispatch shader_dispatch = dispatches[group_index];
        rd->compute_list_bind_compute_pipeline(compute_list, shader_dispatch.shader->get_pipeline_rid());
        if (shader_dispatch.push_constants.size() > 0) {
            PackedByteArray pc_bytes = shader_dispatch.shader->push_constants_to_bytes(shader_dispatch.push_constants);
            uint32_t pc_byte_size = pc_bytes.size();
            rd->compute_list_set_push_constant(compute_list, pc_bytes, pc_byte_size);
        }
        rd->compute_list_bind_uniform_set(compute_list, shader_dispatch.shader->get_uniform_set_rid(), 0);
        rd->compute_list_dispatch(compute_list, shader_dispatch.workgroups[0], shader_dispatch.workgroups[1], shader_dispatch.workgroups[2]);
    }
    for (const auto& [item_type, group_index] : temp_items) {
        if (item_type == Barrier) {
            rd->compute_list_add_barrier(compute_list);
            continue;
        }
        if (item_type == Plan) {
            temp_plans[group_index]->bind(rd, compute_list);
            continue;
        }
        ShaderDispatch shader_dispatch = temp_dispatches[group_index];
        rd->compute_list_bind_compute_pipeline(compute_list, shader_dispatch.shader->get_pipeline_rid());
        if (!shader_dispatch.push_constants.is_empty()) {
            PackedByteArray pc_bytes = shader_dispatch.shader->push_constants_to_bytes(shader_dispatch.push_constants);
            uint32_t pc_byte_size = pc_bytes.size();
            rd->compute_list_set_push_constant(compute_list, pc_bytes, pc_byte_size);
        }
        rd->compute_list_bind_uniform_set(compute_list, shader_dispatch.shader->get_uniform_set_rid(), 0);
        rd->compute_list_dispatch(compute_list, shader_dispatch.workgroups[0], shader_dispatch.workgroups[1], shader_dispatch.workgroups[2]);
    }
    temp_dispatches.clear();
    temp_plans.clear();
    temp_items.clear();
}

Ref<ComputePlan> ComputePlan::make_new(RenderingDevice *rd) {
    auto new_plan = memnew(ComputePlan());
    new_plan->plan_rd = rd;
	return new_plan;
}

void ComputePlan::add_plan(Ref<ComputePlan> subplan, bool is_temp) {
    if (is_temp) {
        temp_items.push_back(Pair(Plan, temp_plans.size()));
        temp_plans.push_back(subplan);
    } else {
        compute_items.push_back(Pair(Plan, compute_plans.size()));
        compute_plans.push_back(subplan);
    }
}

void ComputePlan::dispatch() {
    int64_t compute_list = plan_rd->compute_list_begin();
    bind(plan_rd, compute_list);
    plan_rd->compute_list_end();
}

Ref<ComputePlan> ComputePlan::add_barrier(bool is_temp) {
    if (is_temp) {
        temp_items.push_back(Pair(Barrier, 0u));
    } else {
        compute_items.push_back(Pair(Barrier, 0u));
    }
	return this;
}

Ref<ComputePlan> ComputePlan::add_kernel(Ref<ComputeKernel> kernel, uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants, bool is_temp) {
    ShaderDispatch new_dispatch = {
        kernel,
        {x_invocations, y_invocations, z_invocations},
        push_constants
    };
    if (is_temp) {
        temp_items.push_back({ComputePlanItem::Shader, temp_dispatches.size()});
        temp_dispatches.push_back(new_dispatch);
    } else {
        compute_items.push_back({ComputePlanItem::Shader, dispatches.size()});
        dispatches.push_back(new_dispatch);
    }
	return this;
}
