#include "../fsl_buffer.h"

void FSLVertexBuffer::_bind_methods() {
}


void FSLVertexBuffer::_rebuild() {
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    rduniform = memnew(RDUniform);
    rid = rd->vertex_buffer_create(size_bytes, data_cache, flag_long);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
    rebuilt = true;
}

uint32_t FSLVertexBuffer::_get_fields_size_bytes(uint32_t unsized_count) {
    uint32_t new_size_bytes = 0;
    for (const auto &[_field_name, field] : buffer_info.fields) {
        new_size_bytes += get_fsl_type_size(field.type, unsized_count);
    }
    return new_size_bytes;
}

void FSLVertexBuffer::_init_buffer() {
    _update_size_bytes(_get_fields_size_bytes());
}

void FSLVertexBuffer::_update_size_bytes(uint32_t new_size_bytes, PackedByteArray data) {
    if (size_bytes != new_size_bytes) {
        size_bytes = new_size_bytes;
        data_cache = data;
        _rebuild();
    }
}

void FSLVertexBuffer::_push_buffer_values(uint32_t offset, PackedByteArray values) {
    uint32_t size_required = offset + values.size();
    if (size_required != size_bytes) {
        PackedByteArray new_data;
        _update_size_bytes(size_required, new_data);
    } else {
        rd->buffer_update(rid, offset, values.size(), values);
    }
}

Ref<FSLVertexBuffer> FSLVertexBuffer::new_buffer(RenderingDevice *new_rd, BufferInfo buf_info) {
	Ref<FSLVertexBuffer> new_buf = memnew(FSLVertexBuffer);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLVertexBuffer()));
        }
    }
    new_buf->rd = rd;
    new_buf->buffer_info = buf_info;
    new_buf->_init_buffer();
	return new_buf;
}

Ref<RDUniform> FSLVertexBuffer::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
    needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

FSLVertexBuffer::~FSLVertexBuffer() {
}