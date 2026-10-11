#include "../fsl_sampler_type.h"

/**********************************************************/
/******************* FSLSampler2DArray ********************/
/**********************************************************/

void FSLSampler2DArray::_bind_methods() {
    ClassDB::bind_method(D_METHOD("bind_texture", "texture"), &FSLSampler2DArray::bind_texture);
    ClassDB::bind_method(D_METHOD("_bind_texture_rid_wrapper", "_"), &FSLSampler2DArray::_bind_texture_rid_wrapper);
}

void FSLSampler2DArray::_rebuild() {
    rebuilt = true;
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    Ref<RDSamplerState> rd_sampler_state = memnew(RDSamplerState);
    rd_sampler_state->set_mag_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
    rd_sampler_state->set_min_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
    rd_sampler_state->set_mip_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
    rd_sampler_state->set_repeat_u(RenderingDevice::SAMPLER_REPEAT_MODE_REPEAT);
    rd_sampler_state->set_repeat_v(RenderingDevice::SAMPLER_REPEAT_MODE_REPEAT);
    rd_sampler_state->set_repeat_w(RenderingDevice::SAMPLER_REPEAT_MODE_REPEAT);
    rid = rd->sampler_create(rd_sampler_state);
    rduniform = memnew(RDUniform);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
    rduniform->add_id(rid);
    rduniform->add_id(bound_texture_rid);
    emit_signal("rid_changed", rid);
}

Ref<FSLSampler2DArray> FSLSampler2DArray::new_sampler(RenderingDevice *new_rd, SamplerInfo sampler_info, Ref<FSLTexture2DArray> texture) {
    ERR_FAIL_NULL_V(texture, memnew(FSLSampler2DArray()));
    Ref<FSLSampler2DArray> new_sampler = memnew(FSLSampler2DArray);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLSampler2DArray()));
        }
    }
    new_sampler->rd = rd;
    new_sampler->sampler_info = sampler_info;
    new_sampler->bind_texture(texture);
    new_sampler->_rebuild();
	return new_sampler;
}

void FSLSampler2DArray::bind_texture(Ref<FSLTexture2DArray> texture) {
    ERR_FAIL_NULL(texture);
    texture->connect_and_call(Callable(this, "_bind_texture_rid_wrapper"));
    bound_texture = texture;
}

void FSLSampler2DArray::bind_texture_rid(RID texture_rid) {
    bound_texture_rid = texture_rid;
    _rebuild();
}

Ref<RDUniform> FSLSampler2DArray::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
	needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

FSLSampler2DArray::~FSLSampler2DArray() {
}