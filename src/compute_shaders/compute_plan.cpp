#include "compute_kernel.h"

/******** STATIC METHODS *********/
void ComputePlan::_bind_methods() {
    ClassDB::bind_static_method("ComputePlan", D_METHOD("make_new", "rendering_device"), &ComputePlan::make_new, DEFVAL(nullptr));
    ClassDB::bind_method(D_METHOD("add_barrier", "is_temp"), &ComputePlan::add_barrier, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("add_kernel", "kernel", "x_invocations", "y_invocations", "z_invocations", "push_constants", "is_temp"), &ComputePlan::add_kernel, DEFVAL((TypedDictionary<StringName, Variant>())), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("add_kernel_indirect", "kernel", "command_buffer", "offset", "push_constants", "is_temp"), &ComputePlan::add_kernel_indirect, DEFVAL((TypedDictionary<StringName, Variant>())), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("add_kernel_workgroups", "kernel", "x_workgroups", "y_workgroups", "z_workgroups", "push_constants", "is_temp"), &ComputePlan::add_kernel_workgroups, DEFVAL((TypedDictionary<StringName, Variant>())), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("add_plan", "subplan", "is_temp"), &ComputePlan::add_plan, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("dispatch"), &ComputePlan::dispatch);
}

void ComputePlan::_dispatch_shader(ShaderDispatch &shader_dispatch, int64_t compute_list) {
    RID pipeline_rid = shader_dispatch.shader->get_pipeline_rid();
    if (!plan_rd->compute_pipeline_is_valid(pipeline_rid) || !pipeline_rid.is_valid()) {
        ERR_FAIL_MSG("Shader pipeline failed to create properly, skipping dispatch.");
    }
    plan_rd->compute_list_bind_compute_pipeline(compute_list, pipeline_rid);
    if (shader_dispatch.push_constants.size() > 0) {
        PackedByteArray pc_bytes = shader_dispatch.shader->push_constants_to_bytes(shader_dispatch.push_constants);
        uint32_t pc_byte_size = pc_bytes.size();
        plan_rd->compute_list_set_push_constant(compute_list, pc_bytes, pc_byte_size);
    } else {
        plan_rd->compute_list_set_push_constant(compute_list, {}, 0);
    }
    RID uniform_set_rid = shader_dispatch.shader->get_uniform_set_rid();
    if (plan_rd->uniform_set_is_valid(uniform_set_rid)) {
        plan_rd->compute_list_bind_uniform_set(compute_list, uniform_set_rid, 0);
    }

    match(shader_dispatch.dispatch_target, 
        [&](const ShaderDispatch::IndirectDispatch& indirect) {
            plan_rd->compute_list_dispatch_indirect(compute_list, indirect.command_buffer->get_rid(), indirect.offset);
        },
        [&](const ShaderDispatch::DirectDispatch& workgroups) {
            plan_rd->compute_list_dispatch(compute_list, workgroups.x, workgroups.y, workgroups.z);
        });
}

void ComputePlan::bind(int64_t compute_list) {
    for (const auto& [item_type, group_index] : compute_items) {
        if (item_type == Barrier) {
            plan_rd->compute_list_add_barrier(compute_list);
            continue;
        }
        if (item_type == Plan) {
            compute_plans[group_index]->bind(compute_list);
            continue;
        }

        // ShaderDispatch shader_dispatch = dispatches[group_index];
        _dispatch_shader(dispatches[group_index], compute_list);
    }
    for (const auto& [item_type, group_index] : temp_items) {
        if (item_type == Barrier) {
            plan_rd->compute_list_add_barrier(compute_list);
            continue;
        }
        if (item_type == Plan) {
            temp_plans[group_index]->bind(compute_list);
            continue;
        }
         _dispatch_shader(temp_dispatches[group_index], compute_list);
    }
    temp_dispatches.clear();
    temp_plans.clear();
    temp_items.clear();
}

Ref<ComputePlan> ComputePlan::make_new(RenderingDevice *rd) {
    auto new_plan = memnew(ComputePlan());
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, Ref<ComputePlan>());
        }
    }
    new_plan->plan_rd = rd;
	return new_plan;
}

ComputePlan::ComputePlan() {
    plan_rd = RenderingServer::get_singleton()->get_rendering_device();
    if (plan_rd == nullptr) {
        plan_rd = RenderingServer::get_singleton()->create_local_rendering_device();
        ERR_FAIL_NULL(plan_rd);
    }
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
    bind(compute_list);
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
    auto workgroups = kernel->get_workgroups(x_invocations, y_invocations, z_invocations);
    ShaderDispatch new_dispatch = {
        kernel,
        ShaderDispatch::DirectDispatch{std::get<0>(workgroups), std::get<1>(workgroups), std::get<2>(workgroups)},
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

Ref<ComputePlan> ComputePlan::add_kernel_workgroups(Ref<ComputeKernel> kernel, uint32_t x_workgroups, uint32_t y_workgroups, uint32_t z_workgroups, TypedDictionary<StringName, Variant> push_constants, bool is_temp) {
    ShaderDispatch new_dispatch = {
        kernel,
        ShaderDispatch::DirectDispatch{x_workgroups, y_workgroups, z_workgroups},
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

Ref<ComputePlan> ComputePlan::add_kernel_indirect(Ref<ComputeKernel> kernel, Ref<FSLBuffer> command_buffer, uint32_t offset, TypedDictionary<StringName, Variant> push_constants, bool is_temp) {
	ShaderDispatch new_dispatch = {
        kernel,
        ShaderDispatch::IndirectDispatch{command_buffer, offset},
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
