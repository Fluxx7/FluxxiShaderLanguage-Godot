#include "../fsl_buffer.h"

void FSLUniformBuffer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_field", "field", "value"), &FSLUniformBuffer::set_field);
    ClassDB::bind_method(D_METHOD("set_buffer", "values"), &FSLUniformBuffer::set_buffer);
}

void FSLUniformBuffer::_rebuild() {
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    rduniform = memnew(RDUniform);
    rid = rd->uniform_buffer_create(size_bytes, data_cache, flag_long);
    rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_UNIFORM_BUFFER);
    rduniform->add_id(rid);
    emit_signal("rid_changed", rid);
    rebuilt = true;
}

uint32_t FSLUniformBuffer::get_size_bytes() {
	return size_bytes;
}

uint32_t FSLUniformBuffer::_get_fields_size_bytes() {
    uint32_t size = 0;
    for (const auto& [_name, field] : buffer_info.fields) {
        size += get_fsl_type_layout(field.type, 0, STD140).size;
    }
    if (size % 16 != 0) {
        size += 16 - (size % 16);
    }
    return size;
}

void FSLUniformBuffer::_init_buffer() {
    size_bytes = _get_fields_size_bytes();
    _rebuild();
}

void FSLUniformBuffer::_push_buffer_values(uint32_t offset, PackedByteArray values) {
    rd->buffer_update(rid, offset, values.size(), values);
}

Ref<FSLUniformBuffer> FSLUniformBuffer::new_buffer(RenderingDevice *new_rd, BufferInfo buf_info) {
	Ref<FSLUniformBuffer> new_buf = memnew(FSLUniformBuffer);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLUniformBuffer()));
        }
    }
    new_buf->rd = rd;
    new_buf->buffer_info = buf_info;
    new_buf->_init_buffer();
	return new_buf;
}

void FSLUniformBuffer::set_field(StringName field, Variant value) {
    if (!buffer_info.fields.has(field)) {
        ERR_PRINT(vformat("Buffer has no field named \"%s\"", field));
        return;
    }
    

}

void FSLUniformBuffer::set_buffer(TypedDictionary<StringName, Variant> values) {
    HashMap<StringName, PackedByteArray> data;
    for (const auto& field_name : values.keys()) {
        if (!buffer_info.fields.has(field_name)) {
            ERR_PRINT(vformat("Buffer has no field named \"%s\"", field_name));
            return;
        }
        data[field_name] = (fsl_type_to_bytes(buffer_info.fields[field_name].type, values[field_name], STD140));
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
    if (write_data.size() % 16 != 0) {
        PackedByteArray offset_bits;
        offset_bits.resize(16 - (write_data.size() % 16));
        write_data.append_array(offset_bits);
    }
    if (data_cache != write_data) {
        _push_buffer_values(0, write_data);
        data_cache = write_data;
    }
    
}

void FSLUniformBuffer::update_buffer(TypedDictionary<StringName, Variant> values) {
    for (const auto& field_name : values.keys()) {
        set_field(field_name, values[field_name]);
    }
}

void FSLUniformBuffer::copy_fields(Ref<FSLBuffer> dest, TypedArray<StringName> fields) {

}

Ref<RDUniform> FSLUniformBuffer::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
    needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

FSLUniformBuffer::~FSLUniformBuffer() {
}