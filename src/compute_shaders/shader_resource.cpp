#include "shader_resource.h"

AHashMap<RenderingDevice*, ShaderResourceStorage> *ShaderResourceStorage::rd_storages = nullptr;


ShaderResource::~ShaderResource() {
    if (rd != nullptr && rid.is_valid()) {
        rd->free_rid(rid);
    }
}

BufferResource::BufferResource(RenderingDevice *rd, BufferType buffer_type, uint32_t size_bytes, PackedByteArray *data) {
    this->rd = rd;
    buf_type = buffer_type;
    this->size_bytes = buffer_type == STORAGE ? size_bytes : size_bytes + size_bytes % 16;
}

Ref<RDUniform> BufferResource::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
    if (rebuild) {
        needs_rebuild = true;
        if (rduniform != nullptr && rduniform->get_ids().size() > 0) {
            rd->free_rid(rduniform->get_ids()[0]);
        }
        rduniform = memnew(RDUniform);
        auto rdbuf_type = buf_type == UNIFORM ? RenderingDevice::UNIFORM_TYPE_UNIFORM_BUFFER : RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER;
        rduniform->set_uniform_type(rdbuf_type);
        if (buf_type == UNIFORM) {
            rid = rd->uniform_buffer_create(size_bytes);
        } else {
            rid = rd->storage_buffer_create(size_bytes);
        }
        rduniform->add_id(rid);
        rebuild = false;
    }
    rduniform->set_binding(binding);
	return rduniform;
}

Ref<RDUniform> TextureResource::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
    if (rebuild) {
        needs_rebuild = true;
        if (rduniform != nullptr && rduniform->get_ids().size() > 0) {
            rd->free_rid(rduniform->get_ids()[0]);
        }
        rduniform = memnew(RDUniform);
        rduniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
        auto format = tex_format == RGBA16F ? RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT : RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT;
        Ref<RDTextureFormat> tex_info = memnew(RDTextureFormat);
        tex_info->set_width(x);
        tex_info->set_height(y);
        tex_info->set_format(format);
        tex_info->set_texture_type(RenderingDevice::TextureType::TEXTURE_TYPE_2D);
        tex_info->set_usage_bits(
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_COPY_FROM_BIT |
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_STORAGE_BIT |
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_CAN_UPDATE_BIT |
            RenderingDevice::TextureUsageBits::TEXTURE_USAGE_SAMPLING_BIT
        );
        if (data != nullptr) {
            rid = rd->texture_create(tex_info, memnew(RDTextureView), {data->get_data()});
        } else {
            rid = rd->texture_create(tex_info, memnew(RDTextureView));
        }
        rebuild = false;
        rduniform->add_id(rid);
        Ref<Texture2DRD> texUniform = memnew(Texture2DRD());
        texUniform->set_texture_rd_rid(rid);
        for (auto& callback : callbacks) {
            callback.call(texUniform);
        }
    }
    rduniform->set_binding(binding);
	return rduniform;
}

void TextureResource::bind_texture_callback(Callable &callback) {
    callbacks.push_back(callback);
}

void TextureResource::set_texture(uint32_t size_x, uint32_t size_y, Ref<Image> tex) {
    data = tex;
    x = size_x;
    y = size_y;
    rebuild = true;
}

TextureResource::TextureResource(RenderingDevice *rd, TextureFormat tex_type, uint32_t size_x, uint32_t size_y) : tex_format(tex_type), x(size_x), y(size_y) {
    this->rd = rd;
}

GID ShaderResourceStorage::create_buffer(BufferType buffer_type, uint32_t size_bytes, PackedByteArray *data) {
    shader_resources[next_gid] = Buffer;
    buffers[next_gid] = memnew(BufferResource(rd, buffer_type, size_bytes, data));
	return next_gid++;
}

GID ShaderResourceStorage::create_texture(TextureFormat tex_type, uint32_t size_x, uint32_t size_y) {
    shader_resources[next_gid] = Texture;
    textures[next_gid] = memnew(TextureResource(rd, tex_type, size_x, size_y));
	return next_gid++;
}

Ref<RDUniform> ShaderResourceStorage::get_rd_uniform(GID resource_id, uint32_t binding, bool &rebuilt) {
	if (shader_resources.has(resource_id)) {
        switch (shader_resources[resource_id]) {
            case ShaderResourceStorage::Buffer: return *buffers[resource_id]->get_rd_uniform(binding, rebuilt);
            case ShaderResourceStorage::Texture: return textures[resource_id]->get_rd_uniform(binding, rebuilt);
            case ShaderResourceStorage::Uniform: return uniforms[resource_id]->get_rd_uniform(binding, rebuilt);
        }
    }
	return memnew(RDUniform());
}

void ShaderResourceStorage::bind_texture_callback(GID texture_id, Callable &callback) {
    if (textures.has(texture_id)) {
        textures[texture_id]->bind_texture_callback(callback);
        return;
    }
    WARN_PRINT("Invalid GID provided for texture callback");
}

void ShaderResourceStorage::set_texture(GID texture_id, uint32_t size_x, uint32_t size_y, Ref<Image> tex) {
    if (textures.has(texture_id)) {
        textures[texture_id]->set_texture(size_x, size_y, tex);
        return;
    }
    WARN_PRINT("Invalid GID provided for texture callback");
}

RID UniformSet::get_rid(RID shader_rid, uint32_t set_index) {
    bool rebuild_set = rebuild || !rd->uniform_set_is_valid(uniform_set_id);
    TypedArray<Ref<RDUniform>> set_uniforms;

    for (auto &[binding, uniform_id] : uniforms) {
        set_uniforms.append(shader_resources->get_rd_uniform(uniform_id, binding, rebuild_set));
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

Ref<RDUniform> UniformResource::get_rd_uniform(uint32_t binding, bool &needs_rebuild) {
	return Ref<RDUniform>();
}
