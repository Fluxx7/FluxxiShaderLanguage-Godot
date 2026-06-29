#pragma once
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/templates/hash_map.hpp"
#include "godot_cpp/templates/a_hash_map.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/classes/image.hpp"

#include "ast_defs.h"
#include <optional>



class ShaderResource {
private:
protected:
    RenderingDevice *rd = nullptr;
    RID rid = RID();
    Ref<RDUniform> rduniform = memnew(RDUniform);
    bool rebuild = true;
public:
    virtual ~ShaderResource();
    virtual Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) = 0;
};


class TextureResource : public ShaderResource {
protected:
    TextureFormat tex_format;
    uint32_t x,y;
    Ref<Image> data;
    LocalVector<Callable> callbacks;
public:
    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
    void bind_texture_callback(Callable &callback);
    void set_texture(uint32_t size_x, uint32_t size_y, Ref<Image> tex);
    TextureResource() = default;
    TextureResource(RenderingDevice *rd, TextureFormat tex_type, uint32_t size_x, uint32_t size_y);
};

class BufferResource : public ShaderResource {
private:
protected:
    uint32_t size_bytes;
    BufferType buf_type;
    bool update = true;
public:
    BufferResource() = default;
    BufferResource(RenderingDevice *rd, BufferType buffer_type, uint32_t size_bytes, PackedByteArray *data = nullptr);
    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
};

class UniformResource : public ShaderResource {
public:
    Ref<RDUniform> get_rd_uniform(uint32_t binding, bool &needs_rebuild) override;
};

typedef uint32_t GID;

class ShaderResourceStorage {
private:
protected:
    enum ResourceType {
        Buffer,
        Texture,
        Uniform
    };
    static AHashMap<RenderingDevice*, ShaderResourceStorage> *rd_storages;
    AHashMap<StringName, GID> named_resources;
    AHashMap<GID, ResourceType> shader_resources;
    AHashMap<GID, BufferResource*> buffers;
    AHashMap<GID, TextureResource*> textures;
    AHashMap<GID, UniformResource*> uniforms;
    GID next_gid = 1;
    RenderingDevice* rd;
public:
    static ShaderResourceStorage *get_singleton(RenderingDevice *rd) {
        if (rd_storages == nullptr) {
            rd_storages = new AHashMap<RenderingDevice*, ShaderResourceStorage>();
        }
        if (!rd_storages->has(rd)) {
            (*rd_storages)[rd] = ShaderResourceStorage();
            (*rd_storages)[rd].rd = rd;
        }
        return &(*rd_storages)[rd];
    }
    GID create_buffer(BufferType buffer_type, uint32_t size_bytes, PackedByteArray *data = nullptr);
    GID create_texture(TextureFormat tex_type, uint32_t size_x, uint32_t size_y);

    Ref<RDUniform> get_rd_uniform(GID resource_id, uint32_t binding, bool &rebuilt);
    void bind_texture_callback(GID texture_id, Callable &callback);
    void set_texture(GID texture_id, uint32_t size_x, uint32_t size_y, Ref<Image> tex);
};

class UniformSet {
protected:
    RenderingDevice* rd;
    bool rebuild = true;
    RID uniform_set_id = RID();
    AHashMap<uint32_t, GID> uniforms; 
    ShaderResourceStorage *shader_resources;
public:
    UniformSet() {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        shader_resources = ShaderResourceStorage::get_singleton(rd);
    }
    UniformSet(RenderingDevice *in_rd) {
        rd = in_rd;
        shader_resources = ShaderResourceStorage::get_singleton(in_rd);
    }
    ~UniformSet() {
        if (uniform_set_id.is_valid() && rd->uniform_set_is_valid(uniform_set_id)) {
            rd->free_rid(uniform_set_id);
        }
    }
    void bind_resource(GID resource_id, uint32_t binding) {
        uniforms[binding] = resource_id;
        rebuild = true;
    }
    RID get_rid(RID shader_rid, uint32_t set_index = 0);
};