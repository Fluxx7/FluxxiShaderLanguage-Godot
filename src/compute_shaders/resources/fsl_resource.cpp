#include "fsl_resource.h"

/**************************************************/
/******************* FSLResource ******************/
/**************************************************/

void FSLResource::_bind_methods() {
    ADD_SIGNAL(MethodInfo("rid_changed", PropertyInfo(Variant::RID, "rid")));

    ClassDB::bind_method(D_METHOD("get_rid"), &FSLResource::get_rid);
    ClassDB::bind_method(D_METHOD("connect_and_call", "event_handler"), &FSLResource::connect_and_call);
}

RID FSLResource::get_rid() {
	return rid;
}

void FSLResource::connect_and_call(Callable event_handler) {
    connect("rid_changed", event_handler);
    event_handler.call(rid);
}

FSLResource::~FSLResource() {
	if (rid.is_valid() && rd != nullptr) {
        rd->free_rid(rid);
    }
}

/*****************************************************/
/******************* FSLRawResource ******************/
/*****************************************************/

void FSLRawResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_rid", "rid"), &FSLRawResource::set_rid);
    ClassDB::bind_method(D_METHOD("set_uniform_type"), &FSLRawResource::set_uniform_type);
    ClassDB::bind_method(D_METHOD("get_uniform_type"), &FSLRawResource::get_uniform_type);
    ClassDB::bind_static_method("FSLRawResource", D_METHOD("from_rid", "rid", "uniform_type"), &FSLRawResource::from_rid);
}

void FSLRawResource::_rebuild() {
    rebuilt = true;
    rduniform = memnew(RDUniform);
    rduniform->add_id(rid);
}

Ref<FSLRawResource> FSLRawResource::from_rid(RID rid, RenderingDevice::UniformType uniform_type) {
	Ref<FSLRawResource> new_raw_resource = memnew(FSLRawResource);
    new_raw_resource->rid = rid;
    new_raw_resource->resource_type = uniform_type;
	return new_raw_resource;
}

void FSLRawResource::set_rid(RID rid) {
    this->rid = rid;
    emit_signal("rid_changed", rid);
    _rebuild();
}

void FSLRawResource::set_uniform_type(RenderingDevice::UniformType uniform_type) {
    if (uniform_type != resource_type) {
        resource_type = uniform_type;
        _rebuild();
    }
}

RenderingDevice::UniformType FSLRawResource::get_uniform_type() {
	return resource_type;
}

Ref<RDUniform> FSLRawResource::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
    needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

/******************************************************/
/******************* FSLUniformSet ********************/
/******************************************************/


RID FSLUniformSet::get_rid(RID shader_rid, uint32_t set_index) {
    bool rebuild_set = rebuild || !rd->uniform_set_is_valid(uniform_set_id);
    TypedArray<Ref<RDUniform>> set_uniforms;

    if (!uniforms.is_empty()) {
        for (auto &[binding, resource] : uniforms) {
            set_uniforms.append(resource->get_rd_uniform(binding, rebuild_set));
        }
    }

    if (rebuild_set) {
        if (rd->uniform_set_is_valid(uniform_set_id) && uniform_set_id.is_valid()) {
            rd->free_rid(uniform_set_id);
        }
        if (!uniforms.is_empty()) {
            uniform_set_id = rd->uniform_set_create(set_uniforms, shader_rid, set_index);
        }
        rebuild = false;
    }
    return uniform_set_id;
}
