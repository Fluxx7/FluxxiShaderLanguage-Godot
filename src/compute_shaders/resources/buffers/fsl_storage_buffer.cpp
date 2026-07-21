#include "../fsl_buffer.h"

void FSLStorageBuffer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_field", "field", "value"), &FSLStorageBuffer::set_field);
    ClassDB::bind_method(D_METHOD("set_buffer", "values"), &FSLStorageBuffer::set_buffer);

    ClassDB::bind_method(D_METHOD("set_unsized_element_count", "num_elements"), &FSLStorageBuffer::set_unsized_element_count);
}


void FSLStorageBuffer::_rebuild() {
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    rduniform = memnew(RDUniform);
    rid = rd->storage_buffer_create(size_bytes, data_cache, flag_long);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
    rebuilt = true;
}

uint32_t FSLStorageBuffer::_get_fields_size_bytes(uint32_t unsized_count) {
    uint32_t new_size_bytes = 0;
    for (const auto &[_field_name, field] : buffer_info.fields) {
        new_size_bytes += get_fsl_type_size(field.type, unsized_count);
    }
    return new_size_bytes;
}

void FSLStorageBuffer::_init_buffer() {
    _update_size_bytes(_get_fields_size_bytes());
}

void FSLStorageBuffer::_update_size_bytes(uint32_t new_size_bytes, PackedByteArray data) {
    if (size_bytes != new_size_bytes) {
        size_bytes = new_size_bytes;
        data_cache = data;
        _rebuild();
    }
}

void FSLStorageBuffer::_push_buffer_values(uint32_t offset, PackedByteArray values) {
    uint32_t size_required = offset + values.size();
    if (size_required != size_bytes) {
        PackedByteArray new_data;
        _update_size_bytes(size_required, new_data);
    } else {
        rd->buffer_update(rid, offset, values.size(), values);
    }
}

Ref<FSLStorageBuffer> FSLStorageBuffer::new_buffer(RenderingDevice *new_rd, BufferInfo buf_info) {
	Ref<FSLStorageBuffer> new_buf = memnew(FSLStorageBuffer);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLStorageBuffer()));
        }
    }
    new_buf->rd = rd;
    new_buf->buffer_info = buf_info;
    new_buf->_init_buffer();
	return new_buf;
}

void FSLStorageBuffer::set_field(StringName field, Variant value) {
    if (!buffer_info.fields.has(field)) {
        ERR_PRINT(vformat("Buffer has no field named \"%s\"", field));
        return;
    }
    

}

void FSLStorageBuffer::set_buffer(TypedDictionary<StringName, Variant> values) {
    HashSet<StringName> fields_set = {};
    HashMap<StringName, PackedByteArray> data;
    for (const auto& field_name : values.keys()) {
        if (!buffer_info.fields.has(field_name)) {
            ERR_PRINT(vformat("Buffer has no field named \"%s\"", field_name));
            return;
        }
        data[field_name] = (fsl_type_to_bytes(buffer_info.fields[field_name].type, values[field_name]));
        set_field(field_name, values[field_name]);
        fields_set.insert(field_name);
    }
    for (const auto& [field_name, _] : buffer_info.fields) {
        if (!fields_set.has(field_name)) {
            ERR_PRINT(vformat("No value provided for field \"%s\", default value used", field_name));
            ERR_PRINT_ONCE(vformat("To only set the provided fields, use update_buffer"));

        }
    }
}

void FSLStorageBuffer::set_unsized_element_count(uint32_t num_elements) {
    if (!buffer_info.has_unsized_field) {
        ERR_PRINT_ONCE("Buffer has no unsized array field");
        return;
    }
    _update_size_bytes(_get_fields_size_bytes(num_elements));
}

void FSLStorageBuffer::update_buffer(TypedDictionary<StringName, Variant> values) {
    for (const auto& field_name : values.keys()) {
        set_field(field_name, values[field_name]);
    }
}

Ref<RDUniform> FSLStorageBuffer::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
    needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

FSLStorageBuffer::~FSLStorageBuffer() {
}