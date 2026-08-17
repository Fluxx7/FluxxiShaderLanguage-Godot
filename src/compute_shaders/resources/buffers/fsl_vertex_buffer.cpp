#include "../fsl_buffer.h"
#include "fsl_buffer.h"

void FSLVertexBuffer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_vertex_count", "num_vertices"), &FSLVertexBuffer::set_vertex_count);
    ClassDB::bind_method(D_METHOD("set_vertex_size_bytes", "vertex_size"), &FSLVertexBuffer::set_vertex_size_bytes);
}


void FSLVertexBuffer::_rebuild() {
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    rduniform = memnew(RDUniform);
    rid = rd->vertex_buffer_create(vertex_count * bytes_per_vertex, {}, flag_long | RenderingDevice::BUFFER_CREATION_AS_STORAGE_BIT);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
    rebuilt = true;
}

void FSLVertexBuffer::_init_buffer() {
    _update_vertex_count(1);
}

void FSLVertexBuffer::_update_vertex_count(uint32_t new_vertex_count) {
    if (vertex_count != new_vertex_count) {
        vertex_count = new_vertex_count;
        _rebuild();
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

void FSLVertexBuffer::set_vertex_count(uint32_t num_vertices) {
    _update_vertex_count(num_vertices);
}

void FSLVertexBuffer::set_vertex_size_bytes(uint32_t vertex_size) {
    if (bytes_per_vertex != vertex_size) {
        bytes_per_vertex = vertex_size;
        _rebuild();
    }
}

Ref<RDUniform> FSLVertexBuffer::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
	needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

FSLVertexBuffer::~FSLVertexBuffer() {
}