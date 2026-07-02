#include "fsl_resource.h"

/**************************************************/
/******************* FSLResource ******************/
/**************************************************/

void FSLResource::_bind_methods() {
}

FSLResource::~FSLResource() {
    if (rid.is_valid() && rd != nullptr) {
        rd->free_rid(rid);
    }
}

/**************************************************/
/******************* FSLBuffer ********************/
/**************************************************/

void FSLBuffer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("bind_callback", "callback"), &FSLBuffer::bind_callback);
    ClassDB::bind_method(D_METHOD("set_field", "field", "value"), &FSLBuffer::set_field);
    ClassDB::bind_method(D_METHOD("set_buffer", "values"), &FSLBuffer::set_buffer);
    ClassDB::bind_method(D_METHOD("set_unsized_element_count", "num_elements"), &FSLBuffer::set_unsized_element_count);
}



void FSLBuffer::_init_buffer() {
    _update_size_bytes(1);
    if (buffer_info.type == UNIFORM) {
        rid = rd->uniform_buffer_create(size_bytes);
    } else {
        rid = rd->storage_buffer_create(size_bytes);
    }
}

void FSLBuffer::_update_size_bytes(uint32_t unsized_count) {
    uint32_t new_size_bytes = 0;
    for (const auto &[_field_name, field] : buffer_info.fields) {
        new_size_bytes += get_fsl_type_size(field.type, unsized_count);
    }
    size_bytes = new_size_bytes;
}

Ref<FSLBuffer> FSLBuffer::new_buffer(RenderingDevice *new_rd, BufferInfo buf_info) {
	Ref<FSLBuffer> new_buf = memnew(FSLBuffer);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLBuffer()));
        }
    }
    new_buf->rd = rd;
    new_buf->buffer_info = buf_info;
    new_buf->_init_buffer();
	return new_buf;
}

void FSLBuffer::set_field(StringName field, Variant value) {
}

void FSLBuffer::set_buffer(TypedDictionary<StringName, Variant> values) {
}

void FSLBuffer::set_unsized_element_count(uint32_t num_elements) {
    if (!buffer_info.has_unsized_field) {
        ERR_PRINT_ONCE("Buffer has no unsized array field");
        return;
    }
    _update_size_bytes(num_elements);
    if (rid.is_valid()) {
        rd->free_rid(rid);
    }
    if (buffer_info.type == UNIFORM) {
        rid = rd->uniform_buffer_create(size_bytes);
    } else {
        rid = rd->storage_buffer_create(size_bytes);
    }
    rebuilt = true;
}

Ref<RDUniform> FSLBuffer::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
	if (rebuilt) {
        needs_rebuild = true;
        rduniform = memnew(RDUniform);
        auto rdbuf_type = buffer_info.type == UNIFORM ? RenderingDevice::UNIFORM_TYPE_UNIFORM_BUFFER : RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER;
        rduniform->set_uniform_type(rdbuf_type);
        rduniform->add_id(rid);
        rebuilt = false;
    }
    rduniform->set_binding(binding);
	return rduniform;
}

void FSLBuffer::bind_callback(Callable callback) {
    callbacks.push_back(callback);
}

FSLBuffer::~FSLBuffer() {
}


/**************************************************/
/******************* FSLTexture *******************/
/**************************************************/

void FSLTexture::_bind_methods() {
    ClassDB::bind_method(D_METHOD("bind_callback", "callback"), &FSLTexture::bind_callback);
    ClassDB::bind_method(D_METHOD("set_texture", "size_x", "size_y", "tex"), &FSLTexture::set_texture, DEFVAL(nullptr));
}

