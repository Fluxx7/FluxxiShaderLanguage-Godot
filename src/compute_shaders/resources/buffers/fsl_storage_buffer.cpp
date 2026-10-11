#include "../fsl_buffer.h"

void FSLStorageBuffer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_field", "field", "value"), &FSLStorageBuffer::set_field);
    ClassDB::bind_method(D_METHOD("set_buffer", "values"), &FSLStorageBuffer::set_buffer);
    ClassDB::bind_method(D_METHOD("set_buffer_bytes", "data", "offset", "num_bytes"), &FSLStorageBuffer::set_buffer_bytes, DEFVAL(0), DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_buffer_values_bytes", "offset", "num_bytes"), &FSLStorageBuffer::get_buffer_values_bytes, DEFVAL(0), DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_buffer_values"), &FSLStorageBuffer::get_buffer_values);
    ClassDB::bind_method(D_METHOD("get_field_value"), &FSLStorageBuffer::get_field_value);

    ClassDB::bind_method(D_METHOD("get_buffer_values_bytes_async", "callback", "offset", "num_bytes"), &FSLStorageBuffer::get_buffer_values_bytes_async, DEFVAL(0), DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_buffer_values_async", "callback"), &FSLStorageBuffer::get_buffer_values_async);
    ClassDB::bind_method(D_METHOD("get_field_value_async", "callback"), &FSLStorageBuffer::get_field_value_async);

    ClassDB::bind_method(D_METHOD("set_unsized_element_count", "num_elements"), &FSLStorageBuffer::set_unsized_element_count);
}


void FSLStorageBuffer::_rebuild() {
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    rduniform = memnew(RDUniform);
    rid = rd->storage_buffer_create(size_bytes, {}, flag_long);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
    rebuilt = true;
}

uint32_t FSLStorageBuffer::get_size_bytes() {
	return size_bytes;
}

uint32_t FSLStorageBuffer::_get_fields_size_bytes(uint32_t unsized_count) {
    uint32_t size = 0;
    for (const auto& [name, field] : buffer_info.fields) {
        FSLLayout type_sizing = get_fsl_type_layout(field.type, unsized_count, buffer_info.format);
        if (size % type_sizing.alignment != 0) {
            size += type_sizing.alignment - (size % type_sizing.alignment);
        }
        uint32_t field_size = type_sizing.size;
        field_offsets[name] = {size, field_size};
        size += field_size;
    }
    return size;
}

void FSLStorageBuffer::_init_buffer() {
    for (const auto& flag : buffer_info.usage_flags) {
        flag_long |= flag;
    }
    _update_size_bytes(_get_fields_size_bytes());
}

void FSLStorageBuffer::_update_size_bytes(uint32_t new_size_bytes, PackedByteArray data) {
    if (size_bytes != new_size_bytes) {
        size_bytes = new_size_bytes;
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
    HashMap<StringName, PackedByteArray> data;
    for (const auto& field_name : values.keys()) {
        if (!buffer_info.fields.has(field_name)) {
            ERR_PRINT(vformat("Buffer has no field named \"%s\"", field_name));
            return;
        }
        data[field_name] = (fsl_type_to_bytes(buffer_info.fields[field_name].type, values[field_name]));
        set_field(field_name, values[field_name]);
    }
    PackedByteArray write_data = {};
    for (const auto& [field_name, field_info] : buffer_info.fields) {
        PackedByteArray field_data;
        if (!data.has(field_name)) {
            ERR_PRINT(vformat("No value provided for field \"%s\", default value used", field_name));
            ERR_PRINT_ONCE(vformat("To only set the provided fields, use update_buffer"));
            field_data = get_fsl_default_value(field_info.type);
        } else {
            field_data = data[field_name];
        }
        write_data.append_array(field_data);
    }
    _push_buffer_values(0, write_data);
}

void FSLStorageBuffer::set_buffer_bytes(PackedByteArray data, uint32_t offset, uint32_t num_bytes) {
    if (num_bytes == 0) num_bytes = data.size();
    rd->buffer_update(rid, offset, num_bytes, data);
}

void FSLStorageBuffer::set_unsized_element_count(uint32_t num_elements) {
    ERR_FAIL_COND_MSG(buffer_info.tail_stride == 0, "Buffer has no unsized array field");
    _update_size_bytes(_get_fields_size_bytes(num_elements));
}

TypedDictionary<StringName, Variant> FSLStorageBuffer::_bytes_to_fields(PackedByteArray raw_data) {
    TypedDictionary<StringName, Variant> field_vals;
    for (const auto& [field_name, field_info] : field_offsets) {
        const auto& [field_offset, field_size] = field_info;
        PackedByteArray field_bytes = raw_data.slice(field_offset, field_offset + field_size);
    }
	return field_vals;
}

PackedByteArray FSLStorageBuffer::get_buffer_values_bytes(uint32_t offset, uint32_t num_bytes) {
	return rd->buffer_get_data(rid, offset, num_bytes);
}

TypedDictionary<StringName, Variant> FSLStorageBuffer::get_buffer_values() {
	return _bytes_to_fields(get_buffer_values_bytes());
}

Variant FSLStorageBuffer::get_field_value(StringName field_name) {
    if (!buffer_info.fields.has(field_name)) {
        ERR_PRINT(vformat("Buffer has no field named \"%s\"", field_name));
        return Variant();
    }
    PackedByteArray raw_data = get_buffer_values_bytes();
	return Variant();
}

void FSLStorageBuffer::get_buffer_values_bytes_async(Callable callback, uint32_t offset, uint32_t num_bytes) {
	rd->buffer_get_data_async(rid, callback, offset, num_bytes);
}

void FSLStorageBuffer::get_buffer_values_async(Callable callback) {
	
}

void FSLStorageBuffer::get_field_value_async(StringName field_name, Callable callback) {

}

void FSLStorageBuffer::copy_fields(Ref<FSLBuffer> dest, TypedArray<StringName> fields, uint32_t num_elements) {
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