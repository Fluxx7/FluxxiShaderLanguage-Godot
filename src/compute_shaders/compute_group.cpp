#include "compute_group.h"

void ComputeGroup::_bind_methods() {
}

void ComputeGroup::_assign_resource(Ref<FSLResource> resource, uint32_t set, uint32_t binding) {
}

void ComputeGroup::print_info() {
}

void ComputeGroup::assign_resource(Ref<FSLResource> resource, StringName resource_name) {
}

void ComputeGroup::dispatch(StringName kernel_name, uint32_t x_invocations, uint32_t y_invocations, uint32_t z_invocations, TypedDictionary<StringName, Variant> push_constants) {
    if (kernels.has(kernel_name)) {
        kernels[kernel_name]->dispatch(x_invocations, y_invocations, z_invocations, push_constants);
        return;
    }
    WARN_PRINT(vformat("No kernel named \"%s\" in ComputeGroup", kernel_name));
    return;
}
