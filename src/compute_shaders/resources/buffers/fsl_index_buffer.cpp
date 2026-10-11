#include "../fsl_buffer.h"
#include "fsl_buffer.h"

void FSLIndexBuffer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_index_count", "num_indices"), &FSLIndexBuffer::set_index_count);
    ClassDB::bind_method(D_METHOD("set_index_format", "new_format"), &FSLIndexBuffer::set_index_format);
    ClassDB::bind_method(D_METHOD("copy_indices", "dest", "num_indices", "offset", "dest_offset_bytes"), &FSLIndexBuffer::copy_indices, DEFVAL(0), DEFVAL(0), DEFVAL(0));
}

void FSLIndexBuffer::_rebuild() {
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    rduniform = memnew(RDUniform);
    rid = rd->index_buffer_create(index_count, index_format, {}, false, flag_long | RenderingDevice::BUFFER_CREATION_AS_STORAGE_BIT);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
    rebuilt = true;
}

uint32_t FSLIndexBuffer::get_size_bytes() {
    uint32_t index_size_bytes = index_format == RenderingDevice::INDEX_BUFFER_FORMAT_UINT16 ? 16 : 32;
	return index_count * index_size_bytes;
}

void FSLIndexBuffer::_init_buffer() {
    _update_index_count(1);
}

void FSLIndexBuffer::_update_index_count(uint32_t new_index_count) {
    if (index_count != new_index_count) {
        index_count = new_index_count;
        _rebuild();
    }
}

Ref<FSLIndexBuffer> FSLIndexBuffer::new_buffer(RenderingDevice *new_rd, BufferInfo buf_info) {
	Ref<FSLIndexBuffer> new_buf = memnew(FSLIndexBuffer);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLIndexBuffer()));
        }
    }
    new_buf->rd = rd;
    new_buf->buffer_info = buf_info;
    new_buf->_init_buffer();
	return new_buf;
}

void FSLIndexBuffer::copy_indices(Ref<FSLBuffer> dest, uint32_t num_indices, uint32_t offset, uint32_t dest_offset_bytes) {
    uint32_t indices_to_copy = num_indices == 0 ? index_count : num_indices;
    ERR_FAIL_COND_MSG(indices_to_copy + offset > index_count, "Requested index copy goes beyond end of index buffer");
    uint32_t index_size_bytes = index_format == RenderingDevice::INDEX_BUFFER_FORMAT_UINT16 ? 16 : 32;
    uint32_t bytes_to_copy = num_indices * index_size_bytes;
    ERR_FAIL_COND_MSG(bytes_to_copy + dest_offset_bytes > dest->get_size_bytes(), "Requested index copy goes beyond end of destination buffer");
    rd->buffer_copy(rid, dest->get_rid(), offset * index_size_bytes, dest_offset_bytes, bytes_to_copy);
}

void FSLIndexBuffer::set_index_count(uint32_t num_indices) {
	_update_index_count(num_indices);
}

void FSLIndexBuffer::set_index_format(RenderingDevice::IndexBufferFormat new_format) {
    if (index_format != new_format) {
        index_format = new_format;
        _rebuild();
    }
}

Ref<RDUniform> FSLIndexBuffer::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
	needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

FSLIndexBuffer::~FSLIndexBuffer() {
}