void FSLTexture::_init_texture() {
    Ref<RDTextureFormat> rd_tex_format = memnew(RDTextureFormat);
    rd_tex_format->set_width(x);
    rd_tex_format->set_height(y);
    auto format = texture_info.format == RGBA16F ? RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT : RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT;
    rd_tex_format->set_format(format);
    rd_tex_format->set_texture_type(RenderingDevice::TextureType::TEXTURE_TYPE_2D);
    rd_tex_format->set_usage_bits(
        RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_COPY_FROM_BIT |
        RenderingDevice::TextureUsageBits::TEXTURE_USAGE_STORAGE_BIT |
        RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_UPDATE_BIT |
        RenderingDevice::TextureUsageBits::TEXTURE_USAGE_SAMPLING_BIT
    );
    rid = rd->texture_create(rd_tex_format, memnew(RDTextureView));
}

Ref<FSLTexture> FSLTexture::new_texture(RenderingDevice *new_rd, TextureInfo tex_info) {
    Ref<FSLTexture> new_tex = memnew(FSLTexture);
    RenderingDevice* rd = new_rd;
    if (rd == nullptr) {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (rd == nullptr) {
            rd = RenderingServer::get_singleton()->create_local_rendering_device();
            ERR_FAIL_NULL_V(rd, memnew(FSLTexture()));
        }
    }
    new_tex->rd = rd;
    new_tex->texture_info = tex_info;
    new_tex->_init_texture();
	return new_tex;
}

void FSLTexture::set_texture(uint32_t size_x, uint32_t size_y, Ref<Image> tex) {
    if (size_x != x || size_y != y) {
        x = size_x;
        y = size_y;
        rebuilt = true;
        if (rid.is_valid()) {
            rd->free_rid(rid);
        }
        Ref<RDTextureFormat> rd_tex_format = memnew(RDTextureFormat);
        rd_tex_format->set_width(x);
        rd_tex_format->set_height(y);
        auto format = texture_info.format == RGBA16F ? RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT : RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT;
        rd_tex_format->set_format(format);
        rd_tex_format->set_texture_type(RenderingDevice::TextureType::TEXTURE_TYPE_2D);
        rd_tex_format->set_usage_bits(
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_COPY_FROM_BIT |
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_STORAGE_BIT |
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_UPDATE_BIT |
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_SAMPLING_BIT
        );
        if (tex != nullptr) {
            rid = rd->texture_create(rd_tex_format, memnew(RDTextureView), {tex->get_data()});
        } else {
            rid = rd->texture_create(rd_tex_format, memnew(RDTextureView));
        }
        rduniform = memnew(RDUniform);
        rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
        rduniform->add_id(rid);
        Ref<Texture2DRD> texUniform = memnew(Texture2DRD());
        texUniform->set_texture_rd_rid(rid);
        for (auto& callback : callbacks) {
            callback.call(texUniform);
        }
    } else {
        if (tex != nullptr) {
            rd->texture_update(rid, 0, tex->get_data());
        }   
    }
}

Ref<RDUniform> FSLTexture::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
    needs_rebuild |= rebuilt;
    rebuilt = false;
    rduniform->set_binding(binding);
	return rduniform;
}

void FSLTexture::bind_callback(Callable callback) {
    callbacks.push_back(callback);
    Ref<Texture2DRD> texUniform = memnew(Texture2DRD());
    texUniform->set_texture_rd_rid(rid);
    callback.call(texUniform);
}

FSLTexture::~FSLTexture() {
}


/**************************************************/
/******************* FSLUniformSet ********************/
/**************************************************/


RID FSLUniformSet::get_rid(RID shader_rid, uint32_t set_index) {
    bool rebuild_set = rebuild || !rd->uniform_set_is_valid(uniform_set_id);
    TypedArray<Ref<RDUniform>> set_uniforms;

    for (auto &[binding, resource] : uniforms) {
       set_uniforms.append(resource->get_rd_uniform(binding, rebuild_set));
    }

    if (rebuild_set) {
        if (rd->uniform_set_is_valid(uniform_set_id) && uniform_set_id.is_valid()) {
            rd->free_rid(uniform_set_id);
        }
        uniform_set_id = rd->uniform_set_create(set_uniforms, shader_rid, set_index);
        rebuild = false;
    }
    return uniform_set_id;
}
